#ifndef ALTIMETER_H
#define ALTIMETER_H

#include <stdint.h>

#define MAX_NUM_ADDRESSES 255

int altimeter_connect(const char * const port_name);
int altimeter_get_addresses();
int altimeter_get_recording_count();
int altimeter_get_recording_length(const unsigned int recording);
int altimeter_get_recording(int32_t *buffer, const uint32_t recording, const uint32_t len);
int altimeter_get_data(uint8_t *buffer, const uint32_t start_address, const uint32_t end_address);

#endif //ALTIMETER_H
