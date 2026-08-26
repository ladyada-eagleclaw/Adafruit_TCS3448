#include <Adafruit_TCS3448.h>

#define SAMPLE_COUNT 3
#define MAX_READ_RETRIES 2
#define MINIMUM_COUNT_INCREASE 10
#define MINIMUM_PERCENT 120

Adafruit_TCS3448 tcs;
bool sensorReady = false;

const uint8_t spectralChannels[] = {
    TCS3448_CHANNEL_F1, TCS3448_CHANNEL_F2, TCS3448_CHANNEL_FZ,
    TCS3448_CHANNEL_F3, TCS3448_CHANNEL_F4, TCS3448_CHANNEL_F5,
    TCS3448_CHANNEL_FY, TCS3448_CHANNEL_FXL, TCS3448_CHANNEL_F6,
    TCS3448_CHANNEL_F7, TCS3448_CHANNEL_F8, TCS3448_CHANNEL_NIR};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 onboard LED hardware test"));

  if (!tcs.begin()) {
    failAndHalt(F("Sensor initialization failed"));
  }
  sensorReady = true;
  Serial.println(F("PASS: Begin succeeded"));

  if (!tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_32X) || !tcs.setATIME(9) ||
      !tcs.setASTEP(299)) {
    failAndHalt(F("Sensor configuration failed"));
  }
  Serial.println(F("PASS: Sensor configured for optical measurements"));

  if (!tcs.setLEDCurrent(20) || tcs.getLEDCurrent() != 20) {
    failAndHalt(F("LED current write/readback failed"));
  }
  Serial.println(F("PASS: LED current is 20 mA"));

  if (!tcs.enableLED(false)) {
    failAndHalt(F("Could not set the LED output off"));
  }
  Serial.println(F("PASS: LED output started off"));

  uint16_t firstOff[TCS3448_CHANNEL_COUNT];
  uint16_t ledOn[TCS3448_CHANNEL_COUNT];
  uint16_t finalOff[TCS3448_CHANNEL_COUNT];

  delay(100);
  if (!readAverage(firstOff)) {
    failAndHalt(F("Initial LED-off measurement failed or saturated"));
  }
  Serial.println(F("PASS: Initial LED-off samples averaged"));

  if (!tcs.enableLED(true)) {
    failAndHalt(F("Could not turn on the LED"));
  }
  delay(100);
  if (!readAverage(ledOn)) {
    failAndHalt(F("LED-on measurement failed or saturated"));
  }
  Serial.println(F("PASS: LED-on samples averaged"));

  if (!tcs.enableLED(false)) {
    failAndHalt(F("Could not turn off the LED"));
  }
  delay(100);
  if (!readAverage(finalOff)) {
    failAndHalt(F("Final LED-off measurement failed or saturated"));
  }
  Serial.println(F("PASS: Final LED-off samples averaged"));

  verifyOpticalResponse(firstOff, ledOn, finalOff);

  if (!tcs.enableLED(false)) {
    failAndHalt(F("Could not leave the LED output off"));
  }
  if (!tcs.stopMeasurement()) {
    failAndHalt(F("Could not stop the measurement engine"));
  }
  Serial.println(F("PASS: LED output left safely off"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

bool readAverage(uint16_t *average) {
  uint32_t sums[TCS3448_CHANNEL_COUNT] = {0};
  tcs3448_data_t data;

  // Discard the first complete measurement after each LED transition.
  if (!takeMeasurement(&data) || data.saturated) {
    return false;
  }
  for (uint8_t sample = 0; sample < SAMPLE_COUNT; sample++) {
    if (!takeMeasurement(&data) || data.saturated) {
      return false;
    }
    for (uint8_t channel = 0; channel < TCS3448_CHANNEL_COUNT; channel++) {
      sums[channel] += data.channels[channel];
    }
  }
  for (uint8_t channel = 0; channel < TCS3448_CHANNEL_COUNT; channel++) {
    average[channel] = sums[channel] / SAMPLE_COUNT;
  }
  return true;
}

bool readDataReadyWithRetries(bool *ready) {
  for (uint8_t attempt = 0; attempt <= MAX_READ_RETRIES; attempt++) {
    if (tcs.getDataReady(ready)) {
      if (attempt > 0) {
        Serial.println(F("PASS: Transient data-ready read recovered"));
      }
      return true;
    }
  }
  return false;
}

bool readFrameWithRetries(tcs3448_data_t *data) {
  for (uint8_t attempt = 0; attempt <= MAX_READ_RETRIES; attempt++) {
    if (tcs.readData(data)) {
      if (attempt > 0) {
        Serial.println(F("PASS: Transient result-frame read recovered"));
      }
      return true;
    }
  }
  return false;
}

bool takeMeasurement(tcs3448_data_t *data) {
  if (!tcs.stopMeasurement() || !clearPendingResult() ||
      !tcs.startMeasurement()) {
    return false;
  }

  uint32_t start = millis();
  bool ready = false;
  while (millis() - start < 2000) {
    if (!readDataReadyWithRetries(&ready)) {
      tcs.stopMeasurement();
      return false;
    }
    if (ready) {
      bool readSucceeded = readFrameWithRetries(data);
      bool stopSucceeded = tcs.stopMeasurement();
      return readSucceeded && stopSucceeded;
    }
    delay(1);
  }

  tcs.stopMeasurement();
  return false;
}

bool clearPendingResult() {
  bool ready = false;
  if (!readDataReadyWithRetries(&ready)) {
    return false;
  }
  if (ready) {
    tcs3448_data_t discarded;
    return readFrameWithRetries(&discarded);
  }
  return true;
}

void verifyOpticalResponse(const uint16_t *firstOff, const uint16_t *ledOn,
                           const uint16_t *finalOff) {
  bool foundResponse = false;
  uint8_t bestChannel = spectralChannels[0];
  uint16_t bestMinimumIncrease = 0;

  for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
    uint8_t channel = spectralChannels[index];
    bool increasedFromFirst =
        (uint32_t)ledOn[channel] >=
            (uint32_t)firstOff[channel] + MINIMUM_COUNT_INCREASE &&
        (uint32_t)ledOn[channel] * 100UL >=
            (uint32_t)firstOff[channel] * MINIMUM_PERCENT;
    bool decreasedAtEnd =
        (uint32_t)ledOn[channel] >=
            (uint32_t)finalOff[channel] + MINIMUM_COUNT_INCREASE &&
        (uint32_t)ledOn[channel] * 100UL >=
            (uint32_t)finalOff[channel] * MINIMUM_PERCENT;

    if (increasedFromFirst && decreasedAtEnd) {
      uint16_t firstIncrease = ledOn[channel] - firstOff[channel];
      uint16_t finalIncrease = ledOn[channel] - finalOff[channel];
      uint16_t minimumIncrease = min(firstIncrease, finalIncrease);
      if (!foundResponse || minimumIncrease > bestMinimumIncrease) {
        foundResponse = true;
        bestChannel = channel;
        bestMinimumIncrease = minimumIncrease;
      }
    }
  }

  if (!foundResponse) {
    Serial.println(F("Channel off/on/off readings:"));
    for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
      uint8_t channel = spectralChannels[index];
      Serial.print(channel);
      Serial.print(F(": "));
      Serial.print(firstOff[channel]);
      Serial.print('/');
      Serial.print(ledOn[channel]);
      Serial.print('/');
      Serial.println(finalOff[channel]);
    }
    failAndHalt(F("No spectral channel showed a clear off-on-off response"));
  }

  Serial.print(F("PASS: Onboard LED optical response on channel "));
  Serial.print(bestChannel);
  Serial.print(F(": "));
  Serial.print(firstOff[bestChannel]);
  Serial.print(F(" -> "));
  Serial.print(ledOn[bestChannel]);
  Serial.print(F(" -> "));
  Serial.println(finalOff[bestChannel]);
}

void failAndHalt(const __FlashStringHelper *message) {
  if (sensorReady) {
    tcs.enableLED(false);
    tcs.stopMeasurement();
  }
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
