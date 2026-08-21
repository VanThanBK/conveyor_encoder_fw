#include <Arduino.h>
#include "pin.h"
#include "slider.h"
#include "communication.h"
#include "io_control.h"

// func interrupt for stepper motor
void interrupt_timer_handle()
{
  slider.__timer_handle();
}
void interrupt_tp_timer_handle()
{
  slider.__tp_timer_handle();
}

void setup()
{
#if defined(__STM32F1__)
  disableDebugPorts();
  Serial.end();
  Serial1.end();
  Serial2.end();
  Serial3.end();
  
  io_control.init();
  slider.init();
  control_port.init();

  ExecuteStepTimer.attachInterrupt(TIMER_CH1, interrupt_timer_handle);
  TurnPinTimer.attachInterrupt(TIMER_CH1, interrupt_tp_timer_handle);
#else
  Serial.end();
  control_port.init();
  io_control.init();

  slider.init();

  slider.ExecuteStepTimer->attachInterrupt(interrupt_timer_handle);
  slider.TurnPinTimer->attachInterrupt(interrupt_tp_timer_handle);

#endif

}

void loop()
{
  control_port.execute();
  io_control.execute();
  slider.execute();
}
