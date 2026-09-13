# Exact pre-submission checklist

Software/design readiness does not substitute for a demonstrated physical model.

- [ ] Purchase/identify matching-polarity through-beam pair, positional servo and low-force switches.
- [ ] Follow final D2/D3/D4/D5/D6/D7/D8/D9/A0/A1/A2 pin map; piezo uses 330 Ω.
- [ ] Verify separate positive rails, common ground, supply specifications and decoupling.
- [ ] Finish staged servo, switch, sonar and beam checks; restore production firmware after any bench example.
- [ ] Measure A coverage within [−14,−10] cm, B within [+1.5,+2.5] cm, sweep within ±0.5 cm and body length ≥20 cm.
- [ ] Calibrate thresholds with recorded empty/occupied samples; no repeated false-clear wall readings while vehicle should be detected.
- [ ] Verify ≤3 cm/s operating speed, source release <500 ms and target settling within 2.5 s.
- [ ] Verify diagnostic only while raised; emitter stays on during descent.
- [ ] Measure total beam-to-cessation-of-downward-motion ≤200 ms or revise/revalidate the speed/geometry margin.
- [ ] Complete T01–T14 with actual PASS/FAIL, measurements, evidence and firmware revision; record failures and retests honestly.
- [ ] Capture 20 consecutive normal cycles and five safe reopen trials on the final mounted assembly.
- [ ] Verify long-stop automatic recovery, disconnections, jam/endpoint faults and reset while occupied.
- [ ] Confirm startup/passive power-loss behaviour; do not claim electrical isolation from Servo.detach().
- [ ] Run host tests and Uno compilation again after calibration/code edits; update validation provenance.
- [ ] Capture real overview/wiring/clearance/reopen photos, Serial log and uncut 90 s video.
- [ ] Add real media to README; keep its opening readable in about a minute.
- [ ] Publish repository, verify Actions, and check repository/video while signed out.
- [ ] Select submission paragraph matching actual evidence; use real URL and remove no honest unverified status prematurely.
