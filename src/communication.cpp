#include "communication.h"

void communication::init_eth()
{
    eth_server = new EthernetServer(ethernet_port);

    if (ethernet_ip[0] == 0 && ethernet_ip[1] == 0 && ethernet_ip[2] == 0 && ethernet_ip[3] == 0)
    {
        Ethernet.begin(ethernet_mac);
    }
    else
    {
        Ethernet.begin(ethernet_mac, ethernet_ip, ethernet_dns, ethernet_gateway, ethernet_subnet);
    }
    
	// if (Ethernet.hardwareStatus() == EthernetNoHardware)
	// {
	// 	ethernet_infor = "Unknown:Ethernet hardware error!";
	// }

	// if (Ethernet.linkStatus() == LinkOFF)
	// {
	// 	ethernet_infor = "Unknown:Ethernet cable is not connected!";
	// }

	// USB_PORT.println(Ethernet.localIP());
	eth_server->begin();
    ethernet_infor = "Ethernet init success!";

	Ethernet.maintain();
}

void communication::save_eth()
{
    EEPROM.update(ETHERNET_PORT_ADDRESS, ethernet_port);

    EEPROM.update(ETHERNET_IP_ADDRESS, ethernet_ip[0]);
    EEPROM.update(ETHERNET_IP_ADDRESS+1, ethernet_ip[1]);
    EEPROM.update(ETHERNET_IP_ADDRESS+2, ethernet_ip[2]);
    EEPROM.update(ETHERNET_IP_ADDRESS+3, ethernet_ip[3]);

    EEPROM.update(ETHERNET_DNS_ADDRESS, ethernet_dns[0]);
    EEPROM.update(ETHERNET_DNS_ADDRESS+1, ethernet_dns[1]);
    EEPROM.update(ETHERNET_DNS_ADDRESS+2, ethernet_dns[2]);
    EEPROM.update(ETHERNET_DNS_ADDRESS+3, ethernet_dns[3]);

    EEPROM.update(ETHERNET_GATEWAY_ADDRESS, ethernet_gateway[0]);
    EEPROM.update(ETHERNET_GATEWAY_ADDRESS+1, ethernet_gateway[1]);
    EEPROM.update(ETHERNET_GATEWAY_ADDRESS+2, ethernet_gateway[2]);
    EEPROM.update(ETHERNET_GATEWAY_ADDRESS+3, ethernet_gateway[3]);

    EEPROM.update(ETHERNET_SUBNET_ADDRESS, ethernet_subnet[0]);
    EEPROM.update(ETHERNET_SUBNET_ADDRESS+1, ethernet_subnet[1]);
    EEPROM.update(ETHERNET_SUBNET_ADDRESS+2, ethernet_subnet[2]);
    EEPROM.update(ETHERNET_SUBNET_ADDRESS+3, ethernet_subnet[3]);

    EEPROM.update(ETHERNET_MAC_ADDRESS, ethernet_mac[0]);
    EEPROM.update(ETHERNET_MAC_ADDRESS+1, ethernet_mac[1]);
    EEPROM.update(ETHERNET_MAC_ADDRESS+2, ethernet_mac[2]);
    EEPROM.update(ETHERNET_MAC_ADDRESS+3, ethernet_mac[3]);
    EEPROM.update(ETHERNET_MAC_ADDRESS+4, ethernet_mac[4]);
    EEPROM.update(ETHERNET_MAC_ADDRESS+5, ethernet_mac[5]);
    delay(1);
}

