/**
 * Barbell Velocity Tracker V1 — Main Sketch
 * 
 * Hardware: ESP32-S3 + MPU6500 (SPI)
 * IDE: Arduino IDE
 * 
 * ═══════════════════════════════════════════════════════════
 * ARDUINO IDE SETUP:
 * 
 * 1. Install ESP32 board package:
 *    File → Preferences → Additional Board Manager URLs:
 *    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 * 
 * 2. Board Manager → Install "esp32" by Espressif
 * 
 * 3. Select Board:
 *    Tools → Board → ESP32S3 Dev Module
 * 
 * 4. Board Settings:
 *    - USB CDC On Boot: Enabled
 *    - USB Mode: Hardware CDC and JTAG
 *    - Flash Size: 4MB (or whatever your board has)
 *    - Partition Scheme: Default 4MB with spiffs
 *    - Upload Speed: 921600
 * 
 * 5. Select Port:
 *    Tools → Port → (your ESP32-S3 COM port)
 * ═══════════════════════════════════════════════════════════
 * 
 * Phase 1: Raw Sensor Validation
 * - Initialize MPU6500 over SPI
 * - Verify WHO_AM_I (counterfeit detection)
 * - Configure accel ±8g, gyro ±500°/s, 100Hz, DLPF 41Hz
 * - Interrupt-driven data acquisition
 * - Stream raw data over USB CDC serial
 * 
 * Serial Output Format:
 *   timestamp_ms,ax,ay,az,gx,gy,gz,temp
 *   (acceleration in m/s², gyroscope in °/s, temperature in °C)
 */

#include <SPI.h>
#include <esp_task_wdt.h>
#include "mpu6500.h"
#include "mpu6500_registers.h"
#include "timing.h"
#include "MadgwickAHRS.h"
#include "rep_detector.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "webpage.h"
#include <vector>
#include "cloud_client.h"

CloudClient cloud;

// ─── Configuration ───────────────────────────────────────────────────
#define SERIAL_BAUD            115200   // Ignored for native USB CDC, kept for fallback
#define WATCHDOG_TIMEOUT_S     5        // Hardware watchdog timeout (seconds)
#define SENSOR_CHECK_MS        5000     // Periodic WHO_AM_I health check interval
#define STARTUP_DELAY_MS       2000     // Wait for USB CDC enumeration
#define STATS_INTERVAL_MS      5000     // Print sampling stats every N ms

// ─── Global Objects ──────────────────────────────────────────────────
MPU6500  imu;
Timing   timer;
Madgwick filter;
RepDetector rep_engine;
WebServer server(80);

// ─── Shared Thread-Safe Metrics for Dual-Core ────────────────────────
struct SharedMetrics {
    float velocity;
    float position;
    uint32_t rep_count;
    int state;
    float peak_velocity;
} sharedMetrics;

SemaphoreHandle_t sharedMetricsMutex = NULL;
std::vector<WiFiClient> sseClients;

// ─── Statistics ──────────────────────────────────────────────────────
uint32_t sampleCount       = 0;
uint32_t totalSamples      = 0;
uint32_t lastStatsTime     = 0;
uint32_t lastSensorCheck   = 0;
float    minDeltaTime      = 999.0f;
float    maxDeltaTime      = 0.0f;
float    sumDeltaTime      = 0.0f;

// ─── State ───────────────────────────────────────────────────────────
bool sensorOk = false;

// ─── Velocity Tracking ───────────────────────────────────────────────
float velocity = 0.0f;
float position = 0.0f; // Track vertical displacement
float prev_vert_acc = 0.0f;
float vert_acc_bias = 0.0f; // Dynamic bias to cancel gravity leaks

uint32_t stationary_start_time = 0;
bool is_stationary = false;

// Previous values for Jerk (change in acceleration) calculation
float prev_ax = 0.0f, prev_ay = 0.0f, prev_az = 0.0f;

// Robust ZUPT variables using Exponential Moving Average (EMA)
float jerk_ema = 0.0f;
float gyro_ema = 0.0f;
uint32_t consecutive_stationary_samples = 0;

