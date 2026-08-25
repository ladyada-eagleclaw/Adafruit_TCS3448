#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define INT_PIN 2
#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define MAX_CYCLES 20

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 interrupt persistence hardware test"));
  pinMode(INT_PIN, INPUT);
  pixels.begin();
  pixels.setBrightness(16);
  pixels.clear();
  pixels.show();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_16X)) {
    failAndHalt(F("Sensor configuration failed"));
  }

  uint16_t offReading = averageCH0(false);
  uint16_t onReading = averageCH0(true);
  if ((uint32_t)onReading <= (uint32_t)offReading + 20UL) {
    failAndHalt(F("Illumination range was too small"));
  }
  uint16_t threshold = offReading + ((onReading - offReading) / 2);
  if (!tcs.setThresholdChannel(0) || !tcs.setLowThreshold(0) ||
      !tcs.setHighThreshold(threshold)) {
    failAndHalt(F("Threshold configuration failed"));
  }

  uint8_t oneCycleCount = countCyclesToInterrupt(1);
  uint8_t fiveCycleCount = countCyclesToInterrupt(4);
  pixels.clear();
  pixels.show();

  if (oneCycleCount == 0 || oneCycleCount > MAX_CYCLES ||
      fiveCycleCount == 0 || fiveCycleCount > MAX_CYCLES) {
    failAndHalt(F("Interrupt did not assert within the cycle limit"));
  }
  if (fiveCycleCount <= oneCycleCount) {
    failAndHalt(F("Higher persistence did not require more cycles"));
  }

  Serial.print(F("PASS: Persistence code 1 asserted after "));
  Serial.print(oneCycleCount);
  Serial.println(F(" cycle(s)"));
  Serial.print(F("PASS: Persistence code 4 asserted after "));
  Serial.print(fiveCycleCount);
  Serial.println(F(" cycle(s)"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

uint8_t countCyclesToInterrupt(uint8_t persistence) {
  pixels.clear();
  pixels.show();
  if (!tcs.stopMeasurement() || !tcs.enableSpectralInterrupt(false) ||
      !tcs.clearStatus()) {
    failAndHalt(F("Could not clear the previous persistence state"));
  }
  if (!tcs.setPersistence(persistence) ||
      tcs.getPersistence() != persistence) {
    failAndHalt(F("Persistence write/readback mismatch"));
  }

  setBluePixels(true);
  delay(100);
  if (!clearPendingResult() || !tcs.clearStatus() ||
      !tcs.enableSpectralInterrupt(true) ||
      !tcs.startMeasurement()) {
    failAndHalt(F("Could not start persistence measurement"));
  }

  uint8_t cycles = 0;
  tcs3448_data_t data;
  while (cycles < MAX_CYCLES && digitalRead(INT_PIN) == HIGH) {
    if (!waitForData(&data)) {
      failAndHalt(F("Persistence measurement timed out"));
    }
    cycles++;
  }

  if (!tcs.stopMeasurement() || !tcs.enableSpectralInterrupt(false) ||
      !tcs.clearStatus()) {
    failAndHalt(F("Could not clear the persistence interrupt"));
  }
  return cycles;
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

uint16_t averageCH0(bool illuminated) {
  setBluePixels(illuminated);
  delay(100);
  uint32_t sum = 0;
  tcs3448_data_t data;
  if (!takeMeasurement(&data) || data.saturated) {
    failAndHalt(F("Discard measurement failed"));
  }
  for (uint8_t sample = 0; sample < 3; sample++) {
    if (!takeMeasurement(&data) || data.saturated) {
      failAndHalt(F("Calibration measurement failed"));
    }
    sum += data.channels[0];
  }
  return sum / 3;
}

bool takeMeasurement(tcs3448_data_t *data) {
  if (!tcs.stopMeasurement() || !clearPendingResult() ||
      !tcs.startMeasurement()) {
    return false;
  }
  bool succeeded = waitForData(data);
  tcs.stopMeasurement();
  return succeeded;
}

void setBluePixels(bool enabled) {
  uint32_t color = pixels.Color(0, 0, 0);
  if (enabled) {
    color = pixels.Color(0, 0, 255);
  }
  for (uint8_t pixel = 0; pixel < NEOPIXEL_COUNT; pixel++) {
    pixels.setPixelColor(pixel, color);
  }
  pixels.show();
}

void failAndHalt(const __FlashStringHelper *message) {
  pixels.clear();
  pixels.show();
  tcs.stopMeasurement();
  tcs.enableSpectralInterrupt(false);
  tcs.clearStatus();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
