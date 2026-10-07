/*
 * ============================================================
 *  lcd_display.ino — Battery & Status Display
 * ============================================================
 *  Alternates every DISPLAY_INTERVAL ms between:
 *    Screen A: battery voltage, battery %, USB output state
 *    Screen B: cover state, day/night state
 * ============================================================
 */

extern LiquidCrystal_I2C lcd;
extern bool coverOpen;
extern bool isNight;

extern float readBatteryVoltage();
extern float batteryPercent(float voltage);
extern bool usbOutputEnabled;

void updateDisplay() {
  static bool screenToggle = false;
  screenToggle = !screenToggle;

  lcd.clear();

  if (screenToggle) {
    float battVoltage = readBatteryVoltage();
    float battPct = batteryPercent(battVoltage);

    lcd.setCursor(0, 0);
    lcd.print("Batt:");
    lcd.print(battVoltage, 2);
    lcd.print("V ");
    lcd.print((int)battPct);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("USB: ");
    lcd.print(usbOutputEnabled ? "ON" : "OFF");

  } else {
    lcd.setCursor(0, 0);
    lcd.print("Cover:");
    lcd.print(coverOpen ? "OPEN  " : "CLOSED");

    lcd.setCursor(0, 1);
    lcd.print(isNight ? "Night mode      " : "Tracking sun    ");
  }
}