#ifndef _CONVEYOR_h
#define _CONVEYOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// #define ExecuteStepTimer Timer3
// #define TurnPinTimer Timer4


#define TIME_ISR_ACCEL 2000
#define MIN_STEP_TIMER_PERIOD 50

#include <EEPROM.h>
#include <HardwareTimer.h>
#include "pin.h"
#include "communication.h"

typedef enum
{
    OUTPUT_PIN = 0,
    CONVEYOR_POS,
    CONVEYOR_VEL,
    CONVEYOR_VEL_INPUT,
} CONVEYOR_MODE;

class Conveyor
{
    /* data */
    float current_position;
    float current_speed;

    float current_accel;
    float min_speed;

    float desire_position;
    float desire_speed;
    float desire_speed_pos;

    uint32_t pulse_for_accel;
    uint32_t time_count;
    uint32_t current_period;
    uint32_t pulse_counter;
    uint32_t total_pulse;

    uint32_t checking_potentiometer_period;

    float speed_when_run_with_button;

    void pinInit();
    void timerInit();

    void putFloatToEeprom(uint16_t address, float floatout);
    void getFloatFromEeprom(uint16_t address, float &floatin);

    void save_data();
    void load_data();

    void save_auto_run_data();
    void load_auto_run_data();

    void setTimerPeriodFromSpeed(float _speed);

    void __execute_vel();
    void __execute_pos();

    void execute_speed(float _speed);
    
public:
    void init();
    void timer_handle();
    void tp_timer_handle();

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

    // void startFromButton();
    // void stopFromButton();
    // void increaseSpeedFromButton();
    // void decreaseSpeedFromButton();
    // void setVelocityButton(float _speed);

    void execute();

    float pulse_per_mm;
    bool reverse_conveyor;
    CONVEYOR_MODE conveyor_mode;
    bool is_run_with_encoder;

    bool is_auto_run_speed;
    float auto_run_speed;
    float max_speed;
    int potentiometer_value;
    int raw_potentiometer_values;
    unsigned long last_read_potentiometer_time;
    uint8_t read_potentiometer_counter;
    uint8_t read_potentiometer_period;

    float current_vel_button;
    HardwareTimer* ExecuteStepTimer;
    HardwareTimer* TurnPinTimer;
};

extern Conveyor conveyor;

#endif