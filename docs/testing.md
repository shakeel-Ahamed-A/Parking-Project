# Verification plan and evidence

**Physical status: NOT RUN.** The software report is in [validation.md](validation.md). Simulation does not measure sensor coverage, power integrity, pulse timing on real silicon, mechanical impact or real servo feedback.

Record each physical result in [test-results.csv](test-results.csv). Use PASS / FAIL only after observing the pass criteria; attach log/video names and measured values. Default state sequences below start after BOOT unless reset is the scenario. OPENING/OPEN may be brief depending on sensor timing.

| ID | Input/scenario | Expected state sequence | Barrier behaviour | Pass criteria |
|---|---|---|---|---|
| T01 | Normal vehicle arrival | IDLE → OPENING → OPEN | Opens; green only at endpoint | Three occupied A readings trigger; open switch confirmed within 2.5 s |
| T02 | Vehicle approaches and stops before arm | IDLE → OPENING → OPEN | Remains open | No closing while A occupied; after 30 s FAULT/open |
| T03 | Vehicle crosses normally | OPEN → PASSING → CLEARANCE → CLOSING → IDLE | Closes only after trailing edge clears B and A clears | Fresh 1.2 s clear interval; closed switch confirmed; `p=1` for valid geometry |
| T04 | Very slow vehicle | OPEN → PASSING → CLEARANCE → CLOSING → IDLE | Stays open throughout crossing | Pause 5 s at A/B and 5 s at B; no premature close |
| T05 | Vehicle stops underneath arm | OPEN → PASSING; after deadline FAULT | Holds open | B stays blocked; no downward command even after 30 s |
| T06 | Isolated false/noisy A samples | IDLE | Stays closed | One/two occupied readings followed by clear do not trigger; do not count continuous noise as isolated |
| T07 | B triggers before A / false B pulse | IDLE → OPENING → PASSING/OPEN → CLEARANCE → CLOSING → IDLE | Conservative open, then revalidate clear | No deadlock; no fabricated forward evidence from B-only activation |
| T08 | Vehicle reverses before passing | OPEN → PASSING → CLEARANCE → CLOSING → IDLE | Stays open until both zones empty | Clear B first while A occupied does not close; completed retreat recovers |
| T09 | Second vehicle follows first | PASSING → CLEARANCE → PASSING/OPEN, then normal close | Holds open or reopens for second car | Gap <1.2 s cannot finish clearance; no exact count claim |
| T10 | Disconnect sonar Echo/power | Any → FAULT | Requests open | Latest invalid reading vetoes closing; 3 bad samples or ≥300 ms stale latches SONAR |
| T11 | Controller reset with arm open / partially down / closed | BOOT → OPEN/PASSING → CLEARANCE → CLOSING → IDLE | First command opens | Test all three initial positions; occupied lane never causes automatic close |
| T12 | Rapid repeated beam triggers | OPEN/PASSING, timer repeatedly cancelled | Holds open | 10 ms blocked/clear pulses do not produce a completed-clear interval |
| T13 | New object during closing | CLOSING → OPENING → PASSING/OPEN | Reverses command to open | Measure beam-to-command and mechanical reverse latency; foam block never contacted within permitted geometry/speed |
| T14 | Unplug IR receiver / emitter wire | IDLE → OPENING → PASSING → FAULT | Holds open | D3 pulled HIGH; DWELL after 30 s, not automatic closure |
| T15 | Test D3 shorted LOW | Any → FAULT | Requests open | Emitter-off diagnostic catches fault by next test, nominally <260 ms plus measured loop latency |
| T16 | Jam opening/closing using soft restraint, current-limited supply | OPENING/CLOSING → FAULT | Stops command pulses, does not retry | Source-release timeout 500 ms or target timeout 2.5 s; isolate servo immediately, do not sustain stall |
| T17 | Both endpoint switches active | Any → FAULT | Stops pulses | Contradiction latched after debounce; `r` cannot reattach |
| T18 | Press manual during IDLE/closing; hold 35 s | OPENING → OPEN/PASSING | Holds open with no dwell fault while pressed | Release permits only fresh clearance; cannot override actuator fault |
| T19 | Approach then retreat without reaching B | OPEN → CLEARANCE → CLOSING → IDLE | Closes after verified vacancy | Recovers with no crossing recorded |
| T20 | Ranging inside hysteresis band | OPEN/PASSING | Holds previous occupied state | 160–200 mm does not create confirmed clearance |
| T21 | Supply sag / restart | Fault or BOOT recovery, depending on supply | No intentional startup-close command | Measure 5 V rails during motion; no resets with chosen supply under normal load |
| T22 | Broken endpoint wire / lost endpoint after arrival | Moving/IDLE/OPEN → FAULT | Stops pulses | Target missing → 2.5 s deadline; reported endpoint lost at rest → fault after debounce |
| T23 | Minimum vehicle size, maximum permitted speed, lane extremes | Normal cycle / safe open fault if sensing fails | Never closes on permitted model | Test 18 cm opaque body at ≤3 cm/s throughout guided lateral positions; any contact is FAIL |
| T24 | Recoverable fault repaired | FAULT → OPEN → CLEARANCE → CLOSING → IDLE | Remains open until explicit acknowledgement | SONAR/DWELL: clear lane + open endpoint + `r`; BEAM_TEST/jam requires inspection and reset |
| T25 | Logic power/servo power removed separately | No guarantee while unpowered; BOOT on restore | Observe actual passive mechanics | Document sag/fall, servo loss-of-pulse response and initial pulse behaviour; do not claim mechanical fail-open |

## Repeatability and measurement

- Run T01/T03 at least 20 times, logging actual completion count and any failures.
- Repeat T05/T08/T09/T13/T23 at least five times with the permitted model.
- For T13 use a logic analyser on D3, D5 and D9 if available; separately film arm reversal at high frame rate. Serial is sampled at 4 Hz and cannot prove millisecond response.
- Measure worst-case loop service delay with a spare debug pin or temporary scope instrumentation if available. The 6 ms beam-test blindness figure excludes loop and physical actuator delay.
- Add independent physical setup/reset evidence; the host tests use simulated endpoint motion and do not validate real servo timing.
- Exercise `millis()` and `micros()` rollover in the included software tests rather than waiting 49 days. Do not modify production timing to fabricate bench results.

## Host test method

`tests/firmware_tests.cpp` includes the actual `.ino`, substitutes GPIO/Servo/clock interfaces and feeds sensor scenarios. Each named scenario starts in a fresh process. Normal motion is simulated as 600 ms travel with 50 ms departure; these are **test fixture values**, not measured hardware specifications. Most tests inject sonar samples at the filtering boundary; dedicated cases cover Echo interrupt conversion, timeout and wraparound. Every simulated tick asserts that CLOSING implies all clear and that a stopped actuator never reattaches.

The tests cover control behaviour; they do not emulate AVR interrupts, electrical faults exhaustively, ultrasonic propagation, radiation, a bootloader, brownout or servo inertia. CI also compiles against the real AVR core and Servo library.
