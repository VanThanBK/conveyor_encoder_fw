#ifndef _IO_CONTROL_h
#define _IO_CONTROL_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "pin.h"

#define IO_COUNT 4
#define IO_TIMER_PERIOD_FEEDBACK 2000

class IOControl
{
public:
    void init();

    bool setOutput(uint8_t index, uint8_t value);
    bool getInput(uint8_t index, bool &value);

    bool setInputAutoFeedback(uint8_t index);
    bool setStopInputAutoFeedback(uint8_t index);

    void execute();

private:
    bool input_state[IO_COUNT];
    bool input_state_last[IO_COUNT];
    bool is_auto_feedback[IO_COUNT];

    uint32_t last_time_auto_feedback;
};

extern IOControl io_control;

#endif
