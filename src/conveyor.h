#ifndef _CONVEYOR_h
#define _CONVEYOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif



#define TIME_ISR_ACCEL 2000
#define MIN_STEP_TIMER_PERIOD 50

#define INPUT_HOLD_DELAY 1500   // ms held before speed step starts repeating
#define INPUT_REPEAT_PERIOD 100 // ms between repeated speed steps while held
#define INPUT_DEBOUNCE 25       // ms an input must be stable before its state is accepted

#include <EEPROM.h>
#include "pin.h"
#include "communication.h"
#include "io_control.h"

#if !defined(__STM32F1__)
#include <HardwareTimer.h>

#define EXECUTE_STEP_TIMER TIM3
#define TURN_PIN_TIMER TIM4
#else
#define ExecuteStepTimer Timer3
#define TurnPinTimer Timer4
#endif


#define DEFAULT_ACCEL 1000
#define DEFAULT_SPEED 50

typedef enum
{
    OUTPUT_PIN = 0,
    CONVEYOR_POS,
    CONVEYOR_VEL,
    CONVEYOR_INPUT, // control from IO inputs: the function assigned to each input decides speed or position motion
} CONVEYOR_MODE;

// function assigned to an IO input (set via gcode M322), active in CONVEYOR_INPUT mode
typedef enum
{
    INPUT_FUNC_NONE = 0,
    INPUT_FUNC_SPEED_UP,   // tap = +value, hold = repeat (ignored while a position move runs)
    INPUT_FUNC_SPEED_DOWN, // tap = -value, hold = repeat (ignored while a position move runs)
    INPUT_FUNC_GOTO_POS,   // move by a relative distance (only accepted while fully stopped)
    INPUT_FUNC_STOP,       // always works: stop a position move / velocity run
} INPUT_FUNC;

// function assigned to an IO output (set via gcode M323), driven from the real
// motion state so it works for both gcode and input commanded motions
typedef enum
{
    OUTPUT_FUNC_NONE = 0,    // free output, controlled by M320
    OUTPUT_FUNC_POS_REACHED, // ON while resting at the target of a completed position move
    OUTPUT_FUNC_VEL_REACHED, // V = 0: commanded speed reached; V > 0: forward threshold; V < 0: reverse threshold
} OUTPUT_FUNC;

class Conveyor
{
public:
    void init();
    void __timer_handle();
    void __tp_timer_handle();

    void setConveyorMode(uint8_t _enable);
    void setReverseConveyor(bool _dir);
    void setPulsePerMm(float _pulse);
    void setRunWithEncoder(bool _enable);

    void setPosition(float _pos);
    void setVelocity(float _speed);
    void setVelocityPos(float _speed);
    void stopPosition();

    void setAccel(float _accel);
    void setOutput(uint8_t _pin, uint8_t _value);
    void setAddress(uint16_t _address);

    void setInputFunction(uint8_t index, uint8_t func, float value);
    void setOutputFunction(uint8_t index, uint8_t func, float value);

    void startFromButton();
    void stopFromButton();
    void increaseSpeedFromButton();
    void decreaseSpeedFromButton();
    void setVelocityButton(float _speed);

    void execute();

    uint16_t conveyor_address;

    uint8_t input_function[IO_COUNT];
    float input_value[IO_COUNT];

    uint8_t output_function[IO_COUNT];
    float output_value[IO_COUNT];

    // Read by the step-timer ISR and configurable from the main loop.
    volatile float pulse_per_mm;
    bool reverse_conveyor;
    volatile CONVEYOR_MODE conveyor_mode;
    bool is_run_with_encoder;

    bool is_auto_run_speed;
    float auto_run_speed;
    float current_vel_button;

#if !defined(__STM32F1__)
    HardwareTimer *ExecuteStepTimer;
    HardwareTimer *TurnPinTimer;
#endif
private:
    /* data */
    // Motion state shared between the timer ISR and the main loop.
    volatile float current_position;
    volatile float current_speed;

    volatile float current_accel;
    float min_speed;

    volatile float desire_position;
    volatile float desire_speed;
    float desire_speed_pos;

    volatile uint32_t pulse_for_accel;
    volatile uint32_t time_count;
    volatile uint32_t current_period;
    volatile uint32_t pulse_counter;
    volatile uint32_t total_pulse;

    float speed_when_run_with_button;

    volatile bool is_need_send_done = false;
    float last_target_speed = 0;
    bool wait_for_reverse = false;
    bool current_direction = true;

    // per-input button state machine (for input function feature)
    bool input_raw_last[IO_COUNT];        // last raw reading (for debounce)
    bool input_pressed[IO_COUNT];         // debounced state
    uint32_t input_debounce_time[IO_COUNT];
    bool input_pressed_last[IO_COUNT];    // previous debounced state (edge detect)
    uint32_t input_press_time[IO_COUNT];
    uint32_t input_last_repeat[IO_COUNT];
    float input_target_speed;
    volatile bool input_run_pos = false; // CONVEYOR_INPUT mode: current motion is a position move (else velocity)

    // output function state
    volatile bool at_position = false; // a position move completed and the conveyor rests at its target
    bool output_state_last[IO_COUNT];

    void pinInit();
    void timerInit();

    void putFloatToEeprom(uint16_t address, float floatout);
    void getFloatFromEeprom(uint16_t address, volatile float &floatin);
    void putRawFloat(uint16_t address, float value);
    float getRawFloat(uint16_t address);

    void save_data();
    void load_data();

    void save_auto_run_data();
    void load_auto_run_data();

    float maxSpeed();

    void setTimerPeriodFromSpeed(float _speed);

    void __execute_vel();
    void __execute_pos();
    void __execute_input();
    void __input_action(uint8_t index);
    void __execute_output();
    bool isPosMoveActive();

    void execute_speed(float _speed);
};

extern Conveyor conveyor;

#endif // _CONVEYOR_h
