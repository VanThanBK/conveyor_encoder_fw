#include "communication.h"

void communication::init_eth()
{
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    Ethernet.init(PIN_SPI_SS);
    eth_server = new EthernetServer(ethernet_port);

    ethernet_mac[0] = 0xDE;
    ethernet_mac[1] = 0xAD;
    ethernet_mac[2] = 0xBE;
    ethernet_mac[3] = 0xEF;
    ethernet_mac[4] = 0xFE;
    ethernet_mac[5] = 0x12;

    if (Ethernet.begin(ethernet_mac) == 0)
    {
        USBPort.println("Failed to configure Ethernet using DHCP");

        if (Ethernet.linkStatus() == LinkOFF)
        {
            USBPort.println("LinkOFF:Ethernet cable is not connected!");
        }
        if (Ethernet.linkStatus() == Unknown)
        {
            USBPort.println("Unknown:Ethernet cable is not connected!");
        }
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            USBPort.println("Unknown:Ethernet hardware error!");
        }

        Ethernet.begin(ethernet_mac, ethernet_ip, ethernet_dns, ethernet_gateway, ethernet_subnet);
    }

    // USBPort.print("IP address: ");
    // USBPort.println(Ethernet.localIP());

    eth_server->begin();
    delay(1);
    // Ethernet.maintain();
    // delay(1);
}

void communication::save_eth()
{
    EEPROM.update(IS_ETH_ENABLE_ADDRESS, is_eth_enable);
    EEPROM.update(ETHERNET_PORT_ADDRESS, ethernet_port);

    EEPROM.update(ETHERNET_IP_ADDRESS, ethernet_ip[0]);
    EEPROM.update(ETHERNET_IP_ADDRESS + 1, ethernet_ip[1]);
    EEPROM.update(ETHERNET_IP_ADDRESS + 2, ethernet_ip[2]);
    EEPROM.update(ETHERNET_IP_ADDRESS + 3, ethernet_ip[3]);

    EEPROM.update(ETHERNET_DNS_ADDRESS, ethernet_dns[0]);
    EEPROM.update(ETHERNET_DNS_ADDRESS + 1, ethernet_dns[1]);
    EEPROM.update(ETHERNET_DNS_ADDRESS + 2, ethernet_dns[2]);
    EEPROM.update(ETHERNET_DNS_ADDRESS + 3, ethernet_dns[3]);

    EEPROM.update(ETHERNET_GATEWAY_ADDRESS, ethernet_gateway[0]);
    EEPROM.update(ETHERNET_GATEWAY_ADDRESS + 1, ethernet_gateway[1]);
    EEPROM.update(ETHERNET_GATEWAY_ADDRESS + 2, ethernet_gateway[2]);
    EEPROM.update(ETHERNET_GATEWAY_ADDRESS + 3, ethernet_gateway[3]);

    EEPROM.update(ETHERNET_SUBNET_ADDRESS, ethernet_subnet[0]);
    EEPROM.update(ETHERNET_SUBNET_ADDRESS + 1, ethernet_subnet[1]);
    EEPROM.update(ETHERNET_SUBNET_ADDRESS + 2, ethernet_subnet[2]);
    EEPROM.update(ETHERNET_SUBNET_ADDRESS + 3, ethernet_subnet[3]);

    EEPROM.update(ETHERNET_MAC_ADDRESS, ethernet_mac[0]);
    EEPROM.update(ETHERNET_MAC_ADDRESS + 1, ethernet_mac[1]);
    EEPROM.update(ETHERNET_MAC_ADDRESS + 2, ethernet_mac[2]);
    EEPROM.update(ETHERNET_MAC_ADDRESS + 3, ethernet_mac[3]);
    EEPROM.update(ETHERNET_MAC_ADDRESS + 4, ethernet_mac[4]);
    EEPROM.update(ETHERNET_MAC_ADDRESS + 5, ethernet_mac[5]);
    delay(1);
}

