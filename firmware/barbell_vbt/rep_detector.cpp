#include "rep_detector.h"

RepDetector::RepDetector() {
    _mode = LIFT_SQUAT;
    _state = STATE_IDLE;
    memset(&_last_rep, 0, sizeof(_last_rep));
    _state_start_time = 0;
    _condition_start_time = 0;
    _condition_met = false;
    _descend_start_time = 0;
    _bottom_start_time = 0;
    _ascend_start_time = 0;
    _lockout_start_time = 0;
    _peak_vel = 0.0f;
    _concentric_vel_sum = 0.0f;
    _concentric_samples = 0;
    _profile_count = 0;
}

void RepDetector::changeState(LiftState new_state, uint32_t timestamp_ms) {
    _state = new_state;
    _state_start_time = timestamp_ms;
    _condition_start_time = 0;
    _condition_met = false;
}

bool RepDetector::checkSustained(bool condition, uint32_t duration_ms, uint32_t timestamp_ms) {
    if (condition) {
        if (!_condition_met) {
            _condition_met = true;
            _condition_start_time = timestamp_ms;
        } else if (timestamp_ms - _condition_start_time >= duration_ms) {
            _condition_met = false;
            return true;
        }
    } else {
        _condition_met = false;
    }
    return false;
}

bool RepDetector::update(float velocity, float position, bool is_stationary, uint32_t timestamp_ms) {
    bool rep_completed = false;

    // Record high-resolution profile data when active
    if (_state != STATE_IDLE && _profile_count < MAX_PROFILE_SAMPLES) {
        _profile[_profile_count].velocity = velocity;
        _profile[_profile_count].position = position;
        _profile_count++;
    }

    if (_mode == LIFT_DEADLIFT) {
        // DEADLIFT LOGIC (Up -> Top Pause -> Down -> Rest)
        switch (_state) {
            case STATE_IDLE:
                if (checkSustained(velocity > 0.15f, 100, timestamp_ms)) {
                    _profile_count = 0; // Reset graph buffer
                    changeState(STATE_ASCENDING, timestamp_ms);
                    _ascend_start_time = timestamp_ms;
                    _peak_vel = 0.0f;
                    _concentric_vel_sum = 0.0f;
                    _concentric_samples = 0;
                    Serial.println("# >> ASCENDING (Deadlift)");
                }
                break;
            case STATE_ASCENDING:
                if (velocity > _peak_vel) _peak_vel = velocity;
                _concentric_vel_sum += velocity;
                _concentric_samples++;
                if (timestamp_ms - _state_start_time > 5000) {
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> Aborted: ascent too long");
                    break;
                }
                if (checkSustained(velocity < 0.05f, 100, timestamp_ms)) {
                    changeState(STATE_LOCKOUT, timestamp_ms);
                    _lockout_start_time = timestamp_ms;
                    Serial.println("# >> LOCKOUT (Top)");
                }
                break;
            case STATE_LOCKOUT:
                if (timestamp_ms - _state_start_time > 5000) {
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> Aborted: lockout pause too long");
                    break;
                }
                if (checkSustained(velocity < -0.15f, 100, timestamp_ms)) {
                    changeState(STATE_DESCENDING, timestamp_ms);
                    _descend_start_time = timestamp_ms;
                    Serial.println("# >> DESCENDING");
                }
                break;
            case STATE_DESCENDING:
                if (timestamp_ms - _state_start_time > 5000) {
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> Aborted: descent too long");
                    break;
                }
                if (checkSustained(velocity > -0.05f, 100, timestamp_ms)) {
                    _bottom_start_time = timestamp_ms - 100;
                    changeState(STATE_BOTTOM, timestamp_ms);
                    Serial.println("# >> BOTTOM (Rest)");
                }
                break;
            case STATE_BOTTOM:
                if (checkSustained(is_stationary, 400, timestamp_ms)) {
                    _last_rep.con_time   = (_lockout_start_time - _ascend_start_time) / 1000.0f;
                    _last_rep.pause_time = (_descend_start_time - _lockout_start_time) / 1000.0f;
                    _last_rep.ecc_time   = (_bottom_start_time - _descend_start_time) / 1000.0f;
                    _last_rep.total_time = (_bottom_start_time - _ascend_start_time) / 1000.0f;
                    _last_rep.peak_velocity = _peak_vel;
                    _last_rep.avg_velocity  = (_concentric_samples > 0) ? (_concentric_vel_sum / _concentric_samples) : 0.0f;

                    if (_last_rep.total_time > 0.8f && _last_rep.peak_velocity > 0.25f) {
                        _last_rep.rep_count++;
                        rep_completed = true;
                    }
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> IDLE");
                }
                break;
        }
    } else {
        // SQUAT & BENCH LOGIC (Down -> Bottom Pause -> Up -> Rest)
        switch (_state) {
            case STATE_IDLE:
                if (checkSustained(velocity < -0.15f, 100, timestamp_ms)) {
                    _profile_count = 0; // Reset graph buffer
                    changeState(STATE_DESCENDING, timestamp_ms);
                    _descend_start_time = timestamp_ms;
                    Serial.println("# >> DESCENDING");
                }
                break;
            case STATE_DESCENDING:
                if (timestamp_ms - _state_start_time > 5000) {
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> Aborted: descent too long");
                    break;
                }
                if (checkSustained(velocity > -0.05f, 100, timestamp_ms)) {
                    changeState(STATE_BOTTOM, timestamp_ms);
                    _bottom_start_time = timestamp_ms;
                    Serial.println("# >> BOTTOM");
                }
                break;
            case STATE_BOTTOM:
                if (timestamp_ms - _state_start_time > 5000) {
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> Aborted: bottom pause too long");
                    break;
                }
                if (checkSustained(velocity > 0.15f, 100, timestamp_ms)) {
                    changeState(STATE_ASCENDING, timestamp_ms);
                    _ascend_start_time = timestamp_ms;
                    _peak_vel = 0.0f;
                    _concentric_vel_sum = 0.0f;
                    _concentric_samples = 0;
                    Serial.println("# >> ASCENDING");
                }
                break;
            case STATE_ASCENDING:
                if (velocity > _peak_vel) _peak_vel = velocity;
                _concentric_vel_sum += velocity;
                _concentric_samples++;
                if (timestamp_ms - _state_start_time > 5000) {
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> Aborted: ascent too long");
                    break;
                }
                if (checkSustained(velocity < 0.05f, 100, timestamp_ms)) {
                    _lockout_start_time = timestamp_ms - 100;
                    changeState(STATE_LOCKOUT, timestamp_ms);
                    Serial.println("# >> LOCKOUT");
                }
                break;
            case STATE_LOCKOUT:
                if (checkSustained(is_stationary, 400, timestamp_ms)) {
                    _last_rep.ecc_time   = (_bottom_start_time - _descend_start_time) / 1000.0f;
                    _last_rep.pause_time = (_ascend_start_time - _bottom_start_time) / 1000.0f;
                    _last_rep.con_time   = (_lockout_start_time - _ascend_start_time) / 1000.0f;
                    _last_rep.total_time = (_lockout_start_time - _descend_start_time) / 1000.0f;
                    _last_rep.peak_velocity = _peak_vel;
                    _last_rep.avg_velocity  = (_concentric_samples > 0) ? (_concentric_vel_sum / _concentric_samples) : 0.0f;

                    if (_last_rep.total_time > 0.8f && _last_rep.peak_velocity > 0.25f) {
                        _last_rep.rep_count++;
                        rep_completed = true;
                    }
                    changeState(STATE_IDLE, timestamp_ms);
                    Serial.println("# >> IDLE");
                }
                break;
        }
    }

    return rep_completed;
}
