"""
Barbell Velocity Tracker V1 — Phase 1 Serial Reader & Validator

Reads raw sensor data from ESP32-S3 over USB CDC serial,
validates sampling rate, and provides real-time text output.

Usage:
    python serial_reader.py               # Auto-detect COM port
    python serial_reader.py --port COM5   # Specify port
    python serial_reader.py --log         # Save to CSV file
"""

import serial
import serial.tools.list_ports
import argparse
import time
import sys
import os
from datetime import datetime


def find_esp32_port():
    """Auto-detect ESP32-S3 USB CDC port."""
    ports = serial.tools.list_ports.comports()
    esp_ports = []
    
    for port in ports:
        desc = (port.description or "").lower()
        hwid = (port.hwid or "").lower()
        
        # ESP32-S3 native USB CDC identifiers
        if any(kw in desc for kw in ["esp32", "usb jtag", "usb serial"]):
            esp_ports.append(port)
        elif "303a" in hwid:  # Espressif USB vendor ID
            esp_ports.append(port)
    
    if esp_ports:
        selected = esp_ports[0]
        print(f"[AUTO] Found ESP32-S3 on {selected.device} ({selected.description})")
        return selected.device
    
    # If no ESP32 found, list all ports
    if ports:
        print("[WARN] No ESP32-S3 detected. Available ports:")
        for port in ports:
            print(f"  {port.device}: {port.description} [{port.hwid}]")
        print(f"\n[INFO] Trying first available port: {ports[0].device}")
        return ports[0].device
    
    print("[ERROR] No serial ports found!")
    return None


def parse_data_line(line):
    """Parse a CSV data line into components."""
    parts = line.strip().split(",")
    if len(parts) == 8:
        try:
            return {
                "timestamp_ms": int(parts[0]),
                "accel_x": float(parts[1]),
                "accel_y": float(parts[2]),
                "accel_z": float(parts[3]),
                "gyro_x": float(parts[4]),
                "gyro_y": float(parts[5]),
                "gyro_z": float(parts[6]),
                "temp": float(parts[7]),
            }
        except ValueError:
            return None
    return None


def main():
    parser = argparse.ArgumentParser(description="VBT Phase 1 Serial Reader")
    parser.add_argument("--port", type=str, help="Serial port (e.g., COM5)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--log", action="store_true", help="Save data to CSV log file")
    parser.add_argument("--duration", type=int, default=0, help="Run for N seconds then stop (0=forever)")
    args = parser.parse_args()

    # Find port
    port = args.port or find_esp32_port()
    if not port:
        sys.exit(1)

    # Open log file if requested
    log_file = None
    if args.log:
        os.makedirs("../logs", exist_ok=True)
        log_name = f"../logs/phase1_raw_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        log_file = open(log_name, "w")
        log_file.write("timestamp_ms,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp\n")
        print(f"[LOG] Saving data to {log_name}")

    # Connect to serial
    print(f"\n[SERIAL] Connecting to {port} @ {args.baud} baud...")
    try:
        ser = serial.Serial(port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] Could not open {port}: {e}")
        sys.exit(1)

    print("[SERIAL] Connected! Waiting for data...\n")

    # Statistics
    sample_count = 0
    start_time = time.time()
    last_stats_time = start_time
    last_timestamp = None

    stats_window_count = 0
    stats_window_start = start_time

    try:
        while True:
            # Check duration limit
            if args.duration > 0 and (time.time() - start_time) >= args.duration:
                print(f"\n[INFO] Duration limit reached ({args.duration}s)")
                break

            # Read line
            try:
                raw_line = ser.readline()
                if not raw_line:
                    continue
                line = raw_line.decode("utf-8", errors="replace").strip()
            except serial.SerialException:
                print("[ERROR] Serial connection lost!")
                break

            if not line:
                continue

            # Handle comment/status lines
            if line.startswith("#"):
                print(f"  {line}")
                continue

            # Parse data line
            data = parse_data_line(line)
            if data is None:
                # Non-data, non-comment line (startup messages, etc.)
                print(f"  {line}")
                continue

            sample_count += 1
            stats_window_count += 1

            # Log to file
            if log_file:
                log_file.write(line + "\n")

            # Print every 100th sample to avoid flooding terminal
            if sample_count % 100 == 0:
                accel_mag = (data["accel_x"]**2 + data["accel_y"]**2 + data["accel_z"]**2) ** 0.5
                gyro_mag = (data["gyro_x"]**2 + data["gyro_y"]**2 + data["gyro_z"]**2) ** 0.5

                # Calculate actual sampling rate
                now = time.time()
                window_duration = now - stats_window_start
                actual_rate = stats_window_count / window_duration if window_duration > 0 else 0

                print(f"  [{sample_count:>6}] "
                      f"t={data['timestamp_ms']:>8}ms  "
                      f"acc=[{data['accel_x']:>7.2f}, {data['accel_y']:>7.2f}, {data['accel_z']:>7.2f}] m/s²  "
                      f"|a|={accel_mag:.2f}  "
                      f"gyro=[{data['gyro_x']:>7.1f}, {data['gyro_y']:>7.1f}, {data['gyro_z']:>7.1f}] °/s  "
                      f"|g|={gyro_mag:.1f}  "
                      f"T={data['temp']:.1f}°C  "
                      f"rate={actual_rate:.0f}Hz")

                # Reset window stats
                stats_window_count = 0
                stats_window_start = now

    except KeyboardInterrupt:
        print("\n\n[INFO] Stopped by user (Ctrl+C)")

    finally:
        elapsed = time.time() - start_time
        avg_rate = sample_count / elapsed if elapsed > 0 else 0
        print(f"\n{'='*60}")
        print(f"  Session Summary")
        print(f"  Total Samples : {sample_count}")
        print(f"  Duration      : {elapsed:.1f} s")
        print(f"  Avg Rate      : {avg_rate:.1f} Hz")
        print(f"{'='*60}")

        if log_file:
            log_file.close()
            print(f"  Log saved to  : {log_name}")

        ser.close()
        print("[SERIAL] Port closed")


if __name__ == "__main__":
    main()
