#include "conveyor.h"

void Conveyor::pinInit()
{
    pinMode(MOTOR_PUL_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_EN_PIN, OUTPUT);

    digitalWrite(MOTOR_PUL_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_EN_PIN, HIGH);
}

void Conveyor::timerInit()
{
#if defined(__STM32F1__)
    TurnPinTimer.setPeriod(50000);
    TurnPinTimer.pause();

    ExecuteStepTimer.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
    ExecuteStepTimer.setCompare(TIMER_CH1, 1);
    ExecuteStepTimer.pause();

    TurnPinTimer.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
    TurnPinTimer.setCompare(TIMER_CH1, 1);
    TurnPinTimer.setPeriod(30);
    TurnPinTimer.pause();
#else
    TurnPinTimer = new HardwareTimer(TURN_PIN_TIMER);
    TurnPinTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    TurnPinTimer->setCaptureCompare(1, 1, MICROSEC_COMPARE_FORMAT);
    TurnPinTimer->setOverflow(30, MICROSEC_FORMAT);
    TurnPinTimer->pause();

    ExecuteStepTimer = new HardwareTimer(EXECUTE_STEP_TIMER);
    ExecuteStepTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    ExecuteStepTimer->setCaptureCompare(1, 1);
    ExecuteStepTimer->setPrescaleFactor(ExecuteStepTimer->getTimerClkFreq() / 1000000);
    ExecuteStepTimer->setOverflow(65000);
    ExecuteStepTimer->pause();
#endif
}

// func eeprom -------------------------------------------------------------
void Conveyor::putFloatToEeprom(uint16_t address, float floatout)
{
    uint16_t floatH = floatout / 1;
    uint16_t floatL = (floatout - (float)floatH) * 1000;
    EEPROM.update(address, floatH);
    EEPROM.update(address + 1, floatL);
}

void Conveyor::getFloatFromEeprom(uint16_t address, volatile float &floatin)
{
    uint16_t floatH = EEPROM.read(address);
    uint16_t floatL = EEPROM.read(address + 1);
    floatin = (float)floatH + (float)floatL / 1000;
}

// raw 4-byte float store (handles negative / large values, e.g. target position)
void Conveyor::putRawFloat(uint16_t address, float value)
{
    uint8_t *p = (uint8_t *)&value;
    for (uint8_t i = 0; i < 4; i++)
    {
        EEPROM.update(address + i, p[i]);
    }
}

float Conveyor::getRawFloat(uint16_t address)
{
    float value;
    uint8_t *p = (uint8_t *)&value;
    for (uint8_t i = 0; i < 4; i++)
    {
        p[i] = EEPROM.read(address + i);
    }
    return value;
}

void Conveyor::save_data()
{
    EEPROM.update(CONVEYOR_ADDRESS, conveyor_address & 0xFF);
    EEPROM.update(CONVEYOR_ADDRESS + 1, conveyor_address >> 8);
    EEPROM.update(REVERSE_CONVEYOR_ADDRESS, reverse_conveyor);
    EEPROM.update(IS_CONVEYOR_MODE_ADDRESS, conveyor_mode);
    putFloatToEeprom(PULSE_PER_MM_CONVEYOR_ADDRESS, pulse_per_mm);
    EEPROM.update(IS_RUN_WITH_ENCODER_ADDRESS, is_run_with_encoder);
    putFloatToEeprom(CONVEYOR_ACCEL_ADDRESS, current_accel);
    putFloatToEeprom(SPEED_WHEN_RUN_WITH_BUTTON_ADDRESS, speed_when_run_with_button);
    putRawFloat(POS_SPEED_ADDRESS, desire_speed_pos);
    for (uint8_t i = 0; i < IO_COUNT; i++)
    {
        EEPROM.update(INPUT_FUNCTION_ADDRESS + i, input_function[i]);
        putRawFloat(INPUT_VALUE_ADDRESS + i * 4, input_value[i]);
        EEPROM.update(OUTPUT_FUNCTION_ADDRESS + i, output_function[i]);
        putRawFloat(OUTPUT_VALUE_ADDRESS + i * 4, output_value[i]);
    }
}

