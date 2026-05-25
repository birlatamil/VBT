/**
 * MPU6500 SPI Driver — Implementation
 * Handles SPI communication with MPU6500 on ESP32-S3.
 */

#include "mpu6500.h"
#include "mpu6500_registers.h"
#ifdef ESP32
#include <esp_task_wdt.h>
#endif

volatile bool MPU6500::dataReadyFlag = false;

void IRAM_ATTR mpu6500DataReadyISR() {
    MPU6500::dataReadyFlag = true;
}

MPU6500::MPU6500()
    : _spi(nullptr)
    , _spiSettings(MPU6500_SPI_CLOCK, MSBFIRST, SPI_MODE3) {
    _calData.is_calibrated = false;
    _calData.accel_offset_x = 0;
    _calData.accel_offset_y = 0;
    _calData.accel_offset_z = 0;
    _calData.gyro_offset_x = 0;
    _calData.gyro_offset_y = 0;
    _calData.gyro_offset_z = 0;
}

bool MPU6500::begin() {
    pinMode(MPU6500_SPI_CS, OUTPUT);
    digitalWrite(MPU6500_SPI_CS, HIGH);

    _spi = new SPIClass(FSPI);
    _spi->begin(MPU6500_SPI_SCLK, MPU6500_SPI_MISO, MPU6500_SPI_MOSI, MPU6500_SPI_CS);
    delay(10);

    reset();
    delay(100);

    uint8_t id = whoAmI();
    if (id != 0x70) {
        Serial.printf("[MPU6500] ERROR: WHO_AM_I = 0x%02X (expected 0x70)\n", id);
        if (id == 0x68) Serial.println("[MPU6500] This appears to be an MPU6050 (counterfeit)");
        else if (id == 0xFF) Serial.println("[MPU6500] No response — check SPI wiring!");
        return false;
    }
    Serial.printf("[MPU6500] WHO_AM_I = 0x%02X — Genuine MPU6500 confirmed\n", id);

    configure();
    enableDataReadyInterrupt();
    Serial.println("[MPU6500] Initialization complete");
    return true;
}

uint8_t MPU6500::whoAmI() { return readRegister(MPU6500_WHO_AM_I); }

void MPU6500::readRawData(MPU6500RawData* raw) {
    uint8_t buffer[14];
    readRegisters(MPU6500_ACCEL_XOUT_H, buffer, 14);
    raw->accel_x = (int16_t)((buffer[0]  << 8) | buffer[1]);
    raw->accel_y = (int16_t)((buffer[2]  << 8) | buffer[3]);
    raw->accel_z = (int16_t)((buffer[4]  << 8) | buffer[5]);
    raw->temp    = (int16_t)((buffer[6]  << 8) | buffer[7]);
    raw->gyro_x  = (int16_t)((buffer[8]  << 8) | buffer[9]);
    raw->gyro_y  = (int16_t)((buffer[10] << 8) | buffer[11]);
    raw->gyro_z  = (int16_t)((buffer[12] << 8) | buffer[13]);
}

void MPU6500::scaleData(const MPU6500RawData* raw, MPU6500ScaledData* scaled) {
    scaled->accel_x = (raw->accel_x / MPU6500_ACCEL_SCALE_8G) * GRAVITY_MS2;
    scaled->accel_y = (raw->accel_y / MPU6500_ACCEL_SCALE_8G) * GRAVITY_MS2;
    scaled->accel_z = (raw->accel_z / MPU6500_ACCEL_SCALE_8G) * GRAVITY_MS2;
    scaled->gyro_x = raw->gyro_x / MPU6500_GYRO_SCALE_500DPS;
    scaled->gyro_y = raw->gyro_y / MPU6500_GYRO_SCALE_500DPS;
    scaled->gyro_z = raw->gyro_z / MPU6500_GYRO_SCALE_500DPS;
    scaled->temp = getTemperature(raw->temp);

    if (_calData.is_calibrated) {
        scaled->accel_x -= _calData.accel_offset_x;
        scaled->accel_y -= _calData.accel_offset_y;
        scaled->accel_z -= _calData.accel_offset_z;
        scaled->gyro_x -= _calData.gyro_offset_x;
        scaled->gyro_y -= _calData.gyro_offset_y;
        scaled->gyro_z -= _calData.gyro_offset_z;
    }
}

