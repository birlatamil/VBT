/**
 * MPU6500 SPI Driver — Header
 */

#ifndef MPU6500_H
#define MPU6500_H

#include <Arduino.h>
#include <SPI.h>

// ─── Pin Configuration (ESP32-S3 ↔ MPU6500 Breakout) ─────────────
// Board label → SPI function → ESP32-S3 GPIO
//
//   SCL   → SCLK (SPI Clock)         → GPIO 36
//   SDA   → MOSI (data TO sensor)    → GPIO 35
//   ADD   → MISO/SDO (data FROM sensor) → GPIO 37
//   NCS   → CS (Chip Select)         → GPIO 10
//   INT   → Data Ready interrupt     → GPIO 4
//   VCC   → 3.3V
//   GND   → GND
//   FSYNC → Tie to GND (not used)
//   EDA   → Leave unconnected (auxiliary I2C)
//   ECL   → Leave unconnected (auxiliary I2C)
//
#define MPU6500_SPI_SCLK   36   // Board: SCL
#define MPU6500_SPI_MOSI   35   // Board: SDA
#define MPU6500_SPI_MISO   37   // Board: ADD (SDO in SPI mode)
#define MPU6500_SPI_CS     10   // Board: NCS
#define MPU6500_INT_PIN    4    // Board: INT
#define MPU6500_SPI_CLOCK  10000000  // 10 MHz

struct MPU6500RawData {
    int16_t accel_x, accel_y, accel_z;
    int16_t temp;
    int16_t gyro_x, gyro_y, gyro_z;
};

struct MPU6500ScaledData {
    float accel_x, accel_y, accel_z;  // m/s²
    float temp;                        // °C
    float gyro_x, gyro_y, gyro_z;     // °/s
};

struct MPU6500CalibrationData {
    float accel_offset_x, accel_offset_y, accel_offset_z;
    float gyro_offset_x, gyro_offset_y, gyro_offset_z;
    bool  is_calibrated;
};

class MPU6500 {
public:
    MPU6500();
    bool begin();
    uint8_t whoAmI();
    void readRawData(MPU6500RawData* raw);
    void scaleData(const MPU6500RawData* raw, MPU6500ScaledData* scaled);
    void readScaledData(MPU6500ScaledData* scaled);
    bool isDataReady();
    void clearDataReady();
    void enableDataReadyInterrupt();
    uint8_t readIntStatus();
    void reset();
    float getTemperature(int16_t raw_temp);
    
    /** Calibrate the sensor by taking multiple samples. Should be called while stationary. */
    void calibrate(uint16_t num_samples = 500);

    /** Get the current calibration data */
    MPU6500CalibrationData getCalibration() const;
    
    /** Set calibration data (e.g., loaded from EEPROM or NVS) */
    void setCalibration(const MPU6500CalibrationData& cal);

    static volatile bool dataReadyFlag;

private:
    SPIClass* _spi;
    SPISettings _spiSettings;
    MPU6500CalibrationData _calData;
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void readRegisters(uint8_t reg, uint8_t* buffer, uint8_t count);
    void configure();
};

#endif
