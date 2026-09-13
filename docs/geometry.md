# Sensing geometry and its limits

## Recommended layout

Use the existing 65 × 30 cm base and 14 cm guided lane. Change the starting model recommendation to an opaque **20 × 7 × 5 cm body** with no windows/cutouts at sensing height. Keep nominal sonar A at x = −12 cm; move B to **x = +2 cm**. The arm rotates across the road, centred at x = 0, with its entire swept thickness contained in **−0.5 ≤ x ≤ +0.5 cm**.

```text
TOP VIEW — x increases in the direction of entry

      A nominal -12 cm          pivot     B +2 cm
                               O [OPEN/CLOSED cams beside pivot]
                               | x=0; vertical arm sweep ±0.5 cm
      sensor → across lane       │          TX
             ↓                   │          ↓
ENTRY ═══════╪═══════════════════ │ ═════════╪════════ EXIT
     [ opaque body ≥20 cm ]  →   │          │
      ═══════╪═══════════════════ │ ═════════╪════════
             ▬ backboard                    RX

      |<------------- nominal 14 cm -------->|
      A sensor face to backboard: 24 cm across road
      Double lines are low lane guides, not opaque sensor barriers.
      Servo is outside lane at the pivot, arm spans the lane.
      Lane width: 14 cm; vehicle centred ~12 cm from sensor
      Both sensing axes ~2.5 cm above road; arm pivot ~4 cm
```

Keep guides below the sonar axis and out of its acoustic view. A seven-centimetre-wide body centred 12 cm from the sensor gives a nominal near side at 8.5 cm; the empty backboard is at 24 cm. Defaults 16 cm occupied / 20 cm clear therefore have useful starting separation. These distances are geometric estimates, not measured readings.

## What must be measured

An HC-SR04 has a field of view, not an exact plane. Establish an effective guaranteed body-detection section `a` within **[−14, −10] cm** by moving the actual flat-sided model along the whole permitted lane, at the tested lighting/reflector arrangement. The requirement is that whenever the body spans this section it cannot repeatedly report the empty backboard as clear. Treat spurious background returns as failed coverage; filtering cannot repair them.

Mount B within **[+1.5, +2.5] cm**, downstream of the swept region, and verify its entire beam intersects every permitted body at every allowed lateral position. Sensor height must intersect solid material, not wheel gaps or windows. Measure the actual arm envelope, including coupling flex and mounting uncertainty.

## Conditional geometric proof

Model a permitted straight vehicle body as the closed interval `[r,f]`, rear `r`, front `f`, with continuous opaque length `L=f−r ≥20 cm`. The A/B section separation is at most:

`b_max − a_min = 2.5 − (−14) = 16.5 cm < L_min = 20 cm`.

Suppose the body intersects the arm sweep `[−0.5,+0.5]` but does not block B. There are only two interval possibilities:

1. **Body has not reached B:** `f < b`. Since it intersects the sweep, `f ≥ −0.5 > a`. Also `r=f−L < b−20 ≤−17.5 < a`. Therefore `r<a<f`: A must detect it.
2. **Body is already beyond B:** `r>b≥1.5>0.5`. This contradicts intersection with the arm sweep.

Thus, under the stated coverage/solid-body assumptions, a body intersecting the sweep cannot leave both sections clear. Reversing does not invalidate this static interval argument. It does invalidate simplistic direction/counting claims, which are removed.

The sequence is: A occupied → open command and raised endpoint → B blocked by front → A clears while B still blocks during ordinary forward travel → B clears when rear moves downstream → fresh both-clear interval → close. An abandoned/reversed approach may also leave both zones empty; that permits vacancy recovery without claiming a successful entry.

## Timing margins

Maximum model speed is **3 cm/s**. At the latest accepted approach section x=−10, the front has 9.5 cm before the upstream swept edge. Available time is `9.5/3 = 3.17 s`. Three-sample confirmation can take about `3×65 ms =195 ms`, plus loop/Echo scheduling; the complete opening/settling deadline is 2.5 s. The nominal remaining margin is about **0.47 s** before extra scheduling uncertainty. Verify the actual detection boundary and timing; still require the demonstration driver to wait for green.

For a vehicle entering backwards from the downstream side during closure, the minimum B-to-sweep margin is `1.5−0.5 =1.0 cm`. Require measured total beam interruption → **cessation of downward arm motion** of at most **200 ms**. At 3 cm/s, travel in that interval is 0.6 cm, leaving a nominal 0.4 cm spatial margin. This includes sensing, loop, servo pulse scheduling and mechanical coast; measuring only the software command is insufficient. If that target fails, reduce model speed or increase the margin and re-check the length inequality.

The 1.2 s clearance interval is not a substitute for these margins. Likewise, a 100 ms endpoint dwell is a starting settling target; verify endpoint behaviour during real reversals.

## What the proof does not cover

Short/transparent/gapped bodies, objects inserted vertically or from the side, protrusions invisible at beam height, pedestrians, excessive speed, a wrong/inverted receiver, missed ultrasonic body detection, a persistent false-clear sensor after the diagnostic, linkage failure and power loss violate the assumptions. A single horizontal beam cannot protect arbitrary three-dimensional objects. Any contact within the permitted model envelope fails acceptance; do not fix it by merely increasing the clearance delay.
