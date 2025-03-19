#include <Arduino.h>
#include "pin.h"
#include "encoder.h"
#include "conveyor.h"
#include "communication.h"

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
  // disableDebugPorts();
  Serial.end();
  Serial1.end();
  Serial2.end();
  // Serial3.end();

  control_port.init();
  encoder.init();
  conveyor.init();

  conveyor.ExecuteStepTimer->attachInterrupt(1, interrupt_timer_handle);
  conveyor.TurnPinTimer->attachInterrupt(1, interrupt_tp_timer_handle);
}

void loop() {
  control_port.execute();
  encoder.execute();
}