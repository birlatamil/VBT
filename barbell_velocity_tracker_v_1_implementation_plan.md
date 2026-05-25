# Barbell Velocity Tracker V1 — Implementation Plan

## Project Goal
Build a wired barbell tracking system using:
- ESP32-S3
- MPU6500 (via SPI)
- USB Serial (Native USB CDC)

The system should measure:
- rep count
- concentric/eccentric phases
- pause duration
- peak velocity
- average velocity
- tempo

Supported lifts:
- squat
- bench press
- deadlift

V1 intentionally excludes:
- BLE
- battery operation
- full 3D bar path
- precise position tracking
- mobile app

---

# System Overview

```text
MPU6500 —[SPI]→ ESP32-S3 —[Native USB CDC]→ PC
```

ESP32-S3 Responsibilities:
- sensor acquisition (SPI + interrupt-driven)
- hardware + software filtering
- orientation estimation (Madgwick)
- gravity compensation (quaternion rotation)
- velocity estimation (trapezoidal integration)
- drift correction (ZUPT)
- rep detection (state machine)
- serial transmission (Native USB CDC)

PC Responsibilities:
- visualization (pyqtgraph)
- graphing
- debugging
- logging
- future analytics

---

# Hardware Requirements

| Component | Purpose | Notes |
|---|---|---|
| ESP32-S3 DevKit | Main controller | Dual-core LX7 @ 240MHz, native USB CDC, 512KB SRAM + 8MB PSRAM |
| MPU6500 | 6-axis IMU (accel + gyro) | Use SPI interface; verify WHO_AM_I = 0x70 |
| USB-C Cable | Power + serial communication | Native USB — baud rate is irrelevant |
| Mounting Clamp/Velcro | Attach sensor to barbell | Must be rigid, no wobble |

## MPU6500 Authenticity Check

> **WARNING:** The MPU6500 is frequently counterfeited. Many cheap modules contain relabeled MPU6050s.
>
> **Verify by reading register `WHO_AM_I` (0x75):**
> - `0x70` → Genuine MPU6500 ✓
> - `0x68` → MPU6050 (counterfeit) ✗

## Sensor Configuration

| Parameter | Setting | Reason |
|---|---|---|
| Accelerometer Range | **±8g** | Best balance — squats/bench/deadlift can exceed ±4g during explosive reps |
| Gyroscope Range | **±500°/s** | Sufficient for barbell rotations; ±250°/s saturates during fast transitions |
| Sampling Rate | **100 Hz** (V1) | Industry standard for commercial VBT devices; upgrade to 200Hz if needed |
| Communication | **SPI @ 1–4 MHz** | I2C is too slow for reliable 100Hz+ polling; SPI has negligible latency |
| FIFO | **Enabled** | 512-byte internal buffer prevents missed samples during processing |

## SPI Wiring (ESP32-S3 ↔ MPU6500)

| MPU6500 Pin | ESP32-S3 Pin | Function |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SCL/SCLK | GPIO 36 (FSPI_CLK) | SPI Clock |
| SDA/MOSI | GPIO 35 (FSPI_MOSI) | Master Out Slave In |
| ADO/MISO | GPIO 37 (FSPI_MISO) | Master In Slave Out |
| NCS | GPIO 10 | Chip Select (any free GPIO) |
| INT | GPIO 4 | Data Ready interrupt (any free GPIO) |

> **Note:** Pin assignments are flexible on ESP32-S3. Adjust based on your dev board layout. Using FSPI (SPI2) is recommended; HSPI (SPI3) is also available.

---

# Development Philosophy

Do NOT attempt:
- accurate 3D tracking
- unrestricted spatial reconstruction
- full biomechanics

Instead:
- focus on one dominant movement axis
- reset drift frequently (ZUPT)
- prioritize stable velocity trends
- validate data visually at every phase

This dramatically improves:
- implementation speed
- stability
- debugging simplicity

---

# Recommended Development Stack

