#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define FLICKER_VALID_BIT 5
#define FLICKER_120_VALID_BIT 3
#define FLICKER_100_VALID_BIT 2
#define FLICKER_120_DETECTED_BIT 1
#define FLICKER_100_DETECTED_BIT 0

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 100/120 Hz flicker hardware test"));
  pixels.begin();
  pixels.setBrightness(64);
  clearPixels();

  if (!tcs.begin()) {
    failAndHalt(F("Begin failed"));
  }

  testFlickerFrequency(100, TCS3448_FLICKER_100HZ);
  testFlickerFrequency(120, TCS3448_FLICKER_120HZ);

  clearPixels();
  if (!tcs.enableFlickerDetection(false)) {
    failAndHalt(F("Could not leave flicker detection disabled"));
  }
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void testFlickerFrequency(uint16_t frequency,
  tcs3448_flicker_t expectedResult) {
  // begin() resets sticky flicker status before each generated frequency.
  if (!tcs.begin()) {
    failAndHalt(F("Begin before flicker frequency failed"));
  }
  clearPixels();
  if (!tcs.enableFlickerDetection(true)) {
    failAndHalt(F("Could not enable flicker detection"));
  }

  uint8_t status = 0;
  tcs3448_flicker_t result = TCS3448_FLICKER_NONE;
  uint32_t started = millis();
  uint32_t halfPeriodMicroseconds = 500000UL / frequency;
  uint32_t nextTransition = micros();
  bool pixelsOn = false;
  while (millis() - started < 3000) {
    if ((int32_t)(micros() - nextTransition) >= 0) {
      pixelsOn = !pixelsOn;
      setWhitePixels(pixelsOn);
      nextTransition += halfPeriodMicroseconds;
    }
    if (!tcs.getFlickerStatus(&status)) {
      failAndHalt(F("Could not read flicker status"));
    }
    result = decodeFlickerStatus(status);
    if (bitRead(status, FLICKER_VALID_BIT) && result == expectedResult) {
      break;
    }
  }
  clearPixels();
  if (!tcs.enableFlickerDetection(false)) {
    failAndHalt(F("Could not disable flicker detection"));
  }

  Serial.print(F("Generated "));
  Serial.print(frequency);
  Serial.print(F(" Hz, FD_STATUS=0x"));
  Serial.print(status, HEX);
  Serial.print(F(", decoded="));
  Serial.println((uint16_t)result);

  if (!bitRead(status, FLICKER_VALID_BIT) || result != expectedResult) {
    failAndHalt(F("Flicker detector did not report the generated frequency"));
  }

  Serial.print(F("PASS: Generated and detected "));
  Serial.print(frequency);
  Serial.println(F(" Hz optical flicker"));
}

tcs3448_flicker_t decodeFlickerStatus(uint8_t status) {
  if (bitRead(status, FLICKER_120_VALID_BIT) &&
      bitRead(status, FLICKER_120_DETECTED_BIT)) {
    return TCS3448_FLICKER_120HZ;
  }
  if (bitRead(status, FLICKER_100_VALID_BIT) &&
      bitRead(status, FLICKER_100_DETECTED_BIT)) {
    return TCS3448_FLICKER_100HZ;
  }
  return TCS3448_FLICKER_NONE;
}

void setWhitePixels(bool enabled) {
  uint32_t color = pixels.Color(0, 0, 0);
  if (enabled) {
    color = pixels.Color(255, 255, 255);
  }
  for (uint8_t pixel = 0; pixel < NEOPIXEL_COUNT; pixel++) {
    pixels.setPixelColor(pixel, color);
  }
  pixels.show();
}

void clearPixels() { setWhitePixels(false); }

void failAndHalt(const __FlashStringHelper *message) {
  clearPixels();
  tcs.enableFlickerDetection(false);
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
