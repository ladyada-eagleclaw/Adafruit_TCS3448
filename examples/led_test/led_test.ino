/*!
 * @file led_test.ino
 *
 * Blinks the white LED connected to the TCS3448 LED driver.
 */

#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs3448;

void haltWithMessage(const __FlashStringHelper *message);

void setup() {
  Serial.begin(115200);
  // Wait for the Serial Monitor on native USB boards. Remove this loop to let
  // the sketch run immediately from battery power.
  while (!Serial) {
    delay(10);
  }
  delay(250);

  Serial.println(F("Adafruit TCS3448 onboard LED test"));

  if (!tcs3448.begin()) {
    Serial.println(F("Could not find a TCS3448 sensor!"));
    while (true) {
      delay(10);
    }
  }

  // The LED driver accepts even current settings from 4 mA through 258 mA.
  // This example uses a modest 20 mA setting.
  if (!tcs3448.setLEDCurrent(20)) {
    haltWithMessage(F("Could not set the LED current!"));
  }

  Serial.print(F("LED current: "));
  Serial.print(tcs3448.getLEDCurrent());
  Serial.println(F(" mA"));
}

void loop() {
  Serial.println(F("LED on"));
  if (!tcs3448.enableLED(true)) {
    haltWithMessage(F("Could not turn on the LED!"));
  }
  delay(250);

  Serial.println(F("LED off"));
  if (!tcs3448.enableLED(false)) {
    haltWithMessage(F("Could not turn off the LED!"));
  }
  delay(750);
}

void haltWithMessage(const __FlashStringHelper *message) {
  tcs3448.enableLED(false);
  Serial.println(message);
  while (true) {
    delay(10);
  }
}
