#include "task.h"
#include "tim.h"

static Task taskList[MAX_NUM_TASK];
static uint8_t lastTask = 0;

Task *add_task(Task task) {
  uint32_t time = HAL_GetTick();
  if (lastTask + 1 > MAX_NUM_TASK) {
    return NULL;
  }
  task.nextExecutionTime = time + task.delay;
  taskList[lastTask++] = task;
  return &taskList[lastTask - 1];
}

void execute_task() {
  uint32_t time = HAL_GetTick();
  for (uint8_t task = 0; task < lastTask; task++) {
    if (time >= taskList[task].nextExecutionTime) {
      taskList[task].nextExecutionTime = time + taskList[task].delay;
      taskList[task].callback(taskList[task].param);
    }
  }
}

