#pragma once

#define ENCODER_A_PIN PB7
#define ENCODER_B_PIN PB6

#define INPUT_1_PIN PB7
#define INPUT_2_PIN PB6

#define LED_RUN_PIN PA8
#define LED_FUNC_PIN PB15

#define MOTOR_PUL_PIN PB8
#define MOTOR_DIR_PIN PB9
#define MOTOR_EN_PIN PB5

#define OUTPUT_1_PIN PB8
#define OUTPUT_2_PIN PB9
#define OUTPUT_3_PIN PB5

#define ETH_RESET_PIN PA3

#define ENCODER_SCALE_ADDRESS 1
#define IS_ENCODER_MODE_ADDRESS 2
#define PULSE_PER_MM_ADDRESS 3 // &4
#define REVERSE_ENCODER_ADDRESS 5

#define PULSE_PER_MM_CONVEYOR_ADDRESS 6 // &7
#define REVERSE_CONVEYOR_ADDRESS 8
#define IS_CONVEYOR_MODE_ADDRESS 9
#define IS_RUN_WITH_ENCODER_ADDRESS 10

#define IS_AUTO_RUN_SPEED_ADDRESS 11
#define AUTO_RUN_SPEED_ADDRESS 12 // &13

#define ETHERNET_PORT_ADDRESS 14
#define ETHERNET_IP_ADDRESS 15
#define ETHERNET_DNS_ADDRESS 19
#define ETHERNET_GATEWAY_ADDRESS 23
#define ETHERNET_SUBNET_ADDRESS 27
#define ETHERNET_MAC_ADDRESS 31 // &36

#define TIME_AUTO_FEEDBACK_ADDRESS 37

#define CONVEYOR_ACCEL_ADDRESS 38 // &39

#define SPEED_WHEN_RUN_WITH_BUTTON_ADDRESS 40 // &41
