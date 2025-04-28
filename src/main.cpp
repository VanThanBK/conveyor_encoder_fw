#include <Arduino.h>
#include "pin.h"
#include "encoder.h"
#include "conveyor.h"
#include "communication.h"

void interrupt_timer_handle()
{
  conveyor.timer_handle();
}
void interrupt_tp_timer_handle()
{
  conveyor.tp_timer_handle();
}

void setup()
{
  Serial.end();

  encoder.init();
  conveyor.init();
  control_port.init();

  conveyor.ExecuteStepTimer->attachInterrupt(interrupt_timer_handle);
  conveyor.TurnPinTimer->attachInterrupt(interrupt_tp_timer_handle);
}

void loop()
{
  control_port.execute();
  encoder.execute();
  conveyor.execute();
}
