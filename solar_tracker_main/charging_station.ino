/*
 * ============================================================
 *  charging_station.ino — USB Solar Charging Station
 * ============================================================
 *  Reads battery voltage through a resistor-divider sensor on A3
 *  and switches the USB output relay (CH1, D4, active-LOW) with a
 *  hysteresis band so it doesn't chatter at the cutoff.
 *
 *  Battery: single Li-ion cell (Samsung EB-BG360CBN) charged by a
 *  TP4056, which stops at 4.20 V.
 * ============================================================
 */

#define VOLTAGE_SENSOR_PIN A3

const float DIVIDER_RATIO = 4.22;   // calibrated with a multimeter: 5.0 × (3.77 / 4.47)
const float ADC_REF = 5.0;
const int ADC_MAX = 1023;

#define BATT_FULL_V     4.20
#define BATT_EMPTY_V    3.30
#define BATT_LOW_CUTOFF 3.40
#define BATT_RESUME_V   3.70

bool usbOutputEnabled = false;

float readBatteryVoltage() {
  int raw = analogRead(VOLTAGE_SENSOR_PIN);
  float pinVoltage = raw * (ADC_REF / ADC_MAX);
  return pinVoltage * DIVIDER_RATIO;
}

float batteryPercent(float voltage) {
  if (voltage >= BATT_FULL_V)  return 100.0;
  if (voltage <= BATT_EMPTY_V) return 0.0;
  return ((voltage - BATT_EMPTY_V) / (BATT_FULL_V - BATT_EMPTY_V)) * 100.0;
}

void updateCharging() {
  float battVoltage = readBatteryVoltage();
  float battPct = batteryPercent(battVoltage);

  if (battVoltage < BATT_LOW_CUTOFF) {
    if (usbOutputEnabled) {
      digitalWrite(RELAY_CHARGE, HIGH);
      usbOutputEnabled = false;
      Serial.println("[CHARGE] Battery critical — USB disabled");
    }
  } else if (battVoltage >= BATT_RESUME_V && !usbOutputEnabled) {
    digitalWrite(RELAY_CHARGE, LOW);
    usbOutputEnabled = true;
    Serial.println("[CHARGE] Battery OK — USB enabled");
  }

  Serial.print("[CHARGE] Batt: ");
  Serial.print(battVoltage, 2);
  Serial.print("V (");
  Serial.print(battPct, 0);
  Serial.print("%) | USB: ");
  Serial.println(usbOutputEnabled ? "ON" : "OFF");
}
