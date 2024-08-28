#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "altimeter.h"

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

bool get_filename(char *dest, char *base_filename, char *extension) {
  int file_append = 1;
  
  sprintf(dest, "%s.%s", base_filename, extension);

  while (access(dest, F_OK) == 0 && file_append < 255) {
    sprintf(dest, "%s_%i.%s", base_filename, file_append++, extension);
  }

  if (file_append >= 255) {
    return false;
  } else {
    return true;
  }

}


int main(int argc, char **argv) {
  
  /* Get the port names from the command line. */
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return -1;
  }
  
  char *port_name = argv[1];

  if (altimeter_connect(port_name) >= 0) {
    printf("Succsefully to %s\n", port_name);
  }

  printf("The following recordings were found:\n\n");

  for (int i = 0; i < altimeter_get_recording_count(); i++) {
    printf("  [%i]\tduration %.1f seconds\n", i + 1, altimeter_get_recording_length(i) * 0.02);
  }

  printf("\nWhich would you like to export?\n> ");

  /* get and validate user decision on which recording to download */
  uint32_t recording_id = 0;
  scanf("%d", &recording_id);
  if (recording_id > altimeter_get_recording_count()) {
    printf("Sorry, the input was not recognised.\n");
    return (-1);
  }

  /* determine a filename that's descriptive, but not already in use */
  char filename[64];

  if (recording_id == 0) {
    if (get_filename(filename, "altimeter_dump", "bin")) {
      uint8_t *data = (uint8_t *)malloc(0x00200000);
      altimeter_get_data(data, 0, 0x00200000);

      FILE *fpt = fopen(filename, "w+b");      
      fwrite(data, sizeof(uint8_t), 0x00200000, fpt);
      fclose(fpt);
      free(data);
    } else {
      printf("Unable to save file.\n");
      return -1;
    }
  } else {
    sprintf(filename, "SimpleAlt_log_%i", recording_id);
    if (get_filename(filename, filename, "csv")) {
      /* prepare and download the recording */
      uint32_t recording_length = altimeter_get_recording_length(recording_id - 1);
      int32_t *altitudes = (int32_t *)malloc(recording_length * sizeof(int32_t));
      altimeter_get_recording(altitudes, recording_id - 1, recording_length);
     
      FILE *fpt = fopen(filename, "w+");
      fprintf(fpt, "Time, Altitude\n");
      for (int i = 0; i < recording_length; i++) {
        fprintf(fpt, "%0.2f, %0.2f\n", i * 0.02, (altitudes[i]-altitudes[0])/100.0);
      }
      fclose(fpt);
      free(altitudes);
    } else {
      printf("Unable to save file.\n");
      return -1;
    } 
  }
 
  printf("File saved as %s\n", filename);

  return 0;
}

