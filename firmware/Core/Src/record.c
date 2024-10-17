#include "record.h"

static Setting altitude_sr =       {'a', DEFAULT_ALTITUDE_SR,        DEFAULT_ALTITUDE_SR};
static Setting pressure_sr =       {'p', DEFAULT_PRESSURE_SR,        DEFAULT_PRESSURE_SR};
static Setting temperature_sr =    {'t', DEFAULT_TEMPERATURE_SR,     DEFAULT_TEMPERATURE_SR};
static Setting voltage_sr =        {'v', DEFAULT_VOLTAGE_SR,         DEFAULT_VOLTAGE_SR};
static Setting status_sr =         {'s', DEFAULT_STATUS_SR,          DEFAULT_STATUS_SR};
static Setting power_off_timeout = {'o', DEFAULT_POWER_OFF_TIMEOUT,  DEFAULT_POWER_OFF_TIMEOUT};
static Setting max_altitude =      {'m', 0,                          0};
static Setting name_address =      {'n', FIRST_NAME_ADDRESS,         FIRST_NAME_ADDRESS};

static RecordType altitude =     {'A', 0, 4, &altitude_sr,     NULL};
static RecordType pressure =     {'P', 0, 4, &pressure_sr,     NULL};
static RecordType temperature =  {'T', 0, 2, &temperature_sr,  NULL};
static RecordType voltage =      {'V', 0, 2, &voltage_sr,      NULL};
static RecordType status =       {'S', 0, 1, &status_sr,       NULL};

Setting *settingList[] = {
  &altitude_sr,
  &pressure_sr,
  &temperature_sr,
  &voltage_sr,
  &status_sr,
  &power_off_timeout,
  &max_altitude,
  &name_address,
  NULL
};

RecordType *recordList[] = {
  &altitude,
  &pressure,
  &temperature,
  &voltage,
  &status,
  NULL
};

Setting **get_settings() {
  return settingList;
}

RecordType **get_record_types() {
  return recordList;
}

void setting_reset() {
  uint8_t i = 0;
  while (settingList[i] != NULL) {
    settingList[i]->value = settingList[i]->initial;
    i++;
  }
}

Setting *setting(char label) {
  uint8_t i = 0;
  while (settingList[i] != NULL) {
    if (settingList[i]->label == label) {
      return settingList[i];
    }
    i++;
  }
  return NULL;
}
  
RecordType *record(char label) {
  uint8_t i = 0;
  while (recordList[i] != NULL) {
    if (recordList[i]->label == label) {
      return recordList[i];
    }
    i++;
  }
  return NULL;
}

