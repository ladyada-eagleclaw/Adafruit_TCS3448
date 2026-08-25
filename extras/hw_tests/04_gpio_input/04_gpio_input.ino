#include <Adafruit_TCS3448.h>

#define GPIO_STIMULUS_PIN 3

Adafruit_TCS3448 tcs;
bool initialized = false;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 GPIO input hardware test"));
  releaseStimulus();

  if (!tcs.begin() || !tcs.setGPIOInverted(false) ||
      !tcs.setGPIOOutput(false)) {
    failAndHalt(F("Could not configure sensor GPIO input"));
  }
  initialized = true;

  bool gpioValue = false;
  delay(5);
  if (!tcs.getGPIOValue(&gpioValue) || !gpioValue) {
    failAndHalt(F("Released GPIO input did not read HIGH"));
  }
  Serial.println(F("PASS: Released input reads external 3.3 V HIGH"));

  // The Metro Mini is a 5 V board. It may pull this node LOW, but it must
  // never drive HIGH into the TCS3448 GPIO voltage domain.
  digitalWrite(GPIO_STIMULUS_PIN, LOW);
  pinMode(GPIO_STIMULUS_PIN, OUTPUT);
  delay(5);
  if (!tcs.getGPIOValue(&gpioValue) || gpioValue) {
    failAndHalt(F("Metro pull-low was not read by the sensor"));
  }
  Serial.println(F("PASS: Metro open-drain-style pull-low reads LOW"));

  releaseStimulus();
  delay(5);
  if (!tcs.getGPIOValue(&gpioValue) || !gpioValue) {
    failAndHalt(F("Sensor input did not return HIGH after release"));
  }
  Serial.println(F("PASS: Sensor input returns HIGH after release"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void releaseStimulus() { pinMode(GPIO_STIMULUS_PIN, INPUT); }

void failAndHalt(const __FlashStringHelper *message) {
  releaseStimulus();
  if (initialized) {
    tcs.setGPIOOutput(false);
  }
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
