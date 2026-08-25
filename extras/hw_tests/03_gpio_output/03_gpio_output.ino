#include <Adafruit_TCS3448.h>

#define GPIO_FEEDBACK_PIN 3

Adafruit_TCS3448 tcs;
bool initialized = false;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 GPIO output hardware test"));
  pinMode(GPIO_FEEDBACK_PIN, INPUT);

  if (!tcs.begin()) {
    failAndHalt(F("Begin failed"));
  }
  initialized = true;

  if (digitalRead(GPIO_FEEDBACK_PIN) != HIGH) {
    failAndHalt(F("External 3.3 V pull-up did not produce HIGH"));
  }
  Serial.println(F("PASS: External 10K pull-up reads HIGH"));

  if (!tcs.setGPIOInverted(false) || !tcs.setGPIOOutput(true) ||
      !tcs.setGPIOValue(true)) {
    failAndHalt(F("Could not configure released GPIO output"));
  }
  delay(5);
  if (digitalRead(GPIO_FEEDBACK_PIN) != HIGH) {
    failAndHalt(F("Released GPIO did not read HIGH"));
  }
  Serial.println(F("PASS: Sensor GPIO releases HIGH"));

  if (!tcs.setGPIOValue(false)) {
    failAndHalt(F("Could not pull GPIO LOW"));
  }
  delay(5);
  if (digitalRead(GPIO_FEEDBACK_PIN) != LOW) {
    failAndHalt(F("Sensor GPIO did not pull LOW"));
  }
  Serial.println(F("PASS: Sensor GPIO pulls LOW"));

  if (!releaseGPIO()) {
    failAndHalt(F("Could not release sensor GPIO"));
  }
  if (digitalRead(GPIO_FEEDBACK_PIN) != HIGH) {
    failAndHalt(F("GPIO did not return HIGH after release"));
  }
  Serial.println(F("PASS: Sensor GPIO returns HIGH and high impedance"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

bool releaseGPIO() {
  bool succeeded = true;
  if (initialized) {
    succeeded = tcs.setGPIOValue(true);
    succeeded = tcs.setGPIOOutput(false) && succeeded;
  }
  pinMode(GPIO_FEEDBACK_PIN, INPUT);
  delay(5);
  return succeeded;
}

void failAndHalt(const __FlashStringHelper *message) {
  releaseGPIO();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
