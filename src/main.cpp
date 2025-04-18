#include <Arduino.h>
#include "pin.h"
#include "encoder.h"
#include "conveyor.h"
#include "communication.h"

// func interrupt for stepper motor
void interrupt_timer_handle()
{
  conveyor.__timer_handle();
}
void interrupt_tp_timer_handle()
{
  conveyor.__tp_timer_handle();
}

void setup()
{
#if defined(__STM32F1__)
  disableDebugPorts();
  Serial.end();
  Serial1.end();
  Serial2.end();
  Serial3.end();
  
  encoder.init();
  conveyor.init();
  control_port.init();

  ExecuteStepTimer.attachInterrupt(TIMER_CH1, interrupt_timer_handle);
  TurnPinTimer.attachInterrupt(TIMER_CH1, interrupt_tp_timer_handle);
#else
  Serial.end();
  control_port.init();

  encoder.init();
  conveyor.init();

  conveyor.ExecuteStepTimer->attachInterrupt(1, interrupt_timer_handle);
  conveyor.TurnPinTimer->attachInterrupt(1, interrupt_tp_timer_handle);
  
#endif
}

void loop()
{
  control_port.execute();
  encoder.execute();
}