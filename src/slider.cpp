#include "slider.h"

static_assert(DEFAULT_HOME_INPUT == NEGATIVE_LIMIT_INPUT,
              "The PB7 sensor must be both home and the negative limit");
static_assert(NEGATIVE_LIMIT_INPUT != POSITIVE_LIMIT_INPUT,
              "Home/negative and positive sensors must use different inputs");

void Slider::pinInit()
{
    pinMode(MOTOR_PUL_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_EN_PIN, OUTPUT);

    digitalWrite(MOTOR_PUL_PIN, HIGH); // idle level, each step pulls it low
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_EN_PIN, MOTOR_ENABLE_LEVEL);
    is_stepper_enable = true;
}

void Slider::timerInit()
{
#if defined(__STM32F1__)
    ExecuteStepTimer.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
    ExecuteStepTimer.setCompare(TIMER_CH1, 1);
    ExecuteStepTimer.pause();

    TurnPinTimer.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
    TurnPinTimer.setCompare(TIMER_CH1, 1);
    TurnPinTimer.setPeriod(STEP_PULSE_WIDTH);
    TurnPinTimer.pause();
#else
    TurnPinTimer = new HardwareTimer(TURN_PIN_TIMER);
    TurnPinTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    TurnPinTimer->setCaptureCompare(1, 1, MICROSEC_COMPARE_FORMAT);
    TurnPinTimer->setOverflow(STEP_PULSE_WIDTH, MICROSEC_FORMAT);
    TurnPinTimer->pause();

    ExecuteStepTimer = new HardwareTimer(EXECUTE_STEP_TIMER);
    ExecuteStepTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    ExecuteStepTimer->setCaptureCompare(1, 1);
    ExecuteStepTimer->setPrescaleFactor(ExecuteStepTimer->getTimerClkFreq() / 1000000);
    ExecuteStepTimer->setOverflow(STEP_TIMER_MAX_PERIOD);
    ExecuteStepTimer->pause();
#endif
}

// func eeprom -------------------------------------------------------------
// raw 4-byte float store (handles negative / large values, e.g. target position)
void Slider::putRawFloat(uint16_t address, float value)
{
    uint8_t *p = (uint8_t *)&value;
    for (uint8_t i = 0; i < 4; i++)
    {
        EEPROM.update(address + i, p[i]);
    }
}

float Slider::getRawFloat(uint16_t address)
{
    float value;
    uint8_t *p = (uint8_t *)&value;
    for (uint8_t i = 0; i < 4; i++)
    {
        p[i] = EEPROM.read(address + i);
    }
    return value;
}

void Slider::save_data()
{
    EEPROM.update(SLIDER_ADDRESS, slider_address & 0xFF);
    EEPROM.update(SLIDER_ADDRESS + 1, slider_address >> 8);
    putRawFloat(SLIDER_ACCEL_ADDRESS, fabs(current_accel));
    putRawFloat(POS_SPEED_ADDRESS, desire_speed_pos);
    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        EEPROM.update(INPUT_FUNCTION_ADDRESS + i, input_function[i]);
        putRawFloat(INPUT_VALUE_ADDRESS + i * 4, input_value[i]);
    }

    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        EEPROM.update(OUTPUT_FUNCTION_ADDRESS + i, output_function[i]);
        putRawFloat(OUTPUT_VALUE_ADDRESS + i * 4, output_value[i]);
    }
}

void Slider::load_data()
{
    slider_address = EEPROM.read(SLIDER_ADDRESS) | (EEPROM.read(SLIDER_ADDRESS + 1) << 8);
    current_accel = getRawFloat(SLIDER_ACCEL_ADDRESS);
    if (isnan(current_accel) || current_accel < MIN_ACCEL)
    {
        // 0 would disable the ramp entirely, which makes every stop abrupt. It is
        // also what an upgraded board reads here, since 49..52 used to hold
        // input_value[0].
        current_accel = DEFAULT_ACCEL;
    }
    desire_speed_pos = getRawFloat(POS_SPEED_ADDRESS);
    if (isnan(desire_speed_pos) || desire_speed_pos <= 0)
    {
        desire_speed_pos = DEFAULT_SPEED;
    }
    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        input_function[i] = EEPROM.read(INPUT_FUNCTION_ADDRESS + i);
        if (input_function[i] > INPUT_FUNC_JOG_HOLD)
        {
            input_function[i] = INPUT_FUNC_NONE;
        }
        input_value[i] = getRawFloat(INPUT_VALUE_ADDRESS + i * 4);
        if (isnan(input_value[i]))
        {
            input_value[i] = 0;
        }
        if (i == NEGATIVE_LIMIT_INPUT || i == POSITIVE_LIMIT_INPUT)
        {
            // The two NPN sensors are fixed active-high and cannot also launch
            // a configurable input action, regardless of older EEPROM values.
            input_function[i] = INPUT_FUNC_NONE;
            input_value[i] = 0;
        }
    }

    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        output_function[i] = EEPROM.read(OUTPUT_FUNCTION_ADDRESS + i);
        if (output_function[i] > OUTPUT_FUNC_FAULT)
        {
            output_function[i] = OUTPUT_FUNC_NONE;
        }
        output_value[i] = getRawFloat(OUTPUT_VALUE_ADDRESS + i * 4);
        if (isnan(output_value[i]))
        {
            output_value[i] = 0;
        }
    }
}

void Slider::save_mechanics_data()
{
    putRawFloat(STEPS_PER_REV_ADDRESS, steps_per_rev);
    putRawFloat(SCREW_LEAD_ADDRESS, screw_lead);
    EEPROM.update(REVERSE_SLIDER_ADDRESS, reverse_slider);
}

