/*
 * cmd.h - Command helpers for anyui SDK
 *
 * This file is created and owned by anyui.
 *
 * COPYRIGHT 2025 anyui Team
 * All rights reserved.
 *
 * https://anyui.tech/
 *
 * Author: anyui Team
 */
#ifndef __CMD_H__
#define __CMD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

#define CMD_BT 1
enum bt_cmd_enum {
  CMD_BT_STATUS = 1,
  CMD_BT_CONNECT = 2,
};

#define CMD_WIFI 2
enum wifi_cmd_enum {
  CMD_WIFI_STATUS = 1,
  CMD_WIFI_CONNECT = 2,
};

#define CMD_BATTERY 3
enum battery_cmd_enum {
  CMD_BATTERY_STATUS = 1,
};

#define CMD_DRAWER 4
enum drawer_cmd_enum {
  CMD_DRAWER_OPEN = 1,
};

#define CMD_SCR 5
enum scr_cmd_enum {
  CMD_NAVIGATE_BACK = 1,
};

uint32_t send_command(uint8_t cmd, uint16_t code, uint16_t type, uint16_t len,
                      uint8_t *data);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
