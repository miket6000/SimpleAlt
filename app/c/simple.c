#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "altimeter.h"
#include "recording.h"

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

uint8_t altimeter_raw_data[ALTIMETER_FLASH_SIZE] = {0};

bool get_filename(char *dest, char * const base_filename, char * const extension) {
  int file_append = 1;
  
  sprintf(dest, "%s.%s", base_filename, extension);

  while (access(dest, F_OK) == 0 && file_append < 255) {
    sprintf(dest, "%s_%i.%s", base_filename, file_append++, extension);
  }

  if (file_append >= 255) {
    return false;
  }
  
  return true;  
}

/* returns the index of the first byte that is different between the two buffers */
uint32_t diff(uint8_t * const data1, uint8_t * const data2, uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
    if (data1[i] != data2[i]) {
      return i;
    }
  }
  return length;
}

bool read_altimeter_from_file(char *uid, uint8_t *altimeter_raw_data) {
  char filename[64];
  sprintf(filename, "%s.dump", uid); 
  bool file_exist = (access(filename, F_OK) == 0);

  if (file_exist) {
    FILE *fpt = NULL;
    fpt = fopen(filename, "rb");
    fread(altimeter_raw_data, sizeof(uint8_t), ALTIMETER_FLASH_SIZE, fpt);
    fclose(fpt);
  }

  return file_exist;
}

char *write_altimeter_to_file(char *uid, uint8_t *altimeter_raw_data) {
  static char filename[64];
  sprintf(filename, "%s.dump", uid); 
  FILE *fpt = NULL;
  fpt = fopen(filename, "wb+");
  fwrite(altimeter_raw_data, sizeof(uint8_t), ALTIMETER_FLASH_SIZE, fpt);  
  fclose(fpt);
  return filename;
}

/* update local file and buffers to match altimeter */
void sync_altimeter(char *uid, uint8_t *altimeter_raw_data) {
  char *filename;
  uint8_t altimeter_index[ALTIMETER_INDEX_SIZE];
  uint32_t difference = 0;

  if (!read_altimeter_from_file(uid, altimeter_raw_data)) {
    printf("Local save file not found, downloading...\n");
    altimeter_get_data(altimeter_raw_data, 0, ALTIMETER_FLASH_SIZE);
    filename = write_altimeter_to_file(uid, altimeter_raw_data);
    printf("Saved data as %s.\n", filename);
  } else {
    printf("Local save file found for altimeter %s, comparing to altimeter data...\n", uid);
    altimeter_get_data(altimeter_index, 0, ALTIMETER_INDEX_SIZE);
    difference = diff(altimeter_index, altimeter_raw_data, ALTIMETER_INDEX_SIZE);

    if (difference < ALTIMETER_INDEX_SIZE) {
      printf("Local save file and altimeter are different, updating local copy...\n");
      printf("(%.2x != %.2x @ %.8x)\n", altimeter_index[difference], altimeter_raw_data[difference], difference);
      // we've already downloaded the index, just copy it across, then grab the rest
      memcpy(altimeter_raw_data, altimeter_index, ALTIMETER_INDEX_SIZE);
      altimeter_get_data(&altimeter_raw_data[ALTIMETER_INDEX_SIZE], ALTIMETER_INDEX_SIZE, ALTIMETER_FLASH_SIZE-ALTIMETER_INDEX_SIZE);
      filename = write_altimeter_to_file(uid, altimeter_raw_data);
    } else {
      printf("No difference found, local save file is up to date.\n");
    }
  }
}  
  
void print_recordings() {  
  printf("The following recordings were found:\n");

  // present the options to the user
  uint8_t recording_id = 0;
  Recording *p_recording = get_recording(recording_id);

  while (p_recording != NULL) {  
    printf("[%d], duration %.2fs, ground altitude %.2fm, max altitude (AGL) %.2fm\n", 
      recording_id, 
      p_recording->duration, 
      p_recording->ground_altitude, 
      p_recording->max_altitude
    );
    p_recording = get_recording(++recording_id);
  }
}

void write_csv(char *filename, Recording *p_recording) {
  FILE *fpt = fopen(filename, "w+");
  fprintf(fpt, "%s, %s, %s, %s, %s, %s\n", "Time", "Altitude", "Pressure", "Temperature", "Voltage", "State");
  for (int i = 0; i < p_recording->length; i++) {
    RecordingRow row = p_recording->rows[i];
    fprintf(fpt, "%0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %.2x\n", 
      row.time,
      row.altitude,
      row.pressure,
      row.temperature,
      row.voltage,
      row.state 
    );
  }
  fclose(fpt);
}

/* Application Flow:
 * 1) Open port and get UID
 * 2) Look for filename corresponding to UID
 * 3) If not found, download entire flash and save. Jump to 5)
 * 4) If found, download index and compare, if different download entire flash and save
 *    - later on this could be smarter, maybe just download the new indexes if relevant
 * 5) Walk through index and reconstruct flights as found using latest state info
 * 6) Display options to user for export
 * 7) Export chosen file as csv.
 */
int main(int argc, char **argv) {
  /* Get the port/file name from the command line. */
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return -1;
  }
  
  char *port_name = argv[1];
  char *uid = altimeter_connect(port_name);
  char uid_entry[16];

  if (uid != NULL) {
    printf("Successfully connected to altimeter (UID = %s).\n", uid);
    sync_altimeter(uid, altimeter_raw_data);
  } else {
    printf("Could not connect to altimeter, enter a UID to load from file, or press return to exit.\n> ");
    fgets(uid_entry, 9, stdin);
    switch (strlen(uid_entry)) {
      case 8:
        uid = uid_entry;
        if (!read_altimeter_from_file(uid, altimeter_raw_data)) {
          printf("%s.dump was not found.\n", uid);
          return 0;
        }
        break;
      case 0:
        return 0;
      default:
        printf("Invalid input: %s (%ld)\n", uid_entry, strlen(uid_entry));
        return 0;
    }
  } 

  parse_recordings(altimeter_raw_data);
  print_recordings();

  // get and validate user decision on which recording to export 
  printf("\nWhich would you like to export?\n> ");
  int selected_id = -1;
  scanf("%d", &selected_id);
  Recording *p_recording = NULL;

  if (selected_id >= 0) {
    p_recording = get_recording(selected_id);
  }
  if (p_recording == NULL) {
    printf("Sorry, the input was not recognised.\n");
    return 0;
  }

  // save the selected file.
  char base_filename[64];
  char filename[64];
  sprintf(base_filename, "SimpleAlt_%s_%i", uid, selected_id);
  if (get_filename(filename, base_filename, "csv")) {
    write_csv(filename, p_recording);
  } else {
    printf("Unable to save file.\n");
    return -1;
  } 

  printf("File saved as %s\n", filename);
  return 0;
}

