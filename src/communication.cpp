#include "communication.h"
#include "io_control.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool parse_float_strict(const String &_text, float &_value)
{
    if (_text.length() == 0)
    {
        return false;
    }

    const char *_start = _text.c_str();
    char *_end = NULL;
    errno = 0;
    float _parsed = strtof(_start, &_end);
    if (_end == _start || *_end != '\0' || errno == ERANGE || !isfinite(_parsed))
    {
        return false;
    }

    _value = _parsed;
    return true;
}

static bool parse_long_strict(const String &_text, long &_value)
{
    if (_text.length() == 0)
    {
        return false;
    }

    const char *_start = _text.c_str();
    char *_end = NULL;
    errno = 0;
    long _parsed = strtol(_start, &_end, 10);
    if (_end == _start || *_end != '\0' || errno == ERANGE)
    {
        return false;
    }

    _value = _parsed;
    return true;
}

static bool parse_byte_strict(const String &_text, uint8_t &_value)
{
    long _parsed = 0;
    if (!parse_long_strict(_text, _parsed) || _parsed < 0 || _parsed > 255)
    {
        return false;
    }

    _value = (uint8_t)_parsed;
    return true;
}

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
        // Keep a missing DHCP server from blocking the safety loop for a minute.
        // Runtime reconfiguration is also refused while the axis is moving.
        Ethernet.begin(ethernet_mac, 3000, 1000);
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
            long _address = 0;
            if (!parse_long_strict(valueString, _address) ||
                _address < 0 || _address > UINT16_MAX)
            {
                response("Unknown:Bad parameter!");
            }
            else
            {
                slider.setAddress((uint16_t)_address);
                send_done();
            }
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
    bool too_many_values = false;

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
            else
            {
                too_many_values = true;
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

    if (too_many_values)
    {
        response("Unknown:Bad parameter!");
        gcode = "";
        return;
    }

    // ---------------------------------------------------------- Slider X ----
    if (keyValue == "M320")
    {
        // homing: seek the home switch and zero the position.
        // "Ok" is sent by the slider once the sequence finishes.
        if (index_value != 0)
        {
            response("Unknown:Bad parameter!");
        }
        else
        {
            MOVE_RESULT _result = slider.home();
            if (_result != MOVE_OK)
            {
                response(move_error(_result));
            }
        }
    }
    else if (keyValue == "M321")
    {
        // set / read the moving speed used by M322: M321 V<mm/s>
        if (index_value > 0)
        {
            float _speed = 0;
            if (index_value != 1 || _value1.length() < 2 ||
                (_value1[0] != 'V' && _value1[0] != 'v') ||
                !parse_float_strict(_value1.substring(1), _speed))
            {
                response("Unknown:Bad parameter!");
            }
            else
            {
                slider.setMoveSpeed(_speed);
                send_done();
            }
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
            float _distance = 0;
            if (index_value != 1 ||
                !parse_float_strict(_value1.substring(1), _distance))
            {
                response("Unknown:Bad parameter!");
            }
            else
            {
                MOVE_RESULT _result = slider.movePositionRelative(_distance);
                if (_result != MOVE_OK)
                {
                    response(move_error(_result));
                }
            }
        }
        else if (index_value > 0)
        {
            float _position = 0;
            if (index_value != 1 || !parse_float_strict(_value1, _position))
            {
                response("Unknown:Bad parameter!");
            }
            else
            {
                MOVE_RESULT _result = slider.setPosition(_position);
                if (_result != MOVE_OK)
                {
                    response(move_error(_result));
                }
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
        if (index_value == 0)
        {
            slider.disableStepper();
            send_done();
        }
        else if (index_value == 1 && (_value1[0] == 'E' || _value1[0] == 'e'))
        {
            long _enable = 0;
            if (!parse_long_strict(_value1.substring(1), _enable) ||
                (_enable != 0 && _enable != 1))
            {
                response("Unknown:Bad parameter!");
            }
            else
            {
                if (_enable == 1)
                {
                    slider.enableStepper();
                }
                else
                {
                    slider.disableStepper();
                }
                send_done();
            }
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M324")
    {
        // set / read the acceleration: M324 A<mm/s^2>
        if (index_value > 0)
        {
            float _accel = 0;
            if (index_value != 1 || _value1.length() < 2 ||
                (_value1[0] != 'A' && _value1[0] != 'a') ||
                !parse_float_strict(_value1.substring(1), _accel))
            {
                response("Unknown:Bad parameter!");
            }
            else
            {
                slider.setAccel(_accel);
                send_done();
            }
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
        long _reverse = -1;
        bool _valid = true;

        String *_vals = _values;
        for (uint8_t i = 0; i < index_value; i++)
        {
            if (_vals[i].length() < 2 ||
                (_vals[i][0] != 'R' && _vals[i][0] != 'r'))
            {
                _valid = false;
                break;
            }

            if (!parse_long_strict(_vals[i].substring(1), _reverse) ||
                (_reverse != 0 && _reverse != 1))
            {
                _valid = false;
                break;
            }
        }

        if (index_value > 0)
        {
            if (!_valid || index_value != 1 || _reverse < 0)
            {
                response("Unknown:Bad M325 parameter!");
            }
            else
            {
                slider.setHomeConfig((uint8_t)_reverse);
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
        if (index_value == 0)
        {
            send_slider_status();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M327")
    {
        // M327   -> ramp down and hold the position
        // M327 E -> emergency stop, cuts the pulses at once and latches a fault
        // M327 C -> acknowledge a latched fault so motion is accepted again
        if (index_value == 0)
        {
            slider.stopSmooth();
            send_done();
        }
        else if (index_value == 1 && _value1.equalsIgnoreCase("E"))
        {
            slider.stopEmergency(FAULT_ESTOP);
            send_done();
        }
        else if (index_value == 1 && _value1.equalsIgnoreCase("C"))
        {
            slider.clearFault();
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M328")
    {
        // Mechanics: M328 R<steps/rev> L<lead mm/rev>
        // pulse_per_mm is recomputed as steps_per_rev / screw_lead.
        float _steps_rev = 0;
        float _lead = 0;
        bool _valid = true;
        bool _has_steps = false;
        bool _has_lead = false;

        for (uint8_t i = 0; i < index_value; i++)
        {
            if (_values[i].length() < 2)
            {
                _valid = false;
                break;
            }

            if (_values[i][0] == 'R' || _values[i][0] == 'r')
            {
                if (_has_steps ||
                    !parse_float_strict(_values[i].substring(1), _steps_rev) ||
                    _steps_rev <= 0)
                {
                    _valid = false;
                    break;
                }
                _has_steps = true;
            }
            else if (_values[i][0] == 'L' || _values[i][0] == 'l')
            {
                if (_has_lead ||
                    !parse_float_strict(_values[i].substring(1), _lead) ||
                    _lead <= 0)
                {
                    _valid = false;
                    break;
                }
                _has_lead = true;
            }
            else
            {
                _valid = false;
                break;
            }
        }

        if (index_value > 0)
        {
            if (!_valid || (!_has_steps && !_has_lead))
            {
                response("Unknown:Bad parameter!");
            }
            else
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
        }
        else
        {
            send_mechanics_infor();
        }
    }
    else if (keyValue == "M330")
    {
        // Set a normal output pin: M330 P<i> V<0|1>.
        long _out = -1;
        long _value = -1;
        bool _has_out = false;
        bool _has_value = false;
        bool _valid = (index_value == 2);

        for (uint8_t i = 0; _valid && i < index_value; i++)
        {
            if (_values[i].length() < 2)
            {
                _valid = false;
                break;
            }

            char _key = _values[i][0];
            if (_key >= 'a' && _key <= 'z')
            {
                _key -= ('a' - 'A');
            }

            long _parsed = -1;
            if (!parse_long_strict(_values[i].substring(1), _parsed))
            {
                _valid = false;
            }
            else if (_key == 'P' && !_has_out)
            {
                _out = _parsed;
                _has_out = true;
            }
            else if (_key == 'V' && !_has_value)
            {
                _value = _parsed;
                _has_value = true;
            }
            else
            {
                _valid = false;
            }
        }

        if (!_valid || !_has_out || !_has_value || _out < 0 ||
            (_value != 0 && _value != 1))
        {
            response("Unknown:Bad parameter!");
        }
        else if (_out >= OUTPUT_COUNT)
        {
            response("Unknown:Bad output index!");
        }
        else if (slider.output_function[_out] != OUTPUT_FUNC_NONE)
        {
            response("Unknown:Output has function!");
        }
        else
        {
            io_control.setOutput((uint8_t)_out, (uint8_t)_value);
            send_done();
        }
    }

    else if (keyValue == "M331")
    {
        // Normal digital input access, compatible with the original IO protocol:
        //   M331 V<i>  read the raw state of input i
        //   M331 T<i>  start change feedback for input i
        //   M331 S<i>  stop change feedback for input i
        if (index_value != 1 || _value1.length() < 2)
        {
            response("Unknown:Bad parameter!");
        }
        else
        {
            char _operation = _value1[0];
            if (_operation >= 'a' && _operation <= 'z')
            {
                _operation -= ('a' - 'A');
            }

            long _index = -1;
            if (!parse_long_strict(_value1.substring(1), _index) ||
                _index < 0 || _index >= INPUT_COUNT)
            {
                response("Unknown:Bad input index!");
            }
            else if (_operation == 'V')
            {
                bool _state = false;
                io_control.getInput((uint8_t)_index, _state);

                String fb_string = "I";
                fb_string += String(_index);
                fb_string += " V";
                fb_string += String(_state);
                response(fb_string);
            }
            else if (_operation == 'T')
            {
                io_control.setInputAutoFeedback((uint8_t)_index);
                send_done();
            }
            else if (_operation == 'S')
            {
                io_control.setStopInputAutoFeedback((uint8_t)_index);
                send_done();
            }
            else
            {
                response("Unknown:Bad parameter!");
            }
        }
    }

    else if (keyValue == "M332")
    {
        // Configure or inspect inputs:
        //   M332 I<i> [F<f>] [V<value>]
        //   M332 I<i>  reads one configuration; M332 lists all configurations.
        long _in = -1;
        long _func = -1;
        float _value = 0;
        bool _has_value = false;
        bool _valid = true;

        for (uint8_t i = 0; _valid && i < index_value; i++)
        {
            if (_values[i].length() < 2)
            {
                _valid = false;
                break;
            }

            char _key = _values[i][0];
            if (_key >= 'a' && _key <= 'z')
            {
                _key -= ('a' - 'A');
            }
            String _rest = _values[i].substring(1);
            long _parsed = -1;

            switch (_key)
            {
            case 'I':
                if (_in >= 0 || !parse_long_strict(_rest, _parsed) || _parsed < 0)
                    _valid = false;
                else
                    _in = _parsed;
                break;
            case 'F':
                if (_func >= 0 || !parse_long_strict(_rest, _parsed) || _parsed < 0)
                    _valid = false;
                else
                    _func = _parsed;
                break;
            case 'V':
                if (_has_value || !parse_float_strict(_rest, _value))
                    _valid = false;
                else
                    _has_value = true;
                break;
            default:
                _valid = false;
                break;
            }
        }

        if (!_valid || (index_value > 0 && _in < 0))
        {
            response("Unknown:Bad parameter!");
        }
        else if (_in >= INPUT_COUNT)
        {
            response("Unknown:Bad input index!");
        }
        else if (_func > INPUT_FUNC_JOG_HOLD)
        {
            response("Unknown:Bad parameter!");
        }
        else if (_in >= 0 && (_func >= 0 || _has_value))
        {
            slider.setInputFunction((uint8_t)_in,
                                    (_func >= 0) ? (uint8_t)_func : slider.input_function[_in],
                                    _has_value ? _value : slider.input_value[_in]);
            send_done();
        }
        else if (_in >= 0)
        {
            String fb_string = "I";
            fb_string += String(_in);
            fb_string += " F";
            fb_string += String(slider.input_function[_in]);
            fb_string += " V";
            fb_string += String(slider.input_value[_in], 2);
            response(fb_string);
        }
        else
        {
            String fb_string = "";
            for (uint8_t i = 0; i < INPUT_COUNT; i++)
            {
                fb_string += "I";
                fb_string += String(i);
                fb_string += " F";
                fb_string += String(slider.input_function[i]);
                fb_string += " V";
                fb_string += String(slider.input_value[i], 2);
                if (i < INPUT_COUNT - 1)
                {
                    fb_string += '\n';
                }
            }
            response(fb_string);
        }
    }

    else if (keyValue == "M333")
    {
        // Configure or inspect outputs:
        //   M333 P<i> F<f> [V<value>]
        //   M333 P<i> reads one configuration; M333 lists all configurations.
        long _out = -1;
        long _func = -1;
        float _value = 0;
        bool _has_value = false;
        bool _valid = true;

        for (uint8_t i = 0; _valid && i < index_value; i++)
        {
            if (_values[i].length() < 2)
            {
                _valid = false;
                break;
            }

            char _key = _values[i][0];
            if (_key >= 'a' && _key <= 'z')
            {
                _key -= ('a' - 'A');
            }
            String _rest = _values[i].substring(1);
            long _parsed = -1;

            switch (_key)
            {
            case 'P':
                if (_out >= 0 || !parse_long_strict(_rest, _parsed) || _parsed < 0)
                    _valid = false;
                else
                    _out = _parsed;
                break;
            case 'F':
                if (_func >= 0 || !parse_long_strict(_rest, _parsed) || _parsed < 0)
                    _valid = false;
                else
                    _func = _parsed;
                break;
            case 'V':
                if (_has_value || !parse_float_strict(_rest, _value))
                    _valid = false;
                else
                    _has_value = true;
                break;
            default:
                _valid = false;
                break;
            }
        }

        if (!_valid || (index_value > 0 && _out < 0) || (_has_value && _func < 0))
        {
            response("Unknown:Bad parameter!");
        }
        else if (_out >= OUTPUT_COUNT)
        {
            response("Unknown:Bad output index!");
        }
        else if (_func > OUTPUT_FUNC_FAULT)
        {
            response("Unknown:Bad parameter!");
        }
        else if (_out >= 0 && _func >= 0)
        {
            slider.setOutputFunction((uint8_t)_out, (uint8_t)_func,
                                     _has_value ? _value : slider.output_value[_out]);
            send_done();
        }
        else if (_out >= 0)
        {
            String fb_string = "O";
            fb_string += String(_out);
            fb_string += " F";
            fb_string += String(slider.output_function[_out]);
            fb_string += " V";
            fb_string += String(slider.output_value[_out], 2);
            response(fb_string);
        }
        else
        {
            String fb_string = "";
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
        long _enable = 0;
        if (index_value != 1 || !parse_long_strict(_value1, _enable) ||
            (_enable != 0 && _enable != 1))
        {
            response("Unknown:Bad parameter!");
        }
        else if (slider.isMoving())
        {
            response("Unknown:Slider busy!");
        }
        else
        {
            is_eth_enable = (_enable == 1);
            if (is_eth_enable)
            {
                init_eth();
            }
            save_eth();
            send_done();
        }
    }
    else if (keyValue == "M390")
    {
        long _port = 0;
        if (index_value == 1 && parse_long_strict(_value1, _port) &&
            _port > 0 && _port <= UINT16_MAX)
        {
            ethernet_port = (uint16_t)_port;
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M391")
    {
        uint8_t _a, _b, _c, _d;
        if (index_value == 4 &&
            parse_byte_strict(_value1, _a) && parse_byte_strict(_value2, _b) &&
            parse_byte_strict(_value3, _c) && parse_byte_strict(_value4, _d))
        {
            ethernet_ip = IPAddress(_a, _b, _c, _d);
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M392")
    {
        uint8_t _a, _b, _c, _d;
        if (index_value == 4 &&
            parse_byte_strict(_value1, _a) && parse_byte_strict(_value2, _b) &&
            parse_byte_strict(_value3, _c) && parse_byte_strict(_value4, _d))
        {
            ethernet_dns = IPAddress(_a, _b, _c, _d);
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M393")
    {
        uint8_t _a, _b, _c, _d;
        if (index_value == 4 &&
            parse_byte_strict(_value1, _a) && parse_byte_strict(_value2, _b) &&
            parse_byte_strict(_value3, _c) && parse_byte_strict(_value4, _d))
        {
            ethernet_gateway = IPAddress(_a, _b, _c, _d);
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M394")
    {
        uint8_t _a, _b, _c, _d;
        if (index_value == 4 &&
            parse_byte_strict(_value1, _a) && parse_byte_strict(_value2, _b) &&
            parse_byte_strict(_value3, _c) && parse_byte_strict(_value4, _d))
        {
            ethernet_subnet = IPAddress(_a, _b, _c, _d);
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M395")
    {
        uint8_t _mac[6];
        if (index_value == 6 &&
            parse_byte_strict(_value1, _mac[0]) &&
            parse_byte_strict(_value2, _mac[1]) &&
            parse_byte_strict(_value3, _mac[2]) &&
            parse_byte_strict(_value4, _mac[3]) &&
            parse_byte_strict(_value5, _mac[4]) &&
            parse_byte_strict(_value6, _mac[5]))
        {
            memcpy(ethernet_mac, _mac, sizeof(ethernet_mac));
            send_done();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M396")
    {
        if (index_value != 0)
        {
            response("Unknown:Bad parameter!");
        }
        else if (slider.isMoving())
        {
            response("Unknown:Slider busy!");
        }
        else
        {
            save_eth();
            init_eth();
            send_done();
        }
    }
    else if (keyValue == "M397")
    {
        if (index_value == 0)
        {
            send_ethernet_infor();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M398")
    {
        if (index_value == 0)
        {
            send_ethernet_ip();
        }
        else
        {
            response("Unknown:Bad parameter!");
        }
    }
    else if (keyValue == "M399")
    {
        if (index_value != 0)
        {
            response("Unknown:Bad parameter!");
        }
        else if (slider.isMoving())
        {
            response("Unknown:Slider busy!");
        }
        else
        {
            reset_eth_ips();
            send_ethernet_infor();
        }
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
