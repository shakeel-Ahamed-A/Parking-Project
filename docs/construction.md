# Physical construction and bring-up

## Desk model

Use a 65 × 30 cm base with a 14 cm wide, straight, guided lane. Label entry, zone A, barrier, zone B and exit. Use dark road card, thin white lane markings and a white arm with red stripes. Hide wire runs beneath the base and mount the controller on standoffs beside the lane. Use removable sensor brackets until calibration is complete.

Coordinates: x runs along vehicle travel; the arm centre is x = 0. Place the approach sensor plane at x = −12 cm and the beam at x = +1 cm. Lane guides constrain vehicles laterally. Put the sonar perpendicular to the lane at about 2.5 cm above the road; the opposing flat backboard is 24 cm from the sensor face. Centre the 7 cm wide model body about 12 cm from that face, giving an occupied return near 8.5 cm. Keep both transducers unobstructed; do not mount them behind the roadside guide.

```text
TOP VIEW — travel left to right

             x=-12 cm                x=0   x=+1 cm
             sonar A                 arm   IR beam B
                 ↓                     │     ↓ TX
 ENTRY  ═══════════════════════════════│═════┆══════════ EXIT
        [18 cm opaque model body]  →   │     ┆
        ═══════════════════════════════│═════┆══════════
                 ▬ flat backboard            RX
                 24 cm from sonar

 A and B separation = 13 cm; permitted body length ≥18 cm.
 Beam B is just beyond the arm's downstream swept edge.
```

Use an 18 × 7 × 5 cm opaque foam/cardboard model car with uninterrupted side panels at sensor height. Wheels can be bottle caps; avoid cutouts across the beam height. A tiny die-cast car is not automatically suitable: make an opaque body sleeve or adjust and revalidate the geometry. Move at ≤3 cm/s and wait for green before entering the arm area.

Mount the servo pivot approximately 4 cm above the road. Build a 14–16 cm foam-board or drinking-straw arm, preferably under 5 g. Use a breakaway paper/tape coupling. The arm rotates upward in a vertical plane across the road; keep its thickness along x below 1 cm. Place the IR beam at approximately 2.5 cm high and x=+1 cm, downstream of the entire swept envelope. Confirm that the car blocks B whenever any permitted part could contact the descending arm.

Fit an adjustable cam to the arm pivot to press the open switch only at the raised endpoint and the closed switch only at the lowered endpoint. The switches must sense the arm linkage, not a loose servo horn independently of the arm. Use low-force levers; avoid making the servo push hard against a switch or rigid stop. Calibrate the target angles and switch positions together. Defaults 10°/95° are starting values, not guaranteed mechanics.

For scale: a 5 g uniform 15 cm arm has approximately 0.0375 kg·cm static gravity moment about one end, excluding coupling, friction and switch force. This is a sizing estimate; measure movement and current on the actual assembly. Do not substitute a heavy wooden arm just because a servo's advertised stall torque looks large.

## Bring-up sequence

1. Photograph and label parts. Verify supply voltage and transistor pinout before connecting power. Keep the arm disconnected.
2. Build logic wiring and common ground. Check resistance between +5 V and ground with power off. Keep external servo +5 V separate from USB logic +5 V.
3. Upload the final sketch with the servo isolated. Actuator faults are expected until endpoint feedback works; do not bypass the switches to make the final system appear operational.
4. Verify beam polarity using a meter: illuminated LOW, blocked HIGH, emitter off HIGH. A scope/logic analyser will reveal short emitter-test pulses. Avoid prolonged test jumpers; power off before changing wiring.
5. Calibrate A with the backboard and actual vehicle. Record 30 empty and 30 occupied readings from Serial. Empty readings should be stably ≥200 mm and below 350 mm; occupied readings should be ≤160 mm and ≥20 mm. If distributions overlap, fix placement/target geometry before changing thresholds.
6. Connect the unloaded servo to its independent supply. Check startup commands opening. Fit and calibrate endpoint cams with the arm absent or replaced by a paper pointer; reset after expected calibration faults. Confirm target reached within 2.5 s and source switch released within 500 ms.
7. Attach the light arm, verify the envelope, and test slowly with a foam block. Check that no arm position catches wiring, beam mounts or fingers.
8. Perform T01–T25 in testing.md. Capture power droop, closing-to-opening response and representative Serial logs. Do not mark a test passed without observation.
9. Secure the wiring and brackets only after repeatable runs. Repeat the core normal/obstruction/reset tests after moving from breadboard to perfboard.

## Sensor reliability

Only one ultrasonic module is installed, so there is no two-sonar cross-talk. Do not run another ultrasonic demo beside it. A 65 ms ping interval is above the datasheet's recommended 60 ms spacing. Use a rigid backboard perpendicular to the acoustic axis and minimise other reflecting surfaces. A soft, angled or narrow model may miss echoes; add a flat side panel at the measured height. Never translate no echo into free space.

Shade the IR receiver from direct sunlight and use short opaque collars without reducing the vehicle coverage plane. The emitter-off diagnostic is useful against a stuck-clear signal and strong illumination that holds the output low, but it is not a substitute for optical testing under the actual room lighting.
