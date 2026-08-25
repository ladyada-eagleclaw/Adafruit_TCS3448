#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define GAIN_COUNT 13

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

const uint8_t spectralChannels[] = {0, 1, 2, 3, 6, 7,
                                    8, 9, 12, 13, 14, 15};
uint16_t previousReading[sizeof(spectralChannels)] = {0};
uint8_t unsaturatedLevels[sizeof(spectralChannels)] = {0};
uint8_t clearIncreases[sizeof(spectralChannels)] = {0};

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

  for (uint8_t gainCode = 0; gainCode < GAIN_COUNT; gainCode++) {
    tcs3448_gain_t gain = (tcs3448_gain_t)gainCode;
    if (!tcs.setGain(gain) || tcs.getGain() != gain) {
      failAndHalt(F("Gain write/readback mismatch"));
    }

    tcs3448_data_t data;
    // Discard the first result after changing gain so the reported gain and
    // ADC data both come from a complete measurement at the new setting.
    if (!takeMeasurement(&data) || !takeMeasurement(&data)) {
      failAndHalt(F("Gain measurement failed"));
    }

    Serial.print(F("Gain code "));
    Serial.print(gainCode);
    Serial.print(F(" reported="));
    Serial.print((uint8_t)data.gain);
    Serial.print(F(": F4="));
    Serial.print(data.channels[TCS3448_CHANNEL_F4]);
    if (data.saturated) {
      Serial.print(F(" saturated"));
    }
    Serial.println();

    if (data.gain != gain) {
      failAndHalt(F("Concurrent gain report did not match configured gain"));
    }

    if (!data.saturated) {
      for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
        uint16_t reading = data.channels[spectralChannels[index]];
        if (reading != 0xFFFF) {
          if (unsaturatedLevels[index] > 0) {
            uint16_t noiseMargin = previousReading[index] / 20;
            if (noiseMargin < 2) {
              noiseMargin = 2;
            }
            if ((uint32_t)reading >
                (uint32_t)previousReading[index] + noiseMargin) {
              clearIncreases[index]++;
            }
          }
          previousReading[index] = reading;
          unsaturatedLevels[index]++;
        }
      }
    }
  }

  bool everyChannelPassed = true;
  for (uint8_t index = 0; index < sizeof(spectralChannels); index++) {
    Serial.print(F("Channel "));
    Serial.print(spectralChannels[index]);
    Serial.print(F(": unsaturated levels="));
    Serial.print(unsaturatedLevels[index]);
    Serial.print(F(" clear increases="));
    Serial.println(clearIncreases[index]);
    if (unsaturatedLevels[index] < 4 || clearIncreases[index] < 2) {
      everyChannelPassed = false;
    }
  }
  if (!everyChannelPassed) {
    failAndHalt(F("A spectral channel did not show enough gain response"));
  }

  pixels.clear();
  pixels.show();
  if (!tcs.setGain(TCS3448_GAIN_256X)) {
    failAndHalt(F("Could not restore default gain"));
  }
  Serial.println(F("PASS: Every spectral channel responds across the gain sweep"));
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