void Slider::load_mechanics_data()
{
    steps_per_rev = getRawFloat(STEPS_PER_REV_ADDRESS);
    if (isnan(steps_per_rev) || steps_per_rev <= 0)
    {
        steps_per_rev = DEFAULT_STEPS_PER_REV;
    }

    screw_lead = getRawFloat(SCREW_LEAD_ADDRESS);
    if (isnan(screw_lead) || screw_lead <= 0)
    {
        screw_lead = DEFAULT_SCREW_LEAD;
    }

    reverse_slider = (bool)EEPROM.read(REVERSE_SLIDER_ADDRESS);

    // One-off migration: a previous build kept a separate gear ratio. Fold it into
    // the step count and mark it consumed, otherwise the scale would silently
    // change under a machine that used it.
    float _legacy_gear = getRawFloat(LEGACY_GEAR_RATIO_ADDRESS);
    if (!isnan(_legacy_gear) && _legacy_gear > 0 && _legacy_gear != 1.0f)
    {
        float _folded = steps_per_rev * _legacy_gear;
        float _folded_ppm = _folded / screw_lead;

        // only commit a folded value that survives the same bounds the setter uses
        if (!isnan(_folded_ppm) && _folded_ppm >= MIN_PULSE_PER_MM &&
            _folded_ppm <= MAX_PULSE_PER_MM)
        {
            steps_per_rev = _folded;
            putRawFloat(STEPS_PER_REV_ADDRESS, steps_per_rev);
        }

        putRawFloat(LEGACY_GEAR_RATIO_ADDRESS, NAN);
    }

    // Pulses per mm is always derived from the mechanics, so the two can never
    // drift apart across a firmware upgrade. The fallback rebuilds the mechanics
    // too, otherwise M328 would report a machine that does not match the scale.
    pulse_per_mm = steps_per_rev / screw_lead;
    if (isnan(pulse_per_mm) || pulse_per_mm < MIN_PULSE_PER_MM ||
        pulse_per_mm > MAX_PULSE_PER_MM)
    {
        steps_per_rev = DEFAULT_STEPS_PER_REV;
        screw_lead = DEFAULT_SCREW_LEAD;
        pulse_per_mm = DEFAULT_PULSE_PER_MM;
    }
}

void Slider::init_home_config()
{
    home_input = DEFAULT_HOME_INPUT;
    home_active_level = DEFAULT_HOME_ACTIVE_LEVEL;
    home_speed = DEFAULT_HOME_SPEED;
    home_speed_slow = DEFAULT_HOME_SPEED_SLOW;
    home_backoff = DEFAULT_HOME_BACKOFF;
    home_offset = DEFAULT_HOME_OFFSET;
}

// Fastest speed the step generator can physically produce. Commanded speeds
// above this are phantom: the motor cannot follow them, and a stop would ramp
// down from a speed that never existed.
float Slider::maxSpeed()
{
    if (pulse_per_mm < MIN_PULSE_PER_MM)
    {
        return 0;
    }
    return 1000000.0f / ((float)MIN_STEP_PERIOD * pulse_per_mm);
}

float Slider::getMaxSpeed()
{
    return maxSpeed();
}

void Slider::setTimerPeriodFromSpeed(float _speed)
{
    if (_speed == 0)
    {
        __pause_step_timer();
        return;
    }

    float _period = 1000000.0f / (_speed * pulse_per_mm);

    if (_period < (float)MIN_STEP_PERIOD)
    {
        current_period = MIN_STEP_PERIOD;
    }
    else if (_period > (float)STEP_TIMER_MAX_PERIOD)
    {
        // a speed so low the 16-bit reload cannot express it
        current_period = STEP_TIMER_MAX_PERIOD;
    }
    else
    {
        current_period = (uint32_t)_period;
    }

#if defined(__STM32F1__)
    ExecuteStepTimer.setPeriod(current_period);
#else
    ExecuteStepTimer->setOverflow(current_period);
#endif
}

void Slider::__execute_vel()
{
    if (current_speed != desire_speed)
    {
        time_count += current_period;
        if (current_accel == 0)
        {
            current_speed = desire_speed;
            setTimerPeriodFromSpeed(current_speed);
        }
        else if (time_count >= TIME_ISR_ACCEL)
        {
            current_speed = current_speed + current_accel * time_count / 1000000.0;
            if ((current_accel > 0 && current_speed > desire_speed) || (current_accel < 0 && current_speed < desire_speed))
            {
                current_speed = desire_speed;
            }
            time_count = 0;
            setTimerPeriodFromSpeed(current_speed);
        }
    }
}

// Every pulse the ISR emits is counted here, whatever produced it, so the live
// position is known during position moves, jogs and homing alike. execute()
// folds the count into current_step_position.
void Slider::__count_step()
{
    step_delta += current_direction ? -1 : 1;
}

void Slider::__execute_pos()
{
    pulse_counter++;
    __execute_vel();

    if (current_speed == desire_speed && pulse_for_accel == 0)
    {
        pulse_for_accel = pulse_counter;
    }
    else if ((pulse_counter >= (total_pulse / 2U)) && pulse_for_accel == 0)
    {
        pulse_for_accel = int(total_pulse / 2U);
    }

    // >= rather than ==: total_pulse can be re-evaluated between steps, and an
    // exact match that is missed once would run the axis to the far end
    if (pulse_counter >= (total_pulse - pulse_for_accel) && desire_speed != min_speed)
    {
        execute_speed(min_speed);
    }

    if (pulse_counter >= total_pulse)
    {
        __pause_step_timer();
        // snap to the target and drop the pending step count in one go, so the
        // main loop cannot add stale steps on top of the exact end position
        current_step_position = desire_step_position;
        step_delta = 0;

        pulse_counter = 0;
        total_pulse = 0;
        pulse_for_accel = 0;
        current_speed = 0;
        desire_speed = 0;
        at_position = true;
        is_moving_pos = false;

        if (reply_pending)
        {
            reply_pending = false;
            is_need_send_done = true;
        }
    }
}

