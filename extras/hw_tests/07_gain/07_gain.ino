#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define GAIN_COUNT 13
#define SAMPLE_COUNT 3
#define MAX_READ_RETRIES 2
#define MINIMUM_UNSATURATED_LEVELS 4
#define MINIMUM_ENDPOINT_INCREASE 4
#define MINIMUM_ENDPOINT_PERCENT 400
#define MAXIMUM_LARGE_REGRESSIONS 1

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

const uint8_t spectralChannels[] = {0, 1, 2, 3, 6, 7,
                                    8, 9, 12, 13, 14, 15};
uint16_t firstReading[sizeof(spectralChannels)] = {0};
uint16_t previousReading[sizeof(spectralChannels)] = {0};
uint8_t unsaturatedLevels[sizeof(spectralChannels)] = {0};
uint8_t clearIncreases[sizeof(spectralChannels)] = {0};
uint8_t largeRegressions[sizeof(spectralChannels)] = {0};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 gain hardware test"));
  pixels.begin();
  pixels.setBrightness(1);
  pixels.clear();
  pixels.show();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setATIME(9) || !tcs.setASTEP(299)) {
    failAndHalt(F("Sensor configuration failed"));
  }

  for (uint8_t pixel = 0; pixel < NEOPIXEL_COUNT; pixel++) {
    pixels.setPixelColor(pixel, pixels.Color(255, 255, 255));
  }
  pixels.show();
  delay(100);

  // Ramp to maximum gain for a saturation preflight. The discarded result at
  // each step lets the concurrent gain status settle before the next step.
  for (uint8_t gainCode = 0; gainCode < GAIN_COUNT; gainCode++) {
    tcs3448_gain_t gain = (tcs3448_gain_t)gainCode;
    tcs3448_data_t discarded;
    if (!tcs.setGain(gain) || tcs.getGain() != gain ||
        !takeMeasurement(&discarded)) {
      failAndHalt(F("Maximum-gain preflight ramp failed"));
    }
  }

  uint16_t preflightReadings[TCS3448_CHANNEL_COUNT];
  bool preflightSaturated = false;
  if (!readGainAverage(TCS3448_GAIN_2048X, preflightReadings,
                       &preflightSaturated)) {
    failAndHalt(F("Maximum-gain preflight measurement failed"));
  }
  if (preflightSaturated) {
    if (!tcs.setATIME(0) || !tcs.setASTEP(299) ||
        !readGainAverage(TCS3448_GAIN_2048X, preflightReadings,
                         &preflightSaturated)) {
      failAndHalt(F("Short-integration preflight failed"));
    }
  }
  if (preflightSaturated) {
    Serial.println(F("WARNING: Maximum gain remains saturated in the preflight"));
  } else {
    Serial.println(F("PASS: Maximum gain is unsaturated at short integration"));
  }

  // Restore a longer integration so low-gain channels rise above ADC
  // quantization. Saturated high-gain frames remain excluded from trends.
  if (!tcs.setATIME(9) || !tcs.setASTEP(299)) {
    failAndHalt(F("Could not restore the response-sweep integration time"));
  }

  for (uint8_t gainCode = 0; gainCode < GAIN_COUNT; gainCode++) {
    tcs3448_gain_t gain = (tcs3448_gain_t)gainCode;
    if (!tcs.setGain(gain) || tcs.getGain() != gain) {
      failAndHalt(F("Gain write/readback mismatch"));
    }

    uint16_t readings[TCS3448_CHANNEL_COUNT];
    bool saturated = false;
    if (!readGainAverage(gain, readings, &saturated)) {
      failAndHalt(F("Gain measurement failed"));
    }

    Serial.print(F("Gain code "));
    Serial.print(gainCode);
    Serial.print(F(" reported="));
    Serial.print(gainCode);
    Serial.print(F(": F4="));
    Serial.print(readings[TCS3448_CHANNEL_F4]);
    if (saturated) {
      Serial.print(F(" saturated"));
    }
    Serial.println();

    if (!saturated) {
      for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
        uint16_t reading = readings[spectralChannels[index]];
        if (unsaturatedLevels[index] == 0) {
          firstReading[index] = reading;
        } else {
          uint16_t noiseMargin = previousReading[index] / 20;
          if (noiseMargin < 2) {
            noiseMargin = 2;
          }
          if ((uint32_t)reading >
              (uint32_t)previousReading[index] + noiseMargin) {
            clearIncreases[index]++;
          }
          if ((uint32_t)reading + noiseMargin < previousReading[index]) {
            largeRegressions[index]++;
          }
        }
        previousReading[index] = reading;
        unsaturatedLevels[index]++;
      }
    }
  }

  bool everyChannelPassed = true;
  for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
    uint16_t endpointIncrease = 0;
    if (previousReading[index] > firstReading[index]) {
      endpointIncrease = previousReading[index] - firstReading[index];
    }
    uint16_t fractionalBaseline = firstReading[index];
    if (fractionalBaseline == 0) {
      fractionalBaseline = 1;
    }

    bool enoughLevels =
        unsaturatedLevels[index] >= MINIMUM_UNSATURATED_LEVELS;
    bool absolutePassed = endpointIncrease >= MINIMUM_ENDPOINT_INCREASE;
    bool fractionalPassed =
        (uint32_t)previousReading[index] * 100UL >=
        (uint32_t)fractionalBaseline * MINIMUM_ENDPOINT_PERCENT;
    bool regressionsPassed =
        largeRegressions[index] <= MAXIMUM_LARGE_REGRESSIONS;

    Serial.print(F("Channel "));
    Serial.print(spectralChannels[index]);
    Serial.print(F(": levels="));
    Serial.print(unsaturatedLevels[index]);
    Serial.print(F(" first="));
    Serial.print(firstReading[index]);
    Serial.print(F(" last="));
    Serial.print(previousReading[index]);
    Serial.print(F(" clear increases="));
    Serial.print(clearIncreases[index]);
    Serial.print(F(" regressions="));
    Serial.println(largeRegressions[index]);

    if (!enoughLevels || !absolutePassed || !fractionalPassed ||
        !regressionsPassed) {
      everyChannelPassed = false;
    }
  }
  if (!everyChannelPassed) {
    failAndHalt(F("A spectral channel did not show enough gain response"));
  }

  pixels.clear();
  pixels.show();
  if (!tcs.setGain(TCS3448_GAIN_256X) || !tcs.setATIME(29) ||
      !tcs.setASTEP(599)) {
    failAndHalt(F("Could not restore the default sensor settings"));
  }
  Serial.println(F("PASS: Every spectral channel responds across the gain sweep"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

bool readGainAverage(tcs3448_gain_t expectedGain, uint16_t *average,
                     bool *saturated) {
  uint32_t sums[TCS3448_CHANNEL_COUNT] = {0};
  tcs3448_data_t data;

  // Discard the first result after changing gain so the reported gain and ADC
  // data both come from complete measurements at the new setting.
  if (!takeMeasurement(&data)) {
    return false;
  }

  *saturated = false;
  for (uint8_t sample = 0; sample < SAMPLE_COUNT; sample++) {
    if (!takeMeasurement(&data)) {
      return false;
    }
    if (data.gain != expectedGain) {
      printGainMismatch(expectedGain, data.gain);
      failAndHalt(F("Concurrent gain report did not match configured gain"));
    }
    if (data.saturated) {
      *saturated = true;
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

void printGainMismatch(tcs3448_gain_t expectedGain,
                       tcs3448_gain_t reportedGain) {
  Serial.print(F("Expected gain code "));
  Serial.print((uint8_t)expectedGain);
  Serial.print(F(", reported "));
  Serial.println((uint8_t)reportedGain);
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

void failAndHalt(const __FlashStringHelper *message) {
  pixels.clear();
  pixels.show();
  tcs.stopMeasurement();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
