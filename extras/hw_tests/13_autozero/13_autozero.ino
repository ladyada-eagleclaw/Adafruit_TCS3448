#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 auto-zero hardware test"));
  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_16X) || !tcs.setATIME(9) ||
      !tcs.setASTEP(599)) {
    failAndHalt(F("Sensor configuration failed"));
  }

  float disabledMilliseconds = measureCycleTime(0);
  float everyCycleMilliseconds = measureCycleTime(1);
  float firstOnlyMilliseconds = measureCycleTime(255);

  if (!tcs.setAutoZeroFrequency(255) ||
      !tcs.setSMUXMode(TCS3448_SMUX_18CH) || !tcs.setATIME(29) ||
      !tcs.setASTEP(599) || !tcs.setGain(TCS3448_GAIN_256X)) {
    failAndHalt(F("Could not restore auto-zero and timing defaults"));
  }
  Serial.print(F("Auto-zero disabled/every-cycle/first-only times: "));
  Serial.print(disabledMilliseconds, 1);
  Serial.print(F(" / "));
  Serial.print(everyCycleMilliseconds, 1);
  Serial.print(F(" / "));
  Serial.print(firstOnlyMilliseconds, 1);
  Serial.println(F(" ms"));
  Serial.println(F("PASS: Auto-zero codes 0, 1, and 255 read back and ran"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

float measureCycleTime(uint8_t autoZeroFrequency) {
  if (!tcs.stopMeasurement() ||
      !tcs.setAutoZeroFrequency(autoZeroFrequency) ||
      tcs.getAutoZeroFrequency() != autoZeroFrequency ||
      !clearPendingResult() || !tcs.startMeasurement()) {
    failAndHalt(F("Auto-zero write/readback or start failed"));
  }

  tcs3448_data_t data;
  if (!waitForData(&data)) {
    failAndHalt(F("First auto-zero result timed out"));
  }
  if (data.saturated || data.gain > TCS3448_GAIN_2048X) {
    failAndHalt(F("First auto-zero result was invalid"));
  }
  uint32_t started = millis();
  if (!waitForData(&data) || data.saturated ||
      data.gain > TCS3448_GAIN_2048X) {
    failAndHalt(F("Second auto-zero result was invalid"));
  }
  float elapsedMilliseconds = millis() - started;
  tcs.stopMeasurement();
  return elapsedMilliseconds;
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
  tcs.setAutoZeroFrequency(255);
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