void Slider::execute_speed(float _speed)
{
    desire_speed = abs(_speed);
    if (desire_speed == current_speed)
    {
        return;
    }

    time_count = 0;
    if (desire_speed > current_speed)
    {
        current_accel = abs(current_accel);
    }
    else
    {
        current_accel = -abs(current_accel);
    }

    if (current_speed == 0)
    {
        current_speed = min_speed;
        if (current_accel == 0)
        {
            current_speed = desire_speed;
        }
        time_count = 0;
        setTimerPeriodFromSpeed(current_speed);
        __resume_step_timer();
    }
}

void Slider::init()
{
    current_step_position = 0;
    desire_step_position = 0;
    commanded_position = 0;
    current_accel = 0;
    min_speed = 2;
    slider_address = 1;
    desire_speed_pos = DEFAULT_SPEED; // overridden by load_data() if a valid value is stored
    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        input_raw_last[i] = false;
        input_pressed[i] = false;
        input_debounce_time[i] = 0;
        input_pressed_last[i] = false;
    }

    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        output_state_last[i] = false;
    }
    step_delta = 0;
    homing_state = HOMING_NONE;
    home_move_done = false;
    homing_phase_start = 0;
    home_switch_time = 0;
    home_switch_raw_last = false;
    reply_pending = false;
    is_homed = false;
    is_stepper_enable = true;

    is_stopping = false;
    has_pending_move = false;
    pending_position = 0;
    pending_from_gcode = false;
    is_jogging = false;
    jog_input = 0xFF;

    fault_code = FAULT_NONE;
    is_need_send_fault = false;

    negative_limit_raw_last = false;
    negative_limit_time = 0;
    positive_limit_raw_last = false;
    positive_limit_time = 0;

    load_mechanics_data();
    load_data();
    init_home_config();
    commanded_position = home_offset;

    // a move speed stored under a coarser scale may be unreachable now
    __clamp_move_speed();

    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        __seed_input_state(i);
    }
    pinInit();
    timerInit();
}

void Slider::__pause_step_timer()
{
    // clear the flag first: an interrupt that slips in between is rejected, which
    // costs one step of a motion that is being abandoned anyway
    step_timer_running = false;

#if defined(__STM32F1__)
    ExecuteStepTimer.pause();
#else
    ExecuteStepTimer->pause();
#endif
}

void Slider::__resume_step_timer()
{
    step_timer_running = true;

#if defined(__STM32F1__)
    ExecuteStepTimer.resume();
#else
    ExecuteStepTimer->resume();
#endif
}

void Slider::__timer_handle()
{
    // An update raised just before the timer was paused is still delivered. That
    // interrupt owns no step: stepping on it would move the axis after it was
    // told to stop and, worse, run __execute_pos() against an empty pulse budget,
    // which reads as "arrived at target".
    if (!step_timer_running)
    {
        return;
    }

    // Drive the step pin low and let the pulse timer raise it again after
    // STEP_PULSE_WIDTH. This falling edge is what the driver latches.
    digitalWrite(MOTOR_PUL_PIN, LOW);
#if defined(__STM32F1__)
    TurnPinTimer.resume();
#else
    TurnPinTimer->resume();
#endif

    __count_step();

    if (homing_state != HOMING_NONE)
    {
        if (homing_state == HOMING_BACKOFF)
        {
            __execute_home_pos();
        }
        else
        {
            __execute_vel();
        }
    }
    else if (is_stopping || is_jogging)
    {
        // controlled stops and hold-to-jog both use the velocity ramp
        __execute_vel();
    }
    else
    {
        __execute_pos();
    }
}

void Slider::__tp_timer_handle()
{
    digitalWrite(MOTOR_PUL_PIN, HIGH);
#if defined(__STM32F1__)
    TurnPinTimer.pause();
#else
    TurnPinTimer->pause();
#endif
}

// A move or a homing sequence answers its command port when it ends, not when it
// starts. Only requests that came from a gcode port own a reply: motions started
// from a physical input must stay silent or they desynchronise the host.
void Slider::__finish_reply(bool _ok, const char *_error)
{
    if (!reply_pending)
    {
        return;
    }

    reply_pending = false;

    if (_ok)
    {
        control_port.send_done();
    }
    else
    {
        control_port.response(_error);
    }
}

// ------------------------------------------------------------------ homing --

bool Slider::__home_switch_active()
{
    bool raw = false;
    if (!io_control.getInput(home_input, raw))
    {
        return false;
    }

    bool active = (raw == (bool)home_active_level);

    // the switch must stay stable for a few ms before it is believed
    if (active != home_switch_raw_last)
    {
        home_switch_raw_last = active;
        home_switch_time = millis();
        return false;
    }

    return active && (millis() - home_switch_time >= HOME_SWITCH_DEBOUNCE);
}

// Raw motion helpers used by homing. They bypass the mode gating of
// setVelocity()/setPosition() because homing owns the motor while it runs.
void Slider::__move_raw(float _speed)
{
    float _max = maxSpeed();
    if (_max > 0 && fabs(_speed) > _max)
    {
        _speed = (_speed < 0) ? -_max : _max;
    }

    bool negative = (_speed < 0);
    digitalWrite(MOTOR_DIR_PIN, negative ? !reverse_slider : reverse_slider);
    current_direction = negative;

    execute_speed(fabs(_speed));
}

