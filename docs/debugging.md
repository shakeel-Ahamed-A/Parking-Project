# Debugging guide

Start with the arm disconnected, a current-limited supply and Serial at 115200 baud. Field meanings and numeric state/fault codes are in the README. `mm` holds the last valid distance; always inspect `v` too.

| Symptom | Likely cause | Fix |
|---|---|---|
| Distance always 0 / `v=0` | TRIG/ECHO swapped, missing 5 V/GND, no echo target, disconnected signal | Verify D4 TRIG / D2 ECHO and common GND; place flat target 24 cm away; inspect pulses if available |
| Distance very unstable | Soft/sloped/small target, vibration, power noise, side-wall echoes | Flat opaque side panel, rigid perpendicular backboard, local bypass capacitor, separate motor return |
| Servo jitter | Weak supply, loose ground, rigid stops or switch force | Use separate regulated supply and bulk capacitor; shorten motor wiring; adjust endpoints away from mechanical stops |
| Uno/ESP32 resets when servo moves | Supply sag or motor return current in logic ground | Separate motor supply, common star ground, measure rail transient; ESP32 is not the supplied build |
| Opens but never closes | B blocked/inverted, A not confirmed clear, manual held, latched fault | Inspect `a,b,v,f`; check beam truth table, backboard and thresholds; repair fault then acknowledge only as documented |
| Closes too early | Beam misses body or sits inside/before full arm envelope; undersized/gapped model; wrong sensor placement | Stop demonstration; restore downstream beam, guided opaque body ≥18 cm and validated thresholds; a longer delay cannot fix invisible occupancy |
| Ultrasonic sensors trigger each other | Additional sonar active nearby | Final design has one sonar; disable nearby units, retain 65 ms cadence; a two-sonar redesign needs sequential triggers |
| Detection range too short | Near-field limit, narrow/sloped body, sensor occluded by guide | Keep face clear and ≥2 cm from objects; use flat side panel at sensor height; calibrate physical spacing |
| Servo moves wrong direction | Horn orientation / angle convention | Remove arm, swap and tune OPEN_ANGLE/CLOSED_ANGLE as needed; keep physical OPEN switch on A0 and CLOSED on A1 |
| Immediate BEAM_TEST fault | Wrong receiver polarity, output short LOW, emitter wired permanently on, ambient IR | Check D5 transistor switching and required LOW/HIGH table; shade sensor; do not disable diagnostic |
| ACTUATOR fault | Endpoint not reached, broken switch wire, disconnected servo, binding linkage | Isolate servo power, inspect mechanics and feedback; recalibrate then reset; do not increase timeout to hide a jam |
| LIMITS fault | Both switches pressed/wired LOW simultaneously | Inspect COM/NO wiring and cam geometry; reset only after correction |
| Fault does not clear with `r` | Lane not clear, endpoint missing, manual pressed or nonrecoverable fault | SONAR/DWELL need clear/open conditions; beam self-test and actuator faults require repair and controller reset |

Never test a jam with fingers. Use a soft restraint only long enough to establish a result, watch current and remove actuator power. Stopping pulses is not an electrical power cut.
