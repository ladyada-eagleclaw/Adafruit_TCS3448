/*!
 * @file fulltest.ino
 *
 * Configuration walkthrough for the Adafruit TCS3448 14-Channel
 * Multi-Spectral Sensor.
 *
 * Written by Limor "Ladyada" Fried for Adafruit Industries, with assistance
 * from OpenAI Codex. MIT license; see LICENSE for more information.
 */

#include <Adafruit_TCS3448.h>

Adafruit_TCS3448 tcs3448;

void setup() {
  Serial.begin(115200);
  // Wait for the Serial Monitor on native USB boards. Remove this loop to let
  // the sketch run immediately from battery power.
  while (!Serial) {
    delay(10);
  }
  delay(250);

  Serial.println(F("Adafruit TCS3448 full test"));
  Serial.println(F("=========================="));

  if (!tcs3448.begin()) {
    Serial.println(F("Could not find a TCS3448 sensor!"));
    while (true) {
      delay(10);
    }
  }

  Serial.println(F("\n--- Chip information ---"));
  Serial.print(F("Part ID: 0x"));
  Serial.println(tcs3448.getPartID(), HEX);
  Serial.print(F("Revision ID: 0x"));
  Serial.println(tcs3448.getRevisionID(), HEX);
  Serial.print(F("Aux ID: 0x"));
  Serial.println(tcs3448.getAuxID(), HEX);

  Serial.println(F("\n--- Spectral configuration ---"));
  // Gain options range from 0.5x through 2048x.
  if (!tcs3448.setGain(TCS3448_GAIN_64X)) {
    haltWithMessage(F("Could not set the gain!"));
  }
  printGain(tcs3448.getGain());

  // ATIME is 0 to 255 integration cycles.
  if (!tcs3448.setATIME(29)) {
    haltWithMessage(F("Could not set ATIME!"));
  }
  Serial.print(F("ATIME: "));
  Serial.println(tcs3448.getATIME());

  // ASTEP is 0 to 65534 and sets the duration of each integration step.
  if (!tcs3448.setASTEP(599)) {
    haltWithMessage(F("Could not set ASTEP!"));
  }
  Serial.print(F("ASTEP: "));
  Serial.println(tcs3448.getASTEP());
  Serial.print(F("Integration time: "));
  Serial.print(tcs3448.getIntegrationTime());
  Serial.println(F(" ms"));

  Serial.println(F("\n--- SMUX configuration ---"));
  if (!tcs3448.setSMUXMode(TCS3448_SMUX_18CH)) {
    haltWithMessage(F("Could not set the SMUX mode!"));
  }
  printSMUXMode(tcs3448.getSMUXMode());

  Serial.println(F("\n--- Wait-time configuration ---"));
  // WTIME is 0 to 255. A value of 100 is about 278 ms in normal wait mode.
  if (!tcs3448.setWaitTime(100)) {
    haltWithMessage(F("Could not set the wait time!"));
  }
  Serial.print(F("Wait-time code: "));
  Serial.print(tcs3448.getWaitTime());
  Serial.println(F(" (wait mode remains disabled)"));

  Serial.println(F("\n--- Interrupt configuration ---"));
  // Persistence code 4 requires five consecutive out-of-range results.
  if (!tcs3448.setPersistence(4)) {
    haltWithMessage(F("Could not set interrupt persistence!"));
  }
  Serial.print(F("Persistence code: "));
  Serial.println(tcs3448.getPersistence());

  // Select result channel 0 for spectral threshold comparisons.
  if (!tcs3448.setThresholdChannel(0)) {
    haltWithMessage(F("Could not select the threshold channel!"));
  }
  Serial.print(F("Threshold channel: "));
  Serial.println(tcs3448.getThresholdChannel());

  if (!tcs3448.setLowThreshold(100) ||
      !tcs3448.setHighThreshold(60000)) {
    haltWithMessage(F("Could not set the interrupt thresholds!"));
  }
  Serial.print(F("Low threshold: "));
  Serial.println(tcs3448.getLowThreshold());
  Serial.print(F("High threshold: "));
  Serial.println(tcs3448.getHighThreshold());

  Serial.println(F("\n--- LED driver configuration ---"));
  // The optional sensor LED driver accepts 4 mA through 258 mA.
  if (!tcs3448.setLEDCurrent(20)) {
    haltWithMessage(F("Could not set the LED current!"));
  }
  Serial.print(F("LED current: "));
  Serial.print(tcs3448.getLEDCurrent());
  Serial.println(F(" mA"));
  Serial.println(F("LED output remains off"));

  Serial.println(F("\n--- Spectral readings ---"));
  Serial.println(F("F1\tF2\tFZ\tF3\tF4\tF5\tFY\tFXL\tF6\tF7\tF8\tNIR"));
}

void loop() {
  uint16_t readings[TCS3448_CHANNEL_COUNT];

  if (tcs3448.readAllChannels(readings)) {
    Serial.print(readings[TCS3448_CHANNEL_F1]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F2]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_FZ]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F3]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F4]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F5]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_FY]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_FXL]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F6]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F7]);
    Serial.print('\t');
    Serial.print(readings[TCS3448_CHANNEL_F8]);
    Serial.print('\t');
    Serial.println(readings[TCS3448_CHANNEL_NIR]);
  }

  delay(100);
}

void printGain(tcs3448_gain_t gain) {
  Serial.print(F("Gain: "));
  switch (gain) {
    case TCS3448_GAIN_0_5X:
      Serial.println(F("0.5x"));
      break;
    case TCS3448_GAIN_1X:
      Serial.println(F("1x"));
      break;
    case TCS3448_GAIN_2X:
      Serial.println(F("2x"));
      break;
    case TCS3448_GAIN_4X:
      Serial.println(F("4x"));
      break;
    case TCS3448_GAIN_8X:
      Serial.println(F("8x"));
      break;
    case TCS3448_GAIN_16X:
      Serial.println(F("16x"));
      break;
    case TCS3448_GAIN_32X:
      Serial.println(F("32x"));
      break;
    case TCS3448_GAIN_64X:
      Serial.println(F("64x"));
      break;
    case TCS3448_GAIN_128X:
      Serial.println(F("128x"));
      break;
    case TCS3448_GAIN_256X:
      Serial.println(F("256x"));
      break;
    case TCS3448_GAIN_512X:
      Serial.println(F("512x"));
      break;
    case TCS3448_GAIN_1024X:
      Serial.println(F("1024x"));
      break;
    case TCS3448_GAIN_2048X:
      Serial.println(F("2048x"));
      break;
    default:
      Serial.println(F("unknown"));
      break;
  }
}

void printSMUXMode(tcs3448_smux_mode_t mode) {
  Serial.print(F("SMUX mode: "));
  switch (mode) {
    case TCS3448_SMUX_6CH:
      Serial.println(F("6 results (1 cycle)"));
      break;
    case TCS3448_SMUX_12CH:
      Serial.println(F("12 results (2 cycles)"));
      break;
    case TCS3448_SMUX_18CH:
      Serial.println(F("18 results (3 cycles)"));
      break;
    default:
      Serial.println(F("unknown"));
      break;
  }
}

void haltWithMessage(const __FlashStringHelper *message) {
  Serial.println(message);
  while (true) {
    delay(10);
  }
}
