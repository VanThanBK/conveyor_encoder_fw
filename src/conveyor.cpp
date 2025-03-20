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
    TurnPinTimer = new HardwareTimer(TIM4);
    TurnPinTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    TurnPinTimer->setPreloadEnable(0);
    TurnPinTimer->setPrescaleFactor(TurnPinTimer->getTimerClkFreq() / 1000000);
    TurnPinTimer->setOverflow(30);
    TurnPinTimer->pause();

    ExecuteStepTimer = new HardwareTimer(TIM3);
    ExecuteStepTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    ExecuteStepTimer->setPrescaleFactor(ExecuteStepTimer->getTimerClkFreq() / 1000000);
    ExecuteStepTimer->setPreloadEnable(0);
    ExecuteStepTimer->setOverflow(500000);
    ExecuteStepTimer->pause();
}

// func eeprom -------------------------------------------------------------
void Conveyor::putFloatToEeprom(uint16_t address, float floatout)
{
    uint16_t floatH = floatout / 1;
    uint16_t floatL = (floatout - (float)floatH) * 1000;
    EEPROM.update(address, floatH);
    EEPROM.update(address + 1, floatL);
}

void Conveyor::getFloatFromEeprom(uint16_t address, float &floatin)
{
    uint16_t floatH = EEPROM.read(address);
    uint16_t floatL = EEPROM.read(address + 1);
    floatin = (float)floatH + (float)floatL / 1000;
}

void Conveyor::save_data()
{
    EEPROM.update(REVERSE_CONVEYOR_ADDRESS, reverse_conveyor);
    EEPROM.update(IS_CONVEYOR_MODE_ADDRESS, conveyor_mode);
    putFloatToEeprom(PULSE_PER_MM_CONVEYOR_ADDRESS, pulse_per_mm);
    EEPROM.update(IS_RUN_WITH_ENCODER_ADDRESS, is_run_with_encoder);

    putFloatToEeprom(CONVEYOR_ACCEL_ADDRESS, current_accel);

    putFloatToEeprom(SPEED_WHEN_RUN_WITH_BUTTON_ADDRESS, speed_when_run_with_button);
}

