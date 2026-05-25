/**
 * MPU6500 Register Definitions
 * 
 * Complete register map for the MPU6500 6-axis IMU.
 * Reference: MPU-6500 Register Map and Descriptions, Rev 2.1
 */

#ifndef MPU6500_REGISTERS_H
#define MPU6500_REGISTERS_H

// ─── Self-Test Registers ─────────────────────────────────────────────
#define MPU6500_SELF_TEST_X_GYRO   0x00
#define MPU6500_SELF_TEST_Y_GYRO   0x01
#define MPU6500_SELF_TEST_Z_GYRO   0x02
#define MPU6500_SELF_TEST_X_ACCEL  0x0D
#define MPU6500_SELF_TEST_Y_ACCEL  0x0E
#define MPU6500_SELF_TEST_Z_ACCEL  0x0F

// ─── Offset Registers ────────────────────────────────────────────────
#define MPU6500_XG_OFFSET_H        0x13
#define MPU6500_XG_OFFSET_L        0x14
#define MPU6500_YG_OFFSET_H        0x15
#define MPU6500_YG_OFFSET_L        0x16
#define MPU6500_ZG_OFFSET_H        0x17
#define MPU6500_ZG_OFFSET_L        0x18

// ─── Configuration Registers ─────────────────────────────────────────
#define MPU6500_SMPLRT_DIV         0x19  // Sample Rate Divider
#define MPU6500_CONFIG             0x1A  // DLPF config (gyro)
#define MPU6500_GYRO_CONFIG        0x1B  // Gyro full-scale range
#define MPU6500_ACCEL_CONFIG       0x1C  // Accel full-scale range
#define MPU6500_ACCEL_CONFIG2      0x1D  // Accel DLPF config

// ─── FIFO Enable ─────────────────────────────────────────────────────
#define MPU6500_FIFO_EN            0x23

// ─── Interrupt Configuration ─────────────────────────────────────────
#define MPU6500_INT_PIN_CFG        0x37  // INT pin config
#define MPU6500_INT_ENABLE         0x38  // Interrupt enable
#define MPU6500_INT_STATUS         0x3A  // Interrupt status

// ─── Sensor Data Registers (high byte first) ─────────────────────────
#define MPU6500_ACCEL_XOUT_H       0x3B
#define MPU6500_ACCEL_XOUT_L       0x3C
#define MPU6500_ACCEL_YOUT_H       0x3D
#define MPU6500_ACCEL_YOUT_L       0x3E
#define MPU6500_ACCEL_ZOUT_H       0x3F
#define MPU6500_ACCEL_ZOUT_L       0x40
#define MPU6500_TEMP_OUT_H         0x41
#define MPU6500_TEMP_OUT_L         0x42
#define MPU6500_GYRO_XOUT_H        0x43
#define MPU6500_GYRO_XOUT_L        0x44
#define MPU6500_GYRO_YOUT_H        0x45
#define MPU6500_GYRO_YOUT_L        0x46
#define MPU6500_GYRO_ZOUT_H        0x47
#define MPU6500_GYRO_ZOUT_L        0x48

// ─── FIFO Registers ──────────────────────────────────────────────────
#define MPU6500_FIFO_COUNTH        0x72
#define MPU6500_FIFO_COUNTL        0x73
#define MPU6500_FIFO_R_W           0x74

// ─── Power Management ────────────────────────────────────────────────
#define MPU6500_USER_CTRL          0x6A
#define MPU6500_PWR_MGMT_1         0x6B
#define MPU6500_PWR_MGMT_2         0x6C

// ─── Identity ────────────────────────────────────────────────────────
#define MPU6500_WHO_AM_I           0x75  // Expected value: 0x70

// ─── Configuration Values ────────────────────────────────────────────

