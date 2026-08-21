#include "communication.h"
#include "io_control.h"

// why the slider refused a motion request, in a form the host can read
static const char *move_error(MOVE_RESULT _result)
{
    switch (_result)
    {
    case MOVE_ERR_BUSY:
        return "Unknown:Slider busy!";
    case MOVE_ERR_NOT_HOMED:
        return "Unknown:Not homed! Send M320";
    case MOVE_ERR_FAULT:
        return "Unknown:Fault latched! Send M327 C";
    case MOVE_ERR_RANGE:
        return "Unknown:Value out of range!";
    default:
        return "Unknown:Cannot move!";
    }
}

void communication::init_eth()
{
    // SPI.setBitOrder(MSBFIRST);
    // SPI.setDataMode(SPI_MODE0);
#if !defined(__STM32F1__)
    // SPI.setMISO(PIN_SPI_MISO);
    // SPI.setMOSI(PIN_SPI_MOSI);
    // SPI.setSCLK(PIN_SPI_SCK);
    // SPI.setSSEL(PIN_SPI_SS);
    SPI.begin();

    Ethernet.init(PIN_SPI_SS);
#else
    Ethernet.init(BOARD_SPI1_NSS_PIN);
#endif
    enable_rj45();
}

void communication::enable_rj45()
{
    eth_server = new EthernetServer(ethernet_port);

    if (ethernet_ip == IPAddress(0, 0, 0, 0))
    {
        Ethernet.begin(ethernet_mac);
    }
    else
    {
        Ethernet.begin(ethernet_mac, ethernet_ip, ethernet_dns, ethernet_gateway, ethernet_subnet);
    }

    eth_server->begin();

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        USBPort.println("Unknown:Ethernet hardware error!");
        return;
    }

    if (Ethernet.linkStatus() == LinkOFF)
    {
        USBPort.println("Unknown:Ethernet cable is not connected!");
        return;
    }

    if (Ethernet.linkStatus() == Unknown)
    {
        USBPort.println("Unknown:Ethernet cable is not connected!!");
        return;
    }

    USBPort.println("Ethernet init success!");
    USBPort.println(Ethernet.localIP().toString());
    Ethernet.maintain();
}

