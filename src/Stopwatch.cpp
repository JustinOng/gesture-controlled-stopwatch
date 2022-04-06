#include "Stopwatch.h"

#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>

constexpr int MAX_DIGITS = 3;
static uint8_t digit_count = 0;

static bool active = false;
static uint32_t count = 0;
static uint32_t cur_count = 0;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void stopwatch_redraw();

void stopwatch_setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void stopwatch_loop() {
}

void stopwatch_redraw() {
  uint32_t &draw_count = count;
  if (active) {
    draw_count = cur_count;
  }

  char buf[MAX_DIGITS + 1];
  snprintf(buf, sizeof(buf), "%3lu", draw_count);

  display.fillRect(0, 0, 128, 16, BLACK);
  display.setCursor(0, 0);
  display.print(buf);
  display.display();
}

void stopwatch_add_digit(uint8_t c) {
  display.fillRect(0, 16, 128, 16, BLACK);
  display.setCursor(0, 16);
  display.print(c);
  display.display();

  if (active) return;

  if (digit_count < MAX_DIGITS) {
    count = count * 10 + c;
    digit_count++;

    stopwatch_redraw();
  }
}

void stopwatch_backspace() {
  if (active) return;

  if (digit_count > 0) {
    count = count / 10;
    digit_count--;

    stopwatch_redraw();
  }
}

void stopwatch_set_state(bool _active) {
  active = _active;

  if (active) {
    cur_count = count;
  }
}
