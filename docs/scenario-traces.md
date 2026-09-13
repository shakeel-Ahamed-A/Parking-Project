# Final 24-scenario trace

I=IDLE, O=OPENING, H=OPEN, P=PASSING, C=CLEARANCE, D=CLOSING, F=FAULT. C includes the raised diagnostic, restored beam and fresh 1.2 s continuous clearance. “Conditional” means the command policy is conservative under the documented geometry, functioning sensors and actuator; physical safety remains unverified. STOP means pulses detached, not motor power isolated.

| # / scenario | State sequence | Arm action | Fault / recovery | Assessment |
|---|---|---|---|---|
| 1 Normal approach/crossing | I → O → H/P → C → D → I | Open, hold, close on vacancy | Automatic | Conditional |
| 2 Approach then retreat | I → O → H → C → D → I | Hold until both zones empty | No crossing required for abandoned-session recovery | Conditional |
| 3 Wait before crossing | I → O → H; later P → C → D → I | Stay raised | Automatic after vacancy | Conditional |
| 4 Stop under arm | H → P, or H while only A occupied | Stay raised | No forced closing timer | Conditional on body coverage |
| 5 Wait >30 s | H/P stays; then C → D → I | Stay raised; w=1 | Advisory only; no r needed | Conditional |
| 6 Very slow crossing | H → P → C → D → I | Hold for any duration | Automatic | Conditional |
| 7 Fast crossing | I → O, potentially still opening at arrival | Opening may not finish in time | No software recovery can undo collision | Unsafe outside ≤3 cm/s envelope; physically validate permitted speed |
| 8 Tailgating | P holds; C → H/P; D → O if later | Hold/reopen | Fresh C after follower | Conditional; no individual counting |
| 9 Obstruction during close | D → O → H/P | Cancel ramp; immediate OPEN target | Fresh C after vacancy | Conditional on measured reversal margin |
| 10 Obstruction disappears during reopening | D → O remains until settled, then H → C | Finish opening first | Automatic | Conditional; regression covers withdrawn hazard |
| 11 Beam falsely clears momentarily | P stays, or C cannot complete | Hold | <100 ms clear rejected; longer clear still needs fresh C | Conditional for brief pulse; persistent false clear after diagnostic is not covered |
| 12 Sonar stale | D → F or O then F; other active states → F | OPEN requested | Repaired/clear/open + r → H → C | Conservative command; cannot repair missing torque |
| 13 Late Echo | Same as invalid sample; repeated → F | Veto descent/reopen | Reject transaction, never refresh with late Echo | Conditional |
| 14 Open endpoint absent | BOOT/O → F | STOP at travel deadline or source-release failure | Inspect/repair/reset | No safe physical position guaranteed |
| 15 Closed endpoint absent | D → F | STOP at travel deadline | Inspect/repair/reset | No safe physical position guaranteed |
| 16 Both endpoints active | Any → F (LIMITS) | STOP after contradictory debounced feedback | Inspect/repair/reset | No safe physical position guaranteed |
| 17 Reset closed | BOOT → H/P → C → D → I if vacant | First firmware target OPEN | Fresh sensing/feedback required | Conditional; real boot pulses unverified |
| 18 Reset open | BOOT → H/P → C → D → I if vacant | Reconfirm open, then clearance | Automatic | Conditional; reset is not mechanical fail-open |
| 19 Reset beam blocked | BOOT → P after raised feedback | OPEN, remain raised | Vacancy resumes C | Conditional on working actuator |
| 20 Servo supply failure | O/D → F; at-rest loss may remain undetected until feedback changes or next movement | Missing torque; eventual STOP if travel fails | Restore supply/inspect/reset | Physical safety not guaranteed; switches do not monitor voltage |
| 21 Logic supply disturbance | Execution may stop/reset → BOOT | Undefined during power loss; firmware then requests OPEN | Power integrity must be corrected | Physical safety not guaranteed; requires supply test |
| 22 Repeated approach noise | I stays for isolated near; confirmed near → O; C can cancel | Conservative holds; latest non-clear vetoes descent | Stable vacancy recovers | Conditional; valid false-clear background requires geometry fix |
| 23 Hold-open button | I/D → O; H/P holds; release → C if vacant | OPEN; never forced close | Cannot override actuator STOP latch | Conditional |
| 24 Beam disconnected | I → O → P, or D → O → P | OPEN/hold because external pull-up reads blocked | Restore beam; fresh C; long wait only advisory | Conditional for disconnected receiver wire; short LOW requires pre-close diagnostic |

Two additional useful paths: B-first triggers I → O → P conservatively. If A clears before B, the body-length/coverage constraint ensures a body still in the sweep is detected by at least one zone. If this physical assumption fails, a correct FSM cannot prevent premature closure. See [geometry](geometry.md) and [regression evidence](validation.md).
