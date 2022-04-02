#include "Inference.h"

#include <Arduino.h>

#define DEBUG 1

#include "TensorFlowLite.h"
#include "number_model.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/debug_log.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/version.h"

// TFLite globals, used for compatibility with Arduino-style sketches
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* model_input = nullptr;
TfLiteTensor* model_output = nullptr;

// Create an area of memory to use for input, output, and other TensorFlow
// arrays. You'll need to adjust this by combiling, running, and looking
// for errors.
constexpr int kTensorArenaSize = 16 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

int8_t inference_setup() {
  // Set up logging (will report to Serial, even within TFLite functions)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure
  model = tflite::GetModel(number_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model version does not match Schema");
    return 1;
  }

  // This pulls in the deep learning operations that  are needed for this program
  static tflite::MicroMutableOpResolver<8> micro_resolver(error_reporter);  // choice 2

  micro_resolver.AddFullyConnected();
  micro_resolver.AddConv2D();
  micro_resolver.AddMaxPool2D();
  micro_resolver.AddSoftmax();
  micro_resolver.AddQuantize();
  micro_resolver.AddDequantize();
  micro_resolver.AddRelu();
  micro_resolver.AddReshape();

  // Build an interpreter to run the model
  static tflite::MicroInterpreter static_interpreter(
      // model, resolver, tensor_arena, kTensorArenaSize, error_reporter);  // choice 1 AllOpsResolver
      model, micro_resolver, tensor_arena, kTensorArenaSize, error_reporter);  // choice 2 micromutableOpResolver
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed");
    return 7;
  }

  // Assign model input and output buffers (tensors) to pointers
  model_input = interpreter->input(0);
  model_output = interpreter->output(0);

  return 0;
}

int8_t inference_infer(float distances[]) {
  memcpy(model_input->data.f, distances, 64 * sizeof(float));

  uint32_t start = millis();
  if (interpreter->Invoke() != kTfLiteOk) {
    return -2;
  }
  // Serial.print("Inference: ");
  // Serial.print(millis() - start);
  // Serial.println("ms");

  int8_t ret = -1;

  float* result = model_output->data.f;
  for (uint8_t i = 0; i < NUM_SYMBOLS; i++) {
    if (result[i] > MIN_INFERENCE_CONFIDENCE) {
      if (ret == -1 || result[i] > result[ret]) {
        ret = i;
      }
    }
  }

  return ret;
}
