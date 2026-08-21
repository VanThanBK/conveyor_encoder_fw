# Slider X firmware

Firmware for the Slider X linear axis, running on the STM32F103 mainboard (rev 2.0).
It is derived from the DeltaX conveyor + encoder firmware: the step generation core,
the digital IO and the USB + Ethernet command ports are the same, rebuilt around a
position-controlled lead screw axis.

The command reference is in [`docs/gcode_slider_x.md`](docs/gcode_slider_x.md).

## The command set

Ten commands drive the whole slider:

| | |
| --- | --- |
| `M320` homing | `M325` homing & travel limits |
| `M321` moving speed | `M326` status |
| `M322` position (absolute or relative) | `M327` stop, e-stop, clear fault |
| `M323` motor enable | `M328` mechanics (motor, screw, pulse timing) |
| `M324` acceleration | `M329` digital IO |

Plus the shared DeltaX network commands `M389`–`M399` and the identity strings
(`IsSliderX`, `Address`, `FirmwareVersion`, `Reset`).

## What it does

* **The machine is entered as mechanics** (`M328 R<steps/rev> L<lead>`): pulses per
  mm is derived, never typed in. Defaults to a 1000 pulse/rev motor on a 5 mm lead
  screw — 200 pulses/mm, 5 µm per step, 100 mm/s ceiling.
* Position moves with a trapezoid ramp, absolute or relative, answered when the
  axis arrives. A new target while moving **supersedes** the old one rather than
  being refused.
* Homing with back-off and slow re-approach, optionally at power up.
* **Controlled stop** that keeps the position, separate from an emergency stop that
  cuts the pulses and drops the homed flag.
* Travel protection on both ends: home switch, far-end switch and a soft limit.
* **Latched faults** — after a limit hit or a failed homing the axis refuses to move
  until the fault is acknowledged.
* Live position during every kind of motion, usable as a **position-compare output**
  to trigger other machines.
* 6 digital inputs / 4 digital outputs with assignable functions, USB + Ethernet.

## Layout

| File | Contents |
| --- | --- |
| `src/slider.cpp/.h` | step generation, ramping, position moves, homing, IO functions |
| `src/io_control.cpp/.h` | 6 digital inputs / 4 digital outputs |
| `src/communication.cpp/.h` | gcode parser, USB and Ethernet ports |
| `src/pin.h` | pin map, EEPROM layout, version strings |

## Removed from the conveyor firmware

`src/conveyor.*`, `src/encoder.*`, `src/elapsedMillis.h` and `src/Vector/` are no
longer part of the build. They have been blanked to a single comment line so the
project still compiles; delete them when convenient:

```
git rm src/conveyor.cpp src/conveyor.h src/encoder.cpp src/encoder.h \
       src/elapsedMillis.h src/Vector/Vector.cpp src/Vector/Vector.h \
       src/Vector/VectorDefinitions.h
```

Gone with them: the quadrature encoder module and `M316`–`M319`; the operating-mode
system and `M310` (position moves and IO functions are always available now); the
free-running velocity mode and `M311`; the duplicate `M312`/`M313`; raw motor-pin
control `M314`; the calibration command `M315`, whose parts moved into `M324`,
`M327` and `M328`; and the IO commands `M330`–`M333`, folded into `M329`.

PB7 and PB6, the old encoder header, are now digital inputs 5 and 6.

## Step generator

One interrupt drives the step pin low and starts a second timer; that timer's
interrupt raises the pin again 30 µs later. Two interrupt dispatches and two
`digitalWrite` calls per step put the ceiling at about 20 kHz, so

```
top speed = 1000000 / (50 us * pulses per mm)
```

With the default 200 pulses/mm that is 100 mm/s at 5 µm resolution. Finer
microstepping trades that away proportionally: 400 pulses/mm gives 2.5 µm but only
50 mm/s. `M328` reports the ceiling as `V:`, and `M321` clamps silently to it.

This is the generator the conveyor firmware shipped with, kept deliberately: a
faster one is possible — direct GPIO register writes and a single interrupt per
step would reach roughly 125 kHz — but it needs verification on real hardware
against a real driver.

## Wiring notes

**Home switch.** Read on Input 1 (PA12) by default, treated as triggered when the
input reads HIGH, homing toward negative. Change with `M325`, e.g. `M325 I1 L0 D1`.
Outside of homing the same switch acts as a hard limit. A machine with no home
switch needs `M325 G0 H0`.

**Input levels.** Inputs 1..4 are conditioned on the board and read active-high.
Inputs 5 and 6 (PB7 / PB6) carry the MCU pull-up, so they idle HIGH and default to
active-low. A normally-closed safety button needs `L0` whichever input it sits on.

## EEPROM

Settings survive a firmware upgrade except where noted:

* **Pulses per mm is no longer stored on its own.** It comes from the mechanics
  block at address 176 (`M328`), which defaults to a 1000 pulse/rev motor on a 5 mm
  lead screw — 200 pulses/mm. Send `M328 R<n> L<mm>` for your machine.
* **Acceleration** moved to address 49 as a full 4-byte float. The old 2-byte format
  truncated the integer part to 8 bits, so any value of 256 or more was silently
  corrupted. Set it again after the upgrade.
* **Input functions** moved to 114 and grew to six entries; re-assign them with
  `M329`. Input levels live at 144..149.

Network settings, travel limits and homing configuration keep their addresses.

## Build notes

1. Reconfig the SPI_ETHERNET_SETTINGS clock to 4MHz in
   `.pio\libdeps\genericSTM32F103C8\Ethernet\src\utility\w5100.h`:

   ```cpp
   #define SPI_ETHERNET_SETTINGS SPISettings(4000000, MSBFIRST, SPI_MODE0)
   ```

2. Before running `test_ethernet.py`, install `PySide6`: `pip install PySide6`.

3. The `Ethernet_STM.h` of maple does not seem to work with the W5500 Ethernet chip.

4. The default SERVO_TIMER and TONE_TIMER collide with EXECUTE_STEP_TIMER and
   TURN_PIN_TIMER; the build flags `-DDISABLE_SERVO -DDISABLE_TONE` handle this.
