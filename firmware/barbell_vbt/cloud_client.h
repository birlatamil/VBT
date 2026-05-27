#ifndef CLOUD_CLIENT_H
#define CLOUD_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// User: Configure these for your environment
#define WIFI_STA_SSID "HAZZARD-2.4G"
#define WIFI_STA_PASS "07112016"
#define BACKEND_URL "http://192.168.1.33:3001" // Updated with current local IP
#define DEVICE_ID "esp32_001"

class CloudClient {
public:
    CloudClient() {}

    void begin() {
        Serial.print("[Cloud] Connecting to WiFi STA: ");
        Serial.println(WIFI_STA_SSID);
        
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
        
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
        } else {
            Serial.println("\n[Cloud] Failed to connect. Will retry automatically.");
        }
    }

    void postRepData(const char* exercise, uint32_t rep_count, float avg_v, float peak_v, float ecc_t, float pause_t, float con_t, float tot_t, int state) {
        if (WiFi.status() != WL_CONNECTED) return;

        HTTPClient http;
        String url = String(BACKEND_URL) + "/api/ingest";
        
        // Build JSON payload
        char payload[512];
        snprintf(payload, sizeof(payload), 
            "{\"device_id\":\"%s\",\"type\":\"rep\",\"exercise\":\"%s\",\"rep\":%lu,\"avg_velocity\":%.3f,\"peak_velocity\":%.3f,\"ecc_time\":%.2f,\"pause_time\":%.2f,\"con_time\":%.2f,\"total_time\":%.2f,\"state\":%d,\"timestamp\":%lu}",
            DEVICE_ID, exercise, rep_count, avg_v, peak_v, ecc_t, pause_t, con_t, tot_t, state, millis());

        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(1500); // 1.5s timeout to avoid blocking loop too long
        
        int httpResponseCode = http.POST(payload);
        
        if (httpResponseCode > 0) {
            Serial.printf("[Cloud] POST rep - OK (%d)\n", httpResponseCode);
        } else {
            Serial.printf("[Cloud] POST rep - Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        
        http.end();
    }

    void postHeartbeat(float velocity, int state) {
        if (WiFi.status() != WL_CONNECTED) return;

        HTTPClient http;
        String url = String(BACKEND_URL) + "/api/ingest";
        
        char payload[128];
        snprintf(payload, sizeof(payload), 
            "{\"device_id\":\"%s\",\"type\":\"heartbeat\",\"velocity\":%.3f,\"state\":%d,\"timestamp\":%lu}",
            DEVICE_ID, velocity, state, millis());

        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(500); // Quick timeout for heartbeat
        
        int httpResponseCode = http.POST(payload);
        http.end();
    }
};

extern CloudClient cloud;

#endif
