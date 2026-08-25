#include <Adafruit_TCS3448.h>

#define UNTOUCHED_VALUE 0xA5A5

Adafruit_TCS3448 tcs;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 automatic SMUX hardware test"));

  if (!tcs.begin()) {
    failAndHalt(F("Begin failed"));
  }

  testMode(TCS3448_SMUX_6CH, 6, F("6-result mode"));
  testMode(TCS3448_SMUX_12CH, 12, F("12-result mode"));
  testMode(TCS3448_SMUX_18CH, 18, F("18-result mode"));

  if (!tcs.setSMUXMode(TCS3448_SMUX_18CH)) {
    failAndHalt(F("Could not restore 18-result SMUX mode"));
  }
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void testMode(tcs3448_smux_mode_t mode, uint8_t resultCount,
              const __FlashStringHelper *label) {
  uint16_t readings[TCS3448_CHANNEL_COUNT];
  for (uint8_t channel = 0; channel < TCS3448_CHANNEL_COUNT; channel++) {
    readings[channel] = UNTOUCHED_VALUE;
  }

  if (!tcs.stopMeasurement() || !tcs.setSMUXMode(mode) ||
      tcs.getSMUXMode() != mode || !tcs.readAllChannels(readings)) {
    failAndHalt(F("SMUX configuration or measurement failed"));
  }

  for (uint8_t channel = 0; channel < resultCount; channel++) {
    if (readings[channel] == UNTOUCHED_VALUE) {
      failAndHalt(F("A selected SMUX result was not updated"));
    }
  }
  for (uint8_t channel = resultCount; channel < TCS3448_CHANNEL_COUNT;
       channel++) {
    if (readings[channel] != UNTOUCHED_VALUE) {
      failAndHalt(F("Driver wrote beyond selected SMUX result count"));
    }
  }

  Serial.print(F("PASS: "));
  Serial.println(label);
}

void failAndHalt(const __FlashStringHelper *message) {
  tcs.stopMeasurement();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