void Slider::__stop_raw()
{
    __pause_step_timer();
    __sync_position();
    current_speed = 0;
    desire_speed = 0;
    pulse_counter = 0;
    pulse_for_accel = 0;
    total_pulse = 0;
    time_count = 0;
    step_delta = 0;
}

// fixed length move away from the switch, counted in pulses by the step ISR
void Slider::__start_home_backoff()
{
    home_move_done = false;
    pulse_counter = 0;
    pulse_for_accel = 0;
    current_speed = 0;
    total_pulse = (uint32_t)(home_backoff * pulse_per_mm);
    homing_state = HOMING_BACKOFF;

    if (total_pulse == 0)
    {
        home_move_done = true;
        return;
    }

    // Home is the negative-end sensor, so back off toward positive.
    __move_raw(home_speed_slow);
}

void Slider::__execute_home_pos()
{
    pulse_counter++;
    __execute_vel();

    if (pulse_counter >= total_pulse)
    {
        __pause_step_timer();
        current_speed = 0;
        desire_speed = 0;
        pulse_counter = 0;
        home_move_done = true;
    }
}

MOVE_RESULT Slider::home(bool _from_gcode)
{
    if (fault_code != FAULT_NONE)
    {
        return MOVE_ERR_FAULT;
    }


    // never interrupt a running move or another homing run: their own reply is
    // still owed to the host
    if (homing_state != HOMING_NONE || is_stopping || is_jogging || current_speed != 0 ||
        pulse_counter != 0)
    {
        return MOVE_ERR_BUSY;
    }

    enableStepper();
    __sync_position();

    is_homed = false;
    at_position = false;
    is_moving_pos = false;
    __cancel_pending_move("Unknown:Move stopped!");

    __stop_raw();

    homing_phase_start = millis();
    home_switch_time = millis();
    home_switch_raw_last = false;
    reply_pending = _from_gcode;

    // Already resting on the negative home sensor: skip the fast seek and move
    // away from it before the slow re-approach.
    bool raw = false;
    io_control.getInput(home_input, raw);
    if (raw == (bool)home_active_level)
    {
        home_switch_raw_last = true;
        __start_home_backoff();
        return MOVE_OK;
    }

    homing_state = HOMING_SEEK;
    __move_raw(-home_speed);
    return MOVE_OK;
}

bool Slider::isHoming()
{
    return homing_state != HOMING_NONE;
}

void Slider::abortHoming()
{
    if (homing_state == HOMING_NONE)
    {
        return;
    }

    // stop the step timer *before* clearing the state, otherwise an ISR firing in
    // between would be routed to the normal mode handler with homing pulse counts
    // still loaded
    __stop_raw();

    homing_state = HOMING_NONE;
    home_move_done = false;
    is_homed = false;

    // M320 answers asynchronously, so an abort still owes the host a reply. This
    // runs from the command handler that caused the abort, which is why it is sent
    // straight away: the aborted M320 must be answered before that command's own
    // reply goes out.
    __finish_reply(false, "Unknown:Homing stopped!");
}

void Slider::__execute_homing()
{
    if (homing_state == HOMING_NONE)
    {
        return;
    }

    if (millis() - homing_phase_start > HOMING_TIMEOUT)
    {
        __stop_raw();
        homing_state = HOMING_NONE;
        home_move_done = false;
        is_homed = false;
        __finish_reply(false, "Unknown:Homing timeout!");
        __raise_fault(FAULT_HOMING);
        return;
    }

    switch (homing_state)
    {
    case HOMING_SEEK:
        if (__home_switch_active())
        {
            __stop_raw();
            homing_phase_start = millis();
            __start_home_backoff();
        }
        break;

    case HOMING_BACKOFF:
        if (home_move_done)
        {
            home_move_done = false;

            bool raw = false;
            io_control.getInput(home_input, raw);
            if (raw == (bool)home_active_level)
            {
                // still triggered after backing off: switch stuck or back off too short
                __stop_raw();
                homing_state = HOMING_NONE;
                home_move_done = false;
                is_homed = false;
                __finish_reply(false, "Unknown:Home switch stuck!");
                __raise_fault(FAULT_HOMING);
                break;
            }

            homing_phase_start = millis();
            home_switch_raw_last = false;
            home_switch_time = millis();
            homing_state = HOMING_SEEK_SLOW;
            __move_raw(-home_speed_slow);
        }
        break;

    case HOMING_SEEK_SLOW:
        if (__home_switch_active())
        {
            __stop_raw();
            homing_state = HOMING_NONE;
            current_step_position = 0;
            desire_step_position = 0;
            commanded_position = home_offset;
            is_homed = true;
            at_position = true;
            __finish_reply(true, "");
        }
        break;

    default:
        break;
    }
}

bool Slider::__negative_limit_active()
{
    bool raw = false;
    if (!io_control.getInput(NEGATIVE_LIMIT_INPUT, raw))
    {
        return false;
    }

    bool active = (raw == (bool)DEDICATED_LIMIT_ACTIVE_LEVEL);
    if (active != negative_limit_raw_last)
    {
        negative_limit_raw_last = active;
        negative_limit_time = millis();
        return false;
    }

    return active && (millis() - negative_limit_time >= HOME_SWITCH_DEBOUNCE);
}

