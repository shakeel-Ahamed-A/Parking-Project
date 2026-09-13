# Interview defence

1. **Why not one ultrasonic sensor?**

   It detects approach but does not reliably prove that the rear has crossed the arm. The downstream beam supplies that observation; both zones must become clear.

2. **Why an IR through-beam?**

   The body physically interrupts a defined line instead of relying on reflection strength. Its height, polarity and ambient-light behaviour still need testing. A raised pre-close emitter test catches a stuck-LOW input, not every later false clear.

3. **Why Uno rather than ESP32?**

   This controller needs deterministic digital IO and simple timers, not networking. Uno has enough memory and pins and directly supports the 5 V sonar. The pinned Servo/core timer assignments are compatible.

4. **Why not delay()?**

   A long delay would prevent timely sensing and reopening. The FSM uses elapsed-time checks while continuing sensor updates; only the 10 microsecond sonar trigger is synchronous.

5. **How do you know the arm actually moved?**

   The target switch must be raw and debounced active, the other released, and the target settled for 100 ms after the command. A timeout stops pulses. This proves an endpoint, not continuous angle or an intact arm beyond the linkage.

6. **Why endpoint switches?**

   Servo.write sets a target without reporting arrival. The switches expose missing travel and jams, provided their cams are mounted on the actual linkage and calibrated.

7. **What if a vehicle remains underneath?**

   Observed occupancy keeps the arm open. The operating envelope requires a continuous opaque model long enough that it cannot disappear between A and B while intersecting the sweep.

8. **What happens after a very long stop?**

   After 30 seconds the Serial wait flag becomes an advisory. There is no latched waiting fault; normal clearance resumes automatically when the vehicle leaves.

9. **What if sensing becomes uncertain?**

   Uncertain sonar or an unavailable beam cannot authorize closure. During descent uncertainty requests reopening. Persistent sonar failure latches a fault; actuator failure instead stops pulses, which cannot guarantee a raised arm.

10. **What happens if an obstruction appears during closing?**

   The closing ramp is cancelled and the OPEN target is commanded immediately. Reopening must finish before another closing attempt. Actual downward stopping time must be measured; command reversal is not instant motion.

11. **How do you handle noise?**

   Approach needs three near samples; clearance needs three clear samples and continuous vacancy. Beam blockage acts immediately; clearing needs 100 ms. A latest non-clear sonar reading vetoes descent without waiting for approach confirmation.

12. **Why hysteresis?**

   The 160 mm occupied and 200 mm clear thresholds reduce toggling near a boundary. The middle band retains occupancy but does not establish clear. These are calibration starting values.

13. **What happens after power reset?**

   The first firmware target is OPEN. It verifies the endpoint and sensing before checking vacancy. Bootloader pulses and supply-loss mechanics require physical tests; without torque it cannot guarantee fail-open.

14. **What are the real limitations?**

   It is a supervised tabletop model, not a certified barrier. Short, transparent or gapped bodies, side entry, excessive speed, false-clear failures and power/linkage loss can violate the assumptions. Physical verification is still pending.

15. **What makes this advanced rather than overengineered?**

   The visible differences are passage confirmation, holding for a stopped vehicle and reopening during descent. Feedback and regression tests support those behaviours. There is no dashboard, counting claim or unrelated feature; the diagnostic runs only before closing.
