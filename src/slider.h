#ifndef _SLIDER_h
#define _SLIDER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

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

#define STEP_TIMER_MAX_PERIOD 65000 // us, the 16-bit reload ceiling
#define TIME_ISR_ACCEL 2000         // us between ramp updates

// 50 us between steps is 20 kHz, the rate this step generator sustains: one
// interrupt drives the pin low and starts a second timer, whose interrupt drives
// it high again STEP_PULSE_WIDTH later. Two interrupt dispatches and two
// digitalWrite calls per step is what sets the ceiling.
//
// pulse/mm = steps_per_rev / screw_lead, top speed = 1000000 / (50 * pulse/mm):
//   1000 pulse/rev on a 5 mm screw ->  200 pulse/mm -> 100 mm/s
//   2000 pulse/rev on a 5 mm screw ->  400 pulse/mm ->  50 mm/s
// DEFAULT_SPEED stays below the default machine's ceiling.
// M328 reports the figure for the configured machine as V:.
#define MIN_STEP_PERIOD 50  // us between steps at full speed
#define STEP_PULSE_WIDTH 30 // us the step pin is held low

#define INPUT_DEBOUNCE 25 // ms an input must be stable before its state is accepted

#define DEFAULT_ACCEL 1000
#define MIN_ACCEL 1.0f
#define DEFAULT_SPEED 50

// Mechanics defaults: 1000 pulse/rev motor driving a 5 mm lead screw directly.
// pulse_per_mm = steps_per_rev / screw_lead = 200, i.e. 5 um per step and a
// 100 mm/s ceiling. A finer motor setting trades speed for resolution.
#define DEFAULT_STEPS_PER_REV 1000
#define DEFAULT_SCREW_LEAD 5

#define DEFAULT_PULSE_PER_MM 200
#define MIN_PULSE_PER_MM 0.01f
#define MAX_PULSE_PER_MM 100000.0f

// The two NPN sensors use the old quadrature encoder channels. Their open-
// collector outputs drive the isolated V2 input circuit and arrive at the MCU as
// active HIGH. Input 5 is both home and the negative hard limit; Input 6 is the
// positive hard limit. Both are direction-aware outside homing.
#define NEGATIVE_LIMIT_INPUT 4 // Input 5, PB7: home + negative limit
#define POSITIVE_LIMIT_INPUT 5 // Input 6, PB6: positive limit
#define DEDICATED_LIMIT_ACTIVE_LEVEL 1

// Fixed end-homing motion. Homing only starts on M320; motor direction remains
// configurable for different wiring.
#define DEFAULT_HOME_INPUT NEGATIVE_LIMIT_INPUT
#define DEFAULT_HOME_ACTIVE_LEVEL DEDICATED_LIMIT_ACTIVE_LEVEL
#define DEFAULT_HOME_SPEED 20       // mm/s, first seek
#define DEFAULT_HOME_SPEED_SLOW 5   // mm/s, precise re-seek
#define DEFAULT_HOME_BACKOFF 5      // mm
#define DEFAULT_HOME_OFFSET 0       // negative home sensor is position zero
#define DEFAULT_INPUT_ACTIVE_LEVEL(i) 1

#define HOMING_TIMEOUT 60000  // ms a single homing phase may take before it aborts
#define HOME_SWITCH_DEBOUNCE 3 // ms the switch must stay triggered to count

// a fault that latched: motion is refused until M327 C clears it
typedef enum
{
    FAULT_NONE = 0,
    FAULT_NEGATIVE_LIMIT = 1, // ran into the PB7 home/negative sensor
    FAULT_POSITIVE_LIMIT = 2, // ran into the PB6 positive sensor
    // Code 3 stays unused for protocol compatibility with older hosts.
    FAULT_HOMING = 4, // homing timed out or the sensor stayed stuck
    FAULT_ESTOP = 5,  // emergency stop
} FAULT_CODE;

