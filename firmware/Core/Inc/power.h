#ifndef POWER_H
#define POWER_H

#include <stdint.h>

typedef enum {
    AWAKE = 0,
    SNOOZE = 1,
    SLEEP = 2,
    SHUTDOWN = 3
} PowerMode;

void power_management();
void power_tick();
void power_idle_reset();
void power_set_timeout(uint32_t timeout);
void power_set_mode(PowerMode mode);
void measure_battery_voltage(void);
uint16_t power_get_battery_voltage(void);


#endif //POWER_H
