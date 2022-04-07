#include "Sensor.h"

#include <SparkFun_VL53L5CX_Library.h>

SparkFun_VL53L5CX imager;
// Terrible hack to get access to internal i2c driver of the sensor
extern SparkFun_VL53L5CX_IO VL53L5CX_i2c;

volatile bool data_ready = false;

void isr() {
  data_ready = true;
}

int8_t sensor_setup() {
  attachInterrupt(digitalPinToInterrupt(PIN_INT), isr, FALLING);

  pinMode(PIN_POWER, OUTPUT);
  digitalWrite(PIN_POWER, HIGH);

  delay(10);

  Wire.begin();
  Wire.setClock(400000);

  imager.setWireMaxPacketSize(128);

  Serial.println(F("Initializing sensor board. This can take up to 10s. Please wait."));
  if (!imager.begin()) {
    Serial.println(F("Failed to find sensor - check connections"));
    return 1;
  }

  imager.setSharpenerPercent(20);
  imager.setResolution(SENSOR_IMAGE_WIDTH * SENSOR_IMAGE_WIDTH);
  imager.setRangingFrequency(5);
  // imager.setIntegrationTime(7);
  imager.startRanging();

  return 0;
}

int8_t sensor_read(uint16_t distances[], uint16_t sigma[]) {
  if (!data_ready) {
    return 1;
  }

  if (!imager.isDataReady()) {
    return 1;
  }

  uint32_t start = millis();
  // if (!imager.getRangingData(&measurementData)) {
  //   return 2;
  // }
  uint16_t buf[128 + (4 / 2)];
  // VL53L5CX_RANGE_SIGMA_MM_IDX: offset 892, length 128
  // VL53L5CX_DISTANCE_IDX: offset 1024, length 128
  // Extra 4 bytes in the middle at offset 1020 that we don't care about
  // Because making two separate reads incurs an additional 4ms penalty,
  // We read this extra 4 bytes and just ignore it
  Wire.setClock(400000);
  if (VL53L5CX_i2c.readMultipleBytes(892, (uint8_t *)buf, sizeof(buf))) {
    return 2;
  }

  SwapBuffer((uint8_t *)buf, sizeof(buf));

  uint8_t i = 0;
  for (int y = 0; y <= SENSOR_IMAGE_WIDTH * (SENSOR_IMAGE_WIDTH - 1); y += SENSOR_IMAGE_WIDTH) {
    for (int x = SENSOR_IMAGE_WIDTH - 1; x >= 0; x--) {
      // Scaling factors defined in vl53l5cx_api.cpp
      // + 2 here to skip the 4 bytes at offset 1020
      int16_t distance = (int16_t)buf[64 + 2 + x + y] / 4;
      distances[i] = distance > 0 ? distance : 0;
      sigma[i] = buf[x + y] / 128;
      i++;
    }
  }

  return 0;
}
