#include <Arduino.h>

#include "Inference.h"
#include "Sensor.h"
#include "Stopwatch.h"

// 4 center blocks must be lesser than this distance for inference to take place
constexpr uint16_t SENSOR_PRESENCE_DISTANCE = 250;
constexpr uint16_t SENSOR_MAX_DISTANCE = 400;
constexpr uint16_t SENSOR_MAX_SIGMA = 10;
// Sum of all distances must be less than this to count as a control character
constexpr uint16_t SENSOR_CTRL_THRESHOLD = 3000;
constexpr char SENSOR_CTRL_SYMBOL = 0xA;
constexpr float SENSOR_DELTA_FILTER_ALPHA = 0.5;

// Number of times that a class must appear in the last LEN_HISTORY inferences to be a valid detection
constexpr uint8_t MIN_VALID_COUNT = 2;
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

  stopwatch_setup();

  memset(history, 0xFF, LEN_HISTORY);

  Serial.println("Setup done");
}

bool raw_data_mode = false;

int8_t read_infer() {
  uint16_t distances[SENSOR_IMAGE_SIZE], sigma[SENSOR_IMAGE_SIZE];
  float ndistances[SENSOR_IMAGE_SIZE], nsigma[SENSOR_IMAGE_SIZE];

  int8_t ret = sensor_read(distances, sigma);
  if (ret != 0) {
    return -2;
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < SENSOR_IMAGE_SIZE; i++) {
    sum += distances[i];

    // Normalise distances
    if (distances[i] > SENSOR_MAX_DISTANCE) {
      ndistances[i] = 1.0;
    } else {
      ndistances[i] = (float)distances[i] / SENSOR_MAX_DISTANCE;
    }

    if (sigma[i] > SENSOR_MAX_SIGMA) {
      nsigma[i] = 1.0;
    } else {
      nsigma[i] = (float)sigma[i] / SENSOR_MAX_SIGMA;
    }
  }

  int8_t inference = inference_infer(ndistances, nsigma);
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

  static uint32_t last_ctrl_seen = 0;

  if (sum < SENSOR_CTRL_THRESHOLD) {
    last_ctrl_seen = millis();
    return SENSOR_CTRL_SYMBOL;
  }

  // Crude bandaid for false trigger after hand is removed for control character
  if ((millis() - last_ctrl_seen) < 1000) {
    return -1;
  }

  // If nothing nearby, just quit
  if (
      distances[27] > SENSOR_PRESENCE_DISTANCE ||
      distances[28] > SENSOR_PRESENCE_DISTANCE ||
      distances[35] > SENSOR_PRESENCE_DISTANCE ||
      distances[36] > SENSOR_PRESENCE_DISTANCE) {
    return -1;
  }

  int8_t inference = inference_infer(ndistances, nsigma);

  return inference;
}

void loop() {
  constexpr uint8_t NUM_CLASSES = 10;
  static uint8_t history_pos = 0;
  static uint8_t history_class_count[NUM_CLASSES] = {0};

  stopwatch_loop();

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

  static uint32_t last_ctrl_seen = -1;

  if (inference != -2) {
    if (inference == SENSOR_CTRL_SYMBOL) {
      if (last_ctrl_seen == (uint32_t)-1) {
        last_ctrl_seen = millis();
      }

    } else {
      uint32_t diff = last_ctrl_seen != (uint32_t)-1 ? millis() - last_ctrl_seen : 0;
      if (diff > 1000 && diff < 3000) {
        stopwatch_backspace();
      } else if (diff > 3000) {
        Serial.println("long");
      }

      last_ctrl_seen = (uint32_t)-1;

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
      if (history_class_count[max_class] > MIN_VALID_COUNT && max_class != last_class) {
        last_class = max_class;

        Serial.print("Inference: ");
        Serial.println(max_class);
        stopwatch_add_digit(max_class);
      }
    }
  }
}