void communication::load_eth()
{
    is_eth_enable = EEPROM.read(IS_ETH_ENABLE_ADDRESS);
    ethernet_port = EEPROM.read(ETHERNET_PORT_ADDRESS);

    ethernet_ip[0] = EEPROM.read(ETHERNET_IP_ADDRESS);
    ethernet_ip[1] = EEPROM.read(ETHERNET_IP_ADDRESS + 1);
    ethernet_ip[2] = EEPROM.read(ETHERNET_IP_ADDRESS + 2);
    ethernet_ip[3] = EEPROM.read(ETHERNET_IP_ADDRESS + 3);

    ethernet_dns[0] = EEPROM.read(ETHERNET_DNS_ADDRESS);
    ethernet_dns[1] = EEPROM.read(ETHERNET_DNS_ADDRESS + 1);
    ethernet_dns[2] = EEPROM.read(ETHERNET_DNS_ADDRESS + 2);
    ethernet_dns[3] = EEPROM.read(ETHERNET_DNS_ADDRESS + 3);

    ethernet_gateway[0] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS);
    ethernet_gateway[1] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS + 1);
    ethernet_gateway[2] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS + 2);
    ethernet_gateway[3] = EEPROM.read(ETHERNET_GATEWAY_ADDRESS + 3);

    ethernet_subnet[0] = EEPROM.read(ETHERNET_SUBNET_ADDRESS);
    ethernet_subnet[1] = EEPROM.read(ETHERNET_SUBNET_ADDRESS + 1);
    ethernet_subnet[2] = EEPROM.read(ETHERNET_SUBNET_ADDRESS + 2);
    ethernet_subnet[3] = EEPROM.read(ETHERNET_SUBNET_ADDRESS + 3);

    ethernet_mac[0] = EEPROM.read(ETHERNET_MAC_ADDRESS);
    ethernet_mac[1] = EEPROM.read(ETHERNET_MAC_ADDRESS + 1);
    ethernet_mac[2] = EEPROM.read(ETHERNET_MAC_ADDRESS + 2);
    ethernet_mac[3] = EEPROM.read(ETHERNET_MAC_ADDRESS + 3);
    ethernet_mac[4] = EEPROM.read(ETHERNET_MAC_ADDRESS + 4);
    ethernet_mac[5] = EEPROM.read(ETHERNET_MAC_ADDRESS + 5);
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

void communication::send_ethernet_infor()
{
    String fb_string = "E_port:";
    fb_string += String(ethernet_port) + '\n';
    fb_string += "E_ip:";
    fb_string += ethernet_ip.toString() + '\n';
    fb_string += "E_dns:";
    fb_string += ethernet_dns.toString() + '\n';
    fb_string += "E_gateway:";
    fb_string += ethernet_gateway.toString() + '\n';
    fb_string += "E_subnet:";
    fb_string += ethernet_subnet.toString() + '\n';
    fb_string += "E_mac:";
    fb_string += String(ethernet_mac[0]) + '.';
    fb_string += String(ethernet_mac[1]) + '.';
    fb_string += String(ethernet_mac[2]) + '.';
    fb_string += String(ethernet_mac[3]) + '.';
    fb_string += String(ethernet_mac[4]) + '.';
    fb_string += String(ethernet_mac[5]);

    if (current_cmd_port == usb_port)
    {
        USBPort.println(fb_string);
    }
    else
    {
        eth_client.println(fb_string);
    }
}

void communication::send_ethernet_ip()
{
    String fb_string = "E_port:";
    fb_string += String(ethernet_port) + '\n';
    fb_string += "E_ip:";
    fb_string += Ethernet.localIP().toString() + '\n';
    fb_string += "E_dns:";
    fb_string += Ethernet.dnsServerIP().toString() + '\n';
    fb_string += "E_gateway:";
    fb_string += Ethernet.gatewayIP().toString() + '\n';
    fb_string += "E_subnet:";
    fb_string += Ethernet.subnetMask().toString() + '\n';
    fb_string += "E_mac:";
    fb_string += String(ethernet_mac[0]) + '.';
    fb_string += String(ethernet_mac[1]) + '.';
    fb_string += String(ethernet_mac[2]) + '.';
    fb_string += String(ethernet_mac[3]) + '.';
    fb_string += String(ethernet_mac[4]) + '.';
    fb_string += String(ethernet_mac[5]);

    if (current_cmd_port == usb_port)
    {
        USBPort.println(fb_string);
    }
    else
    {
        eth_client.println(fb_string);
    }
}

void communication::reset_eth_ips()
{
    ethernet_port = 80;
    ethernet_ip = IPAddress(0, 0, 0, 0);
    ethernet_dns = IPAddress(0, 0, 0, 0);
    ethernet_gateway = IPAddress(0, 0, 0, 0);
    ethernet_subnet = IPAddress(0, 0, 0, 0);
    ethernet_mac[0] = 0xDE;
    ethernet_mac[1] = 0xAD;
    ethernet_mac[2] = 0xBE;
    ethernet_mac[3] = 0xEF;
    ethernet_mac[4] = 0xFE;
    ethernet_mac[5] = 0xED;
    save_eth();
    init_eth();
}

void communication::send_input_a_state()
{
    String fb_string = "I0 V";
    fb_string += String(encoder.getInputA());

    if (current_cmd_port == usb_port)
    {
        USBPort.println(fb_string);
    }
    else
    {
        eth_client.println(fb_string);
    }
}