bool Slider::__positive_limit_active()
{
    bool raw = false;
    if (!io_control.getInput(POSITIVE_LIMIT_INPUT, raw))
    {
        return false;
    }

    bool active = (raw == (bool)DEDICATED_LIMIT_ACTIVE_LEVEL);
    if (active != positive_limit_raw_last)
    {
        positive_limit_raw_last = active;
        positive_limit_time = millis();
        return false;
    }

    return active && (millis() - positive_limit_time >= HOME_SWITCH_DEBOUNCE);
}

// Outside homing, the two NPN sensors are direction-aware hard limits. During
// homing, PB7 is owned by the homing state machine itself: treating it as a hard
// limit here would stop the axis before the home sequence could back off and
// re-approach. PB6 still protects an unexpectedly long positive back-off.
void Slider::__execute_travel_guard()
{
    if (current_speed == 0)
    {
        return;
    }

    if (homing_state != HOMING_NONE)
    {
        if (!current_direction && __positive_limit_active())
        {
            stopEmergency(FAULT_HOMING);
        }
        return;
    }

    if (current_direction && __negative_limit_active())
    {
        stopEmergency(FAULT_NEGATIVE_LIMIT);
        return;
    }

    if (!current_direction && __positive_limit_active())
    {
        stopEmergency(FAULT_POSITIVE_LIMIT);
        return;
    }
}

void Slider::setHomeConfig(uint8_t _reverse)
{
    if (_reverse <= 1)
    {
        reverse_slider = _reverse;
        digitalWrite(MOTOR_DIR_PIN, reverse_slider);
        save_mechanics_data();
    }
}

void Slider::enableStepper()
{
    if (is_stepper_enable)
    {
        return;
    }

    digitalWrite(MOTOR_EN_PIN, MOTOR_ENABLE_LEVEL);
    is_stepper_enable = true;
}

void Slider::disableStepper()
{
    // stop first, answer second: replying while the ISR is still stepping races
    // the end-of-move handler and can produce two replies for one command
    abortHoming();
    __stop_raw();
    is_stopping = false;
    is_jogging = false;
    jog_input = 0xFF;
    __finish_reply(false, "Unknown:Move stopped!");
    __cancel_pending_move("Unknown:Move stopped!");
    desire_step_position = current_step_position;
    commanded_position = __steps_to_position(current_step_position);

    digitalWrite(MOTOR_EN_PIN, MOTOR_DISABLE_LEVEL);
    is_stepper_enable = false;

    // the axis can be pushed by hand now, so the known position is lost
    is_homed = false;
    at_position = false;
    is_moving_pos = false;
}

// Fold the steps counted by the ISR into the tracked position. The add happens
// inside the critical section: if the ISR finished a move in between it would
// snap current_step_position to the target and zero the counter, and a late add
// here would corrupt it.
void Slider::__sync_position()
{
    if (step_delta == 0)
    {
        return;
    }

    noInterrupts();
    int32_t _delta = step_delta;
    step_delta = 0;
    current_step_position += _delta;
    interrupts();
}

float Slider::__steps_to_position(int64_t _steps)
{
    return home_offset + (float)((double)_steps / (double)pulse_per_mm);
}

float Slider::getPosition()
{
    noInterrupts();
    int32_t _delta = step_delta;
    step_delta = 0;
    current_step_position += _delta;
    int64_t _steps = current_step_position;
    interrupts();

    return __steps_to_position(_steps);
}

float Slider::getMoveSpeed()
{
    return desire_speed_pos;
}

float Slider::getAccel()
{
    return fabs(current_accel);
}

bool Slider::isMoving()
{
    return (current_speed != 0) || is_stopping || is_jogging ||
           (homing_state != HOMING_NONE);
}

// A stored move speed can become unreachable when the scale or the step period
// changes; ramping toward a speed the timer cannot produce breaks the decel
// calculation and the axis arrives at the target still moving.
void Slider::__clamp_move_speed()
{
    float _max = maxSpeed();
    if (_max > 0 && desire_speed_pos > _max)
    {
        desire_speed_pos = _max;
    }
}

// Enter the machine rather than a raw pulses-per-mm figure:
// pulse_per_mm = steps_per_rev / screw_lead. Either parameter may be 0 to keep
// its current value.
MOVE_RESULT Slider::setMechanics(float _steps_rev, float _lead)
{
    // changing the scale under a running move would corrupt the pulse budget
    if (homing_state != HOMING_NONE || is_stopping || is_jogging || current_speed != 0 ||
        pulse_counter != 0)
    {
        return MOVE_ERR_BUSY;
    }

    // validate into locals: a rejected command must leave the machine untouched
    float _new_steps = (_steps_rev > 0) ? _steps_rev : steps_per_rev;
    float _new_lead = (_lead > 0) ? _lead : screw_lead;

    float _ppm = _new_steps / _new_lead;
    if (isnan(_ppm) || _ppm < MIN_PULSE_PER_MM || _ppm > MAX_PULSE_PER_MM)
    {
        return MOVE_ERR_RANGE;
    }

    steps_per_rev = _new_steps;
    screw_lead = _new_lead;
    pulse_per_mm = _ppm;

    // the mm scale moved, so the stored zero no longer means what it did
    is_homed = false;
    __sync_position();
    desire_step_position = current_step_position;
    commanded_position = __steps_to_position(current_step_position);

    __clamp_move_speed();
    save_mechanics_data();
    save_data();

    return MOVE_OK;
}

bool Slider::__position_to_steps(double _pos, int64_t &_steps)
{
    if (!isfinite(_pos) || !isfinite(pulse_per_mm) || pulse_per_mm <= 0)
    {
        return false;
    }

    double _scaled = (_pos - (double)home_offset) * (double)pulse_per_mm;
    if (!isfinite(_scaled) ||
        _scaled < (double)INT32_MIN || _scaled > (double)INT32_MAX)
    {
        return false;
    }

    _steps = (int64_t)llround(_scaled);
    return true;
}