// Warmup: give Madgwick filter time to converge before integrating velocity
uint32_t warmup_samples = 0;
const uint32_t WARMUP_PERIOD = 200; // 200 samples = 2 seconds at 100Hz


// ─── SSE and WebServer Core 0 Handling ───────────────────────────────
void handleSSE() {
    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/event-stream");
    client.println("Cache-Control: no-cache");
    client.println("Connection: keep-alive");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.flush();
    
    // Lock mutex before modifying shared vector
    if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sseClients.push_back(client);
        xSemaphoreGive(sharedMetricsMutex);
        Serial.println("[SSE] New client connected");
    }
}

void broadcastEvent(const char* data) {
    if (sseClients.empty()) return;
    
    auto it = sseClients.begin();
    while (it != sseClients.end()) {
        if (it->connected()) {
            it->printf("data: %s\n\n", data);
            it->flush();
            ++it;
        } else {
            it->stop();
            it = sseClients.erase(it);
            Serial.println("[SSE] Client disconnected");
        }
    }
}

void webServerTask(void *pvParameters) {
    // WiFi mode is already set to WIFI_AP_STA in setup().
    // We only need to start the soft-AP here.
    Serial.println("[WIFI] Starting Access Point: Barbell_VBT");
    WiFi.softAP("Barbell_VBT"); // Open network, no password
    Serial.print("[WIFI] Connect to AP and visit: http://");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });

    server.on("/data", HTTP_GET, []() {
        char json[128];
        if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            snprintf(json, sizeof(json), "{\"v\":%.3f,\"r\":%lu,\"s\":%d,\"p\":%.3f}", 
                sharedMetrics.velocity, sharedMetrics.rep_count, sharedMetrics.state, sharedMetrics.peak_velocity);
            xSemaphoreGive(sharedMetricsMutex);
        } else {
            snprintf(json, sizeof(json), "{\"v\":0.0,\"r\":0,\"s\":0,\"p\":0.0}");
        }
        server.send(200, "application/json", json);
    });

    server.on("/events", HTTP_GET, handleSSE);

    server.on("/calibrate", HTTP_POST, []() {
        if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            imu.calibrate(500);
            timer.reset();
            xSemaphoreGive(sharedMetricsMutex);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(503, "text/plain", "Busy");
        }
    });

    server.on("/set_mode", HTTP_POST, []() {
        if (server.hasArg("mode")) {
            int mode = server.arg("mode").toInt();
            if (mode >= 0 && mode <= 2) {
                if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    rep_engine.setMode((LiftMode)mode);
                    xSemaphoreGive(sharedMetricsMutex);
                    server.send(200, "text/plain", "Mode updated");
                    return;
                }
            }
        }
        server.send(400, "text/plain", "Invalid mode");
    });

    server.on("/rep_profile", HTTP_GET, []() {
        String json = "[";
        if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            const DataPoint* profile = rep_engine.getProfile();
            uint16_t count = rep_engine.getProfileCount();
            for (uint16_t i = 0; i < count; i++) {
                json += "[";
                json += String(profile[i].position, 3);
                json += ",";
                json += String(profile[i].velocity, 3);
                json += "]";
                if (i < count - 1) json += ",";
            }
            xSemaphoreGive(sharedMetricsMutex);
        }
        json += "]";
        server.send(200, "application/json", json);
    });

    server.on("/wifi", HTTP_GET, []() {
        server.send(200, "text/html", wifi_config_html);
    });

    server.on("/save_wifi", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        String backend = server.arg("backend");
        
        cloud.saveWiFiConfig(ssid, pass, backend);
        
        server.send(200, "text/html", "<h2>Saved! Restarting ESP32...</h2><p>Please wait 5 seconds, then connect to your configured WiFi.</p>");
        delay(1000);
        ESP.restart();
    });

    server.begin();
    Serial.println("[WIFI] HTTP server started on port 80");

    if (MDNS.begin("vbt")) {
        Serial.println("[WIFI] mDNS responder started. You can now access: http://vbt.local");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("[WIFI] Error setting up mDNS responder!");
    }

    uint32_t lastBroadCast = 0;

    while (true) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ═════════════════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════════════════