void MPU6500::readScaledData(MPU6500ScaledData* scaled) {
    MPU6500RawData raw;
    readRawData(&raw);
    scaleData(&raw, scaled);
}

bool MPU6500::isDataReady() { return dataReadyFlag; }
void MPU6500::clearDataReady() { dataReadyFlag = false; }

void MPU6500::enableDataReadyInterrupt() {
    // Enable latching so the INT pin stays high until cleared, preventing missed pulses
    writeRegister(MPU6500_INT_PIN_CFG, MPU6500_INT_LATCH_EN | MPU6500_INT_RD_CLEAR);
    writeRegister(MPU6500_INT_ENABLE, MPU6500_INT_DATA_RDY_EN);
    pinMode(MPU6500_INT_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(MPU6500_INT_PIN), mpu6500DataReadyISR, RISING);
    Serial.println("[MPU6500] Data Ready interrupt enabled on GPIO " + String(MPU6500_INT_PIN));
}

uint8_t MPU6500::readIntStatus() { return readRegister(MPU6500_INT_STATUS); }

void MPU6500::reset() {
    writeRegister(MPU6500_PWR_MGMT_1, MPU6500_PWR1_DEVICE_RESET);
    delay(100);
}

float MPU6500::getTemperature(int16_t raw_temp) {
    return (raw_temp / 333.87f) + 21.0f;
}

void MPU6500::calibrate(uint16_t num_samples) {
    Serial.println("[MPU6500] Starting calibration. DO NOT MOVE SENSOR!");
    
    double sum_ax = 0, sum_ay = 0, sum_az = 0;
    double sum_gx = 0, sum_gy = 0, sum_gz = 0;
    
    // Temporarily bypass applied calibration offsets
    bool was_calibrated = _calData.is_calibrated;
    _calData.is_calibrated = false;

    // Clear any pending interrupts
    readIntStatus();
    clearDataReady();
    
    for (uint16_t i = 0; i < num_samples; i++) {
        // Feed the watchdog to prevent timeout during this 5-second blocking operation
        #ifdef ESP32
        esp_task_wdt_reset();
        #endif
        
        // Wait for data ready
        while (!isDataReady()) {
            delay(1);
        }
        clearDataReady();
        readIntStatus(); // clear hardware interrupt
        
        MPU6500RawData raw;
        readRawData(&raw);
        
        MPU6500ScaledData scaled;
        scaleData(&raw, &scaled); // Using scaleData knowing is_calibrated is false
        
        sum_ax += scaled.accel_x;
        sum_ay += scaled.accel_y;
        sum_az += scaled.accel_z;
        sum_gx += scaled.gyro_x;
        sum_gy += scaled.gyro_y;
        sum_gz += scaled.gyro_z;
    }
    
    _calData.gyro_offset_x = sum_gx / num_samples;
    _calData.gyro_offset_y = sum_gy / num_samples;
    _calData.gyro_offset_z = sum_gz / num_samples;
    
    float avg_ax = sum_ax / num_samples;
    float avg_ay = sum_ay / num_samples;
    float avg_az = sum_az / num_samples;
    
    // Find gravity axis automatically
    float expected_ax = 0.0f, expected_ay = 0.0f, expected_az = 0.0f;
    if (abs(avg_ax) > abs(avg_ay) && abs(avg_ax) > abs(avg_az)) {
        expected_ax = (avg_ax > 0) ? GRAVITY_MS2 : -GRAVITY_MS2;
    } else if (abs(avg_ay) > abs(avg_ax) && abs(avg_ay) > abs(avg_az)) {
        expected_ay = (avg_ay > 0) ? GRAVITY_MS2 : -GRAVITY_MS2;
    } else {
        expected_az = (avg_az > 0) ? GRAVITY_MS2 : -GRAVITY_MS2;
    }
    
    _calData.accel_offset_x = avg_ax - expected_ax;
    _calData.accel_offset_y = avg_ay - expected_ay;
    _calData.accel_offset_z = avg_az - expected_az;
    
    _calData.is_calibrated = true;
    
    Serial.println("[MPU6500] Calibration complete!");
    Serial.printf("  Accel Offsets : X=%.3f Y=%.3f Z=%.3f (m/s2)\n", _calData.accel_offset_x, _calData.accel_offset_y, _calData.accel_offset_z);
    Serial.printf("  Gyro Offsets  : X=%.3f Y=%.3f Z=%.3f (deg/s)\n", _calData.gyro_offset_x, _calData.gyro_offset_y, _calData.gyro_offset_z);
}

