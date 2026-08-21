#pragma once

// ---------------------------------------------------------------- pin map ---

// 6 digital inputs. Inputs 5 and 6 sit on the old quadrature encoder header.
// Slider reserves Input 5 as the active-high NPN home/negative-limit sensor and
// Input 6 as the active-high NPN positive-limit sensor.
#define INPUT_1_PIN PA12
#define INPUT_2_PIN PA11
#define INPUT_3_PIN PB14
#define INPUT_4_PIN PB13
#define INPUT_5_PIN PB7
#define INPUT_6_PIN PB6

// 4 digital outputs
#define OUTPUT_1_PIN PB12
#define OUTPUT_2_PIN PA2
#define OUTPUT_3_PIN PA1
#define OUTPUT_4_PIN PA0

#define LED_RUN_PIN PA8
#define LED_FUNC_PIN PB15

#define MOTOR_PUL_PIN PB8
#define MOTOR_DIR_PIN PB9
#define MOTOR_EN_PIN PB5

// Stepper driver enable pin polarity (the driver is enabled at power up)
#define MOTOR_ENABLE_LEVEL LOW
#define MOTOR_DISABLE_LEVEL HIGH

#define ETH_RESET_PIN PA3

// ----------------------------------------------------------- EEPROM map -----

#define IS_ETH_ENABLE_ADDRESS 0

// 1..5 and 38 held the encoder settings of the conveyor firmware and are free.

#define REVERSE_SLIDER_ADDRESS 8

#define ETHERNET_PORT_ADDRESS 14
#define ETHERNET_IP_ADDRESS 16
#define ETHERNET_DNS_ADDRESS 20
#define ETHERNET_GATEWAY_ADDRESS 24
#define ETHERNET_SUBNET_ADDRESS 28
#define ETHERNET_MAC_ADDRESS 32 // &36

#define SLIDER_ADDRESS 43 // &44

// 45..64 held the 4-input function table of the conveyor firmware. The table grew
// to 6 inputs and moved to 114, freeing this block for two values that no longer
// fit the old 2-byte format.
// 45..48 held pulses per mm. It is derived from the mechanics block at 176 now,
// so there is a single source of truth, and this is free.
#define SLIDER_ACCEL_ADDRESS 49        // 49..52, raw float
#define END_INPUT_ADDRESS 53           // legacy, free
#define END_ACTIVE_LEVEL_ADDRESS 54    // legacy, free
// 55 held auto-home and is free; homing now only starts on M320.
#define OUTPUT_FUNCTION_ADDRESS 65 // 65..68 (function code of 4 outputs)
#define OUTPUT_VALUE_ADDRESS 69    // 69..84 (value of 4 outputs, 4 bytes each)
#define POS_SPEED_ADDRESS 85       // 85..88 (position move speed M313/M321, raw float)

// 89..113 held configurable homing values. Home and travel protection are fixed
// to the two NPN sensors on PB7/PB6, so this block is free.

#define INPUT_FUNCTION_ADDRESS 114 // 114..119 (function code of 6 inputs)
#define INPUT_VALUE_ADDRESS 120    // 120..143 (value of 6 inputs, 4 bytes each)
#define INPUT_LEVEL_ADDRESS 144 // 144..149 (active level of the 6 inputs)
// 150..175 held the stored-position table, which was removed. Free.

// ---- mechanics: motor and lead screw ----
#define STEPS_PER_REV_ADDRESS 176   // 176..179 motor pulses per revolution, raw float
#define SCREW_LEAD_ADDRESS 180      // 180..183 lead screw travel per revolution in mm, raw float
#define LEGACY_GEAR_RATIO_ADDRESS 184 // 184..187, folded into steps_per_rev on first boot
// 188..191 held the step pulse timings, which are compile-time constants now.

// ------------------------------------------------------------- versions -----

#define MAIN_BOARD_VERSION "2.0"

#define DEVICE_NAME "SliderX"
#define FIRMWARE_DATE "21/8/2026"
#define FIRMWARE_VERSION "1.0.1"