// Gyroscope Full-Scale Range (GYRO_CONFIG bits [4:3])
#define MPU6500_GYRO_FS_250DPS     0x00  // ±250 °/s  → 131.0 LSB/°/s
#define MPU6500_GYRO_FS_500DPS     0x08  // ±500 °/s  → 65.5  LSB/°/s
#define MPU6500_GYRO_FS_1000DPS    0x10  // ±1000 °/s → 32.8  LSB/°/s
#define MPU6500_GYRO_FS_2000DPS    0x18  // ±2000 °/s → 16.4  LSB/°/s

// Accelerometer Full-Scale Range (ACCEL_CONFIG bits [4:3])
#define MPU6500_ACCEL_FS_2G        0x00  // ±2g  → 16384 LSB/g
#define MPU6500_ACCEL_FS_4G        0x08  // ±4g  → 8192  LSB/g
#define MPU6500_ACCEL_FS_8G        0x10  // ±8g  → 4096  LSB/g
#define MPU6500_ACCEL_FS_16G       0x18  // ±16g → 2048  LSB/g

// DLPF Configuration (CONFIG bits [2:0] for gyro)
#define MPU6500_DLPF_BW_250HZ     0x00
#define MPU6500_DLPF_BW_184HZ     0x01
#define MPU6500_DLPF_BW_92HZ      0x02
#define MPU6500_DLPF_BW_41HZ      0x03  // ← Recommended for VBT
#define MPU6500_DLPF_BW_20HZ      0x04
#define MPU6500_DLPF_BW_10HZ      0x05
#define MPU6500_DLPF_BW_5HZ       0x06

// Accel DLPF Configuration (ACCEL_CONFIG2 bits [2:0])
#define MPU6500_ACCEL_DLPF_BW_460HZ  0x00
#define MPU6500_ACCEL_DLPF_BW_184HZ  0x01
#define MPU6500_ACCEL_DLPF_BW_92HZ   0x02
#define MPU6500_ACCEL_DLPF_BW_41HZ   0x03  // ← Recommended for VBT
#define MPU6500_ACCEL_DLPF_BW_20HZ   0x04
#define MPU6500_ACCEL_DLPF_BW_10HZ   0x05
#define MPU6500_ACCEL_DLPF_BW_5HZ    0x06

// Interrupt configuration bits
#define MPU6500_INT_ACTIVE_LOW     0x80
#define MPU6500_INT_OPEN_DRAIN     0x40
#define MPU6500_INT_LATCH_EN       0x20
#define MPU6500_INT_RD_CLEAR       0x10
#define MPU6500_INT_DATA_RDY_EN    0x01

// FIFO enable bits (FIFO_EN register)
#define MPU6500_FIFO_GYRO_EN       0x70  // XG, YG, ZG FIFO enable
#define MPU6500_FIFO_ACCEL_EN      0x08  // Accel FIFO enable

// User control bits
#define MPU6500_USERCTRL_FIFO_EN   0x40
#define MPU6500_USERCTRL_I2C_DIS   0x10  // Disable I2C, enable SPI-only
#define MPU6500_USERCTRL_FIFO_RST  0x04

// Power management bits
#define MPU6500_PWR1_DEVICE_RESET  0x80
#define MPU6500_PWR1_SLEEP         0x40
#define MPU6500_PWR1_CLKSEL_PLL    0x01  // Auto-select best clock source

// ─── Sensitivity Scale Factors ───────────────────────────────────────

#define MPU6500_ACCEL_SCALE_2G     16384.0f
#define MPU6500_ACCEL_SCALE_4G     8192.0f
#define MPU6500_ACCEL_SCALE_8G     4096.0f  // ← Using this
#define MPU6500_ACCEL_SCALE_16G    2048.0f

#define MPU6500_GYRO_SCALE_250DPS  131.0f
#define MPU6500_GYRO_SCALE_500DPS  65.5f   // ← Using this
#define MPU6500_GYRO_SCALE_1000DPS 32.8f
#define MPU6500_GYRO_SCALE_2000DPS 16.4f

#define GRAVITY_MS2 9.80665f

#endif // MPU6500_REGISTERS_H