void Conveyor::load_data()
{
    conveyor_address = EEPROM.read(CONVEYOR_ADDRESS) | (EEPROM.read(CONVEYOR_ADDRESS + 1) << 8);
    reverse_conveyor = (bool)EEPROM.read(REVERSE_CONVEYOR_ADDRESS);
    conveyor_mode = (CONVEYOR_MODE)EEPROM.read(IS_CONVEYOR_MODE_ADDRESS);
    if (conveyor_mode > CONVEYOR_INPUT)
    {
        conveyor_mode = CONVEYOR_INPUT;
    }
    getFloatFromEeprom(PULSE_PER_MM_CONVEYOR_ADDRESS, pulse_per_mm);
    is_run_with_encoder = (bool)EEPROM.read(IS_RUN_WITH_ENCODER_ADDRESS);
    getFloatFromEeprom(CONVEYOR_ACCEL_ADDRESS, current_accel);
    getFloatFromEeprom(SPEED_WHEN_RUN_WITH_BUTTON_ADDRESS, speed_when_run_with_button);
    desire_speed_pos = getRawFloat(POS_SPEED_ADDRESS);
    if (isnan(desire_speed_pos) || desire_speed_pos <= 0)
    {
        desire_speed_pos = DEFAULT_SPEED;
    }
    for (uint8_t i = 0; i < IO_COUNT; i++)
    {
        input_function[i] = EEPROM.read(INPUT_FUNCTION_ADDRESS + i);
        if (input_function[i] > INPUT_FUNC_STOP)
        {
            input_function[i] = INPUT_FUNC_NONE;
        }
        input_value[i] = getRawFloat(INPUT_VALUE_ADDRESS + i * 4);
        if (isnan(input_value[i]))
        {
            input_value[i] = 0;
        }
        output_function[i] = EEPROM.read(OUTPUT_FUNCTION_ADDRESS + i);
        if (output_function[i] > OUTPUT_FUNC_VEL_REACHED)
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

void Conveyor::save_auto_run_data()
{
    EEPROM.update(IS_AUTO_RUN_SPEED_ADDRESS, is_auto_run_speed);
    putFloatToEeprom(AUTO_RUN_SPEED_ADDRESS, auto_run_speed);
}

void Conveyor::load_auto_run_data()
{
    is_auto_run_speed = (uint8_t)EEPROM.read(IS_AUTO_RUN_SPEED_ADDRESS);
    getFloatFromEeprom(AUTO_RUN_SPEED_ADDRESS, auto_run_speed);
}

// fastest speed the step timer can physically generate (period clamp at
// MIN_STEP_TIMER_PERIOD): commanded speeds above this are phantom - the motor
// cannot follow them and stopping would ramp down from a speed that never existed
float Conveyor::maxSpeed()
{
    if (pulse_per_mm < 0.01f)
    {
        return 0;
    }
    return 1000000.0f / (MIN_STEP_TIMER_PERIOD * pulse_per_mm);
}

void Conveyor::setTimerPeriodFromSpeed(float _speed)
{
    if (_speed == 0)
    {
#if defined(__STM32F1__)
        ExecuteStepTimer.pause();
#else
        ExecuteStepTimer->pause();
#endif
        return;
    }
    current_period = 1000000.0 / (_speed * pulse_per_mm);
    if (current_period < MIN_STEP_TIMER_PERIOD)
    {
        current_period = MIN_STEP_TIMER_PERIOD;
    }

#if defined(__STM32F1__)
    ExecuteStepTimer.setPeriod(current_period);
#else
    ExecuteStepTimer->setOverflow(current_period);
#endif
}

void Conveyor::__execute_vel()
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

void Conveyor::__execute_pos()
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

    if (pulse_counter == total_pulse - pulse_for_accel)
    {
        execute_speed(min_speed);
    }

    if (pulse_counter == total_pulse)
    {
#if defined(__STM32F1__)
        ExecuteStepTimer.pause();
#else
        ExecuteStepTimer->pause();
#endif
        current_position = desire_position;
        pulse_counter = 0;
        current_speed = 0;
        at_position = true;
        is_need_send_done = true;
    }
}

void Conveyor::execute_speed(float _speed)
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
#if defined(__STM32F1__)
        ExecuteStepTimer.resume();
#else
        ExecuteStepTimer->resume();
#endif
    }
}

