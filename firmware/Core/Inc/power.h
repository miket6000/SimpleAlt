#ifndef POWER_H
#define POWER_H

#include <stdint.h>

typedef enum {
    AWAKE = 0,
    SNOOZE = 1,
    SLEEP = 2,
    SHUTDOWN = 3
} PowerLevel;

void power_down(PowerLevel level);
void measure_battery_voltage(void);
uint16_t get_battery_voltage(void);


#endif //POWER_H
