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
| D8 → 100 Ω | Passive piezo positive; other terminal → GND |
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
 D8 ──100Ω── PIEZO ─── GND      A2 ── hold-open pushbutton ─ GND

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
