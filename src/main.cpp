#include <Arduino.h>

#include "Inference.h"
#include "Sensor.h"

constexpr uint16_t SENSOR_MAX_DISTANCE = 600;

// Number of times that a class must appear in the last LEN_HISTORY inferences to be a valid detection
constexpr uint8_t MIN_VALID_COUNT = 8;
constexpr uint8_t LEN_HISTORY = 16;
static int8_t history[LEN_HISTORY];

void error() {
  Serial.println("Halted");
  while (1) {
    digitalWrite(LED_RED, HIGH);
    delay(100);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Setup begin");

  int8_t ret = inference_setup();
  if (ret != 0) {
    Serial.print("Inference setup failed: ");
    Serial.println(ret);
    error();
  }

  Serial.println("Inference setup done");

  ret = sensor_setup();
  if (ret != 0) {
    Serial.print("Sensor setup failed: ");
    Serial.println(ret);
    error();
  }

  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH);

  memset(history, 0xFF, LEN_HISTORY);

  Serial.println("Setup done");
}

bool raw_data_mode = false;

int8_t read_infer() {
  float distances[SENSOR_IMAGE_WIDTH * SENSOR_IMAGE_WIDTH];
  float normalised_distances[SENSOR_IMAGE_WIDTH * SENSOR_IMAGE_WIDTH];

  int8_t ret = sensor_read(distances);
  if (ret != 0) {
    return -2;
  }

  float sum = 0;
  // Normalise distances
  for (uint8_t i = 0; i < (SENSOR_IMAGE_WIDTH * SENSOR_IMAGE_WIDTH); i++) {
    if (distances[i] > SENSOR_MAX_DISTANCE) {
      normalised_distances[i] = 1.0;
    } else {
      normalised_distances[i] = (float)distances[i] / SENSOR_MAX_DISTANCE;
    }

    sum += normalised_distances[i];
  }

  float ave = sum / 64;

  int8_t inference = inference_infer(normalised_distances);
  if (raw_data_mode) {
    for (uint8_t i = 0; i < 64; i++) {
      Serial.print((uint16_t)distances[i]);
      Serial.print(", ");
    }
    Serial.println();
  }

  return inference;
}

void loop() {
  constexpr uint8_t NUM_CLASSES = 10;
  static uint8_t history_pos = 0;
  static uint8_t history_class_count[NUM_CLASSES] = {0};

  if (Serial.available()) {
    switch (Serial.read()) {
      case 'e':
        raw_data_mode = false;
        break;
      case 'E':
        raw_data_mode = true;
        break;
    }
  }

  int8_t inference = read_infer();

  if (inference != -2) {
    if (history[history_pos] != -1) {
      history_class_count[history[history_pos]]--;
    }

    if (inference > -1) {
      history_class_count[inference]++;
    }

    history[history_pos] = inference;
    history_pos = (history_pos + 1) % LEN_HISTORY;

    uint8_t max_class = 0;
    for (uint8_t i = 0; i < NUM_CLASSES; i++) {
      if (history_class_count[i] > history_class_count[max_class]) {
        max_class = i;
      }
    }

    if (history_class_count[max_class] > MIN_VALID_COUNT) {
      Serial.print("Inference: ");
      Serial.println(max_class);
    }
  }
}
