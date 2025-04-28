#include "communication.h"

void communication::init_eth()
{
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    Ethernet.init(PIN_SPI_SS);
    eth_server = new EthernetServer(ethernet_port);
    int eth_init = 0;
    eth_init = Ethernet.begin(ethernet_mac);
    if (eth_init == 1)
    {
        USBPort.println("Ethernet init success!");
    }
    else if (eth_init == 0)
    {
        USBPort.println("Ethernet init failed!");
    }
    else
    {
        USBPort.println("Unknown:Ethernet init error!");
    }

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        ethernet_infor = "Unknown:Ethernet hardware error!";
    }

    if (Ethernet.linkStatus() == LinkOFF)
    {
        ethernet_infor = "Unknown:Ethernet cable is not connected!";
    }

    USBPort.println(Ethernet.localIP());
    eth_server->begin();
    ethernet_infor = "Ethernet init success!";

    Ethernet.maintain();
}

void communication::save_eth()
{
    EEPROM.update(ETHERNET_ENABLE_ADDRESS, is_enable_eth);
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
    is_enable_eth = EEPROM.read(ETHERNET_ENABLE_ADDRESS);
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

void communication::save_string(uint16_t _address, String _str)
{
    int _len = _str.length();
    EEPROM.update(_address, _len);
    for (int i = 0; i < _len; i++)
    {
        EEPROM.update(_address + i, _str[i]);
    }
}

void communication::load_string(uint16_t _address, String &_str)
{
    int _len = EEPROM.read(_address);
    _str = "";
    for (int i = 0; i < _len; i++)
    {
        _str += (char)EEPROM.read(_address + i + 1);
    }
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
    USBPort.setTx(USB_TX_PIN);
    USBPort.setRx(USB_RX_PIN);
    USBPort.begin(115200);
    USBPort.setTimeout(5);

    receive_string.reserve(100);
    is_string_complete = false;
    receive_string = "";

    ethernet_port = DEFAULT_PORT;
    ethernet_mac[0] = 0xDE;
    ethernet_mac[1] = 0xAD;
    ethernet_mac[2] = 0xBE;
    ethernet_mac[3] = 0xEF;
    ethernet_mac[4] = 0xFE;
    ethernet_mac[5] = 0x13;

    EEPROM.read(ETHERNET_ENABLE_ADDRESS) == 1 ? is_enable_eth = true : is_enable_eth = false;
    load_string(DEVICE_NAME_ADDRESS, device_name);
    load_string(UUID_ADDRESS, UUID);
    load_string(FIRMWARE_VERSION_ADDRESS, firmware_version);
    load_string(BOARD_VERSION_ADDRESS, board_version);
    load_string(SERIAL_NUMBER_ADDRESS, serial_number);

    pinMode(ETH_RESET_PIN, OUTPUT);
    digitalWrite(ETH_RESET_PIN, 0);
    delay(100);
    digitalWrite(ETH_RESET_PIN, 1);
    delay(100);

    if (is_enable_eth)
    {
        load_eth();
        init_eth();
    }

    pinMode(LED_RUN_PIN, OUTPUT);
    digitalWrite(LED_RUN_PIN, 0);
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
        digitalWrite(LED_FUNC_PIN, !digitalRead(LED_FUNC_PIN));
    }
    else if (is_eth_string_complete)
    {
        gcode = eth_receive_string;
        eth_receive_string = "";
        is_eth_string_complete = false;
        current_cmd_port = eth_port;
        digitalWrite(LED_FUNC_PIN, !digitalRead(LED_FUNC_PIN));
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
        if (current_cmd_port == usb_port)
        {
            send_ethernet_ip();
        }
        else
        {
            eth_client.println("IP");
            send_ethernet_ip();
        }
        gcode = "";
        return;
    }
    else if (gcode == "Infor")
    {
        if (current_cmd_port == usb_port)
        {
            USBPort.println("Device Name: " + device_name);
            USBPort.println("UUID: " + UUID);
            USBPort.println("Firmware Version: " + firmware_version);
            USBPort.println("Board Version: " + board_version);
            USBPort.println("Serial No: " + serial_number);
        }
        else
        {
            eth_client.println("Device Name: " + device_name);
            eth_client.println("UUID: " + UUID);
            eth_client.println("Firmware Version: " + firmware_version);
            eth_client.println("Board Version: " + board_version);
            eth_client.println("Serial No: " + serial_number);
        }
        gcode = "";
        return;
    }

    // tach
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
            send_done();
            encoder.setTimeAutoFeedback(_time_response);
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
    else if (keyValue == "M389")
    {
        if (_value1.toInt() == 1)
        {
            is_enable_eth = true;
            init_eth();
        }
        else
        {
            is_enable_eth = false;
        }
        send_done();
    }
    // eth setting
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
    else if (keyValue == "NAME")
    {
        if (_value1.length() > 0)
        {
            device_name = _value1;
            save_string(DEVICE_NAME_ADDRESS, device_name);
            send_done();
        }
    }
    else if (keyValue == "UUID")
    {
        if (_value1.length() > 0)
        {
            UUID = _value1;
            save_string(UUID_ADDRESS, UUID);
            send_done();
        }
    }
    else if (keyValue == "FIRM")
    {
        if (_value1.length() > 0)
        {
            firmware_version = _value1;
            save_string(FIRMWARE_VERSION_ADDRESS, firmware_version);
            send_done();
        }
    }
    else if (keyValue == "BOAR")
    {
        if (_value1.length() > 0)
        {
            board_version = _value1;
            save_string(BOARD_VERSION_ADDRESS, board_version);
            send_done();
        }
    }
    else if (keyValue == "SERI")
    {
        if (_value1.length() > 0)
        {
            serial_number = _value1;
            save_string(SERIAL_NUMBER_ADDRESS, serial_number);
            send_done();
        }
    }
    else
    {
        response("Unknown:G-code!");
    }

    gcode = "";
}

void communication::response(String _mes)
{
    USBPort.println(_mes);
    eth_server->println(_mes);
}

communication control_port;