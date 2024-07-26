#include <stdio.h>
#include <math.h>

#define MIN_PRESSURE  2600
#define MAX_PRESSURE  102000
#define PRESSURE_STEP   1000

int main (void) {
  int h = 0;

  printf("#ifndef ALTITUDE_H\n#define ALTITUDE_H\n\n");
  printf("#define MIN_PRESSURE %d\n", MIN_PRESSURE);
  printf("#define MAX_PRESSURE %d\n", MAX_PRESSURE);
  printf("#define PRESSURE_STEP %d\n\n", PRESSURE_STEP);

  printf("const int32_t altitude_lut[] = {\n");
  for (int p = MAX_PRESSURE; p >= MIN_PRESSURE; p -= PRESSURE_STEP) {
    h = 4433000 * (1.0 - pow((float)p / 101325, 0.1903));
    printf("  %d, // %d Pa\n", h, p);
  }
  printf("};\n\n#endif //ALTITUDE_H");

  return 0;
}
