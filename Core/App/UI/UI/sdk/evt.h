/*
 * evt.h - Event helpers for anyui SDK
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
#ifndef __EVT_H__
#define __EVT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

#define EVT_BT 1
enum bt_evt_enum {
  EVT_BT_CHANGED = 1,
};

#define EVT_WIFI 2
enum wifi_evt_enum {
  EVT_WIFI_CHANGED = 1,
};

#define EVT_BATTERY 3
enum battery_evt_enum {
  EVT_BATTERY_CHANGED = 1,
};

#define EVT_DRAWER 4
enum drawer_evt_enum {
  EVT_DRAWER_OPEN = 1,
};

#define EVT_TIME 5
enum time_evt_enum {
  EVT_TIME_CHANGED = 1,
};

typedef void (*event_cb_func_t)(uint8_t event, uint16_t code, uint16_t type,
                                uint16_t len, uint8_t *data, uint8_t status);

typedef struct event_node_t {
  uint8_t event;
  uint16_t code;
  uint16_t type;
  uint16_t len;
  event_cb_func_t callback;
  struct event_node_t *next;
} event_node_t;

typedef struct event_table_t {
  event_node_t *head;
} event_table_t;

uint32_t register_event(event_table_t *table, uint8_t event, uint16_t code,
                        uint16_t type, uint16_t len, event_cb_func_t callback);

uint32_t unregister_events(event_table_t *table);

uint32_t send_event(event_table_t *table, uint8_t event, uint16_t code,
                    uint8_t *data, uint8_t status);

void set_current_event_table(event_table_t *table);
event_table_t *get_current_event_table();

event_table_t *get_global_event_table();
void print_event_table(event_table_t *table);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
