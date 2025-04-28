#include "encoder.h"

/*****************************************************************/
// func interrupt for encoder
void interrupt_channel_a_handel()
{
    encoder.encoder_handle(CHANNEL_A);
}

void interrupt_channel_b_handel()
{
    encoder.encoder_handle(CHANNEL_B);
}

void interrupt_auto_timer_handle()
{
    control_port.send_encoder_pos();
}

//------------------------------------------------------------------

/********************************************************************/
// encoder func
void Encoder::init()
{
    clear_buffer_data();
    load_data();
    pinInit();
    attachInterruptEncoderPin();

    AutoFeedbackTimer = new HardwareTimer(TIM2);
    AutoFeedbackTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    AutoFeedbackTimer->setPrescaleFactor(AutoFeedbackTimer->getTimerClkFreq() / 1000000);
    AutoFeedbackTimer->setOverflow(1000, MICROSEC_FORMAT);
    AutoFeedbackTimer->setPreloadEnable(0);
    AutoFeedbackTimer->pause();

    AutoFeedbackTimer->attachInterrupt(interrupt_auto_timer_handle);

    a_input_state = digitalRead(a_pin_encoder);
    b_input_state = digitalRead(b_pin_encoder);

    pulse_counter = 0;
}

void Encoder::pinInit()
{
    a_pin_encoder = ENCODER_A_PIN;
    b_pin_encoder = ENCODER_B_PIN;
    pinMode(b_pin_encoder, INPUT_PULLUP);
    pinMode(b_pin_encoder, INPUT_PULLUP);
}

void Encoder::attachInterruptEncoderPin()
{
    if (encoder_mode == AS_INPUT_PIN)
    {
        // attach interrupt a channel encoder
        attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), interrupt_channel_a_handel, CHANGE);
        // attach interrupt b channel encoder
        attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), interrupt_channel_b_handel, CHANGE);
        return;
    }

    detachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN));
    detachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN));

    if (encoder_scale == SCALE_X1)
    {
        // attach interrupt a channel encoder
        attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), interrupt_channel_a_handel, RISING);
    }
    else if (encoder_scale == SCALE_X2)
    {
        // attach interrupt a channel encoder

        attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), interrupt_channel_a_handel, RISING);
        attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), interrupt_channel_b_handel, RISING);
        
    }
    else if (encoder_scale == SCALE_X4)
    {
        // attach interrupt a channel encoder
        attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), interrupt_channel_a_handel, CHANGE);
        // attach interrupt b channel encoder
        attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), interrupt_channel_b_handel, CHANGE);
    }
}

void Encoder::encoder_handle(uint8_t _channel)
{
    if (encoder_mode == AS_INPUT_PIN)
    {
        if (_channel == CHANNEL_A)
        {
            a_input_state = digitalRead(a_pin_encoder);
        }
        else
        {
            b_input_state = digitalRead(b_pin_encoder);
        }

        return;
    }

    // get pin of encoder
    uint8_t _main_pin = 0;
    uint8_t _secondary_pin = 0;

    if (_channel == CHANNEL_A)
    {
        _main_pin = a_pin_encoder;
        _secondary_pin = b_pin_encoder;
    }
    else // if (_channel == CHANNEL_B)
    {
        _main_pin = b_pin_encoder;
        _secondary_pin = a_pin_encoder;
    }

    int8_t _offset = 1;
    if (reverse_encoder)
    {
        _offset = -1;
    }

    if (encoder_scale == SCALE_X1)
    {
        if (digitalRead(_main_pin))
        {
            if (digitalRead(_secondary_pin))
            {
                pulse_counter += _offset;
            }
            else
            {
                pulse_counter -= _offset;
            }
        }
    }
    else if (encoder_scale == SCALE_X2)
    {
        if (digitalRead(_main_pin))
        {
            if (_channel == CHANNEL_A)
            {
                if (digitalRead(_secondary_pin))
                {
                    pulse_counter += _offset;
                }
                else
                {
                    pulse_counter -= _offset;
                }
            }
            else
            {
                if (digitalRead(_secondary_pin))
                {
                    pulse_counter -= _offset;
                }
                else
                {
                    pulse_counter += _offset;
                }
            }
        }
        // else
        // {
        //     if (digitalRead(_secondary_pin))
        //     {
        //         pulse_counter -= _offset;
        //     }
        //     else
        //     {
        //         pulse_counter += _offset;
        //     }
        // }
    }
    else if (encoder_scale == SCALE_X4)
    {
        if (digitalRead(_main_pin))
        {
            if (_channel == CHANNEL_A)
            {
                if (digitalRead(_secondary_pin))
                {
                    pulse_counter += _offset;
                }
                else
                {
                    pulse_counter -= _offset;
                }
            }
            else
            {
                if (digitalRead(_secondary_pin))
                {
                    pulse_counter -= _offset;
                }
                else
                {
                    pulse_counter += _offset;
                }
            }
        }
        else
        {
            if (_channel == CHANNEL_B)
            {
                if (digitalRead(_secondary_pin))
                {
                    pulse_counter += _offset;
                }
                else
                {
                    pulse_counter -= _offset;
                }
            }
            else
            {
                if (digitalRead(_secondary_pin))
                {
                    pulse_counter -= _offset;
                }
                else
                {
                    pulse_counter += _offset;
                }
            }
        }
    }
}

void Encoder::timer_fb_handle()
{
    String _mes = "P0:";
    float _pos = getPosition();
    _mes += String(_pos, 2);
    control_port.response(_mes);
}

