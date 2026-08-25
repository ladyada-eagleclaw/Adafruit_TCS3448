#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs;
bool initialized = false;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 basic hardware test"));

  if (!tcs.begin()) {
    failAndHalt(F("First begin failed"));
  }
  initialized = true;
  Serial.println(F("PASS: First begin succeeded"));

  if (tcs.getPartID() != TCS3448_CHIP_ID) {
    failAndHalt(F("Part ID was not 0x81"));
  }
  Serial.println(F("PASS: Part ID is 0x81"));

  tcs3448_data_t data;
  if (!takeMeasurement(&data)) {
    failAndHalt(F("First coherent measurement failed"));
  }
  Serial.println(F("PASS: Coherent 18-result measurement succeeded"));

  if (!tcs.reset()) {
    failAndHalt(F("Software reset failed"));
  }
  Serial.println(F("PASS: Software reset completed"));

  if (!tcs.begin()) {
    failAndHalt(F("Begin after reset failed"));
  }
  Serial.println(F("PASS: Begin after reset succeeded"));

  if (!tcs.powerOn(false)) {
    failAndHalt(F("Power disable failed"));
  }
  delay(10);
  if (!tcs.powerOn(true)) {
    failAndHalt(F("Power re-enable failed"));
  }
  Serial.println(F("PASS: Power disable and re-enable succeeded"));

  if (!takeMeasurement(&data)) {
    failAndHalt(F("Measurement after power re-enable failed"));
  }
  Serial.println(F("PASS: Measurement recovered after power re-enable"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

bool takeMeasurement(tcs3448_data_t *data) {
  if (!tcs.stopMeasurement() || !clearPendingResult() ||
      !tcs.startMeasurement()) {
    return false;
  }

  uint32_t start = millis();
  bool ready = false;
  while (millis() - start < 2000) {
    if (!tcs.getDataReady(&ready)) {
      return false;
    }
    if (ready) {
      bool readSucceeded = tcs.readData(data);
      tcs.stopMeasurement();
      return readSucceeded;
    }
    delay(1);
  }
  tcs.stopMeasurement();
  return false;
}

bool clearPendingResult() {
  bool ready = false;
  if (!tcs.getDataReady(&ready)) {
    return false;
  }
  if (ready) {
    tcs3448_data_t discarded;
    return tcs.readData(&discarded);
  }
  return true;
}

void failAndHalt(const __FlashStringHelper *message) {
  if (initialized) {
    tcs.stopMeasurement();
  }
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
