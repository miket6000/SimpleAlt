#ifndef RECORD_H
#define RECORD_H
#include "main.h"

#define MAX_RECORD_LIST_SIZE  10
#define MAX_SETTING_LIST_SIZE 10

#define DEFAULT_ALTITUDE_SR       SECONDS_TO_TICKS(0.05)
#define DEFAULT_TEMPERATURE_SR    SECONDS_TO_TICKS(1)
#define DEFAULT_PRESSURE_SR       0
#define DEFAULT_VOLTAGE_SR        SECONDS_TO_TICKS(1)
#define DEFAULT_STATUS_SR         0
#define DEFAULT_POWER_OFF_TIMEOUT SECONDS_TO_TICKS(1200)
#define FIRST_NAME_ADDRESS        0x800

typedef struct {
  char label;
  uint32_t value;
  uint32_t initial;
} Setting;

typedef struct {
  char label;
  uint32_t value;
  uint8_t size;
  Setting *setting;
  uint8_t *state;
} RecordType;

Setting **get_settings();
RecordType **get_record_types();
void setting_reset();
Setting *setting(char label);
RecordType *record(char label);

#endif // RECORD_H
