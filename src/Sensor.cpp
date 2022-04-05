#include "Sensor.h"

#include <SparkFun_VL53L5CX_Library.h>

SparkFun_VL53L5CX imager;

int8_t sensor_setup() {
  pinMode(PIN_POWER, OUTPUT);
  digitalWrite(PIN_POWER, HIGH);

  Wire.begin();
  Wire.setClock(1000000);

  imager.setWireMaxPacketSize(128);

  Serial.println(F("Initializing sensor board. This can take up to 10s. Please wait."));
  if (!imager.begin()) {
    Serial.println(F("Failed to find sensor - check connections"));
    return 1;
  }

  imager.setSharpenerPercent(20);
  imager.setResolution(SENSOR_IMAGE_WIDTH * SENSOR_IMAGE_WIDTH);
  imager.setRangingFrequency(15);
  imager.startRanging();

  return 0;
}

int8_t sensor_read(float distances[], uint16_t sigma[]) {
  VL53L5CX_ResultsData measurementData;

  if (!imager.isDataReady()) {
    return 1;
  }

  if (!imager.getRangingData(&measurementData)) {
    return 2;
  }

  uint8_t i = 0;
  for (int y = 0; y <= SENSOR_IMAGE_WIDTH * (SENSOR_IMAGE_WIDTH - 1); y += SENSOR_IMAGE_WIDTH) {
    for (int x = SENSOR_IMAGE_WIDTH - 1; x >= 0; x--) {
      distances[i] = measurementData.distance_mm[x + y];
      sigma[i] = measurementData.range_sigma_mm[x + y];
      i++;
    }
  }

  return 0;
}