void Conveyor::init()
{
    current_position = 0;
    current_accel = 0;
    min_speed = 2;
    current_vel_button = 0;
    conveyor_address = 1;
    input_target_speed = 0;
    desire_speed_pos = DEFAULT_SPEED; // overridden by load_data() if a valid value is stored
    for (uint8_t i = 0; i < IO_COUNT; i++)
    {
        input_raw_last[i] = false;
        input_pressed[i] = false;
        input_debounce_time[i] = 0;
        input_pressed_last[i] = false;
        input_press_time[i] = 0;
        input_last_repeat[i] = 0;
        output_state_last[i] = false;
    }
    load_data();
    load_auto_run_data();
    pinInit();
    timerInit();
}

void Conveyor::__timer_handle()
{
    digitalWrite(MOTOR_PUL_PIN, LOW);
#if defined(__STM32F1__)
    TurnPinTimer.resume();
#else
    TurnPinTimer->resume();
#endif

    switch (conveyor_mode)
    {
    case OUTPUT_PIN:
        /* code */
        break;
    case CONVEYOR_POS:
        __execute_pos();
        break;
    case CONVEYOR_VEL:
        __execute_vel();
        break;
    case CONVEYOR_INPUT:
        if (input_run_pos)
        {
            __execute_pos();
        }
        else
        {
            __execute_vel();
        }
        break;
    default:
        break;
    }
}

void Conveyor::__tp_timer_handle()
{
    digitalWrite(MOTOR_PUL_PIN, HIGH);
#if defined(__STM32F1__)
    TurnPinTimer.pause();
#else
    TurnPinTimer->pause();
#endif
}

void Conveyor::setConveyorMode(uint8_t _enable)
{
    if ((CONVEYOR_MODE)_enable == conveyor_mode)
    {
        return;
    }
    conveyor_mode = (CONVEYOR_MODE)_enable;

#if defined(__STM32F1__)
    ExecuteStepTimer.pause();
    TurnPinTimer.pause();
#else
    ExecuteStepTimer->pause();
    TurnPinTimer->pause();
#endif
    digitalWrite(MOTOR_EN_PIN, HIGH);

    current_position = 0;
    current_speed = 0;
    pulse_counter = 0;
    input_target_speed = 0;
    input_run_pos = false;
    wait_for_reverse = false;
    at_position = false;
    save_data();
}

void Conveyor::setReverseConveyor(bool _dir)
{
    reverse_conveyor = _dir;
    digitalWrite(MOTOR_DIR_PIN, reverse_conveyor);
    save_data();
}

void Conveyor::setPulsePerMm(float _pulse)
{
    pulse_per_mm = _pulse;
    save_data();
}

void Conveyor::setRunWithEncoder(bool _enable)
{
    is_run_with_encoder = _enable;
    save_data();
}

void Conveyor::setPosition(float _pos)
{
    if (conveyor_mode != CONVEYOR_POS && conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }

    // input mode: don't start a position move while a velocity run is active
    if (conveyor_mode == CONVEYOR_INPUT && !input_run_pos && (current_speed != 0 || wait_for_reverse))
    {
        return;
    }

    if (pulse_counter != 0)
    {
        return;
    }
    desire_position = _pos;
    if (desire_position == current_position)
    {
        at_position = true;
        control_port.send_done();
        return;
    }
    else if (desire_position > current_position)
    {
        digitalWrite(MOTOR_DIR_PIN, reverse_conveyor);
        current_direction = false;
    }
    else
    {
        digitalWrite(MOTOR_DIR_PIN, !reverse_conveyor);
        current_direction = true;
    }

    current_speed = 0;
    pulse_for_accel = 0;
    input_run_pos = true;
    at_position = false;

    total_pulse = abs(desire_position - current_position) * pulse_per_mm;

    execute_speed(desire_speed_pos);
}

