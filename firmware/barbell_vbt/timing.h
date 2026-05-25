/**
 * Timing Utilities — Header
 * Microsecond-precision deltaTime for consistent sampling.
 */

#ifndef TIMING_H
#define TIMING_H

#include <Arduino.h>

class Timing {
public:
    Timing();
    void update();
    float getDeltaTime() const;
    uint32_t getTimestampMs() const;
    float getActualFrequency() const;
    void reset();

private:
    uint32_t _lastMicros;
    float    _deltaTime;
    uint32_t _timestampMs;
};

#endif
