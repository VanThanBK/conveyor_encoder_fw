#include <Arduino.h>
#include "pin.h"
#include "encoder.h"
#include "conveyor.h"
#include "communication.h"

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
  Serial.end();
  Serial1.end();

  encoder.init();
  conveyor.init();
  control_port.init();

  conveyor.ExecuteStepTimer->attachInterrupt(1, interrupt_timer_handle);
  conveyor.TurnPinTimer->attachInterrupt(1, interrupt_tp_timer_handle);
}

void loop()
{
  control_port.execute();
  encoder.execute();
}