// Fires the move. The requested coordinate is retained for chaining relative
// commands, while the physical endpoint is rounded to the nearest whole pulse.
bool Slider::__start_move(double _pos, bool _from_gcode)
{
    int64_t _target_steps = 0;
    if (!__position_to_steps(_pos, _target_steps))
    {
        return false;
    }

    enableStepper();
    __sync_position();
    is_jogging = false;
    jog_input = 0xFF;
    reply_pending = _from_gcode;

    commanded_position = _pos;
    desire_step_position = _target_steps;
    pulse_counter = 0;
    pulse_for_accel = 0;
    total_pulse = 0;
    current_speed = 0;
    at_position = false;

    // only a move that actually steps claims the position handler, otherwise the
    // flag stays set and every later jog is refused as "busy"
    is_moving_pos = false;

    int64_t _current_steps = current_step_position;
    if (desire_step_position == _current_steps)
    {
        at_position = true;
        __finish_reply(true, "");
        return true;
    }
    else if (desire_step_position > _current_steps)
    {
        digitalWrite(MOTOR_DIR_PIN, reverse_slider);
        current_direction = false;
    }
    else
    {
        digitalWrite(MOTOR_DIR_PIN, !reverse_slider);
        current_direction = true;
    }

    uint64_t _distance = (desire_step_position > _current_steps)
                             ? (uint64_t)(desire_step_position - _current_steps)
                             : (uint64_t)(_current_steps - desire_step_position);
    total_pulse = (uint32_t)_distance;

    is_moving_pos = true;
    step_delta = 0;
    execute_speed(desire_speed_pos);
    return true;
}

MOVE_RESULT Slider::__set_position(double _pos, bool _from_gcode, bool _require_homed)
{
    if (fault_code != FAULT_NONE)
    {
        return MOVE_ERR_FAULT;
    }


    if (homing_state != HOMING_NONE || is_jogging)
    {
        return MOVE_ERR_BUSY;
    }

    // Absolute coordinates are referenced to home. Relative moves only need a
    // pulse distance, so they remain useful before homing and during recovery.
    if (_require_homed && !is_homed)
    {
        return MOVE_ERR_NOT_HOMED;
    }

    int64_t _target_steps = 0;
    if (!__position_to_steps(_pos, _target_steps))
    {
        return MOVE_ERR_RANGE;
    }

    // Retarget: something is already moving. Ramp it down and pick the new target
    // up when it stops, rather than refusing. The superseded command is answered
    // right away so the host never waits on a request that will not complete.
    if (is_stopping || current_speed != 0 || pulse_counter != 0)
    {
        // the move on the axis right now
        __finish_reply(false, "Unknown:Move superseded!");

        // and one that was queued by an earlier retarget and never started: it
        // owes its own reply, otherwise a stream of targets loses one per hop
        if (has_pending_move && pending_from_gcode)
        {
            control_port.response("Unknown:Move superseded!");
        }

        pending_position = _pos;
        commanded_position = _pos;
        pending_from_gcode = _from_gcode;
        has_pending_move = true;

        // A position command takes ownership from a physical jog input. Its
        // later release must not stop the newly queued position move.
        is_jogging = false;
        jog_input = 0xFF;

        if (!is_stopping)
        {
            __begin_stop_ramp();
        }
        return MOVE_OK;
    }

    __start_move(_pos, _from_gcode);
    return MOVE_OK;
}

MOVE_RESULT Slider::setPosition(double _pos, bool _from_gcode)
{
    return __set_position(_pos, _from_gcode, true);
}

MOVE_RESULT Slider::movePositionRelative(double _distance, bool _from_gcode)
{
    // Chain on the unquantized command coordinate. Repeated sub-step relative
    // moves therefore accumulate until they cross a real motor-step boundary.
    // This coordinate may be arbitrary before home, but the emitted distance is
    // still exact and is not promoted to a known absolute position.
    double _base = has_pending_move ? pending_position : commanded_position;
    return __set_position(_base + _distance, _from_gcode, false);
}

void Slider::setMoveSpeed(float _speed)
{
    float _new = fabs(_speed);

    // floor first, ceiling last: a speed the step timer cannot produce must never
    // survive, even when the achievable maximum is below min_speed
    if (_new < min_speed)
    {
        _new = min_speed;
    }
    if (maxSpeed() > 0 && _new > maxSpeed())
    {
        _new = maxSpeed();
    }

    bool _is_lower = (_new < desire_speed_pos);
    desire_speed_pos = _new;
    save_data();

    // Apply to the move already running, but only when the speed goes down.
    // The deceleration distance was latched at the old speed: raising it now
    // would leave too little room to slow down and the axis would arrive at the
    // target still moving. A higher speed therefore takes effect on the next move.
    // Test and act inside one critical section. If the ISR were allowed to finish
    // the move in between, execute_speed() would see current_speed == 0, take its
    // start-from-rest branch and resume the step timer with no pulse budget left
    // to stop it.
    noInterrupts();
    bool _apply = _is_lower && !is_stopping && homing_state == HOMING_NONE &&
                  is_moving_pos && current_speed != 0 && pulse_counter != 0 &&
                  total_pulse > pulse_for_accel &&
                  pulse_counter < (total_pulse - pulse_for_accel);
    if (_apply)
    {
        execute_speed(_new);
    }
    interrupts();
}

