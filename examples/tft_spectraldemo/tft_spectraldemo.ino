/*!
 * @file tft_spectraldemo.ino
 *
 * Adafruit TCS3448 spectral display for the ESP32-S2 TFT Feather.
 *
 * Displays a real-time spectral bar chart on the built-in 1.14-inch TFT.
 * Each bar is colored to approximate its channel's wavelength.
 */

#include <Adafruit_ST7789.h>
#include <Adafruit_TCS3448.h>
#include <Fonts/FreeSans9pt7b.h>

Adafruit_TCS3448 tcs3448;
Adafruit_ST7789 display(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);

const uint8_t NUM_BARS = 12;
const tcs3448_channel_t channels[NUM_BARS] = {
    TCS3448_CHANNEL_F1,  TCS3448_CHANNEL_F2,  TCS3448_CHANNEL_FZ,
    TCS3448_CHANNEL_F3,  TCS3448_CHANNEL_F4,  TCS3448_CHANNEL_F5,
    TCS3448_CHANNEL_FY,  TCS3448_CHANNEL_FXL, TCS3448_CHANNEL_F6,
    TCS3448_CHANNEL_F7,  TCS3448_CHANNEL_F8,  TCS3448_CHANNEL_NIR};

const char *labels[NUM_BARS] = {"F1",  "F2",  "FZ",  "F3", "F4", "F5",
                                "FY",  "FXL", "F6",  "F7", "F8", "NIR"};

const uint16_t barColors[NUM_BARS] = {
    0x780F, // F1, 405 nm violet
    0x401F, // F2, 425 nm blue-violet
    0x001F, // FZ, 450 nm blue
    0x02FF, // F3, 475 nm cyan-blue
    0x07E0, // F4, 515 nm green
    0xAFE0, // F5, 550 nm yellow-green
    0xFFE0, // FY, 555 nm yellow
    0xFC00, // FXL, 600 nm orange
    0xF800, // F6, 640 nm red
    0xC000, // F7, 690 nm deep red
    0x8000, // F8, 745 nm dark red
    0x4000, // NIR, 855 nm maroon
};

const uint16_t BAR_TOP = 18;
const uint16_t BAR_BOTTOM = 118;
const uint16_t BAR_LEFT = 4;
const uint16_t BAR_WIDTH = 17;
const uint16_t BAR_GAP = 3;

void setup() {
  Serial.begin(115200);
  delay(100);

  // Turn on the TFT and I2C power supply.
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(100);

  display.init(135, 240);
  display.setRotation(3);
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ST77XX_WHITE);

  if (!tcs3448.begin()) {
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setCursor(0, 40);
    canvas.setTextColor(ST77XX_RED);
    canvas.println(F(" TCS3448 not found!"));
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
    while (true) {
      delay(10);
    }
  }

  if (!tcs3448.setGain(TCS3448_GAIN_64X) || !tcs3448.setATIME(29) ||
      !tcs3448.setASTEP(599) ||
      !tcs3448.setSMUXMode(TCS3448_SMUX_18CH)) {
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setCursor(0, 40);
    canvas.setTextColor(ST77XX_RED);
    canvas.println(F(" Sensor setup failed!"));
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
    while (true) {
      delay(10);
    }
  }

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
}

void loop() {
  uint16_t readings[TCS3448_CHANNEL_COUNT];
  if (!tcs3448.readAllChannels(readings)) {
    delay(500);
    return;
  }

  uint16_t maximumReading = 1;
  for (uint8_t bar = 0; bar < NUM_BARS; bar++) {
    uint16_t reading = readings[channels[bar]];
    if (reading > maximumReading) {
      maximumReading = reading;
    }
  }

  canvas.fillScreen(ST77XX_BLACK);
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(35, 14);
  canvas.print(F("TCS3448 Spectrum"));

  uint16_t chartHeight = BAR_BOTTOM - BAR_TOP;
  for (uint8_t bar = 0; bar < NUM_BARS; bar++) {
    uint16_t reading = readings[channels[bar]];
    uint16_t height = (uint32_t)reading * chartHeight / maximumReading;
    if (height < 1 && reading > 0) {
      height = 1;
    }

    uint16_t x = BAR_LEFT + bar * (BAR_WIDTH + BAR_GAP);
    uint16_t y = BAR_BOTTOM - height;
    canvas.fillRect(x, y, BAR_WIDTH, height, barColors[bar]);

    canvas.setFont(NULL);
    canvas.setTextColor(barColors[bar]);
    uint8_t labelLength = strlen(labels[bar]);
    uint16_t labelX = x + (BAR_WIDTH - labelLength * 6) / 2;
    canvas.setCursor(labelX, BAR_BOTTOM + 4);
    canvas.print(labels[bar]);

    canvas.setTextColor(ST77XX_WHITE);
    char valueBuffer[6];
    itoa(reading, valueBuffer, 10);
    labelLength = strlen(valueBuffer);
    labelX = x + (BAR_WIDTH - labelLength * 6) / 2;
    uint16_t valueY = y - 2;
    if (valueY < BAR_TOP) {
      valueY = BAR_TOP;
    }
    canvas.setCursor(labelX, valueY);
    canvas.print(valueBuffer);
  }

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
  delay(200);
}
