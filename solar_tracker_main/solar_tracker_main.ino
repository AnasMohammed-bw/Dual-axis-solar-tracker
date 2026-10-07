/*
 * ============================================================
 *  INTELLIGENT DUAL-AXIS SOLAR TRACKER — MAIN SKETCH
 *  Board: Arduino UNO R3
 * ============================================================
 *  A0 = LDR top-left    A1 = LDR top-right    A2 = LDR bottom
 *  A3 = Battery voltage sensor
 *  A4 / A5 = I2C (LCD) — never use for analog reads
 *
 *  Azimuth   compares TL vs TR
 *  Elevation compares avg(TL, TR) vs B
 *  The LDRs sit on top of the panel assembly, outside the cover,
 *  so closing the cover does not shade them.
 *
 *  Servo pins:  D11 azimuth | D12 elevation | D5 cover | D6 wiper
 * ============================================================
 */

#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

extern void updateDisplay();
extern void updateCharging();
extern void runWiper();

// ── Pin Definitions ──────────────────────────────────────────
#define LDR_TL  A0
#define LDR_TR  A1
#define LDR_B   A2

#define SERVO_AZIMUTH_PIN   11
#define SERVO_ELEVATION_PIN 12
#define SERVO_COVER_PIN      5
#define SERVO_WIPER_PIN      6

#define RELAY_COVER    2
#define RELAY_WIPER    3
#define RELAY_CHARGE   4

// ── Constants ────────────────────────────────────────────────
#define SERVO_AZIMUTH_MIN    10
#define SERVO_AZIMUTH_MAX   170

#define SERVO_ELEVATION_MIN  10
#define SERVO_ELEVATION_MAX  90

#define LDR_TOLERANCE        50
#define SERVO_STEP            1

// Settle delay after each servo step; limits overlapping servo commands
#define MOVE_DELAY           60

#define NIGHT_THRESHOLD     100   // average LDR reading below this = night
#define DAY_THRESHOLD       150   // must rise above this to count as day again (hysteresis)

// true = track the sun; false = hold the panel still (fixed-position test runs)
#define TRACKING_MODE      true

#define COVER_OPEN_POS       0
#define COVER_CLOSED_POS   180

#define WIPER_PARK_POS       0
#define WIPER_SWEEP_POS    160

// ── Objects ──────────────────────────────────────────────────
Servo servoAzimuth;
Servo servoElevation;
Servo servoCover;
Servo servoWiper;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── State ────────────────────────────────────────────────────
int azimuthPos   = 90;
int elevationPos = 45;

bool coverOpen = true;
bool isNight   = false;
bool wasNight  = false;

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 2000;

unsigned long lastChargeCheck = 0;
const unsigned long CHARGE_INTERVAL = 1000;


// ── Setup ────────────────────────────────────────────────────
void setup() {

  Serial.begin(9600);
  Wire.begin();

  // Attach each servo and write its start position immediately.
  // attach() begins pulsing at 90 deg, so delaying the write causes a twitch on boot.
  servoAzimuth.attach(SERVO_AZIMUTH_PIN);
  servoAzimuth.write(azimuthPos);

  servoElevation.attach(SERVO_ELEVATION_PIN);
  servoElevation.write(elevationPos);

  servoCover.attach(SERVO_COVER_PIN);
  servoCover.write(COVER_OPEN_POS);

  servoWiper.attach(SERVO_WIPER_PIN);
  servoWiper.write(WIPER_PARK_POS);

  // Relays are active-LOW, so HIGH = OFF
  pinMode(RELAY_COVER, OUTPUT);
  digitalWrite(RELAY_COVER, HIGH);

  pinMode(RELAY_WIPER, OUTPUT);
  digitalWrite(RELAY_WIPER, HIGH);

  pinMode(RELAY_CHARGE, OUTPUT);
  digitalWrite(RELAY_CHARGE, HIGH);

  // LCD startup
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Solar Tracker V3");

  lcd.setCursor(0, 1);
  lcd.print("Initialising...");

  delay(2000);

  lcd.clear();

  Serial.println("=================================");
  Serial.println("Solar Tracker Ready");
  Serial.println("3-LDR direct-wire mode");
  Serial.println("Azimuth  = D11");
  Serial.println("Elevation = D12");
  Serial.println("Cover     = D5");
  Serial.println("Wiper     = D6");
  Serial.println("=================================");
}