void communication::load_eth()
{
    ethernet_port = EEPROM.read(ETHERNET_PORT_ADDRESS);

    ethernet_ip[0] = EEPROM.read(ETHERNET_IP_ADDRESS);
    ethernet_ip[1] = EEPROM.read(ETHERNET_IP_ADDRESS+1);
    ethernet_ip[2] = EEPROM.read(ETHERNET_IP_ADDRESS+2);
    ethernet_ip[3] = EEPROM.read(ETHERNET_IP_ADDRESS+3);

    ethernet_dns[0] = EEPROM.read(ETHERNET_DNS_ADDRESS);
    ethernet_dns[1] = EEPROM.read(ETHERNET_DNS_ADDRESS+1);
    ethernet_dns[2] = EEPROM.read(ETHERNET_DNS_ADDRESS+2);
    ethernet_dns[3] = EEPROM.read(ETHERNET_DNS_ADDRESS+3);

    ethernet_gateway[0] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS);
    ethernet_gateway[1] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS+1);
    ethernet_gateway[2] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS+2);
    ethernet_gateway[3] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS+3);

    ethernet_subnet[0] = EEPROM.read(ETHERNET_SUBNET_ADDRESS);
    ethernet_subnet[1] = EEPROM.read(ETHERNET_SUBNET_ADDRESS+1);
    ethernet_subnet[2] = EEPROM.read(ETHERNET_SUBNET_ADDRESS+2);
    ethernet_subnet[3] = EEPROM.read(ETHERNET_SUBNET_ADDRESS+3);

    ethernet_mac[0] = EEPROM.read(ETHERNET_MAC_ADDRESS);
    ethernet_mac[1] = EEPROM.read(ETHERNET_MAC_ADDRESS+1);
    ethernet_mac[2] = EEPROM.read(ETHERNET_MAC_ADDRESS+2);
    ethernet_mac[3] = EEPROM.read(ETHERNET_MAC_ADDRESS+3);
    ethernet_mac[4] = EEPROM.read(ETHERNET_MAC_ADDRESS+4);
    ethernet_mac[5] = EEPROM.read(ETHERNET_MAC_ADDRESS+5);
    delay(1);
}

void communication::send_encoder_pos()
{
    String fb_string = "P0:";
    fb_string += String(encoder.getPosition(), 2);

    if (current_cmd_port == usb_port)
    {
        USBPort.println(fb_string);
    }
    else
    {
        eth_client.println(fb_string);
    }
}

void communication::send_encoder_infor()
{
    String fb_string = "M:";
    fb_string += String(encoder.encoder_mode);
    fb_string += " C:";
    fb_string += String(encoder.encoder_scale);
    fb_string += " S:";
    fb_string += String(encoder.pulse_per_mm);
    fb_string += " R:";
    fb_string += String(encoder.reverse_encoder);
    fb_string += " T:";
    fb_string += String(encoder.time_auto_feedback);

    if (current_cmd_port == usb_port)
    {
        USBPort.println(fb_string);
    }
    else
    {
        eth_client.println(fb_string);
    }
}

void communication::send_done()
{
    if (current_cmd_port == usb_port)
    {
        USBPort.println("Ok");
    }
    else
    {
        eth_client.println("Ok");
    }
}

void communication::send_conveyor_infor()
{
    String fb_string = "M:";
    fb_string += String(conveyor.conveyor_mode);
    fb_string += " S:";
    fb_string += String(conveyor.pulse_per_mm);
    fb_string += " R:";
    fb_string += String(conveyor.reverse_conveyor);

    if (current_cmd_port == usb_port)
    {
        USBPort.println(fb_string);
    }
    else
    {
        eth_client.println(fb_string);
    }
}

void communication::init()
{
    USBPort.begin(115200);
    USBPort.setTimeout(5);

    receive_string.reserve(100);
    is_string_complete = false;
    receive_string = "";

    ethernet_port = 23;
	ethernet_ip = IPAddress(0, 0, 0, 0);
	ethernet_dns = IPAddress(192, 168, 1, 1);
	ethernet_gateway = IPAddress(192, 168, 1, 1);
	ethernet_subnet = IPAddress(255, 255, 0, 0);
	ethernet_mac[0] = 0xDE;
	ethernet_mac[1] = 0xAD;
	ethernet_mac[2] = 0xBE;
	ethernet_mac[3] = 0xEF;
	ethernet_mac[4] = 0xFE;
	ethernet_mac[5] = 0xED;

    pinMode(ETH_RESET_PIN, OUTPUT);
    digitalWrite(ETH_RESET_PIN, 1);
    init_eth();

    pinMode(LED_RUN_PIN, OUTPUT);
    digitalWrite(LED_RUN_PIN, 0);
}

