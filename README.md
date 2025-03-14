## 1.Reconfig the SPI_ETHERNET_SETTINGS clock to 4Mhz

`.pio\libdeps\genericSTM32F103C8\Ethernet\src\utility\w5100.h`

``` C++
#define SPI_ETHERNET_SETTINGS SPISettings(4000000, MSBFIRST, SPI_MODE0)
```

## 2. Before running `test_ethernet.py`, you have to install `PySide6` package by commands `pip install PySide6`.