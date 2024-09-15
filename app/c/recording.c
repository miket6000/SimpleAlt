#include "recording.h"
#include "stdio.h"
#include "altimeter.h"
#include <stdlib.h>
#include <string.h>

static Recording recordings[255]; //Hmm, might want to dynamically allocate this instead...
static uint8_t num_recordings = 0;
static const float tick_duration = 0.01;
char info[255];

void write_log(char* str) {
  FILE *fpt = fopen("simple.log", "a");
  fwrite(str, sizeof(char), strlen(str), fpt);
  fclose(fpt);
}

static SettingType settings[] = {
  {"Altitude Sample Rate",    'a',  5,        read_uint32},
  {"Pressure Sample Rate",    'p',  0,        read_uint32},
  {"Temperature Sample Rate", 't',  100,      read_uint32},
  {"Voltage Sample Rate",     'v',  100,      read_uint32},
  {"State Sample Rate",       's',  0,        read_uint32},
  {"Maxumum Altitude",        'm',  0,        read_uint32},
  {"Power Off Timeout",       'o',  12000,    read_uint32},
  {"Recording End Address",   'r',  0x10000-1,  read_uint32},
};

static RecordType record_types[] = {
  {"Altitude",    'A', &settings[0], read_altitude},
  {"Pressure",    'P', &settings[1], read_pressure},
  {"Temperature", 'T', &settings[2], read_temperature},
  {"Voltage",     'V', &settings[3], read_voltage},
  {"State",       'S', &settings[4], read_state},
};

inline static uint16_t swap16(const uint8_t * const bytes) {
  return (uint16_t)((bytes[1] << 8) + bytes[0]);
}
inline static uint32_t swap32(const uint8_t * const bytes) {
  return (uint32_t)((bytes[3] << 24) + (bytes[2] << 16) + (bytes[1] << 8) + bytes[0]);
}

// We need to know the highest sampled type as reading this will trigger the creation
// of a new row when reading the data from the altimeter. This ensures that we will have a 
// new row every time the data changes. If multiple types are recorded at the same maximum
// datarate the first (lowest index) is used.
void set_highest_sampled_record_type(Recording *recording) {
  uint32_t highest_sample_rate = 0xffffffff;
  recording->highest_sampled_record_type = 0;
  for (uint8_t type = 0; type < NUM_RECORD_TYPES; type++) {
    uint16_t sample_rate = recording->sample_rates[type];
    if (sample_rate < highest_sample_rate && sample_rate != 0) {
      highest_sample_rate = sample_rate;
      recording->highest_sampled_record_type = type;
    }
  }
}

Recording *get_recording(uint8_t index) {
  if (index >= num_recordings) {
    return NULL;
  }
  
  return &recordings[index];
} 

void parse_recordings(uint8_t *data) { 
  uint8_t *p_data = data;
  uint8_t *p_data_end = p_data + ALTIMETER_INDEX_SIZE - 1;
  uint32_t recording_start_address = ALTIMETER_INDEX_SIZE - 1;
  Recording *recording = &recordings[0];

  while (*p_data != 0xff && p_data < p_data_end) {
    for (int setting = 0; setting < NUM_SETTINGS; setting++) {
      if (*p_data == settings[setting].label) {
        settings[setting].read(&settings[setting].value, &p_data);
        sprintf(info, "Read label %c with value 0x%.8x\n", settings[setting].label, settings[setting].value);
        write_log(info);
        if (settings[setting].label == 'r') {
          uint32_t length = settings[setting].value - recording_start_address;
          sprintf(info, "New record starting at 0x%.8x\n", recording_start_address);
          write_log(info);
          add_recording(recording, &data[recording_start_address], length);
          num_recordings += 1;
          recording = &recordings[num_recordings];
          recording_start_address = settings[setting].value;
        }
      } 
    }
  }
}

/* 
 * Determines whether this record was the record with the highest sample rate. 
 * If so, then we should increment the current_row pointer to the next row and copy across all
 * the data from the previous row. This keeps slower updating data in every row. 
 */
void advance_row(Recording *recording, const uint8_t type) {
  if (type == recording->highest_sampled_record_type) {
    recording->current_row++;
    memcpy(recording->current_row, recording->current_row - 1, sizeof(RecordingRow));
    recording->current_row->time += (tick_duration * settings[type].value); 
  }
}


/* 
 * We will go through the data and create a new row just after putting the item with
 * the highest sample rate into the current row. This ensures that we have a new row
 * every time something has changed.
 * A 'recording' includes summary data (duration, ground level, max alt, etc) as well
 * as a pointer to an array of records. We should take a two pass approach to first 
 * determine how much memory to malloc, but so far just take the easy option and assume
 * that each recording uses the full flash available. This over allocates, but we 
 * won't run out. We rely on the application closing to free the mallocs.
 */
void add_recording(Recording *recording, uint8_t *data, uint32_t len) {
  uint8_t *p_data = data;
  uint8_t *p_data_end = p_data + len;

  RecordingRow *rows = malloc(sizeof(RecordingRow) * (ALTIMETER_FLASH_SIZE - ALTIMETER_INDEX_SIZE));
  
  recording->rows = rows;
  recording->current_row = rows;

  // set default data_rates;
  for (uint8_t type = 0; type < NUM_RECORD_TYPES; type++) {
    recording->sample_rates[type] = record_types[type].setting->value;
  }

  set_highest_sampled_record_type(recording);

  while (p_data < p_data_end) {
    bool data_recognised = false;
    for (int type = 0; type < NUM_RECORD_TYPES; type++) {
      if (record_types[type].label == *p_data) {
        data_recognised = true;
        record_types[type].read(recording, &p_data);
        sprintf(info, "Read record %c\n", record_types[type].label);
        write_log(info);
        advance_row(recording, type);
      }
    }
    if (!data_recognised) {
      printf("Unrecoginsed label %c\n", *p_data);
      p_data++;
    }
  }
  
  recording->duration = recording->current_row->time;
  recording->length = recording->current_row - recording->rows;
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

void read_uint32(void *dest, uint8_t **ptr) {
  *(uint32_t *)dest = swap32(*ptr + 1);
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

