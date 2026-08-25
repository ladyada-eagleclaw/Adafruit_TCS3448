#include <Adafruit_TCS3448.h>

#define TEST_DURATION_MS 30000UL
#define DATA_TIMEOUT_MS 2000UL

Adafruit_TCS3448 tcs;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 30-second stability hardware test"));
  Wire.begin();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_16X)) {
    failAndHalt(F("Sensor configuration failed"));
  }
  // BusIO calls Wire.begin() during begin(), so select 400 kHz afterward.
  Wire.setClock(400000);
  if (!clearPendingResult() || !tcs.startMeasurement()) {
    failAndHalt(F("Could not start the 400 kHz measurement stream"));
  }

  tcs3448_data_t discarded;
  if (!waitForData(&discarded)) {
    failAndHalt(F("Could not discard the first result after changing gain"));
  }

  uint32_t started = millis();
  uint32_t frameCount = 0;
  uint32_t saturatedFrameCount = 0;
  while (millis() - started < TEST_DURATION_MS) {
    tcs3448_data_t data;
    if (!waitForData(&data)) {
      failAndHalt(F("A coherent result timed out or failed"));
    }
    if (data.gain != TCS3448_GAIN_16X) {
      failAndHalt(F("Concurrent gain report changed unexpectedly"));
    }

    bool anyNonzero = false;
    for (uint8_t channel = 0; channel < TCS3448_CHANNEL_COUNT; channel++) {
      if (data.channels[channel] != 0) {
        anyNonzero = true;
      }
    }
    if (!anyNonzero) {
      failAndHalt(F("A complete frame contained only zero values"));
    }
    if (data.saturated) {
      saturatedFrameCount++;
    }
    frameCount++;

    if ((frameCount % 100) == 0) {
      Serial.print(F("Frames completed: "));
      Serial.println(frameCount);
    }
  }

  tcs.stopMeasurement();
  Wire.setClock(100000);
  if (frameCount < 100) {
    failAndHalt(F("Too few frames completed during the stability interval"));
  }

  Serial.print(F("PASS: Coherent frames completed: "));
  Serial.println(frameCount);
  Serial.print(F("Saturated frames observed: "));
  Serial.println(saturatedFrameCount);
  Serial.println(F("ALL TESTS PASSED"));
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

void loop() { delay(1000); }

bool waitForData(tcs3448_data_t *data) {
  uint32_t started = millis();
  bool ready = false;
  while (millis() - started < DATA_TIMEOUT_MS) {
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
  Wire.setClock(100000);
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
