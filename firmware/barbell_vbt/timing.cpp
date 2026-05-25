/**
 * Timing Utilities — Implementation
 */

#include "timing.h"

Timing::Timing() : _lastMicros(0), _deltaTime(0.0f), _timestampMs(0) {}

void Timing::update() {
    uint32_t now = micros();
    if (_lastMicros == 0) {
        _deltaTime = 0.01f;  // Assume 100Hz for first sample
    } else {
        uint32_t elapsed = now - _lastMicros;
        _deltaTime = elapsed * 0.000001f;
    }
    _lastMicros = now;
    _timestampMs = millis();
}

float Timing::getDeltaTime() const { return _deltaTime; }
uint32_t Timing::getTimestampMs() const { return _timestampMs; }

float Timing::getActualFrequency() const {
    return (_deltaTime > 0.0f) ? 1.0f / _deltaTime : 0.0f;
}

void Timing::reset() {
    _lastMicros = 0;
    _deltaTime = 0.0f;
}
