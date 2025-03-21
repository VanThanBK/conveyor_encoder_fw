#ifndef _COMMU_h
#define _COMMU_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "encoder.h"
#include "conveyor.h"
#include <Ethernet.h>
#include <SPI.h>

#define USBPort Serial1

class communication
{
private:
    enum COM_Port {
        usb_port = 0,
        eth_port
    };

    String receive_string;
	bool is_string_complete;

    String eth_receive_string;
	bool is_eth_string_complete;

    COM_Port current_cmd_port;

    String gcode;

    EthernetServer *eth_server;
	EthernetClient eth_client;

    uint16_t ethernet_port;
	IPAddress ethernet_ip;
	IPAddress ethernet_dns;
	IPAddress ethernet_gateway;
	IPAddress ethernet_subnet;
	byte ethernet_mac[6];
    String ethernet_infor;

    String device_name = String(DEFAULT_DEVICE_NAME);
    String UUID = String(DEFAULT_UUID);
    String firmware_version = String(DEFAULT_FIRMWARE_VERSION);
    String board_version = String(DEFAULT_BOARD_VERSION);
    String serial_number = String(DEFAULT_SERIAL_NUMBER);

    bool is_enable_eth;
    void init_eth();

    void save_eth();
    void load_eth();

    void send_encoder_pos();
    void send_encoder_infor();

    void send_conveyor_infor();
    void send_ethernet_infor();
    void send_ethernet_ip();

    void save_string(uint16_t _address, String _str);
    void load_string(uint16_t _address, String &_str);
public:
    void init();
    void execute();
    void response(String _mes);
    void send_done();

    void send_input_a_state();
    void send_input_b_state();
};

extern communication control_port;
#endif