// A queued retarget owns its own reply: reply_pending was already consumed when
// the move it superseded was answered, so __finish_reply() cannot cover it.
void Slider::__cancel_pending_move(const char *_message)
{
    if (!has_pending_move)
    {
        return;
    }

    has_pending_move = false;

    if (pending_from_gcode)
    {
        control_port.response(_message);
    }
}

// Chỉ khởi động ramp dừng. Không đụng tới reply hay move đang xếp hàng, nên
// retarget dùng lại được.
void Slider::__begin_stop_ramp()
{

    if (current_speed == 0)
    {
        // nothing is turning, finish immediately
        is_stopping = false;
        __execute_stop_ramp_finish();
        return;
    }

    is_stopping = true;
    execute_speed(0);
}

// Dừng có giảm tốc. Vị trí vẫn được giữ vì mọi xung đều được đếm.
void Slider::stopSmooth()
{
    if (homing_state != HOMING_NONE)
    {
        abortHoming();
        return;
    }

    // An explicit stop cancels a queued retarget - otherwise the axis would pause
    // and then drive off to the queued target on its own.
    __cancel_pending_move("Unknown:Move stopped!");
    is_jogging = false;
    jog_input = 0xFF;

    // Answer the interrupted move now: it lands before the reply of whatever
    // command asked for the stop, which a sequential host needs.
    __finish_reply(false, "Unknown:Move stopped!");

    __begin_stop_ramp();
}

// Called by the main loop once the ramp has reached zero, and directly by
// stopSmooth() when the axis was already at rest.
void Slider::__execute_stop_ramp_finish()
{
    __sync_position();

    pulse_counter = 0;
    total_pulse = 0;
    pulse_for_accel = 0;
    desire_speed = 0;
    current_speed = 0;
    time_count = 0;
    is_moving_pos = false;
    at_position = false;

    if (has_pending_move)
    {
        has_pending_move = false;
        __start_move(pending_position, pending_from_gcode);
    }
    else
    {
        desire_step_position = current_step_position;
        commanded_position = __steps_to_position(current_step_position);
    }
}

void Slider::__execute_stop_ramp()
{
    if (!is_stopping || current_speed != 0)
    {
        return;
    }

    is_stopping = false;
    __execute_stop_ramp_finish();
}

// Dừng khẩn: cắt xung ngay. Có thể mất bước nên vị trí coi như không còn tin được.
void Slider::stopEmergency(FAULT_CODE _fault)
{
    // stop the step generator before anything else touches the reply state
    __pause_step_timer();

    __sync_position();

    is_stopping = false;
    is_jogging = false;
    jog_input = 0xFF;
    homing_state = HOMING_NONE;
    home_move_done = false;

    current_speed = 0;
    desire_speed = 0;
    pulse_counter = 0;
    total_pulse = 0;
    pulse_for_accel = 0;
    time_count = 0;
    is_moving_pos = false;
    at_position = false;
    desire_step_position = current_step_position;
    commanded_position = __steps_to_position(current_step_position);

    // an abrupt stop at speed can drop steps, so the zero is no longer trusted
    is_homed = false;

    const char *_message = "Unknown:Emergency stop!";
    switch (_fault)
    {
    case FAULT_NEGATIVE_LIMIT:
        _message = "Unknown:Negative limit hit!";
        break;
    case FAULT_POSITIVE_LIMIT:
        _message = "Unknown:Positive limit hit!";
        break;
    case FAULT_HOMING:
        _message = "Unknown:Homing failed!";
        break;
    default:
        break;
    }

    __finish_reply(false, _message);
    __cancel_pending_move(_message);

    if (_fault != FAULT_NONE)
    {
        __raise_fault(_fault);
    }
}

// M315 P kept its name; it is the controlled stop now
void Slider::__raise_fault(FAULT_CODE _fault)
{
    if (fault_code != FAULT_NONE)
    {
        // keep the first cause, it is the one that explains the rest
        return;
    }

    fault_code = _fault;
    is_need_send_fault = true;
}

FAULT_CODE Slider::getFault()
{
    return fault_code;
}

bool Slider::isFaulted()
{
    return fault_code != FAULT_NONE;
}

void Slider::clearFault()
{
    fault_code = FAULT_NONE;
    is_need_send_fault = false;
}

void Slider::setAccel(float _accel)
{
    float _new = fabs(_accel);
    if (_new < MIN_ACCEL)
    {
        // without a ramp the axis cannot be stopped in a controlled way
        _new = MIN_ACCEL;
    }

    current_accel = _new;
    save_data();
}

void Slider::setAddress(uint16_t _address)
{
    slider_address = _address;
    save_data();
}

void Slider::setInputFunction(uint8_t index, uint8_t func, float value)
{
    if (index >= INPUT_COUNT)
    {
        return;
    }
    if (is_jogging && jog_input == index)
    {
        __stop_jog(index);
    }
    if (index == NEGATIVE_LIMIT_INPUT || index == POSITIVE_LIMIT_INPUT)
    {
        // Keep the physical limit pins from also launching an IO action.
        input_function[index] = INPUT_FUNC_NONE;
        input_value[index] = 0;
        __seed_input_state(index);
        save_data();
        return;
    }
    if (func > INPUT_FUNC_JOG_HOLD)
    {
        func = INPUT_FUNC_NONE;
    }
    input_function[index] = func;
    input_value[index] = value;

    __seed_input_state(index);
    save_data();
}

