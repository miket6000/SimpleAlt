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


/* update local file and buffers to match altimeter */
void sync_altimeter(char *uid, uint8_t *altimeter_raw_data) {
  char filename[64];
  uint8_t altimeter_index[ALTIMETER_INDEX_SIZE];
  uint32_t difference = 0;
  FILE *fpt = NULL;

  sprintf(filename, "%s.dump", uid);

  if (access(filename, F_OK) != 0) {
    printf("Local save file not found, downloading...\n");
    altimeter_get_data(altimeter_raw_data, 0, ALTIMETER_FLASH_SIZE-1);
    fpt = fopen(filename, "wb+");
    fwrite(altimeter_raw_data, sizeof(uint8_t), ALTIMETER_FLASH_SIZE, fpt);  
    fclose(fpt);
    printf("Saved data as %s.\n", filename);
  } else {
    printf("Local save file found (%s), comparing to altimeter data.\n", filename);
    // Download index, and calculate checksum. Compare to file checksum over same area
    altimeter_get_data(altimeter_index, 0, ALTIMETER_INDEX_SIZE-1);
    fpt = fopen(filename, "rb");
    fread(altimeter_raw_data, sizeof(uint8_t), ALTIMETER_FLASH_SIZE, fpt);
    fclose(fpt);

    difference = diff(altimeter_index, altimeter_raw_data, ALTIMETER_INDEX_SIZE);

    if (difference < ALTIMETER_INDEX_SIZE) {
      printf("Local save file and altimeter differ, updating...\n");
      // we've already downloaded the index, just copy it across, then grab the rest
      memcpy(altimeter_raw_data, altimeter_index, ALTIMETER_INDEX_SIZE);
      altimeter_get_data(&altimeter_raw_data[ALTIMETER_INDEX_SIZE], ALTIMETER_INDEX_SIZE, ALTIMETER_FLASH_SIZE-1);

      fpt = fopen(filename, "wb+");
      fwrite(altimeter_raw_data, sizeof(uint8_t), ALTIMETER_FLASH_SIZE, fpt);  
      fclose(fpt);
    } else {
      printf("No difference found, local save file is up to date.\n");
    }
  }
}  
  
uint8_t get_user_selection() {  
  printf("The following recordings were found:\n");

  // present the options to the user
  uint8_t recording_id = 0;
  Recording *p_recording = get_recording(recording_id);

  while (p_recording != NULL) {  
    printf("[%d], duration %.2fs, ground altitude %.2fm, max altitude (AGL) %.2fm\n", 
      recording_id, 
      p_recording->duration, 
      p_recording->ground_altitude, 
      (p_recording->max_altitude - p_recording->ground_altitude)
    );
    p_recording = get_recording(++recording_id);
  }

  printf("\nWhich would you like to export?\n> ");
  // get and validate user decision on which recording to download 
  int selected_id = 0;
  scanf("%d", &selected_id);
  return selected_id;
}

void write_csv(char *filename, Recording *p_recording) {
  FILE *fpt = fopen(filename, "w+");
  fprintf(fpt, "%s, %s, %s, %s, %s, %s\n", "Time", "Altitude", "Pressure", "Temperature", "Voltage", "State");
  for (int i = 0; i < p_recording->length; i++) {
    RecordingRow row = p_recording->rows[i];
    fprintf(fpt, "%0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %.2x\n", 
      row.time / 1000.0,
      row.altitude / 100.0,
      row.pressure / 100.0,
      row.temperature / 100.0,
      row.voltage / 1000.0,
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

  if (uid == NULL) {
    printf("Could not connect to altimeter\n");
    return 0;
  }

  printf("Successfully connected to altimeter (UID = %s).\n", uid);

  sync_altimeter(uid, altimeter_raw_data);
  parse_recordings(altimeter_raw_data);
  
  uint8_t selection = get_user_selection();

  Recording *p_recording = get_recording(selection);
  if (p_recording == NULL) {
    printf("Sorry, the input was not recognised.\n");
    return 0;
  }

  // save the selected file.
  char base_filename[64];
  char filename[64];
  sprintf(base_filename, "SimpleAlt_%s_%i", uid, selection);
  if (get_filename(filename, base_filename, "csv")) {
    write_csv(filename, p_recording);
  } else {
    printf("Unable to save file.\n");
    return -1;
  } 

  printf("File saved as %s\n", filename);
  return 0;
}