void communication::save_eth()
{
    EEPROM.update(IS_ETH_ENABLE_ADDRESS, is_eth_enable);
    EEPROM.update(ETHERNET_PORT_ADDRESS, ethernet_port & 0xFF);
    EEPROM.update(ETHERNET_PORT_ADDRESS + 1, (ethernet_port >> 8) & 0xFF);

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
    ethernet_port = EEPROM.read(ETHERNET_PORT_ADDRESS) | (EEPROM.read(ETHERNET_PORT_ADDRESS + 1) << 8);

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

void communication::send_done()
{
    if (current_cmd_port == usb_port)
    {
        USBPort.println("Ok");
    }
    else
    {
        if (eth_client && eth_client.connected())
        {
            eth_client.write((const uint8_t *)"Ok\r\n", sizeof("Ok\r\n") - 1);
        }
        else
        {
            USBPort.println("[ETH] not connected");
        }
    }
}

void communication::send_mechanics_infor()
{
    String fb_string = "R:";
    fb_string += String(slider.steps_per_rev, 2);
    fb_string += " L:";
    fb_string += String(slider.screw_lead, 3);
    fb_string += " S:";
    fb_string += String(slider.pulse_per_mm, 3);
    fb_string += " V:";
    fb_string += String(slider.getMaxSpeed(), 2);

    response(fb_string);
}

void communication::send_slider_status()
{
    String fb_string = "P:";
    fb_string += String(slider.getPosition(), 2);
    fb_string += " V:";
    fb_string += String(slider.getMoveSpeed(), 2);
    fb_string += " A:";
    fb_string += String(slider.getAccel(), 2);
    fb_string += " H:";
    fb_string += String(slider.is_homed ? 1 : 0);
    fb_string += " M:";
    fb_string += String(slider.isMoving() ? 1 : 0);
    fb_string += " E:";
    fb_string += String(slider.is_stepper_enable ? 1 : 0);
    fb_string += " F:";
    fb_string += String((int)slider.getFault());

    response(fb_string);
}

void communication::send_home_infor()
{
    String fb_string = "I:";
    fb_string += String(slider.home_input);
    fb_string += " L:";
    fb_string += String(slider.home_active_level);
    fb_string += " S:";
    fb_string += String(slider.home_speed, 2);
    fb_string += " F:";
    fb_string += String(slider.home_speed_slow, 2);
    fb_string += " B:";
    fb_string += String(slider.home_backoff, 2);
    fb_string += " O:";
    fb_string += String(slider.home_offset, 2);
    fb_string += " R:";
    fb_string += String(slider.reverse_slider ? 1 : 0);
    fb_string += " N:";
    fb_string += String(NEGATIVE_LIMIT_INPUT);
    fb_string += " P:";
    fb_string += String(POSITIVE_LIMIT_INPUT);

    response(fb_string);
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
    ethernet_port = 8844;
    ethernet_ip = IPAddress(0, 0, 0, 0);
    ethernet_dns = IPAddress(0, 0, 0, 0);
    ethernet_gateway = IPAddress(0, 0, 0, 0);
    ethernet_subnet = IPAddress(0, 0, 0, 0);
    ethernet_mac[0] = 0xDE;
    ethernet_mac[1] = 0xAD;
    ethernet_mac[2] = 0xBE;
    ethernet_mac[3] = 0xEF;
    ethernet_mac[4] = 0xFE;
    ethernet_mac[5] = 50;
    save_eth();
    init_eth();
}

void communication::init()
{
#if !defined(__STM32F1__)
    USBPort.setTx(PA9);
    USBPort.setRx(PA10);
#endif
    USBPort.begin(115200);
    USBPort.setTimeout(5);

    eth_server = NULL;

    load_eth();

    receive_string.reserve(100);
    is_string_complete = false;
    receive_string = "";

    if (is_eth_enable)
    {
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

    // eth_server only exists once Ethernet has been enabled (M389 1)
    if (eth_server)
    {
        EthernetClient c = eth_server->available();
        if (c)
        {
            if (!(eth_client && eth_client.connected()))
            {
                eth_client.stop();
                eth_client = c;
            }
        }
    }

    if (eth_client && eth_client.connected())
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

    int index_gcode = gcode.indexOf(':');

    if (index_gcode != -1)
    {
        String keyString = gcode.substring(0, index_gcode);
        String valueString = gcode.substring(index_gcode + 1);
        if (keyString == "Address")
        {
            uint16_t _address = valueString.toInt();
            slider.setAddress(_address);
            send_done();
        }
        else
        {
            response("Unknown:Gcode!");
        }
        gcode = "";
        return;
    }

    if (gcode == "IsSliderX" || gcode == "IsXSlider")
    {
        if (current_cmd_port == usb_port)
        {
            USBPort.println("YesSliderX");
        }
        else
        {
            eth_client.println("YesSliderX");
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

    else if (gcode == "Address")
    {
        String fb_string = "Address:";
        fb_string += String(slider.slider_address);
        if (current_cmd_port == usb_port)
        {
            USBPort.println(fb_string);
        }
        else
        {
            eth_client.println(fb_string);
        }
        gcode = "";
        return;
    }

    else if (gcode == "Date")
    {
        String fb_string = "FirmwareDate:";
        fb_string += firmware_date;
        if (current_cmd_port == usb_port)
        {
            USBPort.println(fb_string);
        }
        else
        {
            eth_client.println(fb_string);
        }
        gcode = "";
        return;
    }

    else if (gcode == "FirmwareVersion")
    {
        String fb_string = "FirmwareVersion:";
        fb_string += firmware_version;
        if (current_cmd_port == usb_port)
        {
            USBPort.println(fb_string);
        }
        else
        {
            eth_client.println(fb_string);
        }
        gcode = "";
        return;
    }

    else if (gcode == "MainboardVersion")
    {
        String fb_string = "MainboardVersion:";
        fb_string += MAIN_BOARD_VERSION;
        if (current_cmd_port == usb_port)
        {
            USBPort.println(fb_string);
        }
        else
        {
            eth_client.println(fb_string);
        }
        gcode = "";
        return;
    }

    else if (gcode == "Reset")
    {
        USBPort.println("SliderX Reset");
        gcode = "";
        delay(100);
        NVIC_SystemReset();
    }

    String splitWord = "";
    gcode += " ";
    byte index_value = 0;

    String keyValue = gcode.substring(0, 4);
    String _values[MAX_GCODE_VALUES];

    for (uint16_t i = 4; i < gcode.length(); i++)
    {
        if (gcode[i] == ' ')
        {
            if (splitWord == "")
                continue;

            if (index_value < MAX_GCODE_VALUES)
            {
                _values[index_value] = splitWord;
                index_value++;
            }

            splitWord = "";
            continue;
        }
        splitWord += String(gcode[i]);
    }

    String &_value1 = _values[0];
    String &_value2 = _values[1];
    String &_value3 = _values[2];
    String &_value4 = _values[3];
    String &_value5 = _values[4];
    String &_value6 = _values[5];

    // ---------------------------------------------------------- Slider X ----
    if (keyValue == "M320")
    {
        // homing: seek the home switch and zero the position.
        // "Ok" is sent by the slider once the sequence finishes.
        MOVE_RESULT _result = slider.home();
        if (_result != MOVE_OK)
        {
            response(move_error(_result));
        }
    }
    else if (keyValue == "M321")
    {
        // set / read the moving speed used by M322 (mm/s)
        if (index_value > 0)
        {
            slider.setMoveSpeed(_value1.toFloat());
            send_done();
        }
        else
        {
            String fb_string = "V:";
            fb_string += String(slider.getMoveSpeed(), 2);
            response(fb_string);
        }
    }
    else if (keyValue == "M322")
    {
        // set the desired position (mm from the home point) / read the current one
        if (index_value > 0 && (_value1[0] == 'R' || _value1[0] == 'r'))
        {
            // relative move: M322 R<distance>
            MOVE_RESULT _result = slider.movePositionRelative(_value1.substring(1).toFloat());
            if (_result != MOVE_OK)
            {
                response(move_error(_result));
            }
        }
        else if (index_value > 0)
        {
            MOVE_RESULT _result = slider.setPosition(_value1.toFloat());
            if (_result != MOVE_OK)
            {
                response(move_error(_result));
            }
        }
        else
        {
            String fb_string = "P:";
            fb_string += String(slider.getPosition(), 2);
            response(fb_string);
        }
    }
    else if (keyValue == "M323")
    {
        // release the stepper (M323) or drive the enable pin explicitly (M323 E0/E1)
        if (index_value > 0 && _value1[0] == 'E')
        {
            if (_value1.substring(1).toInt())
            {
                slider.enableStepper();
            }
            else
            {
                slider.disableStepper();
            }
        }
        else
        {
            slider.disableStepper();
        }
        send_done();
    }
    else if (keyValue == "M324")
    {
        // set / read the acceleration (mm/s^2)
        if (index_value > 0)
        {
            slider.setAccel(_value1.toFloat());
            send_done();
        }
        else
        {
            String fb_string = "A:";
            fb_string += String(slider.getAccel(), 2);
            response(fb_string);
        }
    }
    else if (keyValue == "M325")
    {
        // Negative-end homing on PB7 is fixed and only starts on M320. R handles
        // motor wiring direction.
        uint8_t _reverse = 255;
        bool _valid = true;

        String *_vals = _values;
        for (uint8_t i = 0; i < index_value; i++)
        {
            if (_vals[i].length() < 2)
            {
                continue;
            }

            String _rest = _vals[i].substring(1);
            switch (_vals[i][0])
            {
            case 'R':
                _reverse = _rest.toInt();
                break;
            default:
                _valid = false;
                break;
            }
        }

        if (index_value > 0)
        {
            if (!_valid || (_reverse != 255 && _reverse > 1))
            {
                response("Unknown:Bad M325 parameter!");
            }
            else
            {
                slider.setHomeConfig(_reverse);
                send_done();
            }
        }
        else
        {
            send_home_infor();
        }
    }
    else if (keyValue == "M326")
    {
        // status
        send_slider_status();
    }
    else if (keyValue == "M327")
    {
        // M327   -> ramp down and hold the position
        // M327 E -> emergency stop, cuts the pulses at once and latches a fault
        // M327 C -> acknowledge a latched fault so motion is accepted again
        if (index_value > 0 && _value1[0] == 'E')
        {
            slider.stopEmergency(FAULT_ESTOP);
        }
        else if (index_value > 0 && _value1[0] == 'C')
        {
            slider.clearFault();
        }
        else
        {
            slider.stopSmooth();
        }
        send_done();
    }
    else if (keyValue == "M328")
    {
        // Mechanics: M328 R<steps/rev> L<lead mm/rev>
        // pulse_per_mm is recomputed as steps_per_rev / screw_lead.
        float _steps_rev = 0;
        float _lead = 0;

        for (uint8_t i = 0; i < index_value; i++)
        {
            if (_values[i].length() < 2)
            {
                continue;
            }

            if (_values[i][0] == 'R')
            {
                _steps_rev = _values[i].substring(1).toFloat();
            }
            else if (_values[i][0] == 'L')
            {
                _lead = _values[i].substring(1).toFloat();
            }
        }

        if (index_value > 0)
        {
            MOVE_RESULT _result = slider.setMechanics(_steps_rev, _lead);
            if (_result == MOVE_OK)
            {
                send_done();
            }
            else
            {
                response(move_error(_result));
            }
        }
        else
        {
            send_mechanics_infor();
        }
    }
    else if (keyValue == "M329")
    {
        // Digital IO, all of it:
        //   M329 P<i> V<0|1>        set output i
        //   M329 P<i> F<f> V<val>   assign a function to output i
        //   M329 I<i>               read input i
        //   M329 I<i> F<f> V<val>   assign a function to input i
        //   M329 T<i> / M329 S<i>   start / stop auto feedback of input i
        //   M329                    list every input and output
        int _in = -1;
        int _out = -1;
        int _func = -1;
        int _feedback_on = -1;
        int _feedback_off = -1;
        int _level = -1;
        float _val = 0;
        bool _has_val = false;

        for (uint8_t i = 0; i < index_value; i++)
        {
            if (_values[i].length() < 2)
            {
                continue;
            }

            String _rest = _values[i].substring(1);
            switch (_values[i][0])
            {
            case 'I':
                _in = _rest.toInt();
                break;
            case 'P':
                _out = _rest.toInt();
                break;
            case 'F':
                _func = _rest.toInt();
                break;
            case 'V':
                _val = _rest.toFloat();
                _has_val = true;
                break;
            case 'T':
                _feedback_on = _rest.toInt();
                break;
            case 'S':
                _feedback_off = _rest.toInt();
                break;
            case 'L':
                _level = _rest.toInt();
                break;
            default:
                break;
            }
        }

        if (_feedback_on >= 0)
        {
            if (io_control.setInputAutoFeedback(_feedback_on))
            {
                send_done();
            }
            else
            {
                response("Unknown:Bad input index!");
            }
        }
        else if (_feedback_off >= 0)
        {
            if (io_control.setStopInputAutoFeedback(_feedback_off))
            {
                send_done();
            }
            else
            {
                response("Unknown:Bad input index!");
            }
        }
        else if (_in >= 0)
        {
            if (_in >= INPUT_COUNT)
            {
                response("Unknown:Bad input index!");
            }
            else if (_func >= 0 || _level >= 0 || _has_val)
            {
                slider.setInputFunction(_in,
                                        (_func >= 0) ? _func : slider.input_function[_in],
                                        _has_val ? _val : slider.input_value[_in],
                                        (_level >= 0) ? _level : 255);
                send_done();
            }
            else
            {
                bool _state = false;
                io_control.getInput(_in, _state);

                String fb_string = "I";
                fb_string += String(_in);
                fb_string += " V";
                fb_string += String(_state);
                fb_string += " F";
                fb_string += String(slider.input_function[_in]);
                fb_string += " L";
                fb_string += String(slider.input_active_level[_in]);
                response(fb_string);
            }
        }
        else if (_out >= 0)
        {
            if (_out >= OUTPUT_COUNT)
            {
                response("Unknown:Bad output index!");
            }
            else if (_func >= 0)
            {
                slider.setOutputFunction(_out, _func, _has_val ? _val : slider.output_value[_out]);
                send_done();
            }
            else if (_has_val)
            {
                if (slider.output_function[_out] != OUTPUT_FUNC_NONE)
                {
                    response("Unknown:Output has function!");
                }
                else
                {
                    io_control.setOutput(_out, _val != 0);
                    send_done();
                }
            }
            else
            {
                String fb_string = "O";
                fb_string += String(_out);
                fb_string += " F";
                fb_string += String(slider.output_function[_out]);
                fb_string += " V";
                fb_string += String(slider.output_value[_out], 2);
                response(fb_string);
            }
        }
        else
        {
            String fb_string = "";
            for (uint8_t i = 0; i < INPUT_COUNT; i++)
            {
                bool _state = false;
                io_control.getInput(i, _state);
                fb_string += "I";
                fb_string += String(i);
                fb_string += " V";
                fb_string += String(_state);
                fb_string += " F";
                fb_string += String(slider.input_function[i]);
                fb_string += " L";
                fb_string += String(slider.input_active_level[i]);
                fb_string += '\n';
            }
            for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
            {
                fb_string += "O";
                fb_string += String(i);
                fb_string += " F";
                fb_string += String(slider.output_function[i]);
                fb_string += " V";
                fb_string += String(slider.output_value[i], 2);
                if (i < OUTPUT_COUNT - 1)
                {
                    fb_string += '\n';
                }
            }
            response(fb_string);
        }
    }

    else if (keyValue == "M389")
    {
        is_eth_enable = _value1.toInt();
        if (is_eth_enable)
        {
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
        else
        {
            response("Unknown:Gcode!");
        }
    }
    else if (keyValue == "M391")
    {
        if (index_value > 3)
        {
            ethernet_ip = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
        else
        {
            response("Unknown:Gcode!");
        }
    }
    else if (keyValue == "M392")
    {
        if (index_value > 3)
        {
            ethernet_dns = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
        else
        {
            response("Unknown:Gcode!");
        }
    }
    else if (keyValue == "M393")
    {
        if (index_value > 3)
        {
            ethernet_gateway = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
        else
        {
            response("Unknown:Gcode!");
        }
    }
    else if (keyValue == "M394")
    {
        if (index_value > 3)
        {
            ethernet_subnet = IPAddress(_value1.toInt(), _value2.toInt(), _value3.toInt(), _value4.toInt());
            send_done();
        }
        else
        {
            response("Unknown:Gcode!");
        }
    }
    else if (keyValue == "M395")
    {
        if (index_value > 5)
        {
            ethernet_mac[0] = _value1.toInt();
            ethernet_mac[1] = _value2.toInt();
            ethernet_mac[2] = _value3.toInt();
            ethernet_mac[3] = _value4.toInt();
            ethernet_mac[4] = _value5.toInt();
            ethernet_mac[5] = _value6.toInt();

            send_done();
        }
        else
        {
            response("Unknown:Gcode!");
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
