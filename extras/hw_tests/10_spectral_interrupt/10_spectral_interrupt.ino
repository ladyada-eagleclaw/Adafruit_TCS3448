#include <Adafruit_NeoPixel.h>
#include <Adafruit_TCS3448.h>

#define INT_PIN 2
#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 16
#define STATUS_AINT_MASK 0x08

Adafruit_TCS3448 tcs;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN,
                         NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  delay(250);

  Serial.println(F("Adafruit TCS3448 spectral interrupt hardware test"));
  pinMode(INT_PIN, INPUT);
  pixels.begin();
  pixels.setBrightness(16);
  pixels.clear();
  pixels.show();

  if (!tcs.begin() || !tcs.setSMUXMode(TCS3448_SMUX_18CH) ||
      !tcs.setGain(TCS3448_GAIN_16X)) {
    failAndHalt(F("Sensor configuration failed"));
  }
  if (digitalRead(INT_PIN) != HIGH) {
    failAndHalt(F("INT did not idle HIGH from its external pull-up"));
  }
  Serial.println(F("PASS: INT idles HIGH"));

  uint16_t offReading = averageCH0(false);
  uint16_t onReading = averageCH0(true);
  pixels.clear();
  pixels.show();
  if ((uint32_t)onReading <= (uint32_t)offReading + 20UL ||
      (uint32_t)onReading * 100UL < (uint32_t)offReading * 120UL) {
    failAndHalt(F("Blue NeoPixels did not provide enough CH0 range"));
  }
  uint16_t highThreshold = offReading + ((onReading - offReading) / 2);

  if (!tcs.enableSpectralInterrupt(false) || !tcs.setThresholdChannel(0) ||
      !tcs.setLowThreshold(0) || !tcs.setHighThreshold(highThreshold) ||
      !tcs.setPersistence(0) || !tcs.clearStatus()) {
    failAndHalt(F("Interrupt configuration failed"));
  }

  // Take a fresh emitter-off result before routing threshold status to INT.
  tcs3448_data_t data;
  if (!takeMeasurement(&data) || !tcs.clearStatus() ||
      !tcs.enableSpectralInterrupt(true)) {
    failAndHalt(F("Could not prepare the inactive interrupt check"));
  }
  if (!takeMeasurement(&data) || digitalRead(INT_PIN) != HIGH) {
    failAndHalt(F("INT asserted with CH0 inside the thresholds"));
  }
  Serial.println(F("PASS: INT stays HIGH below the high threshold"));

  setBluePixels(true);
  delay(100);
  if (!tcs.stopMeasurement() || !clearPendingResult() || !tcs.clearStatus() ||
      !tcs.startMeasurement()) {
    failAndHalt(F("Could not start illuminated interrupt measurement"));
  }
  uint8_t status = 0;
  bool interruptAsserted = false;
  for (uint8_t cycle = 0; cycle < 3; cycle++) {
    if (!waitForData(&data)) {
      failAndHalt(F("Illuminated interrupt measurement failed"));
    }
    status = tcs.getStatus();
    if ((status & STATUS_AINT_MASK) != 0 && digitalRead(INT_PIN) == LOW) {
      interruptAsserted = true;
      break;
    }
  }
  tcs.stopMeasurement();
  Serial.print(F("Illuminated CH0/threshold: "));
  Serial.print(data.channels[0]);
  Serial.print('/');
  Serial.println(highThreshold);
  Serial.print(F("STATUS=0x"));
  Serial.print(status, HEX);
  Serial.print(F(" INT="));
  Serial.println(interruptAsserted ? F("LOW during active cycle") : F("HIGH"));
  if (data.channels[0] <= highThreshold || !interruptAsserted) {
    failAndHalt(F("High-threshold interrupt did not assert"));
  }
  Serial.println(F("PASS: Threshold status and active-low INT asserted"));

  pixels.clear();
  pixels.show();
  if (!tcs.enableSpectralInterrupt(false) || !tcs.clearStatus()) {
    failAndHalt(F("Could not disable and clear spectral interrupt"));
  }
  delay(5);
  if (digitalRead(INT_PIN) != HIGH) {
    failAndHalt(F("INT did not return HIGH after clear"));
  }
  Serial.println(F("PASS: INT returns HIGH after disable and clear"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

uint16_t averageCH0(bool illuminated) {
  setBluePixels(illuminated);
  delay(100);
  tcs3448_data_t data;
  if (!takeMeasurement(&data) || data.saturated) {
    failAndHalt(F("Discard measurement failed"));
  }
  uint32_t sum = 0;
  for (uint8_t sample = 0; sample < 3; sample++) {
    if (!takeMeasurement(&data) || data.saturated) {
      failAndHalt(F("Calibration measurement failed"));
    }
    sum += data.channels[0];
  }
  return sum / 3;
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

bool takeMeasurement(tcs3448_data_t *data) {
  if (!tcs.stopMeasurement() || !clearPendingResult() ||
      !tcs.startMeasurement()) {
    return false;
  }
  bool succeeded = waitForData(data);
  tcs.stopMeasurement();
  return succeeded;
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
  tcs.enableSpectralInterrupt(false);
  tcs.clearStatus();
  Serial.print(F("FAIL: "));
  Serial.println(message);
  Serial.println(F("RESULT: FAIL"));
  while (true)
    delay(100);
}
