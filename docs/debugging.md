# Troubleshooting

Use Serial at 115200; state names print on transitions. Fault codes: 1 SONAR, 2 BEAM_TEST, 3 LIMITS, 4 ACTUATOR. `w=1` is only a long-open advisory. Keep the arm removed for electrical debugging.

| Symptom | Likely cause | Fix |
|---|---|---|
| Distance zero / v=0 | TRIG/ECHO swap, no backboard, missing ground/power | D4 TRIG, D2 ECHO; flat target ~24 cm; inspect pulses |
| Unstable or false-clear distance | Angled/small body, reflection from wall around body, vibration | Use flat opaque side panel; map actual A coverage; fix placement before thresholds |
| Jitter / reset on movement | Supply sag, poor common ground, excessive load | Independent 5 V supply, local bulk capacitor, separate motor return; inspect rail transients |
| Opens but will not close | B blocked/inverted, A not confirmed clear, manual held or fault | Inspect a/b/v/f; verify receiver truth table and backboard |
| Closes too early | Body or protrusion invisible to sensors, wrong B/sweep placement | Stop; repeat geometric coverage test. A longer delay cannot fix invisible occupancy |
| Sonar interference | Another ultrasonic module nearby | Disable other module; retain 65 ms spacing |
| Detection range too short | Face obstructed, poor side panel or near-field target | Clear sensor face; ≥2 cm range; flat panel at tested height |
| Servo direction wrong | Horn/angle convention | Remove arm; calibrate OPEN_ANGLE/CLOSED_ANGLE and physical endpoint labels |
| BEAM_TEST fault before close | Signal stuck LOW, wrong polarity, permanently powered emitter or slow module response | Verify NPN pinout and switched negative connection; measure emitter-off response; do not bypass test |
| Actuator fault on reversing | Endpoint mounting/bounce or reversal settling outside measured limits | Inspect raw switch/mechanics, source release and 100 ms dwell; verify T08 rather than increasing deadlines blindly |
| LIMITS fault | Both switches wired/pressed active | Check COM/NO and cam spacing; inspect then reset |
| ACTUATOR fault | Jam, disconnected servo, missing target or reached switch lost | Isolate servo supply, inspect and calibrate, then reset |
| Wait exceeds 30 s | Vehicle waiting or beam unavailable | w=1 is advisory; no r needed. Restore vacancy and normal operation resumes |
| r ignored | Non-SONAR fault or lane/open endpoint not ready | r only acknowledges repaired SONAR with clear lane, released manual and settled open endpoint |

Do not use fingers to stop the arm. No-pulse is not no-power. An ordinary magnetic buzzer is not suitable for the direct piezo circuit.
