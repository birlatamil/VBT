#ifndef REP_DETECTOR_H
#define REP_DETECTOR_H

#include <Arduino.h>

enum LiftState {
    STATE_IDLE = 0,
    STATE_DESCENDING = 1,
    STATE_BOTTOM = 2,
    STATE_ASCENDING = 3,
    STATE_LOCKOUT = 4
};

enum LiftMode {
    LIFT_SQUAT = 0,
    LIFT_BENCH = 1,
    LIFT_DEADLIFT = 2
};

struct DataPoint {
    float position;
    float velocity;
};

struct RepMetrics {
    uint32_t rep_count;
    float peak_velocity;
    float avg_velocity;
    float ecc_time;     // Seconds
    float pause_time;   // Seconds
    float con_time;     // Seconds
    float total_time;   // Seconds
};

class RepDetector {
public:
    RepDetector();
    
    // Call this every loop with the latest velocity, position, and ZUPT state
    // Returns true if a rep was just completed this frame
    bool update(float velocity, float position, bool is_stationary, uint32_t timestamp_ms);
    
    void setMode(LiftMode mode) { _mode = mode; }
    LiftMode getMode() const { return _mode; }

    LiftState getState() const { return _state; }
    RepMetrics getLastRep() const { return _last_rep; }
    
    // Graphing data access
    const DataPoint* getProfile() const { return _profile; }
    uint16_t getProfileCount() const { return _profile_count; }

private:
    LiftMode _mode;
    LiftState _state;
    RepMetrics _last_rep;
    
    // High-resolution rep profile
    static const uint16_t MAX_PROFILE_SAMPLES = 500;
    DataPoint _profile[MAX_PROFILE_SAMPLES];
    uint16_t _profile_count;
    
    // Timing states
    uint32_t _state_start_time;
    uint32_t _descend_start_time;
    uint32_t _bottom_start_time;
    uint32_t _ascend_start_time;
    uint32_t _lockout_start_time;
    
    // Metric accumulators
    float _concentric_vel_sum;
    uint32_t _concentric_samples;
    float _peak_vel;
    
    // Threshold timers
    uint32_t _condition_start_time;
    bool _condition_met;

    void changeState(LiftState new_state, uint32_t timestamp_ms);
    bool checkSustained(bool condition, uint32_t duration_ms, uint32_t timestamp_ms);
};

#endif
