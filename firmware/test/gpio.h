#ifndef GPIO_H
#define GPIO_H

#define GPIO_PIN_SET  1
#define GPIO_PIN_RESET  0
#define LED_Pin 0
#define LED_GPIO_Port 0


void HAL_GPIO_WritePin(int port, int pin, int state);
void HAL_GPIO_TogglePin(int port, int pin);

#endif //GPIO_H
