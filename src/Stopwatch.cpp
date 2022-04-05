#include "Stopwatch.h"

#include <LiquidCrystal_I2C.h>

constexpr int MAX_DIGITS = 3;
static uint8_t digit_count = 0;

static bool active = false;
static uint32_t count = 0;
static uint32_t cur_count = 0;

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

void stopwatch_redraw();

void stopwatch_setup() {
  Wire.setClock(100000);
  if (lcd.begin(16, 2)) {
    lcd.setCursor(0, 1);
    lcd.print("hello");
  } else {
    Serial.println("LCD init failed");
  }
  Wire.setClock(400000);
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

  Wire.setClock(100000);
  lcd.setCursor(0, 0);
  lcd.print(buf);
  Wire.setClock(400000);
}

void stopwatch_add_digit(uint8_t c) {
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
