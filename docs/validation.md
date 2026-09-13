# Validation report — 2026-09-13

## Executed locally

| Check | Result |
|---|---|
| Uno compile | PASS, arduino:avr:uno, AVR Boards 1.8.6, Servo 1.3.0, warnings enabled |
| Flash | 8,408 / 32,256 bytes (26%) |
| Static RAM | 354 / 2,048 bytes (17%); 1,694 bytes available for stack/runtime |
| Native compile | PASS, Zig 0.16.0 C++ frontend, C++11, -Wall -Wextra -Werror |
| Software scenarios | 46 / 46 PASS; 0 failed; actual production .ino included |
| Repository consistency | PASS: local links, fences, Python syntax, pin/state tables, test counts, source digest, handout and archive copies |
| Physical acceptance | 14 / 14 NOT RUN |
| Hardware upload | NOT PERFORMED |
| GitHub Actions | Workflow inspected; remote execution pending publication |

Normalized-LF UTF-8 firmware SHA-256: `e803a2c468adbdc80a9e542847e92295b0da941317ca9f85b25c9477e5e4f2e9`.

[Machine-captured results](software-results.txt) list all 46 scenarios. The stable baseline had 30; 16 scenarios were added across this hardening continuation. Timing assertions remain grouped as one scenario rather than inflating the count. Static RAM figures do not establish worst-case stack use. Initial full builds reported unused-parameter warnings in Arduino core new.cpp; no project warning prevented compilation.

## What the important tests establish

| Tests | Property exercised |
|---|---|
| long_stop_recovers | A stop beyond 30 s is advisory and returns to IDLE on vacancy without acknowledgement |
| beam_monitor_closing / preclose_fresh_interval | No emitter blanking during descent; restored sensing precedes a completely new clearance interval |
| reversal_endpoint / reverse_source_coast | No completion on stale endpoint feedback; fresh 100 ms settling; realistic source reassertion does not immediately fault and can finish opening |
| late_echo / buffered_echo_stale | Reject completion after measurement deadline and reject old buffered measurements |
| raw_endpoint_veto | Debounced open with raw endpoint released cannot authorize closing |
| single_near_closing / single_invalid_closing / sonar_fault_closing | Latest near, invalid or persistent failed sonar vetoes descent despite old filtered values |
| closing_speed | Production closing setpoint advances by scheduled steps and stops ramping after reversal |
| obstruction_withdrawn_reopen | Removing a hazard while reopening does not skip fresh opening completion |
| beam_false_clear_pulse | A short false-clear pulse does not authorize closing |
| closed_endpoint_missing / arrival_timeout | Missing closed/open feedback reaches actuator STOP fault |
| repeated_cycles | Twenty same-process normal cycles return to IDLE without stale session flags |
| timing_boundaries | Just-before/exact/after checks for debounce, freshness, beam clear, clearance, source release, travel, ramp, Echo and advisory; includes wraparound |
| random / rollover / reset_* | Fixed-seed varied IO invariants, timer wrap and alternate initial positions |

The original five defects were reproduced against the prior sketch before correction. Boundary review additionally found that a delayed loop could accept a settled endpoint after the 2.5 s travel deadline; the supervisor now rejects observations after that deadline while allowing timely settled completion exactly at it.

## Reproduction and limits

Use the commands in [README](../README.md). `tools/verify_project.py` checks source/document links, pin/state consistency, recorded test names/counts, pending physical results and Python syntax. `tools/package_project.py` generates the ignored review handout and source ZIP; the verifier also checks those generated copies when present.

Native tests substitute GPIO, clocks and Servo. Most sonar inputs enter at the filter boundary; dedicated Echo tests exercise ISR timestamps. Synthetic endpoint travel is 600 ms, not a measured servo value. These tests do not emulate AVR interrupt latency, servo inertia, optical coverage, supply sag or bootloader behaviour. No physical response, supply margin or real hardware pass is claimed. Complete [acceptance](testing.md) before changing the hardware status.