| Part | Tool | Notes |
|---|---|---|
| Firmware | Arduino IDE | With ESP32-S3 board package from Espressif |
| Visualization | Python + pyqtgraph | pyqtgraph uses OpenGL — handles 100Hz+ real-time data; **do NOT use matplotlib for real-time** |
| Serial Debugging | Arduino Serial Monitor | |
| Logging | CSV | |

## Arduino IDE Setup

1. **Install ESP32 board package:**
   - File → Preferences → Additional Board Manager URLs:
   - `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. **Board Manager** → Install **"esp32" by Espressif**
3. **Select Board:** Tools → Board → **ESP32S3 Dev Module**
4. **Board Settings:**
   - USB CDC On Boot: **Enabled**
   - USB Mode: **Hardware CDC and JTAG**
   - Flash Size: 4MB (or match your board)
   - Partition Scheme: Default 4MB with spiffs
   - Upload Speed: 921600
5. **Select Port:** Tools → Port → (your ESP32-S3 COM port)

> **Note:** With native USB CDC, the baud rate set in `Serial.begin()` is ignored. USB 2.0 Full-Speed runs at ~1–7 Mbit/s real-world throughput regardless of the value you pass.

---

# Project Structure

```text
project/
├── firmware/
│   └── barbell_vbt/             # Arduino sketch folder
│       ├── barbell_vbt.ino      # Main sketch
│       ├── mpu6500.h            # MPU6500 driver header
│       ├── mpu6500.cpp          # MPU6500 driver implementation
│       ├── mpu6500_registers.h  # Register map + constants
│       ├── timing.h             # Timing utility header
│       └── timing.cpp           # Timing utility implementation
├── python_visualizer/
│   ├── serial_reader.py
├── logs/
├── docs/
└── README.md
```

---

# Phase 1 — Raw Sensor Validation

## Goal
Verify stable MPU6500 readings over SPI.

## Tasks
- initialize SPI bus on ESP32-S3
- configure MPU6500 registers (accel ±8g, gyro ±500°/s)
- **verify WHO_AM_I register = 0x70** (counterfeit check)
- read accelerometer values
- read gyroscope values
- confirm axis orientation (which axis is vertical when mounted)
- confirm stable sampling rate (100Hz)
- enable MPU6500 Data Ready interrupt on INT pin

## Serial Output

```text
timestamp,ax,ay,az,gx,gy,gz
```

## Success Criteria
- WHO_AM_I returns 0x70
- stable readings at rest
- no random spikes
- expected axis movement when tilted/moved
- consistent 100Hz sampling (verify with timestamp deltas)

---

# Phase 2 — Sensor Calibration

## Goal
Remove sensor bias and ensure thermal stability.

## Calibration Procedure
1. **Power on and wait 30–60 seconds** for sensor thermal warm-up
2. Keep sensor completely stationary on a flat surface
3. Collect 500 samples (5 seconds at 100Hz)
4. Average all readings
5. Compute offsets:
   - Gyro offset = average reading (should be near zero)
   - Accel offset = average reading minus expected gravity on the vertical axis
6. Store offsets in memory
7. Subtract offsets from all live data

> **Why thermal warm-up matters:** The MPU6500 gyroscope has significant temperature-dependent bias drift. Calibrating immediately at boot captures a bias value that becomes stale as the sensor warms up from the ESP32-S3's heat output, causing drift during your workout.

## Required Calibration

### Accelerometer
- offset correction per axis
- verify gravity reads ~9.81 m/s² on vertical axis after calibration

### Gyroscope
- zero-drift calibration per axis
- verify all axes read < 0.01 °/s when stationary after calibration

## Success Criteria
- gyro reads near zero on all axes when still
- accel reads [0, 0, 9.81] (or equivalent based on orientation) when still
- values remain stable for 60+ seconds after calibration

---

# Phase 3 — Orientation Estimation

## Goal
Estimate sensor orientation using sensor fusion.

## Recommended Algorithm
Use:
- **Madgwick Filter** (6-DOF variant, no magnetometer)

Alternative:
- Complementary Filter (simpler but less accurate)

## Madgwick Filter Notes
- Set beta = 0.1 as starting point (tune empirically)
- Output is a **quaternion** (q0, q1, q2, q3) — not Euler angles
- Quaternion avoids gimbal lock and is needed for gravity rotation in Phase 4
- Several Arduino libraries available: `MadgwickAHRS`, `Adafruit_AHRS`

## Outputs

```text
q0,q1,q2,q3    (quaternion — primary, used internally)
roll,pitch,yaw  (Euler angles — for debugging/visualization only)
```

## Why This Is Needed
Orientation estimation allows:
- gravity direction estimation in sensor frame
- gravity compensation (Phase 4)
- stable motion analysis regardless of sensor mounting angle

---

# Phase 4 — Gravity Compensation

## Problem
Accelerometer measures total acceleration including gravity.

```text
total_acceleration = movement + gravity
```

## Goal
Compute:

```text
linear_acceleration = total_acceleration - gravity_in_sensor_frame
```

## Method — Quaternion Gravity Rotation

Use the orientation quaternion from Madgwick (Phase 3) to rotate the known gravity vector into the sensor's coordinate frame, then subtract it.

```c
// Step 1: Define gravity in Earth frame
float gravity_earth[3] = {0.0, 0.0, 9.81};

