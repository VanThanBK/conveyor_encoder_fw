#include "io_control.h"
#include "communication.h"

static const uint8_t input_pins[INPUT_COUNT] = {
    INPUT_1_PIN,
    INPUT_2_PIN,
    INPUT_3_PIN,
    INPUT_4_PIN,
    INPUT_5_PIN,
    INPUT_6_PIN,
};

static const uint8_t output_pins[OUTPUT_COUNT] = {
    OUTPUT_1_PIN,
    OUTPUT_2_PIN,
    OUTPUT_3_PIN,
    OUTPUT_4_PIN,
};

void IOControl::init()
{
    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        pinMode(output_pins[i], OUTPUT);
        digitalWrite(output_pins[i], LOW);
    }

    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        // Every input is driven by the board's conditioning circuitry. Inputs
        // 5/6 come from the isolated J5 encoder channels through HCPL-2630 and
        // 74HC14, so their active state at PB7/PB6 is HIGH.
        pinMode(input_pins[i], INPUT);
        input_state[i] = digitalRead(input_pins[i]);
        input_state_last[i] = input_state[i];
        is_auto_feedback[i] = false;
    }

    last_time_auto_feedback = micros();
}

bool IOControl::setOutput(uint8_t index, uint8_t value)
{
    if (index >= OUTPUT_COUNT)
    {
        return false;
    }

    digitalWrite(output_pins[index], value ? HIGH : LOW);
    return true;
}

bool IOControl::getInput(uint8_t index, bool &value)
{
    if (index >= INPUT_COUNT)
    {
        return false;
    }

    input_state[index] = digitalRead(input_pins[index]);
    value = input_state[index];
    return true;
}

bool IOControl::setInputAutoFeedback(uint8_t index)
{
    if (index >= INPUT_COUNT)
    {
        return false;
    }

    is_auto_feedback[index] = true;
    input_state_last[index] = digitalRead(input_pins[index]);
    return true;
}

bool IOControl::setStopInputAutoFeedback(uint8_t index)
{
    if (index >= INPUT_COUNT)
    {
        return false;
    }

    is_auto_feedback[index] = false;
    return true;
}

void IOControl::execute()
{
    uint32_t now = micros();
    if ((uint32_t)(now - last_time_auto_feedback) < IO_TIMER_PERIOD_FEEDBACK)
    {
        return;
    }

    last_time_auto_feedback = now;

    for (uint8_t i = 0; i < INPUT_COUNT; i++)
    {
        input_state[i] = digitalRead(input_pins[i]);

        if (!is_auto_feedback[i])
        {
            continue;
        }

        if (input_state[i] == input_state_last[i])
        {
            continue;
        }

        String mes = "I";
        mes += String(i);
        mes += " V";
        mes += String(input_state[i]);
        control_port.response(mes);
        input_state_last[i] = input_state[i];
    }
}

IOControl io_control;
