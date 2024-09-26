#ifndef RECORDING_H
#define RECORDING_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_SETTINGS      8
#define NUM_RECORD_TYPES  5

typedef struct {
  float time;
  float altitude;
  float pressure;
  float temperature;
  float voltage;
  uint8_t state;
} RecordingRow;

typedef struct {
  float duration;
  float max_altitude;
  float ground_altitude;
  uint16_t sample_rates[NUM_RECORD_TYPES];
  uint8_t highest_sampled_record_type;
  uint32_t length;
  RecordingRow *rows;
  RecordingRow *current_row;
} Recording;

typedef struct {
  char title[24];
  char label;
  uint32_t value;
  void (* read)(void *, uint8_t **);
} SettingType;

typedef struct {
  char title[16];
  char label;
  SettingType *setting;
  void (* read)(Recording *, uint8_t **);
} RecordType;

Recording *get_recording(uint8_t index);
void parse_recordings(uint8_t *data);
void add_recording(Recording *recording, uint8_t *data, uint32_t length);

void read_altitude(Recording *recording, uint8_t **ptr);
void read_pressure(Recording *recording, uint8_t **ptr);
void read_temperature(Recording *recording, uint8_t **ptr);
void read_voltage(Recording *recording, uint8_t **ptr);
void read_state(Recording *recording, uint8_t **ptr);

void read_uint32(void *dest, uint8_t **ptr);

#endif //RECORDING_H
