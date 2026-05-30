#ifndef CLOUD_CLIENT_H
#define CLOUD_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "rep_detector.h"

// User: Configure your default backend URL here (update before flashing if needed)
#define DEFAULT_BACKEND_URL "http://192.168.1.36:3001"
#define DEVICE_ID "esp32_001"

class CloudClient {
private:
    WiFiClient wifiClient;
    HTTPClient http;
    String url;

    Preferences preferences;

public:
    CloudClient() {}

    void saveWiFiConfig(String ssid, String pass, String backend) {
        preferences.begin("vbt_config", false);
        preferences.putString("ssid", ssid);
        preferences.putString("pass", pass);
        if (backend.length() > 0) {
            preferences.putString("backend", backend);
        }
        preferences.end();
    }

    void begin() {
        preferences.begin("vbt_config", true);
        String ssid = preferences.getString("ssid", "");
        String pass = preferences.getString("pass", "");
        String savedBackend = preferences.getString("backend", DEFAULT_BACKEND_URL);
        preferences.end();

        if (ssid == "") {
            Serial.println("[Cloud] No WiFi credentials saved. Please configure via Access Point.");
            return;
        }

        Serial.print("[Cloud] Connecting to WiFi STA: ");
        Serial.println(ssid);
        
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        // Non-blocking wait for up to 10 seconds
        uint32_t startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[Cloud] Connected!");
            Serial.print("[Cloud] IP Address: ");
            Serial.println(WiFi.localIP());
            url = savedBackend + "/api/ingest";
            http.setReuse(true);
        } else {
            Serial.println("\n[Cloud] Failed to connect. Will retry automatically.");
        }
    }

    void postRepData(const char* exercise, uint32_t rep_count, float avg_v, float peak_v, float ecc_t, float pause_t, float con_t, float tot_t, int state, const DataPoint* profile, uint16_t profile_count) {
        if (WiFi.status() != WL_CONNECTED) return;

        String payload;
        payload.reserve(8192); // Enough space for up to 500 velocity floats

        char basePayload[512];
        snprintf(basePayload, sizeof(basePayload), 
            "{\"device_id\":\"%s\",\"type\":\"rep\",\"exercise\":\"%s\",\"rep\":%lu,\"avg_velocity\":%.3f,\"peak_velocity\":%.3f,\"ecc_time\":%.2f,\"pause_time\":%.2f,\"con_time\":%.2f,\"total_time\":%.2f,\"state\":%d,\"timestamp\":%lu,\"profile\":[",
            DEVICE_ID, exercise, rep_count, avg_v, peak_v, ecc_t, pause_t, con_t, tot_t, state, millis());

        payload = basePayload;

        for (uint16_t i = 0; i < profile_count; i++) {
            payload += String(profile[i].velocity, 3);
            if (i < profile_count - 1) {
                payload += ",";
            }
        }
        payload += "]}";

        http.begin(wifiClient, url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(1500); // 1.5s timeout to avoid blocking loop too long
        
        int httpResponseCode = http.POST(payload);
        
        if (httpResponseCode > 0) {
            Serial.printf("[Cloud] POST rep - OK (%d)\n", httpResponseCode);
            String response = http.getString(); // drain buffer to allow keep-alive
        } else {
            Serial.printf("[Cloud] POST rep - Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        
        http.end(); // Free HTTP resources, TCP connection stays open in wifiClient
    }

    void postHeartbeat(float velocity, int state) {
        if (WiFi.status() != WL_CONNECTED) return;

        char payload[128];
        snprintf(payload, sizeof(payload), 
            "{\"device_id\":\"%s\",\"type\":\"heartbeat\",\"velocity\":%.3f,\"state\":%d,\"timestamp\":%lu}",
            DEVICE_ID, velocity, state, millis());

        http.begin(wifiClient, url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Connection", "keep-alive"); // Explicit keep-alive
        http.setTimeout(500); // Quick timeout for heartbeat
        
        int httpResponseCode = http.POST(payload);
        if (httpResponseCode > 0) {
            String response = http.getString(); // drain buffer
        }
        http.end();
    }
};

extern CloudClient cloud;

#endif
