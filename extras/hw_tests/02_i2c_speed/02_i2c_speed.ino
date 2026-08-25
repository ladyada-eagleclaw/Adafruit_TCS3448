#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 I2C speed hardware test"));

  Wire.begin();
  testSpeed(100000, F("100 kHz"));
  testSpeed(400000, F("400 kHz"));

  Wire.setClock(100000);
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void testSpeed(uint32_t clockFrequency, const __FlashStringHelper *label) {
  if (!tcs.begin()) {
    failAndHalt(F("Begin failed before selected I2C speed test"));
  }
  // BusIO calls Wire.begin() during begin(), so select the test speed after
  // sensor initialization.
  Wire.setClock(clockFrequency);
  if (tcs.getPartID() != TCS3448_CHIP_ID) {
    failAndHalt(F("Identification failed at selected I2C speed"));
  }

  uint16_t readings[TCS3448_CHANNEL_COUNT];
  if (!tcs.readAllChannels(readings)) {
    failAndHalt(F("Measurement failed at selected I2C speed"));
  }

  Serial.print(F("PASS: Identification and measurement at "));
  Serial.println(label);
}

void failAndHalt(const __FlashStringHelper *message) {
  tcs.stopMeasurement();
  Wire.setClock(100000);
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
