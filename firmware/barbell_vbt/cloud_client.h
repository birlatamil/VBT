#ifndef CLOUD_CLIENT_H
#define CLOUD_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <esp_wifi.h>          // for wifi_err_reason_t
#include "rep_detector.h"

// User: Configure your default backend URL here (update before flashing if needed)
#define DEFAULT_BACKEND_URL "https://vbt-n350.onrender.com"
#define DEVICE_ID "esp32_001"

// How long to wait between reconnect attempts (ms)
#define WIFI_RETRY_INTERVAL_MS  30000
// Per-attempt connection timeout (ms) — must be << WDT timeout
#define WIFI_CONNECT_TIMEOUT_MS  12000

class CloudClient {
private:
    WiFiClient wifiClient;
    WiFiClientSecure secureClient;
    HTTPClient http;
    String _backendUrl;    // base URL, e.g. "https://vbt-n350.onrender.com"
    String _ssid;
    String _pass;
    bool   _credentialsOk = false;

    uint32_t _lastRetryMs = 0;   // millis() of last reconnect attempt

    Preferences preferences;

    // ── Translate disconnect reason to human-readable string ────────────
    static const char* _reasonStr(uint8_t reason) {
        switch (reason) {
            case 2:  return "AUTH_EXPIRE (wrong password or AP restarted)";
            case 3:  return "AUTH_LEAVE";
            case 4:  return "ASSOC_EXPIRE";
            case 15: return "4WAY_HANDSHAKE_TIMEOUT (wrong password)";
            case 200: return "NO_AP_FOUND (SSID not visible / out of range)";
            case 201: return "AUTH_FAIL (wrong password)";
            case 202: return "ASSOC_FAIL";
            case 203: return "HANDSHAKE_TIMEOUT (wrong password)";
            default: return "UNKNOWN";
        }
    }

    // ── Scan for the target SSID and report signal strength ─────────────
    // Returns true if the SSID was found in range.
    bool _scanForSSID() {
        Serial.printf("[Cloud] Scanning for '%s'...\n", _ssid.c_str());
        int n = WiFi.scanNetworks(false, false, false, 300); // active scan, 300ms per channel
        if (n <= 0) {
            Serial.println("[Cloud] Scan found NO networks at all (antenna issue?)");
            return false;
        }
        for (int i = 0; i < n; i++) {
            if (WiFi.SSID(i) == _ssid) {
                Serial.printf("[Cloud] Found '%s' — RSSI: %d dBm, Ch: %d, Auth: %d\n",
                    _ssid.c_str(), WiFi.RSSI(i), WiFi.channel(i), (int)WiFi.encryptionType(i));
                WiFi.scanDelete();
                return true;
            }
        }
        Serial.printf("[Cloud] SSID '%s' NOT FOUND in %d networks scanned:\n", _ssid.c_str(), n);
        for (int i = 0; i < n && i < 10; i++) {
            Serial.printf("[Cloud]   > '%s' (RSSI %d dBm)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }
        WiFi.scanDelete();
        return false;
    }

    // ── Internal: attempt a blocking connection with full diagnostics ─────
    bool _connectBlocking(uint32_t timeoutMs) {
        // Disable modem sleep — a common cause of connection failures on ESP32
        WiFi.setSleep(false);
        // Auto-reconnect on drop (handled by WiFi stack internally)
        WiFi.setAutoReconnect(true);

        WiFi.disconnect(false);   // drop STA only; leave AP intact
        delay(200);
        WiFi.begin(_ssid.c_str(), _pass.c_str());

        uint32_t t0 = millis();
        wl_status_t lastStatus = WL_IDLE_STATUS;
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
            delay(500);
            wl_status_t s = WiFi.status();
            if (s != lastStatus) {
                // Print status transitions so we can see what's happening
                Serial.printf("\n[Cloud] WiFi status → %d", (int)s);
                lastStatus = s;
            } else {
                Serial.print(".");
            }
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) return true;

        // ── Print WHY it failed ──────────────────────────────────
        wifi_err_reason_t reason = (wifi_err_reason_t)0;
        wifi_ap_record_t ap;
        // Try to get the last disconnect reason via low-level API
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            Serial.printf("[Cloud] Connected to AP at layer-2 but no IP (DHCP timeout?)\n");
        } else {
            Serial.printf("[Cloud] Failed — final status: %d\n", (int)WiFi.status());
            // WL_NO_SSID_AVAIL = 1, WL_CONNECT_FAILED = 4, WL_CONNECTION_LOST = 5
            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                Serial.println("[Cloud] Cause: SSID not found. Check network name.");
            } else if (WiFi.status() == WL_CONNECT_FAILED) {
                Serial.println("[Cloud] Cause: Auth failed. CHECK YOUR PASSWORD.");
            } else {
                Serial.println("[Cloud] Cause: Unknown. Check router, range, and 2.4GHz band.");
            }
        }
        return false;
    }

