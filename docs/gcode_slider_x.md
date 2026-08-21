# GCODES FOR SLIDER X

Firmware `1.0.1` — STM32F103 mainboard `2.0`.

Commands are plain text lines terminated with `\n`, accepted on the USB/serial port
(115200 baud) and on the Ethernet port. A command answers `Ok` when it has been
carried out, or `Unknown:<reason>` when it has not.

The whole slider is driven by ten commands, `M320`–`M329`.

| | | | |
| --- | --- | --- | --- |
| `M320` homing | `M321` speed | `M322` position | `M323` motor enable |
| `M324` acceleration | `M325` motor direction | `M326` status | `M327` stop & fault |
| `M328` mechanics | `M329` digital IO | | |

**`M320` and `M322` answer when the motion ends**, which can be seconds later.
Exactly one reply is sent per request, but a host that keeps sending while the
axis moves will see those replies first. Wait for the reply, or match loosely.

Two kinds of line arrive unsolicited: `I<idx> V<state>` from input auto-feedback
(opt-in with `M329 T`), and `Fault:<code>` when a fault latches.

---

## M320 - Homing

Moves toward the fixed negative/home NPN endstop on Input 5 (index 4 / PB7), backs
off 5 mm, re-approaches at 5 mm/s and sets the position to zero. Input 5 remains
the negative endstop during normal motion. Motion is refused while homing runs;
`M327` aborts it.

```
M320
```

| Reply | Meaning |
| --- | --- |
| `Ok` | homing finished, position is now the home offset |
| `Unknown:Slider busy!` | a move or another homing sequence is running |
| `Unknown:Fault latched! Send M327 C` | acknowledge the previous fault first |
| `Unknown:Homing timeout!` | a phase ran longer than 60 s — check the Input 5 endstop, direction and wiring |
| `Unknown:Home switch stuck!` | the Input 5 endstop remained active after the fixed 5 mm back-off |
| `Unknown:Homing failed!` | the Input 6 endstop was reached unexpectedly during the back-off |
| `Unknown:Homing stopped!` | aborted by `M327`, `M323` or a stop input |

Timeout, a stuck endstop and failure to find the endstop latch fault code `4`. An
explicitly stopped homing sequence does not latch a fault.

---

## M321 - Set Moving Speed

Speed used by `M322`, in mm/s. Stored in EEPROM and clamped to what the step
generator can produce — see `M328`.

```
M321 100    ;100 mm/s
M321        ;-> V:100.00
```

Sent while a move is running, a **lower** speed takes effect immediately. A higher
one applies from the next move: the deceleration distance is already committed, and
raising the speed now would leave too little room to stop on target.

---

## M322 - Set Desired Position

Move to an absolute position referenced to home, or move by a relative distance.
`Ok` arrives when the axis gets there.

```
M322 150     ;absolute: go to 150 mm
M322 R-20    ;relative: back off 20 mm
M322         ;-> P:150.00  (current position)
```

Absolute moves require a completed `M320`. Relative moves are allowed while
`H:0`, including before the first home and after disabling the driver. They move
by the requested pulse distance but do not set the homed flag; the reported
position before home is only a local software coordinate, not a machine zero.

Sent while another move is running, the new target **supersedes** it: the axis
ramps down and drives to the new target. The superseded command is answered
`Unknown:Move superseded!` straight away, so nothing is left waiting. `M322 R`
chains onto the target being approached, so a stream of relative moves adds up as
written.

| Reply | Meaning |
| --- | --- |
| `Ok` | the slider reached the target |
| `Unknown:Not homed! Send M320` | an absolute move was requested without a known zero |
| `Unknown:Slider busy!` | homing is running |
| `Unknown:Fault latched! Send M327 C` | acknowledge the previous fault first |
| `Unknown:Move superseded!` | replaced by a newer target |
| `Unknown:Move stopped!` | cancelled by `M327`, `M323` or a stop input |
| `Unknown:Negative limit hit!` / `Unknown:Positive limit hit!` | ran into an endstop |

---

## M323 - Disable Stepper Motor

