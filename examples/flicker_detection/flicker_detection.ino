/*!
 * @file flicker_detection.ino
 *
 * Flicker-detection example for the Adafruit TCS3448.
 */

#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs3448;

const uint8_t FLICKER_120_VALID_BIT = 3;
const uint8_t FLICKER_100_VALID_BIT = 2;
const uint8_t FLICKER_120_DETECTED_BIT = 1;
const uint8_t FLICKER_100_DETECTED_BIT = 0;

void setup() {
  Serial.begin(115200);
  // Wait for the Serial Monitor on native USB boards. Remove this loop to let
  // the sketch run immediately from battery power.
  while (!Serial) {
    delay(10);
  }
  delay(250);

  Serial.println(F("Adafruit TCS3448 flicker detection"));

  if (!tcs3448.begin()) {
    Serial.println(F("Could not find a TCS3448 sensor!"));
    while (true) {
      delay(10);
    }
  }

  if (!tcs3448.enableFlickerDetection(true)) {
    Serial.println(F("Could not enable flicker detection!"));
    while (true) {
      delay(10);
    }
  }

  Serial.println(F("Point the sensor at a light source."));
}

void loop() {
  uint8_t rawStatus = 0;
  if (!tcs3448.getFlickerStatus(&rawStatus)) {
    delay(500);
    return;
  }

  tcs3448_flicker_t flicker = decodeFlickerStatus(rawStatus);
  Serial.print(F("Flicker: "));
  switch (flicker) {
    case TCS3448_FLICKER_100HZ:
      Serial.print(F("100 Hz"));
      break;
    case TCS3448_FLICKER_120HZ:
      Serial.print(F("120 Hz"));
      break;
    case TCS3448_FLICKER_NONE:
      Serial.print(F("none"));
      break;
    default:
      Serial.print(F("unknown"));
      break;
  }
  Serial.print(F(" (raw status: 0x"));
  Serial.print(rawStatus, HEX);
  Serial.println(')');

  delay(500);
}

tcs3448_flicker_t decodeFlickerStatus(uint8_t status) {
  if (bitRead(status, FLICKER_120_VALID_BIT) &&
      bitRead(status, FLICKER_120_DETECTED_BIT)) {
    return TCS3448_FLICKER_120HZ;
  }
  if (bitRead(status, FLICKER_100_VALID_BIT) &&
      bitRead(status, FLICKER_100_DETECTED_BIT)) {
    return TCS3448_FLICKER_100HZ;
  }
  return TCS3448_FLICKER_NONE;
}