public:
    CloudClient() {}

    // ── Persist WiFi config via Web UI ──────────────────────────────────
    void saveWiFiConfig(const String& ssid, const String& pass, const String& backend) {
        preferences.begin("vbt_config", false);
        preferences.putString("ssid", ssid);
        preferences.putString("pass", pass);
        if (backend.length() > 0) {
            preferences.putString("backend", backend);
        }
        preferences.end();
    }

    // ── Called once from setup() AFTER WiFi.mode(WIFI_AP_STA) ───────────
    // Does NOT call WiFi.mode() — the main sketch owns that.
    // Does NOT add itself to the watchdog — called before WDT is active.
    void begin() {
        preferences.begin("vbt_config", true);
        _ssid          = preferences.getString("ssid",    "");
        _pass          = preferences.getString("pass",    "");
        _backendUrl    = preferences.getString("backend", DEFAULT_BACKEND_URL);
        preferences.end();

        if (_ssid.isEmpty()) {
            Serial.println("[Cloud] No WiFi credentials saved. Configure via http://192.168.4.1/wifi");
            return;
        }

        _credentialsOk = true;
        Serial.printf("[Cloud] Connecting to WiFi STA: %s\n", _ssid.c_str());

        // Scan first — tells us immediately if SSID is visible vs wrong password
        _scanForSSID();  // informational; connect attempt runs regardless

        if (_connectBlocking(WIFI_CONNECT_TIMEOUT_MS)) {
            Serial.printf("[Cloud] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            http.setReuse(true);
            secureClient.setInsecure(); // Accept any SSL certificate
        } else {
            Serial.println("[Cloud] Initial connect failed. Will retry every 30s automatically.");
            _lastRetryMs = millis();  // start retry countdown
        }
    }

    // ── Call periodically from loop() — non-blocking reconnect ──────────
    void reconnectIfNeeded() {
        if (!_credentialsOk) return;
        if (WiFi.status() == WL_CONNECTED) return;
        if (millis() - _lastRetryMs < WIFI_RETRY_INTERVAL_MS) return;

        _lastRetryMs = millis();
        Serial.printf("[Cloud] Retrying WiFi connection to: %s\n", _ssid.c_str());

        if (_connectBlocking(WIFI_CONNECT_TIMEOUT_MS)) {
            Serial.printf("[Cloud] Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
            http.setReuse(true);
            secureClient.setInsecure();
        } else {
            Serial.println("[Cloud] Retry failed. Will try again in 30s.");
        }
    }

    // ── Post completed rep data ──────────────────────────────────────────
    void postRepData(const char* exercise, uint32_t rep_count, float avg_v, float peak_v,
                     float ecc_t, float pause_t, float con_t, float tot_t, int state,
                     const DataPoint* profile, uint16_t profile_count) {
        if (WiFi.status() != WL_CONNECTED) return;
        if (_backendUrl.isEmpty()) return;

        String payload;
        payload.reserve(8192);

        char basePayload[512];
        snprintf(basePayload, sizeof(basePayload),
            "{\"device_id\":\"%s\",\"type\":\"rep\",\"exercise\":\"%s\","
            "\"rep\":%lu,\"avg_velocity\":%.3f,\"peak_velocity\":%.3f,"
            "\"ecc_time\":%.2f,\"pause_time\":%.2f,\"con_time\":%.2f,"
            "\"total_time\":%.2f,\"state\":%d,\"timestamp\":%lu,\"profile\":[",
            DEVICE_ID, exercise, rep_count, avg_v, peak_v,
            ecc_t, pause_t, con_t, tot_t, state, millis());

        payload = basePayload;
        for (uint16_t i = 0; i < profile_count; i++) {
            payload += String(profile[i].velocity, 3);
            if (i < profile_count - 1) payload += ",";
        }
        payload += "]}";

        String reqUrl = _backendUrl + "/api/ingest";
        
        bool isHttps = reqUrl.startsWith("https://");
        if (isHttps) {
            http.begin(secureClient, reqUrl);
        } else {
            http.begin(wifiClient, reqUrl);
        }
        
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(5000); // 5s timeout: Render free tier can be slow, plus HTTPS overhead

        int code = http.POST(payload);
        if (code == 200) {
            Serial.printf("[Cloud] POST rep - OK (%d)\n", code);
            http.getString(); // drain keep-alive buffer
        } else if (code > 0) {
            Serial.printf("[Cloud] POST rep - Server Error: HTTP %d. Response: %s\n", code, http.getString().c_str());
        } else {
            Serial.printf("[Cloud] POST rep - Connection Error: %s\n", http.errorToString(code).c_str());
        }
        http.end();
    }

    // ── Post a heartbeat (every ~2s from loop) ───────────────────────────
    void postHeartbeat(float velocity, int state) {
        if (WiFi.status() != WL_CONNECTED) return;
        if (_backendUrl.isEmpty()) return;

        char payload[128];
        snprintf(payload, sizeof(payload),
            "{\"device_id\":\"%s\",\"type\":\"heartbeat\","
            "\"velocity\":%.3f,\"state\":%d,\"timestamp\":%lu}",
            DEVICE_ID, velocity, state, millis());

        String reqUrl = _backendUrl + "/api/ingest";
        
        bool isHttps = reqUrl.startsWith("https://");
        if (isHttps) {
            http.begin(secureClient, reqUrl);
        } else {
            http.begin(wifiClient, reqUrl);
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("Connection", "keep-alive");
        http.setTimeout(3000); // 3s timeout for HTTPS handshakes and slow servers

        int code = http.POST(payload);
        if (code == 200) {
            http.getString(); // drain buffer
        } else if (code > 0) {
            Serial.printf("[Cloud] Heartbeat server error: HTTP %d\n", code);
        } else {
            Serial.printf("[Cloud] Heartbeat connect error: %s\n", http.errorToString(code).c_str());
        }
        http.end();
    }

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
};

extern CloudClient cloud;

#endif
