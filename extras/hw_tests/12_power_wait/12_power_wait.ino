#include <Adafruit_TCS3448.h>

#define TIMING_SAMPLES 3

Adafruit_TCS3448 tcs;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 power and wait-time hardware test"));
  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setATIME(0) || !tcs.setASTEP(599)) {
    failAndHalt(F("Sensor configuration failed"));
  }

  tcs3448_data_t data;
  if (!takeOneMeasurement(&data)) {
    failAndHalt(F("Initial powered measurement failed"));
  }
  Serial.println(F("PASS: Powered measurement succeeded"));

  if (!tcs.powerOn(false)) {
    failAndHalt(F("Power disable failed"));
  }
  delay(20);
  if (!tcs.powerOn(true) || !takeOneMeasurement(&data)) {
    failAndHalt(F("Measurement did not recover after power re-enable"));
  }
  Serial.println(F("PASS: Measurement recovered after power cycle"));

  float noWaitMilliseconds = averageCycleTime(false, 0);
  float waitMilliseconds = averageCycleTime(true, 20);
  if (waitMilliseconds < noWaitMilliseconds + 30) {
    failAndHalt(F("Enabled WTIME did not add the expected delay"));
  }
  Serial.print(F("PASS: Wait disabled/enabled cycle times: "));
  Serial.print(noWaitMilliseconds, 1);
  Serial.print(F(" / "));
  Serial.print(waitMilliseconds, 1);
  Serial.println(F(" ms"));

  if (!tcs.enableLowPower(true) || !tcs.enableWait(true) ||
      !tcs.setWaitTime(20) || !takeOneMeasurement(&data)) {
    failAndHalt(F("Measurement failed in low-power wait mode"));
  }
  Serial.println(F("PASS: Measurement succeeds in low-power wait mode"));

  if (!tcs.enableLowPower(false) || !tcs.enableWait(false) ||
      !tcs.setSMUXMode(TCS3448_SMUX_18CH) || !tcs.setATIME(29) ||
      !tcs.setASTEP(599)) {
    failAndHalt(F("Could not restore power and timing defaults"));
  }
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

float averageCycleTime(bool waitEnabled, uint8_t waitTime) {
  float totalMilliseconds = 0;
  for (uint8_t sample = 0; sample < TIMING_SAMPLES; sample++) {
    if (!tcs.stopMeasurement() || !tcs.enableWait(waitEnabled) ||
        !tcs.setWaitTime(waitTime) || !clearPendingResult() ||
        !tcs.startMeasurement()) {
      failAndHalt(F("Could not start continuous timing measurement"));
    }

    tcs3448_data_t data;
    if (!waitForData(&data)) {
      failAndHalt(F("First continuous result timed out"));
    }
    uint32_t started = millis();
    if (!waitForData(&data)) {
      failAndHalt(F("Second continuous result timed out"));
    }
    totalMilliseconds += millis() - started;
    tcs.stopMeasurement();
  }
  return totalMilliseconds / TIMING_SAMPLES;
}

bool takeOneMeasurement(tcs3448_data_t *data) {
  if (!tcs.stopMeasurement() || !clearPendingResult() ||
      !tcs.startMeasurement()) {
    return false;
  }
  bool succeeded = waitForData(data);
  tcs.stopMeasurement();
  return succeeded;
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

bool waitForData(tcs3448_data_t *data) {
  uint32_t start = millis();
  bool ready = false;
  while (millis() - start < 2000) {
    if (!tcs.getDataReady(&ready)) {
      return false;
    }
    if (ready) {
      return tcs.readData(data);
    }
    delay(1);
  }
  return false;
}

void failAndHalt(const __FlashStringHelper *message) {
  tcs.stopMeasurement();
  tcs.enableWait(false);
  tcs.enableLowPower(false);
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
