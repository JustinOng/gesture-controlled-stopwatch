#include <Arduino.h>

#include "Inference.h"
#include "Keyboard.h"
#include "Sensor.h"

constexpr uint16_t SENSOR_MAX_DISTANCE = 400;
constexpr float SENSOR_DELTA_FILTER_ALPHA = 0.5;

// Number of times that a class must appear in the last LEN_HISTORY inferences to be a valid detection
constexpr uint8_t MIN_VALID_COUNT = 4;
constexpr uint8_t LEN_HISTORY = 8;
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
  // digitalWrite(PIN_ENABLE_SENSORS_3V3, LOW);
  while (!Serial) {
  };
  Serial.begin(9600);
  Serial.println("Setup begin");

  delay(1000);

  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH);

  nrf_gpio_cfg_sense_input(44, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

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

  Serial.println("Sensor setup done");

  keyboard_setup();

  memset(history, 0xFF, LEN_HISTORY);

  Serial.println("Setup done");
}

bool raw_data_mode = false;

int8_t read_infer() {
  float distances[SENSOR_IMAGE_SIZE];
  uint16_t sigma[SENSOR_IMAGE_SIZE];
  float normalised_distances[SENSOR_IMAGE_SIZE];

  static float prev_ndistances[SENSOR_IMAGE_SIZE];
  static float delta[SENSOR_IMAGE_SIZE];

  int8_t ret = sensor_read(distances, sigma);
  if (ret != 0) {
    return -2;
  }

  for (uint8_t i = 0; i < SENSOR_IMAGE_SIZE; i++) {
    // Normalise distances
    if (distances[i] > SENSOR_MAX_DISTANCE) {
      normalised_distances[i] = 1.0;
    } else {
      normalised_distances[i] = (float)distances[i] / SENSOR_MAX_DISTANCE;
    }

    delta[i] = SENSOR_DELTA_FILTER_ALPHA * delta[i] +
               (1 - SENSOR_DELTA_FILTER_ALPHA) * abs(normalised_distances[i] - prev_ndistances[i]);
  }

  memcpy(prev_ndistances, normalised_distances, sizeof(prev_ndistances));

  int8_t inference = inference_infer(normalised_distances, delta);
  if (raw_data_mode) {
    for (uint8_t i = 0; i < 64; i++) {
      Serial.print(distances[i]);
      Serial.print(", ");
    }

    for (uint8_t i = 0; i < 64; i++) {
      Serial.print(sigma[i]);
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

  keyboard_loop();

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

  // Serial.print("Sleep:");
  // Serial.println(millis());
  // __SEV();
  // __WFE();
  // Serial.print("Wake:");
  // Serial.println(millis());

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

    static uint8_t last_class = 255;
    if (history_class_count[max_class] > MIN_VALID_COUNT) {  // && max_class != last_class) {
      last_class = max_class;

      Serial.print("Inference: ");
      Serial.println(max_class);
      // keyboard_write('0' + max_class);
    }
  }
}
