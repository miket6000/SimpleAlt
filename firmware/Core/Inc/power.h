#ifndef POWER_H
#define POWER_H

#include <stdint.h>

typedef enum {
    SLEEP = 0,
    DEEPSLEEP = 1,
    SHUTDOWN = 2
} PowerLevel;

void power_down(PowerLevel level);
void measure_battery_voltage(void);
uint16_t get_battery_voltage(void);


#endif //POWER_H