void communication::send_input_b_state()
{
    String fb_string = "I1 V";
    fb_string += String(encoder.getInputB());

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

    load_eth();

    receive_string.reserve(100);
    is_string_complete = false;
    receive_string = "";

    pinMode(ETH_RESET_PIN, OUTPUT);
    digitalWrite(ETH_RESET_PIN, 0);
    delay(100);
    digitalWrite(ETH_RESET_PIN, 1);
    delay(100);
    if (is_eth_enable) {
        init_eth();
    }

    pinMode(LED_RUN_PIN, OUTPUT);
    digitalWrite(LED_RUN_PIN, run_led_state);

    pinMode(LED_FUNC_PIN, OUTPUT);
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
        func_led_state = !func_led_state;
        digitalWrite(LED_FUNC_PIN, func_led_state);
    }
    else if (is_eth_string_complete)
    {
        gcode = eth_receive_string;
        eth_receive_string = "";
        is_eth_string_complete = false;
        current_cmd_port = eth_port;
        func_led_state = !func_led_state;
        digitalWrite(LED_FUNC_PIN, func_led_state);
    }
    else
    {
        return;
    }

    if (gcode == "")
    {
        return;
    }

    if (gcode == "IsXConveyor")
    {
        if (current_cmd_port == usb_port)
        {
            USBPort.println("YesXConveyor");
        }
        else
        {
            eth_client.println("YesXConveyor");
        }
        gcode = "";
        return;
    }
    else if (gcode == "IsXEncoder")
    {
        if (current_cmd_port == usb_port)
        {
            USBPort.println("YesXEncoder");
        }
        else
        {
            eth_client.println("YesXEncoder");
        }
        gcode = "";
        return;
    }

    else if (gcode == "IP")
    {
        send_ethernet_ip();
        gcode = "";
        return;
    }

    String splitWord = "";
    gcode += " ";
    byte index_value = 0;

    String keyValue = gcode.substring(0, 4);
    String _value1 = "";
    String _value2 = "";
    String _value3 = "";
    String _value4 = "";
    String _value5 = "";
    String _value6 = "";

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
            else if (index_value == 2)
            {
                _value3 = splitWord;
                index_value++;
            }
            else if (index_value == 3)
            {
                _value4 = splitWord;
                index_value++;
            }
            else if (index_value == 4)
            {
                _value5 = splitWord;
                index_value++;
            }
            else if (index_value == 5)
            {
                _value6 = splitWord;
                index_value++;
            }

            splitWord = "";
            continue;
        }
        splitWord += String(gcode[i]);
    }

    // conveyor
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
        else if (index_value > 0 && _value1[0] == 'B')
        {
            // set accel
            conveyor.setVelocityButton(_value1.substring(1).toFloat());
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
            encoder.resetCounter();
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
                send_input_a_state();
            }
            else if (_index == 1)
            {
                send_input_b_state();
            }
        }
        else if (index_value > 0 && _value1[0] == 'T')
        {
            encoder.setInputAutoFeecback(_value1.substring(1).toInt());
            send_done();
        }
        else if (index_value > 0 && _value1[0] == 'S')
        {
            encoder.setStopInputAutoFeecback(_value1.substring(1).toInt());
            send_done();
        }
    }

    // eth setting

    else if (keyValue == "M389") 
    {
        is_eth_enable = _value1.toInt();
        if (is_eth_enable) {
            init_eth();
        }
        save_eth();
        send_done();
    }
    else if (keyValue == "M390")
    {
        if (index_value > 0)
        {
            ethernet_port = _value1.toInt();
            send_done();
        }
    }
    else if (keyValue == "M391")
    {
        if (index_value > 2)
        {
            ethernet_ip = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
    }
    else if (keyValue == "M392")
    {
        if (index_value > 2)
        {
            ethernet_dns = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
    }
    else if (keyValue == "M393")
    {
        if (index_value > 2)
        {
            ethernet_gateway = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
    }
    else if (keyValue == "M394")
    {
        if (index_value > 2)
        {
            ethernet_subnet = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
    }
    else if (keyValue == "M395")
    {
        if (index_value > 4)
        {
            ethernet_mac[0] = _value1.toInt();
            ethernet_mac[1] = _value2.toInt();
            ethernet_mac[2] = _value3.toInt();
            ethernet_mac[3] = _value4.toInt();
            ethernet_mac[4] = _value5.toInt();
            ethernet_mac[5] = _value6.toInt();

            send_done();
        }
    }
    else if (keyValue == "M396")
    {
        save_eth();
        init_eth();
        send_done();
    }
    else if (keyValue == "M397")
    {
        send_ethernet_infor();
    }
    else if (keyValue == "M398")
    {
        send_ethernet_ip();
    }
    else if (keyValue == "M399")
    {
        reset_eth_ips();
        send_ethernet_infor();
    }
    else
    {
        response("Unknown:Gcode!");
    }

    gcode = "";
}

void communication::response(String _mes)
{
    if (current_cmd_port == usb_port)
    {
        USBPort.println(_mes);
    }
    else
    {
        eth_client.println(_mes);
    }
}

communication control_port;