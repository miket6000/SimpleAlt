#include "recording.h"
#include "stdio.h"
#include "altimeter.h"
#include <stdlib.h>
#include <string.h>

static Recording recordings[255];
static uint8_t num_recordings = 0;
static const uint8_t tick_duration = 10;

static RecordType record_types[] = {
  {"Altitude",    'A',  4,  true,  100.0,  5},
  {"Pressure",    'P',  4,  false, 100.0,  0},
  {"Temperature", 'T',  2,  true,  10.0,   100},
  {"Voltage",     'V',  2,  false, 1000.0, 100},
  {"State",       'S',  1,  false, 1.0,    0},
  {"Unknown",     '?',  0,  false, 1.0,    0}
};

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
  return &record_types[UNKNOWN];
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
        add_next_recording(&data[recording_start_address], length);
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
uint32_t advance_record(DataType datatype, RecordingRow *rows, uint32_t index) {
  if (is_highest_sampled_datatype(datatype)) {
    rows[index].time = index * tick_duration * record_types[datatype].sample_rate;
    index++;
    memcpy(&rows[index], &rows[index-1], sizeof(RecordingRow));
  }
  return index;
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
void add_next_recording(uint8_t *data, uint32_t len) {
  uint32_t address = 0;
  uint8_t label;
  uint32_t index = 0;
  RecordingRow *rows = malloc(sizeof(RecordingRow) * (ALTIMETER_FLASH_SIZE - ALTIMETER_INDEX_SIZE));
  recordings[num_recordings].rows = rows;

  while (address < len) {
    // address is increamented after reading the label so that it points to the value
    label = data[address++];
    RecordType *record_type = get_record_type(label);

    //printf("[info] address = %.8x, label=%c, stepping %d bytes\n", address-1 + ALTIMETER_INDEX_SIZE, record_type->label, record_type->size);


    // how could the switch statement be improved/removed?
    switch (label) {
      case 'A':
        // There are a couple of special cases we want to record for altitude
        // namely, ground and max.
        rows[index].altitude = *(int32_t *)&data[address];
        if (rows[index].altitude > recordings[num_recordings].max_altitude) {
          recordings[num_recordings].max_altitude = rows[index].altitude;
        }
        if (index == 0) {
          recordings[num_recordings].ground_altitude = rows[index].altitude;
        }
        index = advance_record(ALTITUDE, rows, index);
        break;
      case 'T':
        rows[index].temperature = *(int16_t *)&data[address];
        index = advance_record(TEMPERATURE, rows, index);
        break;
      case 'P':
        rows[index].pressure = *(uint32_t *)&data[address];
        index = advance_record(PRESSURE, rows, index);
        break;
      case 'V':
        rows[index].voltage = *(uint16_t *)&data[address];
        index = advance_record(VOLTAGE, rows, index);
        break;
      case 'S':
        index = advance_record(STATE, rows, index);
        break;
      default:
        // If we lose sync somehow we'll get a few of these, but the address increments
        // when reading the labels will eventually push us back into sync.
        printf("[warning] Unrecognised record label '%c' (0x%.2x) at address 0x%.8x\n", label, label, address);
        break;
    }
    
    address += record_type->size;
    
  }
        
  recordings[num_recordings].duration = rows[index-1].time;
  recordings[num_recordings].length = index-1;

  num_recordings += 1;

}

