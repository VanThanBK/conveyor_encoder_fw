# GCODES FOR SLIDER X

Firmware `3.0.0` — STM32F103 mainboard `2.0`.

Commands are plain text lines terminated with `\n`, accepted on the USB/serial port
(115200 baud) and on the Ethernet port. A command answers `Ok` when it has been
carried out, or `Unknown:<reason>` when it has not.

The whole slider is driven by ten commands, `M320`–`M329`.

| | | | |
| --- | --- | --- | --- |
| `M320` homing | `M321` speed | `M322` position | `M323` motor enable |
| `M324` acceleration | `M325` homing & limits | `M326` status | `M327` stop & fault |
| `M328` mechanics | `M329` digital IO | | |

**`M320` and `M322` answer when the motion ends**, which can be seconds later.
Exactly one reply is sent per request, but a host that keeps sending while the
axis moves will see those replies first. Wait for the reply, or match loosely.

Two kinds of line arrive unsolicited: `I<idx> V<state>` from input auto-feedback
(opt-in with `M329 T`), and `Fault:<code>` when a fault latches.

---

## M320 - Homing

Moves back to the home switch, backs off, re-approaches slowly and sets the
current position to the home offset. Motion is refused while it runs; `M327`
aborts it.

```
M320
```

| Reply | Meaning |
| --- | --- |
| `Ok` | homing finished, position is now the home offset |
| `Unknown:Slider busy!` | a move or another homing sequence is running |
| `Unknown:Fault latched! Send M327 C` | acknowledge the previous fault first |
| `Unknown:Homing timeout!` | a phase ran longer than 60 s — check the wiring and `M325 L` |
| `Unknown:Home switch stuck!` | still triggered after backing off — increase `M325 B` |
| `Unknown:Homing stopped!` | aborted by `M327`, `M323` or a stop input |

The last three also latch fault code `4`.

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

Move to a point relative to the home point. `Ok` arrives when the axis gets there.

```
M322 150     ;absolute: go to 150 mm
M322 R-20    ;relative: back off 20 mm
M322         ;-> P:150.00  (current position)
```

Sent while another move is running, the new target **supersedes** it: the axis
ramps down and drives to the new target. The superseded command is answered
`Unknown:Move superseded!` straight away, so nothing is left waiting. `M322 R`
chains onto the target being approached, so a stream of relative moves adds up as
written.

| Reply | Meaning |
| --- | --- |
| `Ok` | the slider reached the target |
| `Unknown:Not homed! Send M320 or M325 H0` | no known zero yet |
| `Unknown:Out of travel!` | outside the soft travel limit set with `M325 T` |
| `Unknown:Slider busy!` | homing is running |
| `Unknown:Fault latched! Send M327 C` | acknowledge the previous fault first |
| `Unknown:Move superseded!` | replaced by a newer target |
| `Unknown:Move stopped!` | cancelled by `M327`, `M323` or a stop input |
| `Unknown:Home limit hit!` / `Unknown:End limit hit!` | ran into a limit switch |

---

## M323 - Disable Stepper Motor

Releases the motor so the slider can be moved by hand. The homed flag is cleared —
home again before using `M322`. Any motion command re-enables the driver.

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

## M325 - Homing and Limits

Every parameter is optional; the ones left out keep their current value. Stored in
EEPROM.

```
M325 [I<n>] [L<0|1>] [D<0|1>] [S<mm/s>] [F<mm/s>] [B<mm>] [O<mm>]
     [T<mm>] [G<0|1>] [H<0|1>] [J<n>] [K<0|1>] [A<0|1>] [R<0|1>]
```

| Par | Meaning | Default |
| --- | --- | --- |
| `I` | input wired to the home switch (0..5) | `0` |
| `L` | level that means "home switch triggered" | `1` |
| `D` | homing direction: `0` toward negative, `1` toward positive | `0` |
| `S` | fast seek speed, mm/s | `40` |
| `F` | slow re-approach speed, mm/s | `5` |
| `B` | back-off distance, mm | `5` |
| `O` | position assigned at the switch, mm | `0` |
| `T` | travel length in mm from `O` away from the switch; `0` disables | `0` |
| `G` | `1` = the home switch also stops motion running into it | `1` |
| `H` | `1` = refuse position moves until homed | `1` |
| `J` | input wired to the **far end** limit switch, `255` = none | `255` |
| `K` | level that means "end switch triggered" | `1` |
| `A` | `1` = home automatically at power up | `0` |
| `R` | `1` inverts the direction output | `0` |

Sent bare it reports everything:

```
I:0 L:1 D:0 S:40.00 F:5.00 B:5.00 O:0.00 T:0.00 G:1 H:1 J:255 K:1 A:0 R:0
```

A machine with no home switch needs `M325 G0 H0`, otherwise every `M322` answers
`Unknown:Not homed!`.

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
| `1` | ran into the home end switch |
| `2` | ran into the far end switch |
| `3` | ran past the configured travel |
| `4` | homing timed out or the switch stayed stuck |
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

`L` is the level that counts as pressed. **Inputs 0..3 are conditioned on the board
and read active-high (`L1`, the default). Inputs 4 and 5 are PB7 and PB6 — the old
quadrature encoder header — configured with the MCU pull-up, so they idle HIGH and
default to `L0`.** A normally-closed safety button needs `L0` on whichever input it
uses.

Motion started from an input never writes to the command ports. A jog button that
gets refused simply does nothing, so wire output function `2` or `4` to a lamp if
the operator needs to see why.

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
M325 I0 L1 D0 B5 T400 J1 K1     ;home switch on input 1, end switch on input 2, 400 mm travel
M320                            ;home, wait for Ok
M322 150                        ;move to 150 mm, wait for Ok
```

---

## Contact Us

* Store: [https://deltaxstore.com](https://deltaxstore.com)
* Website: [https://www.deltaxrobot.com](https://www.deltaxrobot.com)
* Email: [deltaxrobot@gmail.com](mailto:deltaxrobot@gmail.com)
* Phone: +84 38 875 2005