// ── Main Loop ────────────────────────────────────────────────
void loop() {

  // Read the three LDRs
  int tl = analogRead(LDR_TL);
  int tr = analogRead(LDR_TR);
  int b  = analogRead(LDR_B);

  // Calculate average light level
  int avgLight = (tl + tr + b) / 3;

  // Determine day/night state
  wasNight = isNight;
  if (isNight) {
    if (avgLight > DAY_THRESHOLD) isNight = false;
  } else {
    if (avgLight < NIGHT_THRESHOLD) isNight = true;
  }


  // ── Cover Control ─────────────────────────────────────────
  if (isNight) {
    closeCover();
  } 
  else {
    openCover();
  }


  // ── Dawn Wiper ────────────────────────────────────────────
  // Runs once when the system changes from night -> day.
  //
  // Azimuth and elevation are temporarily detached so they
  // are not continuously holding torque during the sweep.
  if (wasNight && !isNight) {

    delay(1000);   // let the cover finish opening before the wiper moves

    servoAzimuth.detach();
    servoElevation.detach();

    runWiper();

    servoAzimuth.attach(SERVO_AZIMUTH_PIN);
    servoElevation.attach(SERVO_ELEVATION_PIN);

    // Restore the last known tracking positions
    servoAzimuth.write(azimuthPos);
    servoElevation.write(elevationPos);
  }


  // ── Sun Tracking ──────────────────────────────────────────
  if (!isNight && TRACKING_MODE) {
    trackSun(tl, tr, b);
  }


  // ── Charging Update ───────────────────────────────────────
  if (millis() - lastChargeCheck > CHARGE_INTERVAL) {

    updateCharging();

    lastChargeCheck = millis();
  }


  // ── LCD Update ─────────────────────────────────────────────
  if (millis() - lastDisplayUpdate > DISPLAY_INTERVAL) {

    updateDisplay();

    lastDisplayUpdate = millis();
  }


  // Small loop delay
  delay(50);
}


// ── Sun Tracking (3-LDR) ─────────────────────────────────────
void trackSun(int tl, int tr, int b) {

  // Horizontal error:
  // Positive = TL is brighter than TR
  // Negative = TR is brighter than TL
  int diffH = tl - tr;

  // Vertical error:
  // Compare average of top sensors against bottom sensor
  int avgTop = (tl + tr) / 2;
  int diffV = avgTop - b;


  // ── Azimuth ────────────────────────────────────────────────
  if (abs(diffH) > LDR_TOLERANCE) {

    if (diffH > 0 && azimuthPos > SERVO_AZIMUTH_MIN) {

      azimuthPos -= SERVO_STEP;

    }
    else if (diffH < 0 && azimuthPos < SERVO_AZIMUTH_MAX) {

      azimuthPos += SERVO_STEP;

    }

    servoAzimuth.write(azimuthPos);

    delay(MOVE_DELAY);
  }


  // ── Elevation ──────────────────────────────────────────────
  //
  // IMPORTANT:
  // Elevation is NOT blocked when azimuth moves.
  // Both axes are allowed to correct independently.
  //
  if (abs(diffV) > LDR_TOLERANCE) {

    if (diffV > 0 && elevationPos < SERVO_ELEVATION_MAX) {

      elevationPos += SERVO_STEP;

    }
    else if (diffV < 0 && elevationPos > SERVO_ELEVATION_MIN) {

      elevationPos -= SERVO_STEP;

    }

    servoElevation.write(elevationPos);

    delay(MOVE_DELAY);
  }


  // ── Serial Debug Information ──────────────────────────────
  Serial.print("Az: ");
  Serial.print(azimuthPos);

  Serial.print(" El: ");
  Serial.print(elevationPos);

  Serial.print(" | TL: ");
  Serial.print(tl);

  Serial.print(" TR: ");
  Serial.print(tr);

  Serial.print(" B: ");
  Serial.print(b);

  Serial.print(" | dH: ");
  Serial.print(diffH);

  Serial.print(" dV: ");
  Serial.println(diffV);
}


// ── Cover Control ────────────────────────────────────────────
void closeCover() {

  if (coverOpen) {

    servoCover.write(COVER_CLOSED_POS);

    coverOpen = false;

    Serial.println("Cover: CLOSED");
  }
}


void openCover() {

  if (!coverOpen) {

    servoCover.write(COVER_OPEN_POS);

    coverOpen = true;

    Serial.println("Cover: OPEN");
  }
}