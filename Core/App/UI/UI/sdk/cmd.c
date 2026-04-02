/*
 * cmd.c - Command helpers for anyui SDK
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
#include "cmd.h"
#include "evt.h"
#include "scr_list.h"

extern uint32_t process_command(uint8_t cmd, uint16_t code, uint16_t type,
                                uint16_t len, uint8_t *data);

uint32_t send_command(uint8_t cmd, uint16_t code, uint16_t type, uint16_t len,
                      uint8_t *data) {
  LV_LOG_USER("cmd %d code %d type %d len %d\n", cmd, code, type, len);

  switch (cmd) {
  case CMD_DRAWER: {
    switch (code) {
    case CMD_DRAWER_OPEN: {
      LV_LOG_TRACE("cmd drawer open\n");
      if (len > 0) {
        send_event(get_current_event_table(), EVT_DRAWER, EVT_DRAWER_OPEN, data,
                   data[0]);
        send_event(get_global_event_table(), EVT_DRAWER, EVT_DRAWER_OPEN, data,
                   data[0]);
      } else {
        send_event(get_current_event_table(), EVT_DRAWER, EVT_DRAWER_OPEN,
                   &(uint8_t){true}, 1);
        send_event(get_global_event_table(), EVT_DRAWER, EVT_DRAWER_OPEN,
                   &(uint8_t){true}, 1);
      }
      break;
    }
    default:
      LV_LOG_WARN("Invalid command code %d\n", code);
      break;
    }
    break;
  }

  case CMD_SCR: {
    if (count_scr() <= 1) {
      LV_LOG_USER("Cannot pop last screen.\n");
      return 0;
    }
    (void)pop_scr();
    lv_obj_clean(lv_scr_act());
    setup_func_t setup = current_scr();
    lv_scr_load(setup());
    break;
  }

  default: {
    process_command(cmd, code, type, len, data);
    break;
  }
  }

  return 0;
}
