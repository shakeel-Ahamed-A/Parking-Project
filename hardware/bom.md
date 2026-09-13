# Bill of materials

No exact local prices are claimed. Reuse the Uno, sonar, LEDs, jumpers and piezo from a student kit; the useful additions are the beam pair, two switches and a stable servo supply. Obtain local quotes before buying.

| Component | Qty | Purpose | Recommended model/specification | Alternatives / constraints |
|---|---:|---|---|---|
| Controller | 1 | FSM and diagnostics | Arduino Uno R3, ATmega328P, 5 V | Compatible Uno R3 clone; classic 5 V Nano with board-selection adjustment |
| Ultrasonic module | 1 | Approach presence and empty-lane echo | HC-SR04, 5 V | Same pin-compatible module after distance validation |
| IR emitter + receiver pair | 1 pair | Crossing and trailing-edge clearance | 5 V IR through-beam pair; receiver open-collector, LOW when illuminated, HIGH when blocked; e.g. verify Adafruit 2168 | Matching through-beam module with this truth table; reflective IR module is not equivalent |
| Positional servo | 1 | Raise lightweight arm | MG90S, 5 V, standard positional | Kit SG90 for foam arm; do not use continuous-rotation servo |
| Lever microswitch | 2 | Actual open/closed endpoints | SPDT lever switch, use COM and NO | Low-force lever switches with adjustable mounts |
| NPN transistor | 1 | Pre-close check: switch IR emitter ground | PN2222A/2N2222A; verify exact package pinout | BC547 for emitter current within its rating; pinout differs |
| Base resistor | 1 | Limit NPN base current | 1 kΩ, 1/4 W | 820 Ω–1.5 kΩ after current check |
| Base pull-down | 1 | Emitter off while controller resets | 10 kΩ | 10–47 kΩ |
| Beam pull-up | 1 | Defined blocked/unplugged HIGH | 10 kΩ | 4.7–10 kΩ |
| Echo pull-down | 1 | Defined idle Echo if unplugged | 10 kΩ | 10–47 kΩ |
| Servo signal pull-down | 1 | Defined LOW while controller resets | 10 kΩ | 10–47 kΩ |
| LEDs | 2 | Red stop/fault, green open | Red and green 3/5 mm | Kit LEDs |
| LED resistors | 2 | LED current limiting | 330 Ω, 1/4 W | 220–470 Ω, confirm LED current |
| Passive piezo | 1 | Audible closing/fault status | Low-current bare piezo transducer | Omit physically if unavailable; no firmware change needed |
| Piezo resistor | 1 | Limit transient GPIO current | 330 Ω | 330–470 Ω |
| Momentary pushbutton | 1 | Hold-open request | Normally open tactile switch | Any dry-contact momentary NO button |
| Servo power supply | 1 | Handle motor current separately | Regulated 5 V, 2 A supply from reputable source | Current-limited bench supply set to 5 V; verify actual servo peak/stall demand |
| Servo isolator | 1 | Remove actuator power for setup/jam | Inline switch rated for supply current | Bench supply output switch; not a certified E-stop |
| Bulk capacitor | 1 | Reduce servo supply transients | 470–1000 µF electrolytic, ≥10 V | Size after supply transient measurement |
| Decoupling capacitor | 2–3 | Local logic supply bypass | 100 nF ceramic | At sonar/receiver and near emitter supply |
| USB cable | 1 | Logic power, programming, Serial | Uno-compatible USB data cable | Verify data-capable cable |
| Breadboard/perfboard, jumpers | 1 set | Interconnect | Breadboard for bring-up, perfboard for final | Screw terminals for servo power |
| Base, foam arm, backboard, fasteners | 1 set | Physical prototype | 65 × 30 cm base, lightweight breakaway arm | Foam board/cardboard/plywood base |

## Practical fallback

Use the same final firmware with an Uno clone, SG90, and matching-polarity inexpensive through-beam pair. Keep the arm light and recalibrate angles. Piezo may be omitted; red/green status remains. Two NO pushbuttons may substitute for endpoint switches **only during supervised bench logic testing**, never as a finished automated build.

If only HC-SR04/reflective IR kit sensors are available, assemble the road, sonar and servo for bring-up and purchase the missing beam pair and endpoint switches before claiming this final design is implemented. Do not silently bypass the beam, spoof a limit input, or swap an ultrasonic sensor onto the beam pin. There is one maintained firmware, with no weaker automatic fallback mode.