// Step 2: Rotate gravity into sensor frame using quaternion
float gx = 2.0 * (q1*q3 - q0*q2) * 9.81;
float gy = 2.0 * (q0*q1 + q2*q3) * 9.81;
float gz = (q0*q0 - q1*q1 - q2*q2 + q3*q3) * 9.81;

// Step 3: Subtract to get linear acceleration
float lin_ax = raw_ax - gx;
float lin_ay = raw_ay - gy;
float lin_az = raw_az - gz;
```

> **Critical:** If this step is wrong, gravity "leaks" into linear acceleration and velocity integration will explode. Validate thoroughly.

## Phase 4.5 — Gravity Compensation Sanity Check

Before proceeding to Phase 5, validate gravity compensation:

1. **Hold sensor still** → linear acceleration should read ≈ 0 on all axes
2. **Move sensor slowly up/down** → only vertical axis should show acceleration
3. **Rotate sensor 90°** → linear acceleration should still read ≈ 0 when stationary
4. **Tilt sensor at 45°** → gravity compensation should still correctly isolate motion

If any of these fail, do NOT proceed. Fix the orientation estimation or gravity rotation first.

## Expected Result
Motion data becomes dramatically cleaner.

## Success Criteria
- stationary sensor outputs near-zero linear acceleration (< 0.05 m/s²)
- movement spikes become clear and directional
- sanity checks pass in all orientations

---

# Phase 5 — Live Visualization

## Goal
Visualize data before advanced processing.

## Recommended Tools
- Python
- **pyqtgraph** (OpenGL-accelerated, handles 100Hz+ real-time data)

> **Do NOT use matplotlib** for real-time visualization. It redraws the entire figure each frame and will drop below 10 FPS at 100Hz data rates.

## Visualize (separate subplots)
- raw acceleration (3 axes)
- linear acceleration (gravity-compensated)
- gyro data (3 axes)
- orientation (roll, pitch, yaw)

## Why This Is Critical
Visual debugging quickly reveals:
- noise patterns
- calibration drift
- gravity compensation errors
- incorrect filtering
- axis misalignment

---

# Phase 6 — Filtering Pipeline

## Goal
Reduce sensor noise using both hardware and software filters.

## Hardware Filter (MPU6500 Built-in DLPF)

The MPU6500 has a configurable Digital Low-Pass Filter. Configure it **before** applying software filters.

### Recommended DLPF Settings for VBT at 100Hz

| Sensor | Register | Setting | Bandwidth | Delay |
|---|---|---|---|---|
| Accelerometer | ACCEL_CONFIG2 | A_DLPF_CFG = 3 | ~41 Hz | 11.8 ms |
| Gyroscope | CONFIG | DLPF_CFG = 3 | ~41 Hz | 4.8 ms |

This removes high-frequency noise from barbell vibrations, plate rattling, and knurling resonance **in hardware** before the data reaches your code.

## Software Filters

### Low-pass Filter (Butterworth or simple IIR)
Smooth remaining high-frequency noise.

```c
// Simple first-order IIR low-pass filter
float alpha = 0.2;  // tune: lower = smoother, higher = more responsive
filtered = alpha * raw + (1.0 - alpha) * prev_filtered;
```

### Moving Average
Window:

```text
5–10 samples (50–100 ms at 100Hz)
```

## Avoid Initially
- Kalman filters (complex, hard to tune)
- advanced probabilistic filters

---

# Phase 7 — Motion Axis Selection

## Goal
Reduce complexity and drift by tracking a single dominant axis.

## Strategy
After gravity compensation, extract only the **vertical component** of linear acceleration.

| Lift | Primary Axis | Notes |
|---|---|---|
| Squat | Vertical (Earth Z) | Bar moves up/down |
| Deadlift | Vertical (Earth Z) | Bar moves up/down |
| Bench Press | Vertical (Earth Z) | Bar moves up/down (relative to gravity) |

## How to Extract Vertical Acceleration

Use the orientation quaternion to rotate linear acceleration into the Earth frame, then take only the Z component:

```c
// Rotate linear_acc from sensor frame to Earth frame using quaternion
float vertical_acc = /* Z component of rotated linear acceleration */;
```

## Benefits
- simpler math
- less drift (integrating 1 axis instead of 3)
- more stable velocity estimates
- easier debugging

---

# Phase 8 — Velocity Estimation

## Goal
Estimate bar velocity from vertical linear acceleration.

Velocity from acceleration:

```text
v(t) = v(t-1) + ∫ a(t) dt
```

## Implementation — Trapezoidal Integration

```c
// Trapezoidal integration (significantly more accurate than Euler)
velocity += 0.5f * (prev_acceleration + acceleration) * deltaTime;
prev_acceleration = acceleration;
```

> **Why trapezoidal, not Euler?** Euler integration (`v += a * dt`) assumes acceleration is constant between samples. During explosive barbell movements, acceleration changes rapidly. Trapezoidal integration assumes linear change between samples — same computational cost, significantly better accuracy. This is the industry standard for VBT devices.

## Important Notes
- use consistent time steps (interrupt-driven sampling ensures this)
- only estimate short-term velocity (within a single rep)
- apply ZUPT reset between reps (Phase 9)
- avoid integrating for more than ~5 seconds without a zero-velocity reset

## Metrics
Track:
- peak velocity (max absolute velocity during concentric phase)
- average velocity (mean velocity during concentric phase)
- movement speed trends (rep-to-rep comparison)

---

# Phase 9 — Drift Reset Logic (ZUPT)

## Critical Feature — Zero Velocity Update (ZUPT)
Velocity drift accumulates quickly with numerical integration. ZUPT is the primary correction mechanism.

Whenever motion stops:

```c
if (is_stationary) {
    velocity = 0.0;
}
```

## Stationary Detection Conditions
All must be true simultaneously for a minimum duration:

| Condition | Threshold | Duration |
|---|---|---|
| abs(linear_acceleration) < threshold | < 0.15 m/s² | > 100 ms |
| abs(gyro_magnitude) < threshold | < 5 °/s | > 100 ms |
| orientation is stable | delta_q < 0.01 | > 100 ms |

> **Tune these thresholds empirically.** Start with the values above and adjust based on your sensor's noise floor.

## Why It Works
Barbell lifts naturally include:
- pauses at the top (lockout)
- pauses at the bottom (pause squats, touch-and-go bench)
- rest between reps
- setup and unrack

These allow frequent drift correction, bounding the integration error.

---

# Phase 10 — Rep Detection Engine

## Goal
Reliably count reps and detect movement phases.

## Recommended Architecture
Finite state machine with hysteresis.

## States

```text
IDLE
DESCENDING (eccentric)
BOTTOM (pause / reversal)
ASCENDING (concentric)
LOCKOUT (top hold)
```

## State Transition Thresholds

| Transition | Condition | Suggested Starting Value |
|---|---|---|
| IDLE → DESCENDING | velocity < threshold sustained | velocity < -0.05 m/s for > 100 ms |
| DESCENDING → BOTTOM | velocity crosses zero (going positive) | velocity > -0.02 m/s after sustained descent |
| BOTTOM → ASCENDING | velocity > threshold sustained | velocity > 0.05 m/s for > 50 ms |
| ASCENDING → LOCKOUT | velocity drops near zero | velocity < 0.03 m/s for > 200 ms |
| LOCKOUT → IDLE | stable for duration | ZUPT active for > 500 ms |

## Guards Against False Triggers

| Guard | Value | Purpose |
|---|---|---|
| Minimum rep duration | > 0.8 seconds | Reject vibrations from re-gripping or plate rattle |
| Minimum descent depth | velocity must have exceeded -0.1 m/s | Reject minor bar adjustments |
| Debounce time | 200 ms between state changes | Prevent rapid oscillation between states |

## Example Squat Flow

```text
IDLE
→ velocity < -0.05 m/s for 100ms → DESCENDING
→ velocity crosses zero → BOTTOM
→ velocity > 0.05 m/s for 50ms → ASCENDING
→ velocity < 0.03 m/s for 200ms → LOCKOUT
→ rep++ → emit rep summary → IDLE (after 500ms stable)
```

## Inputs
- velocity (magnitude and direction)
- linear acceleration
- timing thresholds
- ZUPT state

---

# Phase 11 — Metrics Engine

## Per Rep Metrics
Calculate:
- rep count
- peak velocity (concentric phase only)
- average velocity (concentric phase only)
- eccentric duration
- concentric duration
- pause duration (time at BOTTOM state)
- total rep duration
- estimated 1RM (optional, using Bryzcki or load-velocity profile)

## Example Output

```text
Rep 4
Peak Velocity: 0.72 m/s
Average Velocity: 0.48 m/s
Eccentric Time: 1.9 s
Pause Time: 0.8 s
Concentric Time: 1.1 s
Total Rep Time: 3.8 s
```

---

# Serial Communication Protocol

## Dual Message Format

Use a prefix-based CSV format with two message types:

### Streaming Data (sent every sample, 100Hz)

```text
D,timestamp_ms,linear_acc,velocity,state_id
```

Example:
```text
D,102340,0.42,0.31,3
```

- `D` = data message
- `state_id`: 0=IDLE, 1=DESCENDING, 2=BOTTOM, 3=ASCENDING, 4=LOCKOUT

### Rep Summary (sent once per completed rep)

```text
R,rep_num,peak_vel,avg_vel,ecc_time,pause_time,con_time,total_time
```

Example:
```text
R,4,0.72,0.48,1.90,0.80,1.10,3.80
```

### Debug Data (optional, toggled on/off)

```text
X,timestamp_ms,ax,ay,az,gx,gy,gz,q0,q1,q2,q3,lin_ax,lin_ay,lin_az,vel
```

## Benefits
- Python parser switches on first character (`D`, `R`, `X`)
- rep summaries are never lost in the data stream
- streaming data is compact (fewer fields = less USB bandwidth)
- debug data can be enabled/disabled without changing streaming format

---

# Python Visualizer Goals

## V1 Dashboard
Display:
- live vertical acceleration graph (pyqtgraph)
- live velocity graph (pyqtgraph)
- rep count (large numeric display)
- current movement phase (color-coded state indicator)
- peak velocity per rep (bar chart or rolling display)
- last rep summary panel

## Optional Later
- workout history
- velocity trend analysis (set-to-set fatigue)
- fatigue estimation
- load-velocity profiling
- estimated 1RM

---

# Sampling Rate

## V1 Target

```text
100 Hz
```

## Rationale
- 100Hz is the standard for commercial VBT devices (GymAware, PUSH Band)
- provides 10ms resolution — sufficient for squat/bench/deadlift velocity profiles
- lower processing/transmission burden than 200Hz
- upgrade to 200Hz in V2 if needed for Olympic lifts or fast movements

## Timing Enforcement
Use **interrupt-driven sampling** (MPU6500 Data Ready interrupt) to ensure consistent timing. Do NOT rely on `delay()` or main loop timing.

```c
// Configure MPU6500 to generate interrupt on Data Ready
// Attach ESP32-S3 GPIO interrupt handler
void IRAM_ATTR dataReadyISR() {
    dataReady = true;
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(INT_PIN), dataReadyISR, RISING);
}
```

---

# Error Handling & Watchdog

## ESP32-S3 Watchdog Timer
Enable the hardware watchdog to recover from hangs:

```c
#include <esp_task_wdt.h>

