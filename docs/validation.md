# Validation report — 2026-09-12

## Actually executed

| Check | Result |
|---|---|
| Arduino Uno R3 compile | PASS, FQBN `arduino:avr:uno`, AVR Boards 1.8.6, Servo 1.3.0 |
| Final flash usage | 7,576 / 32,256 bytes (23%) |
| Final static RAM usage | 343 / 2,048 bytes (16%); 1,705 bytes remain for stack/runtime |
| Native test compilation | PASS, C++11, `-Wall -Wextra -Werror`, Zig 0.16.0 C++ frontend on Windows |
| Firmware host scenarios | 30 / 30 PASS; actual `.ino` included with hardware interface stubs |
| Deliverable consistency | PASS: local Markdown links, all 17 guide sections, exact embedded firmware and source ZIP exclusions |
| Physical prototype | NOT BUILT / NOT TESTED in this session |
| Hardware upload | NOT PERFORMED |
| GitHub Actions execution | Pending repository publication; local results are not a remote CI result |

An initial full AVR build emitted unused-parameter warnings in Arduino AVR core `new.cpp`. The final sketch compiled successfully; those were dependency warnings, not errors in this project. Static RAM usage does not establish worst-case stack depth.

## Passing software scenarios

`normal`, `stop_approach`, `slow`, `under_arm`, `noisy_approach`, `exit_first`, `reverse`, `abandon`, `tailgate`, `sonar_disconnect`, `beam_disconnect`, `beam_short`, `rapid`, `reopen`, `single_invalid_closing`, `jam`, `limits`, `manual`, `dwell`, `hysteresis`, `echo`, `echo_timeout`, `rollover`, `random`, `reset_open`, `reset_mid`, `reset_blocked`, `arrival_timeout`, `lost_endpoint`, `stale`.

The fixed-seed random scenario performs 20,000 scheduler iterations with changing zone inputs and checks closing/stopped-actuator invariants. Other scenarios explicitly test a continuous 30-second occupied hold, manual hold beyond the deadline and timer wraparound.

## Defect found and corrected

The first normal startup test exposed an endpoint initialization defect. Starting both software switch values as inactive could make an initially pressed closed switch appear to reactivate after departure, falsely tripping the actuator supervisor. Setup now seeds the debouncers from actual switch inputs before the first movement command. The normal boot and explicit open/mid/occupied reset cases pass with that correction.

## Important limits

- Host scenarios substitute GPIO, clock and servo interfaces. They do not emulate AVR ISR scheduling, motor dynamics or sensor optics/acoustics.
- Most sonar scenarios inject measurements into the production filter; separate Echo tests cover pulse conversion, timeout and `micros()` wraparound.
- Simulated travel is 600 ms; this is not a measured servo result.
- No measured obstruction reaction time, supply transient, target detection margin or real power-loss behaviour is available yet.
- Two-zone occupancy does not guarantee exact direction/counting for arbitrary reversals or multiple vehicles.
- The beam diagnostic detects the specified static stuck-clear failure; it is not a redundant safety circuit.

Re-run software checks after changes, and complete [the physical matrix](testing.md) before changing the README to claim a demonstrated hardware build.