void communication::execute()
{
    // read cm
    while (USBPort.available())
    {
        char inChar = (char)USBPort.read();

        if (inChar == '\n')
        {
            is_string_complete = true;
            break;
        }

        if (inChar != '\r')
        {
            receive_string += inChar;
        }
    }

    eth_client = eth_server->available();
    if (eth_client)
    {
        while (eth_client.available())
        {
            char inChar = (char)eth_client.read();

            if (inChar == '\n')
            {
                is_eth_string_complete = true;
                break;
            }

            if (inChar != '\r')
            {
                eth_receive_string += inChar;
            }
        }
    }

    if (is_string_complete)
    {
        gcode = receive_string;
        receive_string = "";
        is_string_complete = false;
        current_cmd_port = usb_port;
    }
    else if (is_eth_string_complete)
    {
        gcode = eth_receive_string;
        eth_receive_string = "";
        is_eth_string_complete = false;
        current_cmd_port = eth_port;
    }
    else
    {
        return;
    }

    if (gcode == "")
    {
        return;
    }
        
    // tach
    String splitWord = "";
    gcode += " ";
    byte index_value = 0;

    String keyValue = gcode.substring(0, 4);
    String _value1 = "";
    String _value2 = "";

    for (uint16_t i = 4; i < gcode.length(); i++)
    {
        if (gcode[i] == ' ')
        {
            if (splitWord == "")
                continue;

            if (index_value == 0)
            {
                _value1 = splitWord;
                index_value++;
            }
            else if (index_value == 1)
            {
                _value2 = splitWord;
                index_value++;
            }
            
            splitWord = "";
            continue;
        }
        splitWord += String(gcode[i]);
    }

//conveyor
    if (keyValue == "M310")
    {
        // set mode
        conveyor.setConveyorMode(_value1.toFloat());
        send_done();
    }
    else if (keyValue == "M311")
    {
        // set speed
        conveyor.setVelocity(_value1.toFloat());
        send_done();
    }
    else if (keyValue == "M312")
    {
        // set position
        conveyor.setPosition(_value1.toFloat());

    }
    else if (keyValue == "M313")
    {
        // set speed position
        conveyor.setVelocityPos(_value1.toFloat());
        send_done();
    }
    else if (keyValue == "M314")
    {
        if (index_value > 1 && _value1[0] == 'P' && _value2[0] == 'V')
        {
            float _pin = _value1.substring(1).toInt();
            float _value = _value2.substring(1).toInt();
            conveyor.setOutput(_pin, _value);
            send_done();
        }
    }
    else if (keyValue == "M315")
    {
        if (index_value > 0 && _value1[0] == 'S')
        {
            float _pulse = _value1.substring(1).toFloat();
            conveyor.setPulsePerMm(_pulse);
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'R')
        {
            bool _reverse = _value1.substring(1).toInt();
            conveyor.setReverseConveyor(_reverse);
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'E')
        {
            bool _run = _value1.substring(1).toInt();
            conveyor.setRunWithEncoder(_run);
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'P')
        {
            // set run with encoder
            conveyor.stopPosition();
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'A')
        {
            // set accel
            conveyor.setAccel(_value1.substring(1).toFloat());
            send_done();
        }
        else
        {
            send_conveyor_infor();
        }
    }

// encoder
    else if (keyValue == "M316")
    {
        // set mode
        encoder.setEncoderMode(_value1.toInt());
        send_done();
    }
    else if (keyValue == "M317")
    {
        if (index_value > 0 && _value1[0] == 'T')
        {
            int _time_response = _value1.substring(1).toInt();
            encoder.setTimeAutoFeedback(_time_response);
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'R')
        {
            
            send_done();
        }
        else if (index_value == 0)
        {
            send_encoder_pos();
        }
    }
    else if (keyValue == "M318")
    {
        // get pulse per mm
        if (index_value > 0 && _value1[0] == 'S')
        {
            float _pulse = _value1.substring(1).toFloat();
            encoder.setPulsePerMm(_pulse);
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'R')
        {
            bool _reverse = _value1.substring(1).toInt();
            encoder.setReverseEncoder(_reverse);
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'C')
        {
            int _scale = _value1.substring(1).toInt();
            encoder.setEncoderScale(_scale);
            send_done();
        }
        else
        {   
            send_encoder_infor();
        }
    }
    else if (keyValue == "M319")
    {
        // read input
        if (index_value > 0 && _value1[0] == 'V')
        {
            int _index = _value1.substring(1).toInt();
            if (_index == 0)
            {
                encoder.getInputA();
            }
            else if (_index == 1)
            {
                encoder.getInputB();
            }
        }
        else if (index_value > 1 && _value1[0] == 'T')
        {
            encoder.setInputAutoFeecback(_value1.substring(1).toInt());
            send_done();
        }
        else if (index_value > 1 && _value1[0] == 'S')
        {
            encoder.setStopInputAutoFeecback(_value1.substring(1).toInt());
            send_done();
        }

    }

    gcode = "";
}

void communication::response(String _mes)
{
    USBPort.println(_mes);
    eth_server->println(_mes);
}

communication control_port;