Releases the motor so the slider can be moved by hand. The homed flag is cleared.
Home again before using an absolute `M322`; relative `M322 R...` remains available.
Any motion command re-enables the driver.

```
M323       ;disable stepper
M323 E1    ;enable stepper again
```

---

## M324 - Set Acceleration

```
M324 500    ;500 mm/s2
M324        ;-> A:500.00
```

Values below 1 mm/s² are clamped: without a ramp the axis cannot be stopped in a
controlled way.

---

## M325 - Motor Direction and Fixed Homing Configuration

The two NPN endstop assignments, active levels, homing speeds, back-off and zero
offset are fixed in firmware. `M325` only changes the polarity of the motor
direction output; the setting is stored in EEPROM.

```
M325 R<0|1>    ;set motor direction inversion
M325           ;read the fixed homing configuration and current inversion
```

| Par | Meaning |
| --- | --- |
| `R0` | normal direction-output polarity |
| `R1` | inverted direction-output polarity |

Any parameter other than `R`, or an `R` value outside `0..1`, is rejected with
`Unknown:Bad M325 parameter!`.

The readback format is:

```
I:4 L:1 S:20.00 F:5.00 B:5.00 O:0.00 R:<0|1> N:4 P:5
```

| Field | Fixed meaning |
| --- | --- |
| `I:4 L:1` | negative/home NPN endstop on Input 5 (index 4 / PB7), active HIGH |
| `S:20.00` | initial search speed, 20 mm/s |
| `F:5.00` | slow re-approach speed, 5 mm/s |
| `B:5.00` | back-off distance, 5 mm |
| `O:0.00` | position assigned at the Input 5 endstop, 0 mm |
| `R` | current direction inversion |
| `N:4 P:5` | negative/home and positive endstops on Input 5/6 (PB7/PB6), active HIGH |

Absolute position moves require a completed `M320`; relative moves remain
available before homing. There is no `M325` option to bypass homing for absolute
moves. The current firmware has no configurable soft-travel limit and does not
home automatically at power-up.

---

## M326 - Status

```
M326
```

```
P:<position> V:<speed> A:<accel> H:<homed> M:<moving> E:<driver on> F:<fault>
```

The position is live — it updates during moves and homing alike.

---

## M327 - Stop and Fault

```
M327      ;ramp down and hold the position
M327 E    ;emergency stop - cuts the pulses at once
M327 C    ;acknowledge a latched fault
```

`M327` decelerates, so the position stays trustworthy. `M327 E` stops instantly:
steps may be lost, so the homed flag is cleared and fault `5` latches.

A latched fault refuses all motion until `M327 C`. This is deliberate — after a
limit hit or a failed homing the position is not trustworthy. Recovery is normally
`M327 C` then `M320`.

| Code | Cause |
| --- | --- |
| `0` | none |
| `1` | ran into the negative endstop (Input 5) |
| `2` | ran into the positive endstop (Input 6) |
| `3` | reserved; not raised by the current firmware |
| `4` | homing timed out or the Input 5 endstop stayed active |
| `5` | emergency stop |

The interrupted move is answered before `M327` replies `Ok`, and a queued retarget
is cancelled.

---

## M328 - Mechanics

Enter the machine rather than a raw pulses-per-mm figure. The firmware works out

```
pulse_per_mm = steps_per_rev / screw_lead
```

and that is the only place the mm scale comes from, so it cannot drift out of step
with the mechanics.

```
M328 [R<steps/rev>] [L<lead mm/rev>]
M328    ;-> R:1000.00 L:5.000 S:200.000 V:100.00
```

| Par | Meaning | Default |
| --- | --- | --- |
| `R` | motor pulses per revolution, including microstepping | `1000` |
| `L` | lead screw travel per revolution, mm | `5` |

`S:` is the resulting pulses per mm and `V:` the top speed the step generator can
produce. Refused with `Unknown:Slider busy!` while the axis moves, and homing is
cleared afterwards because the mm scale changed.

Use `M325 R<0|1>` if the axis runs the wrong way; a gear reduction between motor
and screw just multiplies `R`.

### Resolution against top speed

The step generator runs at 20 kHz, so resolution and speed trade off directly.
With a 5 mm lead screw:

| Motor | pulses/mm | Resolution | Top speed |
| --- | --- | --- | --- |
| **1000 P/R** | **200** | **5.0 µm** | **100 mm/s** |
| 2000 P/R | 400 | 2.5 µm | 50 mm/s |
| 5000 P/R | 1000 | 1.0 µm | 20 mm/s |

`M321` clamps to the ceiling silently, so a speed that looks ignored means the
motor is set too fine — set the driver coarser and send the matching `M328 R`.
`M328` always reports the ceiling for the configured machine as `V:`.

The pulse itself needs no setting: the pin idles high and each step pulls it low
for 30 µs, which suits every common step and servo driver.

---

## M329 - Digital IO

Six inputs (0..5) and four outputs (0..3).

Inputs 4 and 5 (human-facing Input 5/6) are reserved for the two NPN endstops.
Input 5 is the negative/home endstop and Input 6 is the positive endstop. In G-code
they are addressed as `I4` and `I5`. They may be read or monitored, but attempts
to assign them an input function keep the function at `0` and the active level at
`1`.

```
M329 P<i> V<0|1>          set output i
M329 P<i> F<f> [V<val>]   assign a function to output i
M329 P<i>                 read output i's function
M329 I<i>                 read input i
M329 I<i> [F<f>] [V<val>] [L<0|1>]   configure input i
M329 T<i> / M329 S<i>     start / stop auto feedback of input i
M329                      list every input and output
```

Input functions (`F`):

| Code | Function |
| --- | --- |
| `0` | none |
| `1` | move by `V` mm, relative |
| `2` | stop, with deceleration (also aborts homing) |
| `3` | start homing |
| `4` | emergency stop — works in any state, latches fault `5` |

Output functions (`F`):

| Code | Function |
| --- | --- |
| `0` | none — free for `M329 P<i> V<0\|1>` |
| `1` | ON while resting at the target of a completed move |
| `2` | ON once homing has completed |
| `3` | position compare: ON while the live position is at or past `V` mm — switches **during** the move, so it can trigger a camera or a robot |
| `4` | ON while a fault is latched |

`L` is the level that counts as pressed. Inputs 0..3 are conditioned by the V2
board and default to active-high (`L1`); their action level can be changed. Inputs
4 and 5 are PB7 and PB6 on the old quadrature encoder channels. They are also
conditioned by the board, reserved for the negative/home and positive endstops,
and fixed active-high.

Motion started from an input never writes to the command ports. A jog button that
gets refused simply does nothing, so wire output function `2` or `4` to a lamp if
the operator needs to see why.

Input function `1` is also available before homing and does not set the homed flag.

---

## Identity and network

| Gcode | Description |
| --- | --- |
| `IsSliderX` | replies `YesSliderX` |
| `Address` / `Address:<n>` | read / set the device address |
| `Date`, `FirmwareVersion`, `MainboardVersion` | version information |
| `IP` | current IP configuration |
| `Reset` | reboot the mainboard |
| `M389 <0\|1>` | enable Ethernet |
| `M390 <port>` | TCP port |
| `M391`/`M392`/`M393`/`M394 <a> <b> <c> <d>` | IP / DNS / gateway / subnet |
| `M395 <6 bytes>` | MAC address |
| `M396` | save and restart Ethernet |
| `M397` / `M398` | report the stored / active network settings |
| `M399` | reset the network settings to their defaults |

---

## Typical start-up sequence

```
M328 R1000 L5                   ;1000 pulse/rev motor, 5 mm lead screw
M324 500                        ;500 mm/s2
M321 100                        ;100 mm/s, the ceiling at 200 pulse/mm
M325 R0                         ;normal direction polarity; use R1 if reversed
M320                            ;home, wait for Ok
M322 150                        ;move to 150 mm, wait for Ok
```

---

## Contact Us

* Store: [https://deltaxstore.com](https://deltaxstore.com)
* Website: [https://www.deltaxrobot.com](https://www.deltaxrobot.com)
* Email: [deltaxrobot@gmail.com](mailto:deltaxrobot@gmail.com)
* Phone: +84 38 875 2005