void setup() {
    esp_task_wdt_init(5, true);  // 5 second timeout, panic on timeout
    esp_task_wdt_add(NULL);      // add current task
}

void loop() {
    esp_task_wdt_reset();  // feed the watchdog each loop iteration
    // ... processing ...
}
```

## SPI Bus Recovery
If an SPI transaction fails or times out:
1. De-assert chip select
2. Send 8 clock pulses to clear any stuck state
3. Re-initialize the MPU6500
4. Re-run calibration if needed

## Sensor Disconnect Detection
Periodically (every 1 second) read `WHO_AM_I` register:
- If it returns 0x70 → sensor OK
- If it returns 0xFF or any other value → sensor disconnected or SPI failure
- Log error and attempt recovery

---

# Mounting Guidelines

## Placement
Mount near:
- center of barbell (between the hands for bench, on the sleeve for squat/deadlift)

## Mount Requirements
- rigid (no play or wobble)
- low vibration transmission
- consistent orientation between sessions

## Recommended Methods
- 3D-printed barbell clamp (best rigidity)
- hose clamp with rubber padding
- velcro straps (adequate for V1)
- magnetic mount (convenient but may shift under heavy loads)

## Orientation Convention
Document which sensor axis aligns with which direction when mounted:
- e.g., "MPU6500 X-axis = bar long axis, Z-axis = vertical (up)"
- This mapping is essential for axis selection in Phase 7

---

# Debugging Rules

## Never Debug Multiple Stages Simultaneously

Correct order:
1. SPI communication + WHO_AM_I verification
2. raw sensor data stability
3. calibration (after thermal warm-up)
4. orientation estimation (quaternion output)
5. gravity compensation (sanity checks)
6. filtering (compare raw vs filtered visually)
7. velocity estimation (trapezoidal + ZUPT)
8. rep detection (state machine transitions)

Skipping this order will create major debugging problems.

## Debugging Tips
- Use the `X` (debug) serial message type to stream full raw data when needed
- Log everything to CSV during development — you can replay data offline
- Use pyqtgraph to plot live data at every phase
- Compare against a video recording of the lift for ground truth

---

# V1 Success Criteria

## Must Work
- reliable rep counting (±0 error in controlled testing)
- stable phase detection (no oscillation between states)
- usable velocity trends (rep-to-rep consistency)
- pause detection
- ZUPT resets correctly between reps

## Acceptable Limitations
- approximate absolute velocity (±10-15% vs. linear position transducer)
- minor drift during movement (corrected by ZUPT)
- no precise spatial reconstruction
- may need per-exercise threshold tuning

## Not Required
- full 3D tracking
- centimeter-level positioning
- advanced biomechanics
- wireless operation

---

# Future V2 Ideas

Possible future upgrades:
- BLE streaming (ESP32-S3 has BLE 5.0)
- battery operation (LiPo + deep sleep between sets)
- mobile app (Flutter / React Native)
- cloud logging
- AI lift analysis (TinyML on ESP32-S3 — it supports TensorFlow Lite Micro)
- camera-assisted tracking
- bar path estimation (double integration with heavy filtering)
- multiple sensor support
- smartwatch integration
- 200Hz sampling rate upgrade
- binary serial protocol for higher throughput
- OTA firmware updates (ESP32-S3 supports this natively)

---

# Final Recommendation

Focus entirely on:
- stable sensor data (SPI + interrupt-driven + FIFO)
- good filtering (hardware DLPF + software LPF)
- reliable rep detection (state machine with tuned thresholds)
- velocity consistency (trapezoidal integration + ZUPT)

A simple and stable V1 is significantly more valuable than an unstable feature-heavy prototype.