void Encoder::setEncoderMode(uint8_t _enable)
{
    encoder_mode = (ENCODER_MODE)_enable;
    attachInterruptEncoderPin();
    clear_buffer_data();
    save_data();
}

void Encoder::setPulsePerMm(float _pulse)
{
    pulse_per_mm = _pulse;
    clear_buffer_data();
    save_data();
}

void Encoder::setEncoderScale(uint8_t _scale)
{
    if (_scale != SCALE_X1 && _scale != SCALE_X2 && _scale != SCALE_X4)
    {
        return;
    }
    encoder_scale = _scale;
    attachInterruptEncoderPin();

    clear_buffer_data();

    save_data();
}

void Encoder::setReverseEncoder(bool _dir)
{
    reverse_encoder = _dir;
    clear_buffer_data();
    save_data();
}

void Encoder::setTimeAutoFeedback(uint16_t _time)
{
    if (_time < 50)
    {
        _time = 0;
        time_auto_feedback = _time;
        AutoFeedbackTimer->pause();
        return;
    }
    time_auto_feedback = _time;

    AutoFeedbackTimer->pause();
    AutoFeedbackTimer->setOverflow(_time * 1000, MICROSEC_FORMAT);
    AutoFeedbackTimer->resume();
}

// func eeprom -------------------------------------------------------------
void Encoder::putFloatToEeprom(uint16_t address, float floatout)
{
    uint16_t floatH = floatout / 1;
    uint16_t floatL = (floatout - (float)floatH) * 1000;
    EEPROM.update(address, floatH);
    EEPROM.update(address + 1, floatL);
}

void Encoder::getFloatFromEeprom(uint16_t address, float &floatin)
{
    uint16_t floatH = EEPROM.read(address);
    uint16_t floatL = EEPROM.read(address + 1);
    floatin = (float)floatH + (float)floatL / 1000;
}
void Encoder::save_data()
{
    EEPROM.update(ENCODER_SCALE_ADDRESS, encoder_scale);
    EEPROM.update(IS_ENCODER_MODE_ADDRESS, encoder_mode);
    putFloatToEeprom(PULSE_PER_MM_ADDRESS, pulse_per_mm);
    EEPROM.update(REVERSE_ENCODER_ADDRESS, reverse_encoder);
    EEPROM.update(TIME_AUTO_FEEDBACK_ADDRESS, time_auto_feedback);
}
void Encoder::load_data()
{
    encoder_scale = (uint8_t)EEPROM.read(ENCODER_SCALE_ADDRESS);
    encoder_mode = (ENCODER_MODE)EEPROM.read(IS_ENCODER_MODE_ADDRESS);
    getFloatFromEeprom(PULSE_PER_MM_ADDRESS, pulse_per_mm);
    reverse_encoder = (bool)EEPROM.read(REVERSE_ENCODER_ADDRESS);
    time_auto_feedback = EEPROM.read(TIME_AUTO_FEEDBACK_ADDRESS);
    if (time_auto_feedback == 65535)
    {
        time_auto_feedback = 0;
    }
}
void Encoder::clear_buffer_data()
{
    pulse_counter = 0;
    current_position = 0;
    last_relative_position = 0;
    last_relative_pulse_counter = 0;
}
//----------------------------------------------------------------------------

int32_t Encoder::getEncoderPulse()
{
    return pulse_counter;
}

int32_t Encoder::getRelativeEncoderPulse()
{
    int32_t _offset = pulse_counter - last_relative_pulse_counter;
    last_relative_pulse_counter = pulse_counter;
    return _offset;
}

float Encoder::getPosition()
{
    if (encoder_mode == RELATIVE)
    {
        return getRelativePosition();
    }

    current_position = pulse_counter / pulse_per_mm;
    if (current_position > MAX_CURRENT_POS || current_position < MIN_CURRENT_POS)
    {
        clear_buffer_data();
    }
    return current_position;
}

float Encoder::getRelativePosition()
{
    current_position = pulse_counter / pulse_per_mm;

    if (current_position > MAX_CURRENT_POS || current_position < MIN_CURRENT_POS)
    {
        clear_buffer_data();
    }

    float _offset = current_position - last_relative_position;
    last_relative_position = current_position;
    return _offset;
}

bool Encoder::getInputA()
{
    return a_input_state;
}

bool Encoder::getInputB()
{
    return b_input_state;
}

void Encoder::resetCounter()
{
    pulse_counter = 0;
    last_relative_position = 0;
    current_position = 0;
}

void Encoder::setInputAutoFeecback(uint8_t index)
{
    if (index == 0)
    {
        is_auto_feedback_a = true;
    }
    else
    {
        is_auto_feedback_b = true;
    }
}

void Encoder::setStopInputAutoFeecback(uint8_t index)
{
    if (index == 0)
    {
        is_auto_feedback_a = false;
    }
    else
    {
        is_auto_feedback_b = false;
    }
}

void Encoder::execute()
{
    if (encoder_mode == AS_INPUT_PIN)
    {
        if (micros() - last_time_auto_feedback < TIMER_PERIOD_FEEDBACK)
        {
            return;
        }
        
        last_time_auto_feedback = micros();

        if (a_input_state != a_input_state_last && is_auto_feedback_a == true)
        {
            String _mes = "I0 V" + String(a_input_state);
            control_port.response(_mes);
            a_input_state_last = a_input_state;
        }

        if (b_input_state != b_input_state_last && is_auto_feedback_b == true)
        {
            String _mes = "I1 V" + String(b_input_state);
            control_port.response(_mes);
            b_input_state_last = b_input_state;
        }
    }
}

Encoder encoder;