#ifndef TASK_H
#define TAKS_H
#include <stdint.h>

#define MAX_NUM_TASK  20

typedef struct {
  void (*callback)(void *);
  void *param;
  uint32_t delay;
  uint32_t nextExecutionTime;
} Task;

int add_task(Task task);
void execute_task();

#endif //TASK_H
