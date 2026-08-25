#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define UVA_LED_PIN 4
#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define SAMPLE_COUNT 3

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

const uint8_t redChannels[] = {TCS3448_CHANNEL_FXL, TCS3448_CHANNEL_F6,
                               TCS3448_CHANNEL_F7};
const uint8_t greenChannels[] = {TCS3448_CHANNEL_FY, TCS3448_CHANNEL_F4,
                                 TCS3448_CHANNEL_F5};
const uint8_t blueChannels[] = {TCS3448_CHANNEL_FZ, TCS3448_CHANNEL_F2,
                                TCS3448_CHANNEL_F3};
const uint8_t uvaChannels[] = {TCS3448_CHANNEL_F1, TCS3448_CHANNEL_F2,
                               TCS3448_CHANNEL_FZ};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 RGB and UVA hardware test"));
  pinMode(UVA_LED_PIN, OUTPUT);
  digitalWrite(UVA_LED_PIN, LOW);
  pixels.begin();
  pixels.setBrightness(16);
  clearEmitters();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_16X)) {
    failAndHalt(F("Sensor initialization failed"));
  }

  testNeoPixelColor(F("Red NeoPixels"), 255, 0, 0, redChannels,
                    sizeof(redChannels));
  testNeoPixelColor(F("Green NeoPixels"), 0, 255, 0, greenChannels,
                    sizeof(greenChannels));
  testNeoPixelColor(F("Blue NeoPixels"), 0, 0, 255, blueChannels,
                    sizeof(blueChannels));
  testUVA();

  clearEmitters();
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void testNeoPixelColor(const __FlashStringHelper *label, uint8_t red,
                       uint8_t green, uint8_t blue,
                       const uint8_t *expectedChannels,
                       uint8_t expectedChannelCount) {
  uint16_t offReadings[TCS3448_CHANNEL_COUNT];
  uint16_t onReadings[TCS3448_CHANNEL_COUNT];

  clearEmitters();
  delay(100);
  if (!readAverage(offReadings)) {
    failAndHalt(F("Emitter-off measurement failed"));
  }

  for (uint8_t pixel = 0; pixel < NEOPIXEL_COUNT; pixel++) {
    pixels.setPixelColor(pixel, pixels.Color(red, green, blue));
  }
  pixels.show();
  delay(100);
  if (!readAverage(onReadings)) {
    failAndHalt(F("NeoPixel-on measurement failed"));
  }
  clearEmitters();

  verifyExpectedResponse(label, offReadings, onReadings, expectedChannels,
                         expectedChannelCount);
}

void testUVA() {
  uint16_t offReadings[TCS3448_CHANNEL_COUNT];
  uint16_t onReadings[TCS3448_CHANNEL_COUNT];

  clearEmitters();
  delay(100);
  if (!readAverage(offReadings)) {
    failAndHalt(F("UVA-off measurement failed"));
  }

  digitalWrite(UVA_LED_PIN, HIGH);
  delay(100);
  if (!readAverage(onReadings)) {
    failAndHalt(F("UVA-on measurement failed"));
  }
  clearEmitters();

  verifyExpectedResponse(F("Active-high UVA LED"), offReadings, onReadings,
                         uvaChannels, sizeof(uvaChannels));
}

bool readAverage(uint16_t *average) {
  uint32_t sums[TCS3448_CHANNEL_COUNT] = {0};
  tcs3448_data_t data;

  // Discard the first complete measurement after each emitter transition.
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

void verifyExpectedResponse(const __FlashStringHelper *label,
                            const uint16_t *offReadings,
                            const uint16_t *onReadings,
                            const uint8_t *expectedChannels,
                            uint8_t expectedChannelCount) {
  bool foundResponse = false;
  uint8_t bestChannel = expectedChannels[0];
  uint16_t bestIncrease = 0;

  for (uint8_t index = 0; index < expectedChannelCount; index++) {
    uint8_t channel = expectedChannels[index];
    if (onReadings[channel] > offReadings[channel]) {
      uint16_t increase = onReadings[channel] - offReadings[channel];
      bool absolutePassed = increase >= 10;
      bool fractionalPassed =
          (uint32_t)onReadings[channel] * 100UL >=
          (uint32_t)offReadings[channel] * 120UL;
      if (absolutePassed && fractionalPassed && increase > bestIncrease) {
        foundResponse = true;
        bestIncrease = increase;
        bestChannel = channel;
      }
    }
  }

  if (!foundResponse) {
    failAndHalt(F("Expected wavelength channels did not respond clearly"));
  }

  Serial.print(F("PASS: "));
  Serial.print(label);
  Serial.print(F("; channel "));
  Serial.print(bestChannel);
  Serial.print(' ');
  Serial.print(offReadings[bestChannel]);
  Serial.print(F(" -> "));
  Serial.println(onReadings[bestChannel]);
}

void clearEmitters() {
  pixels.clear();
  pixels.show();
  digitalWrite(UVA_LED_PIN, LOW);
}

void failAndHalt(const __FlashStringHelper *message) {
  clearEmitters();
  tcs.stopMeasurement();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