void setup() {
    // Initialize USB CDC serial
    Serial.begin(SERIAL_BAUD);

    // Wait for USB CDC to enumerate (important for ESP32-S3 native USB)
    delay(STARTUP_DELAY_MS);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  Barbell Velocity Tracker V1");
    Serial.println("  Phase 1: Raw Sensor Validation");
    Serial.println("  ESP32-S3 + MPU6500 (SPI)");
    Serial.println("========================================");
    Serial.println();

    // Initialize MPU6500
    Serial.println("[INIT] Initializing MPU6500 over SPI...");
    sensorOk = imu.begin();

    if (!sensorOk) {
        Serial.println();
        Serial.println("========================================");
        Serial.println("  !! SENSOR INITIALIZATION FAILED !!   ");
        Serial.println("========================================");
        Serial.println("  Check the following:");
        Serial.println("  1. SPI wiring (SCLK, MOSI, MISO, CS)");
        Serial.println("  2. Power supply (3.3V to VCC)");
        Serial.println("  3. Sensor module authenticity");
        Serial.println("  4. Solder joints / breadboard contacts");
        Serial.println("========================================");
        Serial.println();
        Serial.println("[INIT] Entering error loop — reset to retry");

        #ifdef LED_BUILTIN
        pinMode(LED_BUILTIN, OUTPUT);
        #endif

        while (true) {
            #ifdef LED_BUILTIN
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            #endif
            delay(250);
        }
    }

    // ── WiFi: set mode ONCE here, before cloud.begin() ──────────────────
    // WIFI_AP_STA lets the soft-AP (for the dashboard) and STA (for cloud)
    // coexist on the same radio.  cloud.begin() must NOT call WiFi.mode().
    WiFi.mode(WIFI_AP_STA);

    // ── Watchdog: init BEFORE cloud.begin() with a generous timeout ──────
    // cloud.begin() may block up to WIFI_CONNECT_TIMEOUT_MS (8 s) while
    // trying to connect.  Use 15 s so the WDT doesn't fire during that window.
    // After setup() the loop task resets the WDT every cycle (~10 ms).
    esp_task_wdt_init(20, true);  // 20s: covers WiFi scan (~3s) + connect timeout (12s)
    esp_task_wdt_add(NULL);

    // ── Initialize Cloud Connection (WiFi STA) ───────────────────────────
    cloud.begin();

    // ── Reduce WDT to operational timeout after WiFi attempt ────────────
    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);

    Serial.println();
    Serial.println("[INIT] Setup complete — streaming data at 100Hz");
    Serial.println("[INIT] Type 'c' and press Enter to run calibration");
    Serial.println("[INIT] Format: D,timestamp_ms,vert_acc,velocity,is_stationary,state");
    Serial.println();

    // Print CSV header
    Serial.println("# D,timestamp_ms,vert_acc(m/s2),velocity(m/s),is_stationary,state");

    // Initialize components
    filter.begin(100.0f);
    timer.reset();
    lastStatsTime   = millis();
    lastSensorCheck = millis();

    // Create FreeRTOS Mutex before spawning the WebServer task
    sharedMetricsMutex = xSemaphoreCreateMutex();
    if (sharedMetricsMutex != NULL) {
        sharedMetrics.velocity = 0.0f;
        sharedMetrics.position = 0.0f;
        sharedMetrics.rep_count = 0;
        sharedMetrics.state = 0;
        sharedMetrics.peak_velocity = 0.0f;

        // Spawn WebServer task on Core 0
        // Stack increased to 12 KB: mDNS + WebServer + SSE vector + String ops
        // can easily overflow an 8 KB stack, causing silent crashes.
        xTaskCreatePinnedToCore(
            webServerTask,
            "WebServerTask",
            12288,  // 12 KB — was 8 KB (too small for mDNS + SSE)
            NULL,
            1,
            NULL,
            0 // Pin to Core 0
        );
        Serial.println("[INIT] WebServer and WiFi AP successfully offloaded to Core 0");
    } else {
        Serial.println("[INIT] ERROR: Failed to create thread synchronization mutex!");
    }
}