MPU6500CalibrationData MPU6500::getCalibration() const {
    return _calData;
}

void MPU6500::setCalibration(const MPU6500CalibrationData& cal) {
    _calData = cal;
}

void MPU6500::configure() {
    // Wake up, select PLL clock
    writeRegister(MPU6500_PWR_MGMT_1, MPU6500_PWR1_CLKSEL_PLL);
    delay(10);
    // Enable all accel and gyro axes
    writeRegister(MPU6500_PWR_MGMT_2, 0x00);
    delay(5);
    // Disable I2C (SPI-only mode)
    writeRegister(MPU6500_USER_CTRL, MPU6500_USERCTRL_I2C_DIS);
    delay(5);
    // Sample rate: 1000 / (1 + 9) = 100 Hz
    writeRegister(MPU6500_SMPLRT_DIV, 9);
    delay(5);
    // Gyro: ±500°/s
    writeRegister(MPU6500_GYRO_CONFIG, MPU6500_GYRO_FS_500DPS);
    delay(5);
    // Accel: ±8g
    writeRegister(MPU6500_ACCEL_CONFIG, MPU6500_ACCEL_FS_8G);
    delay(5);
    // Gyro DLPF: 41Hz bandwidth
    writeRegister(MPU6500_CONFIG, MPU6500_DLPF_BW_41HZ);
    delay(5);
    // Accel DLPF: 41Hz bandwidth
    writeRegister(MPU6500_ACCEL_CONFIG2, MPU6500_ACCEL_DLPF_BW_41HZ);
    delay(5);

    Serial.println("[MPU6500] Config: ±8g, ±500°/s, 100Hz, DLPF 41Hz, SPI 4MHz");
}

void MPU6500::writeRegister(uint8_t reg, uint8_t value) {
    _spi->beginTransaction(_spiSettings);
    digitalWrite(MPU6500_SPI_CS, LOW);
    _spi->transfer(reg & 0x7F);   // Write: MSB = 0
    _spi->transfer(value);
    digitalWrite(MPU6500_SPI_CS, HIGH);
    _spi->endTransaction();
}

uint8_t MPU6500::readRegister(uint8_t reg) {
    _spi->beginTransaction(_spiSettings);
    digitalWrite(MPU6500_SPI_CS, LOW);
    _spi->transfer(reg | 0x80);   // Read: MSB = 1
    uint8_t value = _spi->transfer(0x00);
    digitalWrite(MPU6500_SPI_CS, HIGH);
    _spi->endTransaction();
    return value;
}

void MPU6500::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t count) {
    _spi->beginTransaction(_spiSettings);
    digitalWrite(MPU6500_SPI_CS, LOW);
    _spi->transfer(reg | 0x80);   // Read: MSB = 1
    _spi->transferBytes(nullptr, buffer, count);
    digitalWrite(MPU6500_SPI_CS, HIGH);
    _spi->endTransaction();
}
