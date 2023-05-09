#include <Arduino.h>
#include "pin.h"
#include "encoder.h"
#include "conveyor.h"
#include "communication.h"
// #include <Ethernet_STM.h>

// func interrupt for encoder
void interrupt_timer_handle()
{
    conveyor.__timer_handle();
}
void interrupt_tp_timer_handle()
{
    conveyor.__tp_timer_handle();
}

void setup() {
  disableDebugPorts();
  Serial.end();
  
  encoder.init();
  conveyor.init();
  control_port.init();

  ExecuteStepTimer.attachInterrupt(TIMER_CH1, interrupt_timer_handle);
  TurnPinTimer.attachInterrupt(TIMER_CH1, interrupt_tp_timer_handle);
}

void loop() {
  control_port.execute();
}