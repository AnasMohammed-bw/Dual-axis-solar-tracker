# Status and Roadmap

## Working
- Dual-axis tracking logic (azimuth + elevation servos respond to LDR input)
- Night cover and dawn wiper sequence
- Battery voltage readout and LCD display
- Charging chain: battery → TP4056 → boost converter → USB charges a phone
- Master power switch
- All four solar panels with inline diodes verified outdoors

## In progress
- Frame fabrication (MDF enclosure and azimuth deck, acrylic elevation frame)
- Sustained multi-servo load test (10–15 min continuous)
- Battery bay and final servo power packs (moving to NiMH AA cells)
- Strain relief on soldered panel/sensor joints

## Planned
- Current sensing and a fixed-vs-tracked energy comparison

## Design notes and limitations
- **Sensing:** three LDRs, so elevation compares the top pair's average against a
  single bottom sensor rather than a full four-quadrant comparison. Azimuth is a true
  two-sensor comparison.
- **Azimuth range is 10°–170°.** Early planning included a slip ring; further research
  showed the sweep never needs continuous rotation, so cables are wired directly.
- **Frame material:** 20 mm MDF (reclaimed stock) for the base and azimuth deck. MDF
  is moisture-sensitive; the enclosure will be sealed and kept sheltered.
- **Dust:** the wiper clears settled dust each dawn; there is no airborne-dust
  detection.
