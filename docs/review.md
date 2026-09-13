# Final engineering review

Verdict: **C — advanced student implementation, with physical validation pending.** The software/design merits C; a completed hardware submission has not yet been demonstrated. D is not justified without repeatable real runs, measured clearance/reversal margins, power tests and clear evidence. No evaluator rubric was supplied, so judging expectations below are inferences.

## Requirements audit

The task explicitly requires approach detection, opening, passage detection and closing. Implicit requirements are repeatability, recovery after a completed or abandoned passage, and a coherent demonstrable build. Safety requires checking occupancy before and during descent, treating uncertainty conservatively, and bounding actuator drive attempts. It does not explicitly require access control, networking, exact counting, certified safety or a display.

Scores are engineering judgements, not measurements. V = evaluation value, R = reliability benefit, I = implementation risk, D = demonstration visibility; each is out of 10. The decisions consider coupled risks rather than treating a score product as an objective safety calculation.

| Feature | Requirement served | Classification | V | R | I | D | Decision |
|---|---|---|---:|---:|---:|---:|---|
| HC-SR04 approach | Detect before arm | ESSENTIAL role; RISKY if target is poor | 8 | 7 | 5 | 9 | KEEP; require backboard and measured target coverage |
| Through-beam crossing | Detect occupation/trailing-edge clearance | ESSENTIAL | 10 | 9 | 4 | 10 | KEEP; geometry and polarity are acceptance gates |
| Endpoint microswitches | Know whether opening/closing completed | HIGH-VALUE | 8 | 8 | 5 | 8 | KEEP; adjustable low-force cams, fresh settling check |
| Periodic emitter test | Diagnose stuck-clear input | RISKY original timing | 5 | 7 | 6 | 4 | SIMPLIFY to a raised, pre-close test; retain NPN and two resistors |
| Manual hold-open | Setup/recovery convenience | HIGH-VALUE | 6 | 6 | 2 | 8 | KEEP; never provides forced close or jam override |
| Piezo | Indicate downward movement/fault | OPTIONAL | 4 | 2 | 2 | 7 | KEEP optional physically; omit if only a high-current buzzer is available |
| Red/green LEDs | Show command/movement result | HIGH-VALUE | 8 | 5 | 1 | 10 | KEEP; green requires confirmed open endpoint |
| FAULT state | Bound failure behaviour | ESSENTIAL | 9 | 9 | 4 | 8 | KEEP; sensor-open and actuator-stop policies remain distinct |
| Latched 30 s session fault | Detect long wait | OVERENGINEERED / nuisance | 2 | 2 | 5 | 3 | REMOVE fault; retain Serial wait advisory with automatic recovery |
| Startup opening | Resolve uncertain initial position | HIGH-VALUE | 7 | 7 | 4 | 8 | KEEP; first command OPEN, never claim power-loss fail-open |
| Fixed thresholds | Separate occupied/background | ESSENTIAL configuration; RISKY if unmeasured | 7 | 7 | 5 | 6 | KEEP as starting values, verify distributions before use |
| Three-sample confirmation | Reject isolated noise | HIGH-VALUE | 7 | 8 | 2 | 7 | KEEP; downward motion uses latest-reading veto |
| Hysteresis | Avoid threshold chatter | HIGH-VALUE | 7 | 8 | 2 | 6 | KEEP; middle band never establishes clearance |
| Reopen on obstruction | Monitor during descent | ESSENTIAL | 10 | 9 | 5 | 10 | KEEP; fix endpoint race and measure physical reversal |
| Instant full-speed closing | Actuate closure | RISKY for model/demo | 3 | 2 | 5 | 3 | SIMPLIFY behaviour to 1°/20 ms descent; opening remains immediate demand |
| Direction-evidence flag | Suggested forward progression | OPTIONAL / ambiguous | 3 | 1 | 3 | 3 | REMOVE; retain only crossing-occupation-seen flag |
| Giant duplicated build guide | Explain project | OVERENGINEERED documentation | 2 | 1 | 4 | 2 | REMOVE tracked duplicate; generate review handout under ignored dist/ |

The emitter driver stays because removing it would make a signal shorted LOW look clear even before closure. Moving the test to the raised position removes the known blind interval during descent. It remains a single-channel diagnostic, not continuous fault coverage. Removing endpoint switches would make a movement deadline merely a timer; that would weaken the existing implementation. If cam installation is unreliable, correct the mounts before submission rather than claiming feedback that is not dependable.

## Reproduced defects and fixes

Five new tests were first run against the existing production sketch. All five failed, then passed after targeted changes:

| Finding | Old behaviour | Correction / regression |
|---|---|---|
| Long ordinary stop became a latched fault | At 30 s, FAULT/DWELL required `r` even after vacancy | Advisory `w=1`; `long_stop_recovers` returns to IDLE automatically |
| Diagnostic interrupted beam during closing | Every 250 ms, previous clear observation substituted for actual coverage | Test only while raised before each closing attempt; `beam_monitor_closing` |
| Stale open endpoint defeated reopening | Debounced open input could finish reversed travel immediately, then disappear and stop actuator | Require raw + debounced target and 100 ms continuous settling after each command; `reversal_endpoint` |
| Reverse coasting caused false jam | Source reassertion caused immediate STOP even during legitimate reversal | Remove reassertion shortcut; retain contradiction, 500 ms departure and 2.5 s arrival deadlines; `reverse_source_coast` |
| Late Echo could become valid | Short pulse width was accepted even when pulse arrived after the transaction deadline | Capture completion time atomically, reject late/stale completion; `late_echo`, `buffered_echo_stale` |

An additional boundary fix rejects endpoint completion observed **after** the 2.5 s travel deadline, even if a delayed loop otherwise sees a settled switch (`timing_boundaries`). Exactly-at-deadline settled completion remains valid.

