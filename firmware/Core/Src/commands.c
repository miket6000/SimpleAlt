#include "command.h"
#include "commands.h"
#include "filesystem.h"
#include "record.h"
#include <stdlib.h>
#include <string.h>

void print_uint32(void *parameter) {
  char buffer[8];
  itoa(*(uint32_t *)parameter, buffer, 10);
  print(buffer, strlen(buffer));
}

void print_int32(void *parameter) {
  char buffer[8];
  itoa(*(int32_t *)parameter, buffer, 10);
  print(buffer, strlen(buffer));
}

void print_uint16(void *parameter) {
  char buffer[8];
  itoa(*(uint16_t *)parameter, buffer, 10);
  print(buffer, strlen(buffer));
}

void print_int16(void *parameter) {
  char buffer[8];
  itoa(*(int16_t *)parameter, buffer, 10);
  print(buffer, strlen(buffer));
}

void print_uint8(void *parameter) {
  char buffer[8];
  itoa(*(uint8_t *)parameter, buffer, 10);
  print(buffer, strlen(buffer));
}

void print_int8(void *parameter) {
  char buffer[8];
  itoa(*(int8_t *)parameter, buffer, 10);
  print(buffer, strlen(buffer));
}

// W address bytes
void write_flash(void *parameter) {
  char* param;
  uint32_t address = 0;
  uint8_t len = 0;
  uint8_t flash_buffer[64];

  param = cmd_get_param();
  if (param != NULL) {
    address = atoi(param);
  }

  param = cmd_get_param();
  // WARNING NO CHECKS!!
  while (param != NULL) {
    flash_buffer[len++] = atoi(param);
    param = cmd_get_param();
  }

  if (len > 0) {
    fs_raw_write(address, flash_buffer, len);
  } 
}

// R address bytes
void read_flash(void *parameter) {
  uint32_t address;
  uint8_t len;
  uint8_t flash_buffer[64];

  char str_buf[3] = {0};
  
  address = atoi(cmd_get_param());
  len = atoi(cmd_get_param());

  if (len > 0) {
    fs_raw_read(address, flash_buffer, len);
    for (int i = 0; i < len; i++) {
      if (flash_buffer[i] < 0x10) {
        print("0",1);
      }
      print(itoa(flash_buffer[i], str_buf, 16), strlen(str_buf));
    }
  }
}

// r address bytes
void read_flash_binary(void *parameter) {
  uint32_t address;
  uint8_t len;
  uint8_t flash_buffer[64];
  
  address = atoi(cmd_get_param());
  len = atoi(cmd_get_param());

  if (len > 0 && len <= sizeof(flash_buffer)) {
    fs_raw_read(address, flash_buffer, len);
    print((char *)flash_buffer, len);
  }
}

void erase_flash(void *parameter) {
  // delete everything
  fs_erase();

  // restore current config so it can be loaded on power up
  uint8_t i = 0;
  Setting **settingList = get_settings();
  while (settingList[i] != NULL) {
    fs_save_config(settingList[i]->label, &settingList[i]->value);
    i++;
  }
  print("OK", 2);
}

void factory_reset(void *parameter) {
  // set the default config, then call erase flash. 
  setting_reset(); 
  erase_flash(NULL);
}

void reboot(void *parameter) {
  HAL_NVIC_SystemReset();
}

void set_config(void *parameter) {
  char *label = cmd_get_param();
  uint32_t value = atoi(cmd_get_param());
  fs_save_config(label[0], &value);
  Setting *s = setting(label[0]);
  if (s != NULL) {
    s->value = value;
    print("OK", 2);
  } else {
    print("ERR", 3);
  }
}

void get_config(void *parameter) {
  char *label = cmd_get_param();
  uint32_t value = 0xFFFFFFFF;
  fs_read_config(label[0], &value);
  char str_buf[10] = {0};
  print(itoa(value, str_buf, 10), strlen(str_buf));
}

void get_uid(void *parameter) {
  uint32_t uid = *((uint32_t *)parameter);
  char str_buf[10] = {0};
  print(itoa(uid, str_buf, 16), strlen(str_buf));
}

