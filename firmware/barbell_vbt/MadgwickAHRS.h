//=====================================================================================================
// MadgwickAHRS.h — With Dynamic Beta for VBT
//=====================================================================================================
#ifndef MadgwickAHRS_h
#define MadgwickAHRS_h

#include <math.h>

class Madgwick {
private:
    float beta;
    float q0, q1, q2, q3;
    float invSampleFreq;
    float roll, pitch, yaw;
    bool anglesComputed;
    void computeAngles();

public:
    Madgwick();
    void begin(float sampleFrequency);
    void setBeta(float b) { beta = b; }
    float getBeta() const { return beta; }
    void updateIMU(float gx, float gy, float gz, float ax, float ay, float az);
    
    float getRoll()  { if (!anglesComputed) computeAngles(); return roll; }
    float getPitch() { if (!anglesComputed) computeAngles(); return pitch; }
    float getYaw()   { if (!anglesComputed) computeAngles(); return yaw; }
    
    void getQuaternion(float *w, float *x, float *y, float *z) {
        *w = q0; *x = q1; *y = q2; *z = q3;
    }
};

#endif
