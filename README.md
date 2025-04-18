## 1.Reconfig the SPI_ETHERNET_SETTINGS clock to 4Mhz

`.pio\libdeps\genericSTM32F103C8\Ethernet\src\utility\w5100.h`

``` C++
#define SPI_ETHERNET_SETTINGS SPISettings(4000000, MSBFIRST, SPI_MODE0)
```

## 2. Before running `test_ethernet.py`, you have to install `PySide6` package by commands `pip install PySide6`.

## 3. Seem like the Ethernet_STM.h of maple not working with the W5500 Ethernet chip.

## 4. The default SERVO_TIMER, TONE_TIMER are the same as EXECUTE_STEP_TIMER, TURN_PIN_TIMER, so change SERVO_TIMER to TIM1 and disable TONE_TIMER may solve the problem.