// why a motion request was refused, so the port can answer with a real reason
typedef enum
{
    MOVE_OK = 0,
    MOVE_ERR_BUSY,      // another move or a homing sequence is running
    MOVE_ERR_NOT_HOMED, // absolute moves need a known zero first
    MOVE_ERR_FAULT,     // a fault is latched, clear it with M327 C
    MOVE_ERR_RANGE,     // parameter out of range
} MOVE_RESULT;

// homing sequence: fast seek -> back off -> slow re-seek -> zero
typedef enum
{
    HOMING_NONE = 0,
    HOMING_SEEK,
    HOMING_BACKOFF,
    HOMING_SEEK_SLOW,
} HOMING_STATE;

// function assigned to an IO input with M329
typedef enum
{
    INPUT_FUNC_NONE = 0,
    INPUT_FUNC_GOTO_POS, // move by V mm, relative
    INPUT_FUNC_STOP,     // stop with deceleration, also aborts homing
    INPUT_FUNC_HOME,     // start the homing sequence
    INPUT_FUNC_ESTOP,    // emergency stop, works in any state
} INPUT_FUNC;

// function assigned to an IO output with M329, driven from the real motion state
typedef enum
{
    OUTPUT_FUNC_NONE = 0,    // free output, set directly with M329 P<i> V<0|1>
    OUTPUT_FUNC_POS_REACHED, // ON while resting at the target of a completed move
    OUTPUT_FUNC_HOMED,       // ON once homing has completed
    OUTPUT_FUNC_POS_OVER,    // ON while the live position is at or past V
    OUTPUT_FUNC_FAULT,       // ON while a fault is latched
} OUTPUT_FUNC;

class Slider
{
public:
    void init();
    void __timer_handle();
    void __tp_timer_handle();
    void execute();

    // ---- M328 mechanics ----
    MOVE_RESULT setMechanics(float _steps_rev, float _lead);
    float getMaxSpeed();

    // ---- M320 homing / M325 homing configuration ----
    // _from_gcode marks a request that owes the command port a terminal reply
    MOVE_RESULT home(bool _from_gcode = true);
    bool isHoming();
    void abortHoming();
    void setHomeConfig(uint8_t _reverse);

    // ---- M321 speed / M324 acceleration ----
    void setMoveSpeed(float _speed);
    void setAccel(float _accel);
    float getMoveSpeed();
    float getAccel();

    // ---- M322 position ----
    MOVE_RESULT setPosition(double _pos, bool _from_gcode = true);
    MOVE_RESULT movePositionRelative(double _distance, bool _from_gcode = true);
    float getPosition();
    bool isMoving();

    // ---- M323 motor enable ----
    void enableStepper();
    void disableStepper();

    // ---- M327 stop and fault ----
    void stopSmooth();
    void stopEmergency(FAULT_CODE _fault);
    void clearFault();
    FAULT_CODE getFault();
    bool isFaulted();

    // ---- M329 IO functions ----
    void setInputFunction(uint8_t index, uint8_t func, float value, uint8_t level);
    void setOutputFunction(uint8_t index, uint8_t func, float value);

    void setAddress(uint16_t _address);

    uint16_t slider_address;

    // mechanics (persisted)
    volatile float pulse_per_mm; // derived, never stored on its own
    float steps_per_rev;
    float screw_lead;
    bool reverse_slider;

    // fixed negative-end homing values plus the persisted direction option
    uint8_t home_input;
    uint8_t home_active_level;
    float home_speed;
    float home_speed_slow;
    float home_backoff;
    float home_offset;

    bool is_homed;
    bool is_stepper_enable;

    uint8_t input_function[INPUT_COUNT];
    float input_value[INPUT_COUNT];
    // Level that counts as "pressed". Every channel is conditioned by the board;
    // encoder-header inputs 4/5 are active-high after their optocoupler and
    // Schmitt inverter.
    uint8_t input_active_level[INPUT_COUNT];

