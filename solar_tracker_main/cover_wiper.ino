/*
 * ============================================================
 *  cover_wiper.ino — Clamshell Cover + Dawn Wiper
 *  Part of: Intelligent Dual-Axis Solar Tracker
 * ============================================================
 *  Cover (SG90, driven from the main sketch):
 *    0° = open (panel exposed), 180° = closed (night)
 *
 *  Wiper (SG90): runWiper() sweeps 0° -> 160° -> 0° once on each
 *  night -> day transition, then parks.
 * ============================================================
 */

extern Servo servoWiper;

// ── Run a full wiper sweep ────────────────────────────────────
void runWiper() {
  Serial.println("[WIPER] Dawn detected — sweeping panel");

  // Sweep from park → full extension
  for (int pos = WIPER_PARK_POS; pos <= WIPER_SWEEP_POS; pos += 2) {
    servoWiper.write(pos);
    delay(12);
  }

  delay(200); // brief pause at end of sweep

  // Return to park position
  for (int pos = WIPER_SWEEP_POS; pos >= WIPER_PARK_POS; pos -= 2) {
    servoWiper.write(pos);
    delay(12);
  }

  servoWiper.write(WIPER_PARK_POS);
  Serial.println("[WIPER] Sweep complete — parked");
}
