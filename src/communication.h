#ifndef _COMMU_h
#define _COMMU_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "encoder.h"
#include "conveyor.h"
#include <Ethernet_STM.h>

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

    void init_eth();

    void save_eth();
    void load_eth();

    void send_encoder_pos();
    void send_encoder_infor();

    void send_conveyor_infor();

public:
    void init();
    void execute();
    void response(String _mes);
    void send_done();
};

extern communication control_port;
#endif