// ═════════════════════════════════════════════════════════════════════
// MAIN LOOP
// ═════════════════════════════════════════════════════════════════════
void loop() {
    // Feed the watchdog
    esp_task_wdt_reset();

    // Check for serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'c' || cmd == 'C') {
            if (xSemaphoreTake(sharedMetricsMutex, portMAX_DELAY) == pdTRUE) {
                imu.calibrate(500); // Take 500 samples (5 seconds at 100Hz)
                timer.reset();      // Reset timer so deltaTime doesn't spike
                xSemaphoreGive(sharedMetricsMutex);
            }
        }
    }

    uint32_t now = millis();

    // ═════════════════════════════════════════════════════════════════════
    // NETWORK & HEALTH TASKS (Runs independently of sensor interrupts)
    // ═════════════════════════════════════════════════════════════════════
    
    // ─── Non-blocking WiFi reconnect (every 30s when disconnected) ──
    cloud.reconnectIfNeeded();

    // ─── Periodic Heartbeat for Cloud Dashboard (every 2s) ───────
    static uint32_t lastHeartbeat = 0;
    if (now - lastHeartbeat >= 2000) {
        float current_vel = 0.0f;
        int current_state = 0;
        if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            current_vel = sharedMetrics.velocity;
            current_state = sharedMetrics.state;
            xSemaphoreGive(sharedMetricsMutex);
        }
        cloud.postHeartbeat(current_vel, current_state);
        lastHeartbeat = now;
    }

    // ─── Periodic sensor health check ────────────────────────────
    if (now - lastSensorCheck >= SENSOR_CHECK_MS) {
        checkSensorHealth();
        lastSensorCheck = now;
    }

    // ═════════════════════════════════════════════════════════════════════
    // SENSOR PROCESSING TASKS (Runs at 100Hz on interrupt)
    // ═════════════════════════════════════════════════════════════════════

    // Wait for Data Ready interrupt from MPU6500
    if (!imu.isDataReady()) {
        // Fallback: If interrupt is missed (should fire every 10ms), poll the sensor manually
        if (millis() - timer.getTimestampMs() > 20 && timer.getTimestampMs() > 0) {
            if (imu.readIntStatus() & 0x01) {
                MPU6500::dataReadyFlag = true; // Force flag to recover
            }
        }
        return;  // No new data — exit loop immediately
    }

    if (xSemaphoreTake(sharedMetricsMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        // Clear the interrupt flag
        imu.clearDataReady();

        // Update timing
        timer.update();
        float dt = timer.getDeltaTime();

        // Read and scale sensor data
        MPU6500ScaledData data;
        imu.readScaledData(&data);

        // Also clear the hardware interrupt status register
        imu.readIntStatus();

        // ─── Warmup counter ─────────────────────────────────────────
        if (warmup_samples < WARMUP_PERIOD) {
            warmup_samples++;
            // During warmup keep beta HIGH so orientation converges rapidly
            filter.setBeta(0.5f);
        }

        // ─── Phase 3 & 4: Orientation and Gravity Compensation ───────
        // Update Madgwick filter with scaled data
        filter.updateIMU(data.gyro_x, data.gyro_y, data.gyro_z, 
                         data.accel_x, data.accel_y, data.accel_z);
                         
        float q0, q1, q2, q3;
        filter.getQuaternion(&q0, &q1, &q2, &q3);
        
        // Calculate expected gravity vector in sensor frame
        // Assume Earth gravity vector is [0, 0, 9.80665]
        float gx = 2.0f * (q1 * q3 - q0 * q2) * GRAVITY_MS2;
        float gy = 2.0f * (q0 * q1 + q2 * q3) * GRAVITY_MS2;
        float gz = (q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3) * GRAVITY_MS2;
        
        // Calculate linear acceleration (motion without gravity)
        float lin_ax = data.accel_x - gx;
        float lin_ay = data.accel_y - gy;
        float lin_az = data.accel_z - gz;

        // ─── Phase 7: Motion Axis Selection ──────────────────────────
        // Per user request, strictly use the sensor's Z-axis (z+ and z-) 
        // for vertical motion instead of omni-directional gravity projection.
        // lin_az already has gravity removed by the Madgwick filter.
        // Positive lin_az = Accelerating UP, Negative lin_az = Accelerating DOWN.
        float vert_acc_raw = lin_az;

        // ─── Phase 9: ZUPT & Dynamic Bias (Drift Reset) ──────────────
        // Calculate "Jerk" (change in acceleration) to detect if stationary regardless of offset errors
        float delta_ax = data.accel_x - prev_ax;
        float delta_ay = data.accel_y - prev_ay;
        float delta_az = data.accel_z - prev_az;
        float jerk_mag = sqrtf(delta_ax*delta_ax + delta_ay*delta_ay + delta_az*delta_az);
        float gyro_mag = sqrtf(data.gyro_x*data.gyro_x + data.gyro_y*data.gyro_y + data.gyro_z*data.gyro_z);
        
        prev_ax = data.accel_x;
        prev_ay = data.accel_y;
        prev_az = data.accel_z;

        // Filter jerk and gyro values using EMA to remove high-frequency noise spikes
        jerk_ema = jerk_ema * 0.9f + jerk_mag * 0.1f;
        gyro_ema = gyro_ema * 0.9f + gyro_mag * 0.1f;

        // Robust stationary detection using smoothed EMA metrics
        bool is_currently_still = (jerk_ema < 0.20f && gyro_ema < 4.0f);

        if (is_currently_still) {
            consecutive_stationary_samples++;
            
            // Dynamically auto-calibrate at boot if resting flat for ~1.5s
            if (consecutive_stationary_samples >= 150 && !imu.getCalibration().is_calibrated) {
                Serial.println();
                Serial.println("[AUTO-CALIBRATION] Stable sensor detected. Calibrating in background...");
                imu.calibrate(100); // Quick 1-second auto-calibration
                timer.reset();
                consecutive_stationary_samples = 0;
            }

            if (stationary_start_time == 0) {
                stationary_start_time = timer.getTimestampMs();
            } else if (timer.getTimestampMs() - stationary_start_time > 150) { 
                is_stationary = true;
                // Slowly adjust the dynamic bias to cancel gravity leaks
                vert_acc_bias = vert_acc_bias * 0.98f + vert_acc_raw * 0.02f;
            }
        } else {
            consecutive_stationary_samples = 0;
            stationary_start_time = 0;
            is_stationary = false;
        }

        // Dynamic Beta: set for NEXT sample (only after warmup has finished)
        if (warmup_samples >= WARMUP_PERIOD) {
            if (is_stationary) {
                filter.setBeta(0.25f);
            } else {
                filter.setBeta(0.015f);
            }
        }

        // Apply the dynamic bias
        float vert_acc = vert_acc_raw - vert_acc_bias;

        // Apply a small acceleration deadband to block noise from integrating into velocity
        if (fabsf(vert_acc) < 0.05f) {
            vert_acc = 0.0f;
        }

        // ─── Phase 8: Velocity Estimation ────────────────────────────
        if (warmup_samples < WARMUP_PERIOD) {
            // Still warming up — don't integrate, just track acceleration for bias
            velocity = 0.0f;
            position = 0.0f;
        } else if (is_stationary) {
            velocity = 0.0f; // Reset drift
            position = 0.0f; // Reset position baseline between reps
        } else {
            // Pure trapezoidal integration
            velocity += 0.5f * (prev_vert_acc + vert_acc) * dt;
            position += velocity * dt; // Integrate velocity for position
            
            // Safe clamping to prevent runaway velocity integration
            if (velocity > 3.0f) velocity = 3.0f;
            else if (velocity < -3.0f) velocity = -3.0f;
        }
        prev_vert_acc = vert_acc;

        // ─── Stream data as CSV ──────────────────────────────────────
        Serial.printf("D,%lu,%.3f,%.3f,%d,%d\n",
            timer.getTimestampMs(),
            vert_acc,
            velocity,
            is_stationary ? 1 : 0,
            (int)rep_engine.getState()
        );

        // ─── Phase 10 & 11: Rep Detection & Metrics ──────────────────
        if (rep_engine.update(velocity, position, is_stationary, timer.getTimestampMs())) {
            RepMetrics rep = rep_engine.getLastRep();
            Serial.println();
            Serial.println("==================================================");
            Serial.printf("  REP %lu COMPLETED!\n", rep.rep_count);
            Serial.printf("  Peak Velocity  : %.2f m/s\n", rep.peak_velocity);
            Serial.printf("  Avg Velocity   : %.2f m/s\n", rep.avg_velocity);
            Serial.printf("  Eccentric Time : %.2f s\n", rep.ecc_time);
            Serial.printf("  Pause Time     : %.2f s\n", rep.pause_time);
            Serial.printf("  Concentric Time: %.2f s\n", rep.con_time);
            Serial.printf("  Total Time     : %.2f s\n", rep.total_time);
            Serial.println("==================================================");
            
            // Machine-readable format
            Serial.printf("R,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                rep.rep_count, rep.peak_velocity, rep.avg_velocity, 
                rep.ecc_time, rep.pause_time, rep.con_time, rep.total_time);
            Serial.println();

            // Cloud sync
            String modeStr = "squat";
            if (rep_engine.getMode() == LIFT_BENCH) modeStr = "bench";
            else if (rep_engine.getMode() == LIFT_DEADLIFT) modeStr = "deadlift";
            
            cloud.postRepData(modeStr.c_str(), rep.rep_count, rep.avg_velocity, rep.peak_velocity, rep.ecc_time, rep.pause_time, rep.con_time, rep.total_time, (int)rep_engine.getState(), rep_engine.getProfile(), rep_engine.getProfileCount());
        }

        // ─── Update statistics ───────────────────────────────────────
        sampleCount++;
        totalSamples++;
        sumDeltaTime += dt;
        if (dt < minDeltaTime && dt > 0.001f) minDeltaTime = dt;
        if (dt > maxDeltaTime) maxDeltaTime = dt;

        // Update the SharedMetrics structure
        sharedMetrics.velocity = velocity;
        sharedMetrics.position = position;
        sharedMetrics.rep_count = rep_engine.getLastRep().rep_count;
        sharedMetrics.state = (int)rep_engine.getState();
        sharedMetrics.peak_velocity = rep_engine.getLastRep().peak_velocity;

        // ─── Periodic statistics output ──────────────────────────────
        if (now - lastStatsTime >= STATS_INTERVAL_MS) {
            printStats();
            sampleCount  = 0;
            minDeltaTime = 999.0f;
            maxDeltaTime = 0.0f;
            sumDeltaTime = 0.0f;
            lastStatsTime = now;
        }

        // Release the mutex
        xSemaphoreGive(sharedMetricsMutex);
    }
}

