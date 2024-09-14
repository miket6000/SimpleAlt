#ifndef RECORDING_H
#define RECORDING_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {ALTITUDE, PRESSURE, TEMPERATURE, VOLTAGE, STATE, NUM_DATATYPES} DataType;

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
  uint16_t sample_rates[NUM_DATATYPES];
  DataType highest_sampled_datatype;
  uint32_t length;
  RecordingRow *rows;
  RecordingRow *current_row;
} Recording;

typedef struct {
  char Title[16];
  char label;
  uint16_t sample_rate;
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






#endif //RECORDING_H
