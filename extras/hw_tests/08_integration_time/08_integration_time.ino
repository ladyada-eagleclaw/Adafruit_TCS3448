#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define CONFIG_COUNT 3
#define SAMPLE_COUNT 3

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

typedef struct {
  uint8_t atime;
  uint16_t astep;
  float expectedMilliseconds;
} integration_config_t;

const integration_config_t configurations[CONFIG_COUNT] = {
    {0, 299, 0.834}, {9, 299, 8.34}, {29, 599, 50.04}};
const uint8_t spectralChannels[] = {0, 1, 2, 3, 6, 7,
                                    8, 9, 12, 13, 14, 15};
uint16_t averagedReadings[CONFIG_COUNT][sizeof(spectralChannels)];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 integration-time hardware test"));
  pixels.begin();
  pixels.setBrightness(16);
  pixels.clear();
  pixels.show();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_16X)) {
    failAndHalt(F("Sensor configuration failed"));
  }

  for (uint8_t pixel = 0; pixel < NEOPIXEL_COUNT; pixel++) {
    pixels.setPixelColor(pixel, pixels.Color(255, 255, 255));
  }
  pixels.show();
  delay(100);

  for (uint8_t config = 0; config < CONFIG_COUNT; config++) {
    if (!tcs.setATIME(configurations[config].atime) ||
        !tcs.setASTEP(configurations[config].astep) ||
        tcs.getATIME() != configurations[config].atime ||
        tcs.getASTEP() != configurations[config].astep) {
      failAndHalt(F("ATIME or ASTEP write/readback mismatch"));
    }

    float calculatedMilliseconds = tcs.getIntegrationTime();
    float error = calculatedMilliseconds - configurations[config].expectedMilliseconds;
    if (error < 0) {
      error = -error;
    }
    if (error > 0.2) {
      failAndHalt(F("Calculated integration time was incorrect"));
    }

    if (!readAverage(averagedReadings[config])) {
      failAndHalt(F("Integration-time measurement failed or saturated"));
    }
    Serial.print(F("PASS: ATIME="));
    Serial.print(configurations[config].atime);
    Serial.print(F(" ASTEP="));
    Serial.print(configurations[config].astep);
    Serial.print(F(" time="));
    Serial.print(calculatedMilliseconds, 3);
    Serial.println(F(" ms"));
  }

  for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
    uint16_t shortReading = averagedReadings[0][index];
    uint16_t longReading = averagedReadings[2][index];
    bool absolutePassed =
        (uint32_t)longReading >= (uint32_t)shortReading + 5UL;
    bool fractionalPassed =
        (uint32_t)longReading * 100UL >= (uint32_t)shortReading * 150UL;
    if (!absolutePassed || !fractionalPassed) {
      failAndHalt(F("A spectral channel did not scale with integration time"));
    }
  }

  pixels.clear();
  pixels.show();
  if (!tcs.setATIME(29) || !tcs.setASTEP(599)) {
    failAndHalt(F("Could not restore integration-time defaults"));
  }
  Serial.println(F("PASS: Every spectral channel scales with integration time"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

bool readAverage(uint16_t *average) {
  uint32_t sums[sizeof(spectralChannels)] = {0};
  tcs3448_data_t data;

  if (!takeMeasurement(&data) || data.saturated) {
    return false;
  }
  for (uint8_t sample = 0; sample < SAMPLE_COUNT; sample++) {
    if (!takeMeasurement(&data) || data.saturated) {
      return false;
    }
    for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
      sums[index] += data.channels[spectralChannels[index]];
    }
  }
  for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
    average[index] = sums[index] / SAMPLE_COUNT;
  }
  return true;
}

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
      bool succeeded = tcs.readData(data);
      tcs.stopMeasurement();
      return succeeded;
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
  pixels.clear();
  pixels.show();
  tcs.stopMeasurement();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
