# Submission wording

Use the first paragraph **now**, before building. Use the second only after the physical prototype has been built and the claimed behaviour demonstrated. Add the actual public GitHub URL above the selected paragraph; do not submit an uncreated URL.

## Current truthful description (89 words)

This repository contains the complete design and firmware for a two-zone smart parking barrier using an Arduino Uno, an ultrasonic approach sensor and an infrared crossing beam. An enum-based state machine controls opening, passage monitoring, clearance verification and closing. Closure requires both zones to remain clear; renewed obstruction or uncertain sensing during closure requests reopening. Endpoint switches verify arm movement, while sensor confirmation, hysteresis, beam self-testing and latched faults address common failure cases. The firmware has been compile-checked and software-tested. Physical construction, calibration and demonstration results are still pending.

## After physical verification (103 words)

I built a tabletop smart parking barrier using an Arduino Uno, an ultrasonic approach sensor and an infrared beam at the crossing. An enum-based state machine detects an approaching vehicle, opens the arm, monitors passage and closes only after both sensing zones remain clear. If an obstruction reappears during closing, the controller requests reopening. The prototype includes confirmed sensor readings, hysteresis, non-blocking timing, beam self-testing, endpoint switches, traffic LEDs and latched fault handling. The repository includes firmware, wiring, construction notes, software checks and recorded physical tests. The demonstration shows normal passage, a vehicle stopping beneath the arm and recovery from an abandoned approach.
