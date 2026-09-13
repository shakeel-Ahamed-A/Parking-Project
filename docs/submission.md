# Form submission

Place the **actual accessible repository URL** above the paragraph. Publication and physical evidence are pending; do not substitute an assumed URL.

## Current evidence-qualified version

This project implements a two-zone smart parking barrier using an Arduino Uno, ultrasonic approach sensing and an infrared crossing beam. An enum-based state machine opens the arm, monitors occupation and closes only after a fresh clearance interval. New obstruction or uncertain sensing during descent requests reopening. Endpoint switches supervise movement, while filtered readings, hysteresis, a raised-position beam diagnostic and bounded fault handling improve reliability. The repository contains the firmware, wiring, build procedure, software regressions and physical acceptance plan. The Uno build and software tests pass; physical assembly, calibration and demonstration evidence are still pending.

## Use only after the physical claims are evidenced

I built a tabletop smart parking barrier using an Arduino Uno, an ultrasonic approach sensor and an infrared crossing beam. A non-blocking state machine opens the arm, holds it while a vehicle occupies the crossing and closes after verified clearance. Obstruction during gradual descent requests reopening. Endpoint switches confirm movement, and filtered sensing, hysteresis, a raised-position beam diagnostic and bounded faults address common failures. The repository includes firmware, wiring, software checks and recorded physical acceptance results. The demonstration shows normal passage, a vehicle intentionally stopped beneath the arm and successful reopening when the crossing is obstructed during closing.