void Conveyor::setVelocity(float _speed)
{
    if (conveyor_mode != CONVEYOR_VEL && conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }

    // input mode: don't disturb an ongoing position move
    if (isPosMoveActive())
    {
        return;
    }

    // clamp to the physically achievable speed
    float _max = maxSpeed();
    if (_max > 0)
    {
        if (_speed > _max)
        {
            _speed = _max;
        }
        else if (_speed < -_max)
        {
            _speed = -_max;
        }
    }

    input_run_pos = false;
    if (_speed != 0)
    {
        at_position = false;
    }

    last_target_speed = _speed;

    if (_speed == 0)
    {
        wait_for_reverse = false;
        execute_speed(0);
        return;
    }

    bool new_direction = _speed < 0;

    if (current_speed > 0 && new_direction != current_direction)
    {
        wait_for_reverse = true;
        execute_speed(0);
        return;
    }
    wait_for_reverse = false;
    if (new_direction)
    {
        digitalWrite(MOTOR_DIR_PIN, !reverse_conveyor);
        current_direction = true;
    }
    else
    {
        digitalWrite(MOTOR_DIR_PIN, reverse_conveyor);
        current_direction = false;
    }

    execute_speed(_speed);
}

void Conveyor::setVelocityPos(float _speed)
{
    desire_speed_pos = abs(_speed);
    if (maxSpeed() > 0 && desire_speed_pos > maxSpeed())
    {
        desire_speed_pos = maxSpeed();
    }
    save_data();
}

void Conveyor::stopPosition()
{
#if defined(__STM32F1__)
    ExecuteStepTimer.pause();
#else
    ExecuteStepTimer->pause();
#endif
    current_speed = 0;
    if (desire_position != current_position)
    {
        current_position += (pulse_counter / pulse_per_mm) * (desire_position - current_position) / abs(desire_position - current_position);
    }
    pulse_counter = 0;
}

void Conveyor::setAccel(float _accel)
{
    current_accel = _accel;
    save_data();
}

void Conveyor::setOutput(uint8_t _pin, uint8_t _value)
{
    if (conveyor_mode != OUTPUT_PIN)
        return;

    if (_pin == 0)
    {
        digitalWrite(MOTOR_PUL_PIN, _value);
    }
    else if (_pin == 1)
    {
        digitalWrite(MOTOR_DIR_PIN, _value);
    }
    else if (_pin == 2)
    {
        digitalWrite(MOTOR_EN_PIN, _value);
    }
}

void Conveyor::startFromButton()
{
    if (conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }
    current_vel_button = speed_when_run_with_button;
    setVelocity(current_vel_button);
}

void Conveyor::stopFromButton()
{
    if (conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }
    current_vel_button = 0;
    setVelocity(current_vel_button);
}

void Conveyor::increaseSpeedFromButton()
{
    if (conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }
    current_vel_button += 5;
    if (maxSpeed() > 0 && current_vel_button > maxSpeed())
    {
        current_vel_button = maxSpeed();
    }
    setVelocity(current_vel_button);
}

void Conveyor::decreaseSpeedFromButton()
{
    if (conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }
    current_vel_button -= 5;
    if (maxSpeed() > 0 && current_vel_button < -maxSpeed())
    {
        current_vel_button = -maxSpeed();
    }
    setVelocity(current_vel_button);
}

void Conveyor::setVelocityButton(float _speed)
{
    speed_when_run_with_button = _speed;
}

void Conveyor::setAddress(uint16_t _address)
{
    conveyor_address = _address;
    save_data();
}

void Conveyor::setInputFunction(uint8_t index, uint8_t func, float value)
{
    if (index >= IO_COUNT)
    {
        return;
    }
    if (func > INPUT_FUNC_STOP)
    {
        func = INPUT_FUNC_NONE;
    }
    input_function[index] = func;
    input_value[index] = value;
    save_data();
}

