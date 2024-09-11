#include "recording.h"
#include "stdio.h"
#include "altimeter.h"
#include <stdlib.h>
#include <string.h>

static Recording recordings[255];
static uint8_t num_recordings = 0;
static const float tick_duration = 0.01;

static RecordType record_types[] = {
  {"Altitude",    'A', 5,    read_altitude},
  {"Pressure",    'P', 0,    read_pressure},
  {"Temperature", 'T', 100,  read_temperature},
  {"Voltage",     'V', 100,  read_voltage},
  {"State",       'S', 0,    read_state},
};

inline static uint16_t swap16(const uint8_t * const bytes) {
  return (uint16_t)((bytes[1] << 8) + bytes[0]);
}
inline static uint32_t swap32(const uint8_t * const bytes) {
  return (uint32_t)((bytes[3] << 24) + (bytes[2] << 16) + (bytes[1] << 8) + bytes[0]);
}

bool is_highest_sampled_datatype(DataType datatype) {
  uint16_t highest_sample_rate;
  DataType highest_sampled_datatype = ALTITUDE;
  for (uint8_t datatype = 0; datatype < NUM_DATATYPES; datatype++) {
    uint16_t sample_rate = record_types[datatype].sample_rate;
    if (sample_rate < highest_sample_rate && sample_rate != 0) {
      highest_sample_rate = sample_rate;
      highest_sampled_datatype = datatype;
    }
  }
  if (highest_sampled_datatype == datatype) {
    return true;
  }
  return false;
}

Recording *get_recording(uint8_t index) {
  if (index >= num_recordings) {
    return NULL;
  }
  
  return &recordings[index];
} 

RecordType *get_record_type(char label) {
  for (DataType datatype = 0; datatype < NUM_DATATYPES; datatype++) {
    if (record_types[datatype].label == label) {
      return &record_types[datatype];
    }
  }
  return NULL;
}

void parse_recordings(uint8_t *data) { 
  uint8_t label = data[0];
  uint32_t value = swap32(&data[1]);
  int i = 0;
  uint32_t recording_start_address = ALTIMETER_INDEX_SIZE;
  
  while (label != 0xff && i < ALTIMETER_INDEX_SIZE) {
    switch (label) {
      case 'R':
        // We know where the recording starts/ends and we know the sampling config
        // Now's as good a time as any to unpack the data.
        uint32_t length = value - recording_start_address;
        add_recording(&data[recording_start_address], length);
        recording_start_address = value;
        break;
      case 'T':
        record_types[TEMPERATURE].sample_rate = value;
        break;
      case 'P':
        record_types[PRESSURE].sample_rate = value;
        break;
      case 'V':
        record_types[VOLTAGE].sample_rate = value;
        break;
      case 'A':
        record_types[ALTITUDE].sample_rate = value;
        break;
      case 'S':
        record_types[STATE].sample_rate = value;
        break;
      case 'M':
        break;
      default:
        printf("[warning] Unknown label %c (0x%x), ignoring.\n", label, label);
        break;
    }
    // get ready for the next item in the index.
    i += 5;
    label = data[i];
    value =swap32(&data[i+1]);
  }
}

/* 
 * Determines whether this record was the record with the highest sample rate. 
 * If so, then we should create a new row which is a duplicate of the current
 * row so we can update the relevant fields. This keeps slower updating data still
 * in each row. 
 * Returns the new current row index.
 */
void advance_row(Recording *recording, DataType datatype) {
  if (is_highest_sampled_datatype(datatype)) {
    recording->current_row->time += (tick_duration * record_types[datatype].sample_rate);
    recording->current_row++;
    memcpy(recording->current_row, recording->current_row - 1, sizeof(RecordingRow));
  }
}

/* 
 * A record is a full row in the ultimate csv file. 
 * We will go through the data and create a new row just after putting the item with
 * the highest sample rate into the current row. This ensures that we have a new row
 * every time something has changed.
 * A 'recording' includes summary data (duration, ground level, max alt, etc) as well
 * as a pointer to an array of records. We should take a two pass approach to first 
 * determine how much memory to malloc, but so far just take the easy option and assume
 * that each recording uses the full flash available. This over allocates, but we 
 * won't run out. We rely on the application closing to free the mallocs.
 */
void add_recording(uint8_t *data, uint32_t len) {
  uint8_t *p_data = data;
  uint8_t *p_data_end = p_data + len;
  RecordingRow *rows = malloc(sizeof(RecordingRow) * (ALTIMETER_FLASH_SIZE - ALTIMETER_INDEX_SIZE));
  Recording *recording = &recordings[num_recordings];
  recording->rows = rows;
  recording->current_row = rows;

  while (p_data < p_data_end) {
    bool data_recognised = false;
    for (int data_type = 0; data_type < NUM_DATATYPES; data_type++) {
      if (record_types[data_type].label == *p_data) {
        data_recognised = true;
        record_types[data_type].read(recording, &p_data);
        advance_row(recording, data_type);
      }
    }
    if (!data_recognised) {
      printf("[warning] label %c (0x%.2x) at address 0x%.8x not recognised\n", *p_data, *p_data, *p_data); 
      p_data++;
    }
  }

  recording->duration = (recording->current_row - 1)->time;
  recording->length = recording->current_row - recording->rows;

  num_recordings += 1;

}


void read_altitude(Recording *recording, uint8_t **ptr) {
  float altitude_AGL = 0;
  float altitude = (int32_t)swap32(*ptr + 1) / 100.0;  

  recording->current_row->altitude = altitude;

  if (recording->current_row == recording->rows) {
    recording->ground_altitude = recording->current_row->altitude;
  }

  altitude_AGL = altitude - recording->ground_altitude;

  if (altitude_AGL > recording->max_altitude) {
    recording->max_altitude = altitude_AGL;
  }

  *ptr += 5;
}

void read_pressure(Recording *recording, uint8_t **ptr) {
  recording->current_row->pressure = swap32(*ptr + 1) / 100.0;
  *ptr += 5;
}

void read_temperature(Recording *recording, uint8_t **ptr) {
  recording->current_row->temperature = (int16_t)swap16(*ptr + 1) / 100.0;
  *ptr += 3;
}

void read_voltage(Recording *recording, uint8_t **ptr) {
  recording->current_row->voltage = swap16(*ptr + 1) / 1000.0;
  *ptr += 3;
}

void read_state(Recording *recording, uint8_t **ptr) {
  recording->current_row->state = *(uint8_t *)(*ptr + 1);
  *ptr += 2;
}


