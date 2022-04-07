#include "Keyboard.h"

#include <PluggableUSBHID.h>
#include <USBKeyboard.h>

USBKeyboard keyboard;

void keyboard_setup() {
}

void keyboard_loop() {
}

void keyboard_write(char c) {
  keyboard.key_code(c);
}