    uint8_t output_function[OUTPUT_COUNT];
    float output_value[OUTPUT_COUNT];

#if !defined(__STM32F1__)
    HardwareTimer *ExecuteStepTimer;
    HardwareTimer *TurnPinTimer;
#endif

private:
    // Position is stored as whole motor pulses. This keeps software position
    // exactly aligned with physical steps and avoids cumulative float drift.
    volatile int64_t current_step_position;
    volatile int64_t desire_step_position;
    double commanded_position;

    // Motion state shared between the timer ISR and the main loop.
    volatile float current_speed;
    volatile float current_accel;
    float min_speed;

    volatile float desire_speed;
    float desire_speed_pos;

    volatile uint32_t pulse_for_accel;
    volatile uint32_t time_count;
    volatile uint32_t current_period;
    volatile uint32_t pulse_counter;
    volatile uint32_t total_pulse;

    // every step the ISR emits lands here signed; execute() folds it into
    // current_step_position, so live position is known during every motion type
    volatile int32_t step_delta;

    // Pausing the timer does not withdraw an update already latched in the NVIC,
    // so one more step interrupt can arrive after a stop. This flag lets that one
    // be rejected; a bare register read would do the same but would have to be
    // written twice for the two cores.
    volatile bool step_timer_running = false;

    volatile bool is_need_send_done = false;
    bool current_direction = true; // true = moving toward negative
    volatile bool is_moving_pos = false;

    // controlled stop / retarget
    volatile bool is_stopping;
    bool has_pending_move;
    double pending_position;
    bool pending_from_gcode;

    // latched fault
    volatile FAULT_CODE fault_code;
    bool is_need_send_fault;

    // homing state
    volatile HOMING_STATE homing_state;
    volatile bool home_move_done;
    uint32_t homing_phase_start;
    uint32_t home_switch_time;
    bool home_switch_raw_last;
    bool negative_limit_raw_last;
    uint32_t negative_limit_time;
    bool positive_limit_raw_last;
    uint32_t positive_limit_time;

    // a host command is waiting for the motion layer to report how it ended
    volatile bool reply_pending;

    // per-input debounce and edge detection
    bool input_raw_last[INPUT_COUNT];
    bool input_pressed[INPUT_COUNT];
    uint32_t input_debounce_time[INPUT_COUNT];
    bool input_pressed_last[INPUT_COUNT];

    // output function state
    volatile bool at_position = false;
    bool output_state_last[OUTPUT_COUNT];

    void pinInit();
    void timerInit();

    void putRawFloat(uint16_t address, float value);
    float getRawFloat(uint16_t address);

    void save_data();
    void load_data();
    void init_home_config();
    void save_mechanics_data();
    void load_mechanics_data();

    float maxSpeed();
    void __clamp_move_speed();
    void __pause_step_timer();
    void __resume_step_timer();
    void setTimerPeriodFromSpeed(float _speed);
    void execute_speed(float _speed);

    void __count_step();
    void __execute_vel();
    void __execute_pos();
    void __execute_home_pos();

    void __execute_homing();
    bool __home_switch_active();
    bool __negative_limit_active();
    bool __positive_limit_active();
    void __move_raw(float _speed);
    void __stop_raw();
    void __start_home_backoff();

    bool __position_to_steps(double _pos, int64_t &_steps);
    float __steps_to_position(int64_t _steps);
    bool __start_move(double _pos, bool _from_gcode);
    MOVE_RESULT __set_position(double _pos, bool _from_gcode, bool _require_homed);
    void __cancel_pending_move(const char *_message);
    void __begin_stop_ramp();
    void __execute_stop_ramp();
    void __execute_stop_ramp_finish();
    void __execute_travel_guard();
    void __raise_fault(FAULT_CODE _fault);
    void __finish_reply(bool _ok, const char *_error);
    void __sync_position();

    void __seed_input_state(uint8_t index);
    void __execute_input();
    void __input_action(uint8_t index);
    void __execute_output();
};

extern Slider slider;

#endif // _SLIDER_h
