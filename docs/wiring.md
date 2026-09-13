# Exact wiring — Uno R3 only

All signal names match the final sketch. Switches use `INPUT_PULLUP`: COM to ground, NO to input, pressed endpoint = LOW. Identify the transistor's base/collector/emitter from its specific datasheet; similar-looking packages have different pin orders.

| Uno pin / rail | Connection |
|---|---|
| D4 | HC-SR04 TRIG |
| D2 | HC-SR04 ECHO; add 10 kΩ from D2 to GND |
| 5V logic rail | HC-SR04 VCC |
| GND | HC-SR04 GND |
| D3 | IR receiver signal; add 10 kΩ from D3 to Uno 5V |
| Uno 5V / GND | IR receiver VCC / GND |
| Uno 5V | IR emitter positive (module with internal current limiting) |
| D5 → 1 kΩ | NPN base |
| NPN base → 10 kΩ | GND |
| NPN emitter | GND |
| NPN collector | IR emitter negative; do not also connect emitter negative to GND |
| D9 | Servo signal, usually orange/yellow; 10 kΩ from D9 to GND |
| External regulated +5V, through supply switch | Servo positive, usually red |
| External supply GND | Servo ground, usually brown/black, AND Uno GND |
| D6 → 330 Ω | Red LED anode; cathode → GND |
| D7 → 330 Ω | Green LED anode; cathode → GND |
| D8 → 330 Ω | Passive piezo positive; other terminal → GND |
| A0 | OPEN endpoint switch NO; COM → GND |
| A1 | CLOSED endpoint switch NO; COM → GND |
| A2 | Hold-open button; other contact → GND |
| USB | Uno logic supply and Serial connection |

Add 470–1000 µF across the external servo +5V/GND close to its connector, observing electrolytic polarity. Add 100 nF at HC-SR04 and IR receiver supply pins. Route servo return directly to the external supply; join logic ground at a common point. Avoid carrying motor current through breadboard rails or the Uno.

```text
 COMPUTER USB ───────────── UNO R3 (5 V logic)
                             D4 ───────────── HC-SR04 TRIG
                             D2 ──────+────── HC-SR04 ECHO
                                      10k
                                       │
                                      GND
                             5V ───────────── HC-SR04 VCC
                            GND ───────────── HC-SR04 GND

              UNO 5V ──10k───+── D3 ───────── IR receiver OUT
              UNO 5V ──────────────────────── IR receiver VCC
                 GND ──────────────────────── IR receiver GND

              UNO 5V ───────── IR emitter + (current-limited module)
                               IR emitter − ─── collector Q1
                             D5 ──1k── base Q1
                                       │       emitter Q1 ── GND
                                      10k
                                       │
                                      GND

 D6 ──330Ω──>| RED ── GND       A0 ── OPEN switch NO; COM ── GND
 D7 ──330Ω──>| GREEN ─ GND      A1 ── CLOSED switch NO; COM ─ GND
 D8 ──330Ω── PIEZO ─── GND      A2 ── hold-open pushbutton ─ GND

                             D9 ──────+────── SERVO signal
                                      10k
                                       │
                                      GND
 EXTERNAL 5 V / 2 A ── switch ──────+──────── SERVO +
                                    │ +
                                 470–1000µF
                                    │ −
 EXTERNAL GND ───────────────────────+──────── SERVO GND
        │
        └──────────────────────────────────── UNO GND
```

## Power and electrical cautions

- Do not power the servo from an Uno GPIO, its onboard regulator or the computer USB path. A 2 A supply is a design allowance, not a measured MG90S peak specification.
- Common ground is mandatory. Do not connect external servo +5 V to Uno +5 V while USB powers the Uno; only join grounds.
- The external supply's regulated 5 V is for the servo; do not feed it into Uno VIN, which is a regulator input requiring headroom.
- The selected IR emitter is a ready-made current-limited module. A bare IR LED requires a calculated series resistor and is not a wire-for-wire replacement.
- A passive piezo is appropriate for this GPIO circuit. A high-current active/magnetic buzzer requires a driver stage.
- Uno R3 uses 5 V logic, so HC-SR04 Echo connects directly. **If ported to an ESP32, its GPIO is 3.3 V only:** use a 10 kΩ top / 15 kΩ bottom Echo divider (5 V → 3.0 V), pull receiver output to 3.3 V, and redesign the complete pin map/software. Do not run this Uno wiring on ESP32.
- Never connect the two endpoint inputs permanently to ground. That simulates endpoints and defeats motion verification.

## Mandatory beam truth-table check

With the arm removed, measure D3 relative to GND:

| Condition | Required input |
|---|---|
| Emitter powered and correctly aligned | LOW |
| Opaque model blocks beam | HIGH |
| Emitter disabled by D5 LOW | HIGH |
| Receiver signal disconnected from D3 | HIGH due to pull-up |

Do not reverse a constant merely to suppress a fault. An inverted receiver needs a deliberately redesigned diagnostic circuit and tests. The current final firmware assumes this exact table.

The 330 Ω piezo resistor limits the nominal 5 V edge current to about 15 mA before output resistance. The emitter diagnostic now runs only while raised, before each closing attempt: 10 ms off, 10 ms settling, restored clear confirmation, then a new 1.2 s vacancy interval. It never intentionally blanks the beam during descent.


## Receiver variants: identify the actual part before wiring

The table and drawing above describe a powered receiver module. All alternatives must meet the same truth table; firmware polarity is not configurable.

| Actual receiver | Interface to this design | Required checks |
|---|---|---|
| Recommended 5 V through-beam digital module with open-collector output | VCC → Uno 5V, GND → common GND, OUT → D3; external 10 kΩ D3 → Uno 5V | Illuminated LOW; blocked/off/disconnected HIGH. Verify both optical transitions settle within the 10 ms diagnostic phase under demo lighting |
| Bare phototransistor | Collector → D3, emitter → GND, 10 kΩ D3 → Uno 5V; **no separate VCC terminal** | Identify actual C/E pins. Illumination must pull below the Uno LOW threshold and darkness release above HIGH. Ambient light/saturation can defeat this; not the preferred substitute without measured margins |
| Comparator module | VCC/GND as its datasheet permits; DO → D3 only if output is 5 V compatible and meets the required truth table; use external pull-up for open-collector DO | AO is not DO. Verify threshold, sunlight immunity, propagation/settling, and emitter-off HIGH. Many reflective obstacle modules are not through-beam receivers |
| Push-pull or demodulating digital module | Not an assumed drop-in replacement | Verify voltage, polarity and response to this continuous emitter. Some receivers require a modulated carrier; inverted outputs need an interface redesign and renewed tests |

A bare IR LED needs a series resistor chosen from `R=(5 V−Vf−VCEsat)/I_LED`, with current and resistor power checked against its datasheet. Do not apply the module circuit without that resistor. Use a current-limited emitter drawing no more than the selected transistor can reliably switch with approximately 4.3 mA base drive; verify collector voltage when on. Transistor pin order is part-specific.

The **330 Ω** series resistor is suitable for a small passive piezo: nominal edge current is bounded near 5/330 = 15.2 mA before GPIO output resistance, below the Uno's published 20 mA per-pin specification. It is not a current budget for an active/magnetic buzzer. LED currents with 330 Ω are lower because of their forward voltage. D2 uses INT0, Servo uses Timer1, `tone()` Timer2 and `millis()` Timer0; LEDs use digital outputs, so PWM timer sharing is irrelevant. See the primary component/source links in [the review](review.md).