void Conveyor::load_data()
{
    max_speed = DEFAULT_MAX_SPEED;
    reverse_conveyor = (bool)EEPROM.read(REVERSE_CONVEYOR_ADDRESS);
    conveyor_mode = (CONVEYOR_MODE)EEPROM.read(IS_CONVEYOR_MODE_ADDRESS);
    getFloatFromEeprom(PULSE_PER_MM_CONVEYOR_ADDRESS, pulse_per_mm);
    is_run_with_encoder = (bool)EEPROM.read(IS_RUN_WITH_ENCODER_ADDRESS);
    getFloatFromEeprom(CONVEYOR_ACCEL_ADDRESS, current_accel);
    getFloatFromEeprom(SPEED_WHEN_RUN_WITH_BUTTON_ADDRESS, speed_when_run_with_button);
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

void Conveyor::setTimerPeriodFromSpeed(float _speed)
{
    if (_speed == 0)
    {
        ExecuteStepTimer->pause();
        return;
    }
    current_period = 1000000.0 / (_speed * pulse_per_mm);
    if (current_period < MIN_STEP_TIMER_PERIOD)
    {
        current_period = MIN_STEP_TIMER_PERIOD;
    }
    ExecuteStepTimer->pause();
    ExecuteStepTimer->setOverflow(current_period);
    ExecuteStepTimer->resume();
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
    else if ((pulse_counter >= (uint16_t)(total_pulse / 2)) && pulse_for_accel == 0)
    {
        pulse_for_accel = int(total_pulse / 2);
    }
    
    if (pulse_counter == total_pulse - pulse_for_accel)
    {
        execute_speed(min_speed);
    }

    if (pulse_counter == total_pulse)
    {
        // setVelocity(0);
        current_position = desire_position;
        pulse_counter = 0;
        ExecuteStepTimer->pause();
        control_port.send_done();
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

    if (current_speed == 0 )
    {
        current_speed = min_speed;
        if (current_accel == 0)
        {
            current_speed = desire_speed;
        }
        time_count = 0;
        setTimerPeriodFromSpeed(current_speed);
        ExecuteStepTimer->resume();
    }
}

void Conveyor::init()
{
    current_position = 0;
    current_accel = 0;
    min_speed = 2;
    current_vel_button = 0;
    load_data();
    load_auto_run_data();
    pinInit();
    timerInit();
}

void Conveyor::__timer_handle()
{
    digitalWrite(MOTOR_PUL_PIN, LOW);
    TurnPinTimer->resume();

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
    case CONVEYOR_VEL_INPUT:
        __execute_vel();
        break;
    default:
        break;
    }
}

void Conveyor::__tp_timer_handle()
{
    digitalWrite(MOTOR_PUL_PIN, HIGH);
}
                        
void Conveyor::setConveyorMode(uint8_t _enable)
{
    if ((CONVEYOR_MODE)_enable == conveyor_mode)
    {
        return;
    }
    conveyor_mode = (CONVEYOR_MODE)_enable;

    ExecuteStepTimer->pause();
    TurnPinTimer->pause();

    digitalWrite(MOTOR_EN_PIN, HIGH);

    current_position = 0;
    current_speed = 0;
    pulse_counter = 0;
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
    max_speed = 1000000 / (pulse_per_mm * MIN_STEP_TIMER_PERIOD);
    save_data();
}

void Conveyor::setRunWithEncoder(bool _enable)
{
    is_run_with_encoder = _enable;
    save_data();
}

void Conveyor::setPosition(float _pos)
{
    if (conveyor_mode != CONVEYOR_POS)
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
        control_port.send_done();
        return;
    }
    else if (desire_position > current_position)
    {
        digitalWrite(MOTOR_DIR_PIN, reverse_conveyor);
    }
    else
    {
        digitalWrite(MOTOR_DIR_PIN, !reverse_conveyor);
    }
    
    current_speed = 0;
    pulse_for_accel = 0;

    total_pulse = abs(desire_position - current_position) * pulse_per_mm;

    execute_speed(desire_speed_pos);
}

void Conveyor::setVelocity(float _speed)
{
    if (conveyor_mode != CONVEYOR_VEL && conveyor_mode != CONVEYOR_VEL_INPUT)
    {
        return;
    }

    if (_speed < 0)
    {
        digitalWrite(MOTOR_DIR_PIN, !reverse_conveyor);
    }
    else
    {
        digitalWrite(MOTOR_DIR_PIN, reverse_conveyor);
    }

    execute_speed(_speed);
}

void Conveyor::setVelocityPos(float _speed)
{
    desire_speed_pos = abs(_speed);
}

void Conveyor::stopPosition()
{
    current_speed = 0;
    current_position += (pulse_counter / pulse_per_mm) * (desire_position - current_position) / abs(desire_position - current_position);
    ExecuteStepTimer->pause();
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

void Conveyor::execute()
{
    if (conveyor_mode == CONVEYOR_VEL_INPUT)
    {
        if (millis() - last_read_potentiometer_time > read_potentiometer_period)
        {
            last_read_potentiometer_time = millis();
            
            if (read_potentiometer_counter < 10)
            {
                raw_potentiometer_values += analogRead(POTENTIOMETER_PIN);
                read_potentiometer_counter++;
            }
            else
            {
                potentiometer_value = raw_potentiometer_values / 10;
                raw_potentiometer_values = 0;
                read_potentiometer_counter = 0;
                float _speed = map(potentiometer_value, 0, 1023, -max_speed, max_speed);
                if (abs(_speed) < 5)
                {
                    _speed = 0;
                }
                setVelocity(_speed);
            }
            
        }
    }
}

Conveyor conveyor;
