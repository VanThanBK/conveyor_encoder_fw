#ifndef _ENCODER_h
#define _ENCODER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <EEPROM.h>
#include "pin.h"
#include "communication.h"

#define AutoFeedbackTimer Timer2

//scale of encoder pulse
#define SCALE_X1 1
#define SCALE_X2 2
#define SCALE_X4 4

//channel in encoder
#define CHANNEL_A 0
#define CHANNEL_B 1

#define MAX_CURRENT_POS 999999.99
#define MIN_CURRENT_POS -999999.99

#define MAX_

typedef enum
{
    ABSOLUTE = 0,
    RELATIVE,
    AS_INPUT_PIN,
} ENCODER_MODE;

class Encoder
{
private:
    //encoder pin array
    uint8_t a_pin_encoder;
    uint8_t b_pin_encoder;

    int32_t pulse_counter;  //encoder pulse counter
    int32_t last_relative_pulse_counter;
    float current_position;  //current angle of robot rf arm
    float last_relative_position;

    bool a_input_state;
    bool b_input_state;
    bool a_input_state_last;
    bool b_input_state_last;
    
    bool is_auto_feedback_a;
    bool is_auto_feedback_b;

    void pinInit();
    void attachInterruptEncoderPin();

    void putFloatToEeprom(uint16 address, float floatout);
    void getFloatFromEeprom(uint16 address, float &floatin);

    void save_data();
    void load_data();

    void clear_buffer_data();
public:
    void init();
    void __encoder_handle(uint8_t _channel);
    void __timer_fb_handle();

    void setEncoderMode(uint8_t _enable);
    void setPulsePerMm(float _pulse);
    void setEncoderScale(uint8_t _scale);
    void setReverseEncoder(bool _dir);
    void setTimeAutoFeedback(uint16 _time);

    int32_t getEncoderPulse();
    int32_t getRelativeEncoderPulse();
    float getPosition();
    float getRelativePosition();

    bool getInputA();
    bool getInputB();

    void setInputAutoFeecback(uint8_t index);
    void setStopInputAutoFeecback(uint8_t index);

    uint8_t encoder_scale;
    float pulse_per_mm;
    ENCODER_MODE encoder_mode;
    bool reverse_encoder;
    uint16 time_auto_feedback;
};

extern Encoder encoder;
#endif