// ═════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═════════════════════════════════════════════════════════════════════
void printStats() {
    if (sampleCount == 0) return;

    float avgDt   = sumDeltaTime / sampleCount;
    float avgFreq = (avgDt > 0) ? 1.0f / avgDt : 0.0f;
    float minFreq = (maxDeltaTime > 0) ? 1.0f / maxDeltaTime : 0.0f;
    float maxFreq = (minDeltaTime > 0 && minDeltaTime < 999.0f) ? 1.0f / minDeltaTime : 0.0f;

    Serial.println();
    Serial.println("# -- Statistics ---------------------------");
    Serial.printf( "#   Samples (window) : %lu\n", sampleCount);
    Serial.printf( "#   Samples (total)  : %lu\n", totalSamples);
    Serial.printf( "#   Avg Frequency    : %.1f Hz\n", avgFreq);
    Serial.printf( "#   Min Frequency    : %.1f Hz\n", minFreq);
    Serial.printf( "#   Max Frequency    : %.1f Hz\n", maxFreq);
    Serial.printf( "#   Avg DeltaTime    : %.4f s\n", avgDt);
    Serial.printf( "#   Min DeltaTime    : %.4f s\n", minDeltaTime < 999.0f ? minDeltaTime : 0.0f);
    Serial.printf( "#   Max DeltaTime    : %.4f s\n", maxDeltaTime);
    Serial.println("# -----------------------------------------");
    Serial.println();
}

void checkSensorHealth() {
    uint8_t id = imu.whoAmI();
    if (id != 0x70) {
        Serial.printf("# [WARNING] Sensor health check failed! WHO_AM_I = 0x%02X (expected 0x70)\n", id);
        Serial.println("# [WARNING] Sensor may have disconnected — check wiring");
    }
}
