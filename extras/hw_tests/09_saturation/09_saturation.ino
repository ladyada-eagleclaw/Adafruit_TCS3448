#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define UVA_LED_PIN 4
#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 saturation hardware test"));
  pinMode(UVA_LED_PIN, OUTPUT);
  pixels.begin();
  clearEmitters();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH)) {
    failAndHalt(F("Begin or SMUX configuration failed"));
  }

  if (!tcs.setGain(TCS3448_GAIN_0_5X) || !tcs.setATIME(0) ||
      !tcs.setASTEP(99)) {
    failAndHalt(F("Low-signal configuration failed"));
  }
  tcs3448_data_t lowData;
  if (!takeMeasurement(&lowData) || !takeMeasurement(&lowData) ||
      lowData.saturated) {
    failAndHalt(F("Low-signal frame was missing or saturated"));
  }
  Serial.println(F("PASS: Low-signal frame is not saturated"));

  pixels.setBrightness(64);
  for (uint8_t pixel = 0; pixel < NEOPIXEL_COUNT; pixel++) {
    pixels.setPixelColor(pixel, pixels.Color(255, 255, 255));
  }
  pixels.show();
  digitalWrite(UVA_LED_PIN, HIGH);
  delay(100);

  if (!tcs.setGain(TCS3448_GAIN_2048X) || !tcs.setATIME(99) ||
      !tcs.setASTEP(999)) {
    failAndHalt(F("High-signal configuration failed"));
  }
  tcs3448_data_t highData;
  if (!takeMeasurement(&highData) || !takeMeasurement(&highData)) {
    failAndHalt(F("High-signal measurement failed"));
  }

  bool fullScaleChannel = false;
  for (uint8_t channel = 0; channel < TCS3448_CHANNEL_COUNT; channel++) {
    if (highData.channels[channel] == 0xFFFF) {
      fullScaleChannel = true;
    }
  }
  if (!highData.saturated && !fullScaleChannel) {
    failAndHalt(F("Bright, maximum-gain frame did not report saturation"));
  }

  clearEmitters();
  if (!tcs.setGain(TCS3448_GAIN_256X) || !tcs.setATIME(29) ||
      !tcs.setASTEP(599)) {
    failAndHalt(F("Could not restore measurement defaults"));
  }
  Serial.print(F("PASS: High-signal saturation detected by "));
  Serial.println(highData.saturated ? F("ASTATUS") : F("0xFFFF data"));
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
  while (millis() - start < 5000) {
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
