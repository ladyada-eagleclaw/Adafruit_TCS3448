/*!
 * @file plotter.ino
 *
 * Arduino Serial Plotter example for the Adafruit TCS3448.
 *
 * Open Tools > Serial Plotter at 115200 baud. This sketch intentionally emits
 * only stable numeric label:value records so the plot legend stays clean.
 */

#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs3448;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  delay(250);

  if (!tcs3448.begin() || !tcs3448.setGain(TCS3448_GAIN_64X) ||
      !tcs3448.setATIME(29) || !tcs3448.setASTEP(599) ||
      !tcs3448.setSMUXMode(TCS3448_SMUX_18CH)) {
    while (true) {
      delay(10);
    }
  }
}

void loop() {
  uint16_t readings[TCS3448_CHANNEL_COUNT];

  if (tcs3448.readAllChannels(readings)) {
    Serial.print(F("405nm_F1:"));
    Serial.print(readings[TCS3448_CHANNEL_F1]);
    Serial.print(F(",\t425nm_F2:"));
    Serial.print(readings[TCS3448_CHANNEL_F2]);
    Serial.print(F(",\t450nm_FZ:"));
    Serial.print(readings[TCS3448_CHANNEL_FZ]);
    Serial.print(F(",\t475nm_F3:"));
    Serial.print(readings[TCS3448_CHANNEL_F3]);
    Serial.print(F(",\t515nm_F4:"));
    Serial.print(readings[TCS3448_CHANNEL_F4]);
    Serial.print(F(",\t550nm_F5:"));
    Serial.print(readings[TCS3448_CHANNEL_F5]);
    Serial.print(F(",\t555nm_FY:"));
    Serial.print(readings[TCS3448_CHANNEL_FY]);
    Serial.print(F(",\t600nm_FXL:"));
    Serial.print(readings[TCS3448_CHANNEL_FXL]);
    Serial.print(F(",\t640nm_F6:"));
    Serial.print(readings[TCS3448_CHANNEL_F6]);
    Serial.print(F(",\t690nm_F7:"));
    Serial.print(readings[TCS3448_CHANNEL_F7]);
    Serial.print(F(",\t745nm_F8:"));
    Serial.print(readings[TCS3448_CHANNEL_F8]);
    Serial.print(F(",\t855nm_NIR:"));
    Serial.print(readings[TCS3448_CHANNEL_NIR]);
    Serial.print(F(",\tClear:"));
    Serial.println(readings[TCS3448_CHANNEL_VIS_TL_0]);
  }

  delay(50);
}
