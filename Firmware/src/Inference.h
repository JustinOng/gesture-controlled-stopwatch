#ifndef INFERENCE_H
#define INFERENCE_H

#include <stdint.h>

// Minimum confidence to report symbol as found
constexpr float MIN_INFERENCE_CONFIDENCE = 0.9;
constexpr int NUM_SYMBOLS = 10;

/**
 * @brief Setup inference
 *
 * @return int8_t 0 if successful, else non zero
 */
int8_t inference_setup();
/**
 * @brief Run inference on an array of distances from the sensor
 *
 * @param ndistances array of 64 distances, where looking forward,
 *                  element 0 is top left, 7 is top right, 63 is bottom right
 * @param nsigma    array of 64 deltas, same format as \p ndistances
 * @return int8_t 0-9 if valid symbol found, -1 if no valid symbol. -2 on inference error,
 */
int8_t inference_infer(float ndistances[], float nsigma[]);

#endif