void Conveyor::setOutputFunction(uint8_t index, uint8_t func, float value)
{
    if (index >= IO_COUNT)
    {
        return;
    }
    if (func > OUTPUT_FUNC_VEL_REACHED)
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
void Conveyor::__execute_output()
{
    for (uint8_t i = 0; i < IO_COUNT; i++)
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
        case OUTPUT_FUNC_VEL_REACHED:
            if (output_value[i] != 0)
            {
                float signed_speed = current_direction ? -current_speed : current_speed;
                if (output_value[i] > 0)
                {
                    on = (signed_speed >= output_value[i]);
                }
                else
                {
                    on = (signed_speed <= output_value[i]);
                }
            }
            else
            {
                bool pos_motion = (conveyor_mode == CONVEYOR_POS) ||
                                  (conveyor_mode == CONVEYOR_INPUT && input_run_pos);
                on = (!pos_motion && desire_speed > 0 && current_speed == desire_speed);
            }
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

bool Conveyor::isPosMoveActive()
{
    return input_run_pos && (pulse_counter != 0 || current_speed != 0);
}

void Conveyor::__input_action(uint8_t index)
{
    if (conveyor_mode != CONVEYOR_INPUT)
    {
        return;
    }

    switch (input_function[index])
    {
    case INPUT_FUNC_SPEED_UP:
        if (!isPosMoveActive())
        {
            input_target_speed += input_value[index];
            if (maxSpeed() > 0 && input_target_speed > maxSpeed())
            {
                input_target_speed = maxSpeed();
            }
            setVelocity(input_target_speed);
        }
        break;
    case INPUT_FUNC_SPEED_DOWN:
        if (!isPosMoveActive())
        {
            input_target_speed -= input_value[index];
            if (maxSpeed() > 0 && input_target_speed < -maxSpeed())
            {
                input_target_speed = -maxSpeed();
            }
            setVelocity(input_target_speed);
        }
        break;
    case INPUT_FUNC_GOTO_POS:
        setPosition(current_position + input_value[index]);
        break;
    case INPUT_FUNC_STOP:
        if (isPosMoveActive())
        {
            stopPosition();
        }
        input_target_speed = 0;
        setVelocity(0);
        break;
    default:
        break;
    }
}

void Conveyor::__execute_input()
{
    for (uint8_t i = 0; i < IO_COUNT; i++)
    {
        if (input_function[i] == INPUT_FUNC_NONE)
        {
            continue;
        }

        bool raw = false;
        io_control.getInput(i, raw);

        if (raw != input_raw_last[i])
        {
            input_raw_last[i] = raw;
            input_debounce_time[i] = millis();
        }
        else if (millis() - input_debounce_time[i] > INPUT_DEBOUNCE)
        {
            input_pressed[i] = raw;
        }

        bool pressed = input_pressed[i];

        if (pressed && !input_pressed_last[i])
        {
            __input_action(i);
            input_press_time[i] = millis();
            input_last_repeat[i] = millis();
        }
        else if (pressed && input_pressed_last[i])
        {
            if ((input_function[i] == INPUT_FUNC_SPEED_UP || input_function[i] == INPUT_FUNC_SPEED_DOWN) &&
                millis() - input_press_time[i] > INPUT_HOLD_DELAY &&
                millis() - input_last_repeat[i] > INPUT_REPEAT_PERIOD)
            {
                __input_action(i);
                input_last_repeat[i] = millis();
            }
        }

        input_pressed_last[i] = pressed;
    }
}

void Conveyor::execute()
{
    if (is_need_send_done)
    {
        control_port.send_done();
        is_need_send_done = false;
    }

    __execute_input();
    __execute_output();

    if (current_speed == 0 && wait_for_reverse)
    {

        bool new_dir_is_reverse = (last_target_speed < 0);

        if (new_dir_is_reverse)
        {
            digitalWrite(MOTOR_DIR_PIN, !reverse_conveyor);
            current_direction = true;
        }
        else
        {
            digitalWrite(MOTOR_DIR_PIN, reverse_conveyor);
            current_direction = false;
        }

        wait_for_reverse = false;
        execute_speed(last_target_speed);
    }
}

Conveyor conveyor;