void Slider::setOutputFunction(uint8_t index, uint8_t func, float value)
{
    if (index >= OUTPUT_COUNT)
    {
        return;
    }
    if (func > OUTPUT_FUNC_FAULT)
    {
        func = OUTPUT_FUNC_NONE;
    }
    output_function[index] = func;
    output_value[index] = value;
    output_state_last[index] = false;
    io_control.setOutput(index, 0);
    save_data();
}

// drive the outputs that have a function assigned from the real motion state,
// so they follow both gcode (M311/M312) and input commanded motions
void Slider::__execute_output()
{
    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        if (output_function[i] == OUTPUT_FUNC_NONE)
        {
            continue;
        }

        bool on = false;
        switch (output_function[i])
        {
        case OUTPUT_FUNC_POS_REACHED:
            on = at_position;
            break;
        case OUTPUT_FUNC_HOMED:
            on = is_homed;
            break;
        case OUTPUT_FUNC_POS_OVER:
            // position compare: the live position is up to date every loop, so
            // this switches while the move is still running
            on = (getPosition() >= output_value[i]);
            break;
        case OUTPUT_FUNC_FAULT:
            on = (fault_code != FAULT_NONE);
            break;
        default:
            break;
        }

        if (on != output_state_last[i])
        {
            io_control.setOutput(i, on);
            output_state_last[i] = on;
        }
    }
}

void Slider::__input_action(uint8_t index)
{
    // the emergency stop always works, even while homing or faulted
    if (input_function[index] == INPUT_FUNC_ESTOP)
    {
        stopEmergency(FAULT_ESTOP);
        return;
    }

    switch (input_function[index])
    {
    case INPUT_FUNC_GOTO_POS:
        movePositionRelative(input_value[index], false);
        break;
    case INPUT_FUNC_STOP:
        if (homing_state != HOMING_NONE)
        {
            abortHoming();
            break;
        }
        stopSmooth();
        break;
    case INPUT_FUNC_HOME:
        home(false);
        break;
    case INPUT_FUNC_JOG_HOLD:
        __start_jog(index, input_value[index]);
        break;
    default:
        break;
    }
}

// Start a free-running jog owned by one physical input. The sign selects the
// direction; the magnitude is the target speed in mm/s. Jogging is available
// before homing, but the two hard limits and latched faults still apply.
MOVE_RESULT Slider::__start_jog(uint8_t index, float speed)
{
    if (fault_code != FAULT_NONE)
    {
        return MOVE_ERR_FAULT;
    }
    if (index >= INPUT_COUNT || !isfinite(speed) || speed == 0)
    {
        return MOVE_ERR_RANGE;
    }
    if (homing_state != HOMING_NONE || is_stopping || is_jogging ||
        current_speed != 0 || pulse_counter != 0)
    {
        return MOVE_ERR_BUSY;
    }

    bool negative = speed < 0;
    float jog_speed = fabs(speed);
    if (jog_speed < min_speed)
    {
        jog_speed = min_speed;
    }
    float max_speed = maxSpeed();
    if (max_speed > 0 && jog_speed > max_speed)
    {
        jog_speed = max_speed;
    }

    enableStepper();
    __sync_position();
    __cancel_pending_move("Unknown:Move stopped!");

    pulse_counter = 0;
    total_pulse = 0;
    pulse_for_accel = 0;
    step_delta = 0;
    is_moving_pos = false;
    at_position = false;
    is_jogging = true;
    jog_input = index;

    __move_raw(negative ? -jog_speed : jog_speed);
    return MOVE_OK;
}

void Slider::__stop_jog(uint8_t index)
{
    if (!is_jogging || jog_input != index)
    {
        return;
    }

    is_jogging = false;
    jog_input = 0xFF;
    __begin_stop_ramp();
}

// An input only starts being sampled once it has a function, so its debounce
// state must be primed from the live pin. Without this an input that idles in the
// active state - a pulled-up pin with nothing wired, or a normally-closed safety
// button - produces a phantom press one debounce period later.
void Slider::__seed_input_state(uint8_t index)
{
    if (index >= INPUT_COUNT)
    {
        return;
    }

    bool raw = false;
    io_control.getInput(index, raw);

    bool active = raw;

    input_raw_last[index] = active;
    input_pressed[index] = active;
    input_pressed_last[index] = active;
    input_debounce_time[index] = millis();
}

void Slider::__execute_input()
{
    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        if (input_function[i] == INPUT_FUNC_NONE)
        {
            continue;
        }

        bool raw = false;
        io_control.getInput(i, raw);

        bool active = raw;

        if (active != input_raw_last[i])
        {
            input_raw_last[i] = active;
            input_debounce_time[i] = millis();
        }
        else if (millis() - input_debounce_time[i] > INPUT_DEBOUNCE)
        {
            input_pressed[i] = active;
        }

        // Most functions act on the press edge. Hold-to-jog additionally owns
        // the release edge so it can ramp down without stopping another motion.
        if (input_pressed[i] && !input_pressed_last[i])
        {
            __input_action(i);
        }
        else if (!input_pressed[i] && input_pressed_last[i] &&
                 input_function[i] == INPUT_FUNC_JOG_HOLD)
        {
            __stop_jog(i);
        }

        input_pressed_last[i] = input_pressed[i];
    }
}

void Slider::execute()
{
    __sync_position();
    __execute_travel_guard();
    __execute_homing();
    __execute_stop_ramp();

    if (is_need_send_fault)
    {
        is_need_send_fault = false;

        // an event, not a reply: prefixed so a host can tell them apart
        String _mes = "Fault:";
        _mes += String((int)fault_code);
        control_port.response(_mes);
    }

    if (is_need_send_done)
    {
        control_port.send_done();
        is_need_send_done = false;
    }

    __execute_input();
    __execute_output();
}

Slider slider;
