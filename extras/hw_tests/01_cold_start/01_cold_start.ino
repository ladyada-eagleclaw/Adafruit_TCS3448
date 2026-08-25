#include <Adafruit_TCS3448.h>

#define GPIO_STRAP_PIN 3

Adafruit_TCS3448 tcs;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 repeated cold-start test"));
  Serial.println(F("Remove all USB power, wait two seconds, then reconnect."));
  Serial.println(F("Do not use the Metro reset button between attempts."));
  Serial.println(F("Record ten separate ALL TESTS PASSED results externally."));

  pinMode(GPIO_STRAP_PIN, INPUT);
  if (digitalRead(GPIO_STRAP_PIN) != HIGH) {
    failAndHalt(F("GPIO startup strap was not pulled to 3.3 V"));
  }
  Serial.println(F("PASS: GPIO startup strap reads HIGH"));

  if (!tcs.begin() || tcs.getPartID() != TCS3448_CHIP_ID) {
    failAndHalt(F("TCS3448 was not identified after power-up"));
  }
  Serial.println(F("PASS: TCS3448 identified after power-up"));

  uint16_t readings[TCS3448_CHANNEL_COUNT];
  if (!tcs.readAllChannels(readings)) {
    failAndHalt(F("Measurement failed after power-up"));
  }
  Serial.println(F("PASS: Measurement succeeded after power-up"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void failAndHalt(const __FlashStringHelper *message) {
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
