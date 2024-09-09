#ifndef RECORDING_H
#define RECORDING_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {ALTITUDE, PRESSURE, TEMPERATURE, VOLTAGE, STATE, UNKNOWN, NUM_DATATYPES} DataType;

typedef struct {
  uint32_t time;
  int32_t altitude;
  uint32_t pressure;
  int16_t temperature;
  uint16_t voltage;
  uint8_t state;
} RecordingRow;

typedef struct {
  char Title[16];
  char label;
  uint8_t size;
  bool is_signed;
  float scale;
  uint16_t sample_rate;
} RecordType;

typedef struct {
  float duration;
  float max_altitude;
  float ground_altitude;
  uint32_t length;
  RecordingRow *rows;
} Recording;

Recording *get_recording(uint8_t index);
void parse_recordings(uint8_t *data);
void add_next_recording(uint8_t *data, uint32_t length);

#endif //RECORDING_H
