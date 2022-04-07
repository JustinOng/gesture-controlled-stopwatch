#include "Stopwatch.h"

#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>

constexpr int MAX_DIGITS = 3;
static uint8_t digit_count = 0;

static bool active = false;
static int32_t count = 0;
static int32_t cur_count = 0;
static uint32_t start_time = 0;
static uint32_t done_time = 0;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void stopwatch_redraw();

void stopwatch_setup() {
  Wire.setClock(100000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Inf:");
  display.display();

  stopwatch_redraw();
}

void stopwatch_loop() {
  static int32_t last_drawn_count = 0;

  if (active) {
    int32_t elapsed = (millis() - start_time) / 1000;

    if (elapsed == last_drawn_count) {
      return;
    }

    last_drawn_count = elapsed;

    cur_count = count - elapsed;

    if (cur_count < 0) {
      active = false;
      done_time = millis();
    } else {
      stopwatch_redraw();
    }
  } else {
    if (done_time != (uint32_t)-1) {
      if ((millis() - done_time) < 1000) {
        display.fillRect(0, 24, 128, 24, BLACK);
        display.setCursor(0, 24);
        display.print("Time's Up!");
        display.display();
      } else {
        done_time = (uint32_t)-1;
        stopwatch_redraw();
      }
    }
  }
}

void stopwatch_redraw() {
  int32_t draw_count = count;
  if (active) {
    draw_count = cur_count;
  }

  char buf[6 + MAX_DIGITS + 1];
  snprintf(buf, sizeof(buf), "Timer %3lu", draw_count);

  Wire.setClock(100000);
  display.fillRect(0, 24, 128, 24, BLACK);
  display.setCursor(0, 24);
  display.print(buf);
  display.display();
}

void stopwatch_add_digit(uint8_t c) {
  Wire.setClock(100000);
  display.fillRect(64, 0, 128 - 64, 24, BLACK);
  display.setCursor(64, 0);
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

void stopwatch_toggle_state() {
  active = !active;

  if (active) {
    cur_count = count;
    start_time = millis();
  }
}