Closing also now uses a gradual setpoint ramp so the reopen test can be performed predictably. This is not measured arm-position control. A compile-time constraint prevents the configured ramp from exceeding its travel deadline. Human-readable STATE records replace reliance on a numeric legend during the demo. Sonar fault recovery is explicitly restricted to repaired SONAR faults; a long wait no longer needs any recovery action.

## Architecture red-team

The full requested [24-scenario trace](scenario-traces.md) records state sequence, arm action, recovery and physical limitations. It includes withdrawn obstructions, false-clear pulses, both missing endpoints and separate servo/logic power failures.

## Firmware review coverage

Every production function was inspected; no wholesale FSM rewrite was needed.

| Code area | Review result |
|---|---|
| Constants/enums/globals | Preserve eight explainable states; remove DWELL fault and ambiguous direction globals; constrain angle/ramp configuration |
| Debounced inputs | Startup seeded from pins; 20 ms debounce retained; motion completion now also requires raw level and fresh 100 ms dwell |
| Range::sample/healthy/clear/failed | Run-length filter and hysteresis retained; invalid sample clears confirmation; stale/invalid cannot authorize closure |
| echoISR/readDistance | ISR-shared data volatile, multi-byte snapshots protected; width and transaction completion checked; old completion is not timestamped as a fresh measurement |
| updateBeam/startClearance | No diagnostic while moving; UNKNOWN during test; fresh beam and full clear interval after restoration |
| setBarrier/updateServo | No repeated OPEN writes; closing advances at most one degree per step; no catch-up jump; endpoint feedback still required |
| superviseActuator | Raw/debounced settling and bounded coasting replace fragile immediate reactivation rule |
| updateStateMachine | Safe normal vacancy recovery; C explicitly waits through diagnostic; D loss of clear → O; timeout cannot command CLOSE |
| enterFault/serviceSerial | SONAR requests OPEN and needs acknowledgement after repair; beam-test needs reset; actuator/limit faults latch STOP |
| setIndicators/printDebug | No blocking Serial wait; bounded records with buffer-space checks; state names visible; LEDs use digital IO, not timer-dependent PWM |
| setup/loop | Boot commands OPEN, seeds switch states; sensor update precedes safety checks; actuator STOP dominates subsequent demands |
| Timer arithmetic | Control deadlines use unsigned subtraction; wraparound tested; LED phase changes at wrap affect only presentation |

The old Echo snapshot/timeout boundary could conservatively discard an edge; the revised atomic consume makes the boundary explicit. No unbounded delay, dynamic allocation, Wi-Fi or ISR printing is introduced. `delayMicroseconds(10)` is the bounded sonar trigger, not a vehicle-control delay. Native stubs cannot prove AVR interrupt latency, electrical noise immunity or mechanical timing.

## Hardware sanity result

D2 is INT0 for Echo; D3 is a polled digital beam input. Servo uses Timer1, tone uses Timer2, and millis uses Timer0 on this Uno core. D6/D7 LEDs use digitalWrite, so PWM conflicts are irrelevant. A0/A1/A2 are valid digital inputs. Uno logic and sonar are 5 V. The receiver must be open-collector, LOW illuminated and HIGH blocked with the external pull-up; module polarity is a measured acceptance gate, not an assumption based on wire colour.

The NPN low-side driver is valid for the specified current-limited emitter module. With approximately 4.3 mA base drive through 1 kΩ, check actual emitter current and the exact transistor pinout/rating. A bare IR LED or magnetic buzzer is not a drop-in substitution. Separate servo +5 V from USB +5 V, join grounds, route motor current separately, and fit local bypass/bulk capacitance. 2 A is a supply allowance, not a measured servo current.

Geometric coverage, supply behaviour, switch mounting and physical reversal remain unverified. An actuator fault cannot be honestly described as always producing a physically safe state: the controller stops pulses, but a jammed/downward arm may remain in place. This project is a supervised model with a light breakaway arm, not a safety-rated road barrier.

## Visible differentiators from the delay-based tutorial

1. Stop the vehicle under the arm: it remains open indefinitely and resumes automatically after clearance.
2. Show its rear crossing B: closure starts only after vacancy, not an arbitrary open-duration timer.
3. Obstruct gradual descent: it reopens and checks clearance again.
4. Disconnect a sonar wire: the controller reports a fault and requests opening.
5. Prevent an endpoint from arriving with a soft restraint: movement supervision catches it; do not sustain a stall.

The strongest primary demo is the first three. Endpoint feedback and fault logs support the interview; the emitter circuit can be defended with one pre-close stuck-LOW test, without making it the centrepiece.

## Consistency contract

Task → two occupied/clear observations → specified pin map → eight-state sketch → fresh clearance and motion supervision → software regressions and physical acceptance cases → concise README → uncut demo → evidence-qualified submission text. No exact counting, continuous sensor self-test, latched waiting fault, measured servo angle, certified safety or completed physical testing is claimed.

See [architecture](architecture.md), [geometry](geometry.md), [acceptance tests](testing.md), [build steps](construction.md), [validation](validation.md), and [interview answers](interview.md).

Sources: [Uno specifications](https://store.arduino.cc/products/arduino-uno-rev3), [AVR Servo 1.3.0](https://github.com/arduino-libraries/Servo/blob/1.3.0/src/avr/Servo.cpp), [AVR tone implementation](https://github.com/arduino/ArduinoCore-avr/blob/1.8.6/cores/arduino/Tone.cpp), [HC-SR04 datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf), [open-collector IR receiver](https://www.adafruit.com/product/2168). Sources support component/API facts; scores, geometry and acceptance targets are engineering decisions to validate.
