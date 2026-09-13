# Control architecture

Task: detect approach → open → detect passage → close. A is ultrasonic approach presence; B is crossing-body interruption just beyond the arm. The two endpoint switches provide independent observations of the arm linkage's two end positions. They do not measure torque or intermediate angle.

## State table

| State | Entry condition | Action | Exit condition | Failure condition |
|---|---|---|---|---|
| BOOT | Controller starts | Command OPEN; seed endpoint debounce from pins | Open endpoint settled and sonar available → OPEN/PASSING | Three invalid sonar results, no initial valid result by 2 s, or motion fault |
| IDLE | Closed endpoint settled | Red; monitor A/B/manual | Confirmed A occupied, B occupied or manual → OPENING | Sensing or reached-endpoint fault |
| OPENING | Arrival or reopen request | Immediate OPEN setpoint; red; supervise travel | Raw + debounced open endpoint stable 100 ms → OPEN/PASSING | Opposite switch fails to release by 500 ms; target not settled by 2.5 s; contradiction |
| OPEN | Raised endpoint settled | Green; hold | B blocked → PASSING; both zones clear → CLEARANCE | Sensor or reached-endpoint fault |
| PASSING | B observed occupied | Hold raised; `seen=1` | Both zones clear → CLEARANCE | Sensor or reached-endpoint fault; occupation itself is not a fault |
| CLEARANCE | Both clear while raised | Turn emitter off 10 ms; require HIGH; restore and settle 10 ms; confirm beam clear 100 ms; then require full 1.2 s clear | Hazard after test → OPEN/PASSING; successful fresh clearance → CLOSING | Emitter-off LOW → BEAM_TEST; sensor or actuator fault |
| CLOSING | Successful diagnostic and fresh clearance | 1°/20 ms CLOSE ramp; red/chirps; continually monitor | Any loss of all-clear → OPENING; closed command reached and endpoint settled → IDLE | 500 ms source-release or 2.5 s total travel deadline, contradiction or sensing fault |
| FAULT | Fault detected | SONAR/BEAM_TEST request OPEN; actuator/LIMITS stop pulses | Repaired SONAR + clear lane + open settled + `r` → OPEN; other faults need inspection/reset | Never automatically force CLOSE or reattach a stopped actuator |

CLEARANCE remains raised while the beam is diagnostically unavailable. Sensing is explicitly not clear during this interval. After restoration, real occupancy cancels closure, and a new interval starts only after beam-clear confirmation. The diagnostic is repeated after a cancelled closing attempt; it never runs during downward movement.

## Priority and invariants

The loop updates inputs, supervises the actuator, evaluates the FSM, handles a bounded Serial command, updates the closing setpoint and updates indicators. An actuator STOP latch blocks all later drive requests.

1. **Closure-entry invariant:** the only production path requesting CLOSE is CLEARANCE with healthy/confirmed-clear A, restored/clear B, diagnostic pass, released manual input and a fresh 1.2 s interval.
2. **Closing invariant:** if those live clear conditions cease, CLOSING requests OPEN before another closing step. Beam testing is never active while demand is CLOSE.
3. **Timeout invariant:** sonar deadlines lead to OPEN fault; movement deadlines lead to STOP; the 30 s advisory has no state transition and cannot command CLOSE.
4. **Normal liveness:** with healthy inputs, valid geometry and reachable endpoints, vacancy leads OPEN/PASSING → CLEARANCE → CLOSING → IDLE. New occupation cancels this path, as intended. An indefinite occupied lane is a hold, not a deadlock.
5. **Recovery:** repaired sonar faults require explicit `r`; genuine beam-test/actuator faults require repair/reset. No automatic recovery promises are made for broken mechanics.

These are code-path arguments backed by host scenarios, not a model-checked or hardware-certified proof. 'Failure produces a safe state' is too strong: STOP removes command pulses but cannot raise a physically jammed arm or prevent gravity motion. Use a light breakaway arm and isolate servo power before fault inspection.

## Timing and sensing

| Parameter | Final value / meaning |
|---|---|
| Sonar ping / transaction timeout | 65 ms / 25 ms |
| Sonar stale deadline | 300 ms since actual accepted measurement |
| Detection / clearing | 3 consecutive readings; occupied ≤160 mm, clear ≥200 mm |
| Valid sonar envelope | 20–350 mm; fixed backboard nominally 240 mm |
| Beam clear confirmation | 100 ms; blocked assertion unfiltered outside diagnostic |
| Pre-close diagnostic | 10 ms emitter off + 10 ms settling, only while raised |
| Fresh clearance | 1.2 s after restored beam becomes confirmed clear |
| Endpoint debounce / settling | 20 ms debounce; 100 ms raw-and-debounced target dwell per command |
| Source release / total motion | 500 ms / 2.5 s, including ramp and endpoint settling |
| Closing ramp | 1°/20 ms, nominal 1.7 s for 95° → 10° |
| Long-open notice | 30 s advisory; manual hold also may show it |

Thresholds and timings are initial settings, not measured physical performance. The code rejects a pulse that completes outside its transaction deadline and a completion buffered longer than the stale interval. Control intervals use unsigned subtraction across clock rollover. The 10 µs sonar trigger is the only deliberate synchronous wait.

## Serial and indicators

At transitions: `STATE OPENING fault=0`, `STATE PASSING fault=0`, etc. Telemetry at up to 4 Hz:

`s=4 f=0 mm=240 v=1 a=0 b=1 o=1 c=0 seen=1 w=0`

This is a format example, not a captured hardware log.

| Field | Meaning |
|---|---|
| s | 0 BOOT, 1 IDLE, 2 OPENING, 3 OPEN, 4 PASSING, 5 CLEARANCE, 6 CLOSING, 7 FAULT |
| f | 0 NONE, 1 SONAR, 2 BEAM_TEST, 3 LIMITS, 4 ACTUATOR |
| mm / v | Last valid distance / latest result validity; mm alone may be stale |
| a / b | Confirmed A occupied / latest normal B blocked observation |
| o / c | Debounced open / closed endpoint |
| seen | B occupation observed in this session; not direction or exact counting |
| w | Session has been open ≥30 s; advisory clears at IDLE or a new session |

During the pre-close diagnostic, `b` retains the last normal observation but firmware explicitly forbids clearance through `beamPhase`; 4 Hz telemetry cannot establish diagnostic pulse timing. Steady red: IDLE/movement/boot. Green: confirmed raised OPEN/PASSING/CLEARANCE. Flashing red and periodic chirps: FAULT. Short chirps accompany closing. No LED state certifies clearance for arbitrary people or objects.

For SONAR recovery: repair the cause, clear both zones, verify open endpoint, release manual, send `r`. BEAM_TEST and actuator faults require repair/reset. A disconnected beam usually reads blocked and holds open indefinitely; distinguish a fault from a waiting vehicle by inspection. Do not bypass feedback to clear a fault.

The complete Mermaid diagram is maintained in the [README](../README.md#state-machine). [Review traces](review.md) · [Coverage proof and limits](geometry.md)
