#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <stdint.h>

void stopwatch_setup();
void stopwatch_loop();

void stopwatch_add_digit(uint8_t c);
void stopwatch_backspace();

void stopwatch_set_state(bool active);

#endif
