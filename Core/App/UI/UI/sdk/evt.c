/*
 * evt.c - Event helpers for anyui SDK
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

#include "evt.h"
#include "mem.h"

static event_table_t *current_event_table = NULL;
static event_table_t global_event_table = {0};

typedef void (*event_handler_func_t)(uint8_t event, uint16_t code,
                                     uint16_t type, uint16_t len, uint8_t *data,
                                     uint8_t status);
static event_node_t *create_event(uint8_t event, uint16_t code, uint16_t type,
                                  uint16_t len, event_cb_func_t callback);
static event_node_t *find_event(event_node_t *table_ptr, uint8_t event,
                                uint16_t code, event_cb_func_t callback);
static uint32_t insert_event_at_head(event_node_t **table_pptr, uint8_t event,
                                     uint16_t code, uint16_t type, uint16_t len,
                                     event_cb_func_t callback);

uint32_t send_event(event_table_t *table, uint8_t event, uint16_t code,
                    uint8_t *data, uint8_t status) {
  if (table == NULL) {
    LV_LOG_USER("table is NULL\n");
    return 1;
  }

  LV_LOG_USER("event %d code %d\n", event, code);
  event_node_t *tbl_ptr = table->head;

  while (tbl_ptr != NULL) {
    if (tbl_ptr->event == event && tbl_ptr->code == code) {
      tbl_ptr->callback(event, code, tbl_ptr->type, tbl_ptr->len, data, status);
    }
    tbl_ptr = tbl_ptr->next;
  }

  return 0;
}

static event_node_t *find_event(event_node_t *table_ptr, uint8_t event,
                                uint16_t code, event_cb_func_t callback) {
  event_node_t *tbl_ptr = table_ptr;

  while (tbl_ptr != NULL) {
    if (tbl_ptr->event == event && tbl_ptr->code == code &&
        tbl_ptr->callback == callback) {
      return tbl_ptr;
    }
    tbl_ptr = tbl_ptr->next;
  }
  return NULL;
}

static event_node_t *create_event(uint8_t event, uint16_t code, uint16_t type,
                                  uint16_t len, event_cb_func_t callback) {
  event_node_t *table = (event_node_t *)LV_MEM_ALLOC(sizeof(event_node_t));
  if (table == NULL) {
    LV_LOG_ERROR("LV_MEM_ALLOC failed\n");
    return NULL;
  }
  table->event = event;
  table->code = code;
  table->type = type;
  table->len = len;
  table->callback = callback;
  table->next = NULL;

  return table;
}

/**
 * Inserts a new event node at the head of the event table.
 *
 * @param table_pptr A pointer to a pointer to the head of the event table.
 * @param event The event type to be inserted.
 * @param code The event code to be inserted.
 * @param type The event type to be inserted.
 * @param len The length of the event data.
 * @param callback The callback function to be associated with the event.
 * @return
 *  0 if the event was inserted successfully,
 *  1 if the table pointer is NULL,
 *  2 if the event already exists in the table.
 */
static uint32_t insert_event_at_head(event_node_t **table_pptr, uint8_t event,
                                     uint16_t code, uint16_t type, uint16_t len,
                                     event_cb_func_t callback) {
  if (table_pptr == NULL) {
    LV_LOG_ERROR("table is NULL\n");
    return 1;
  }

  if (find_event(*table_pptr, event, code, callback) != NULL) {
    LV_LOG_USER("event %d code %d already exists\n",
                event, code);
    return 2;
  }

  event_node_t *event_node_ptr = create_event(event, code, type, len, callback);

  if (*table_pptr == NULL) {
    *table_pptr = event_node_ptr;
  } else {
    event_node_ptr->next = *table_pptr;
    *table_pptr = event_node_ptr;
  }

  return 0;
}

/**
 * Registers a new event in the event table.
 *
 * This function takes an event table, an event type, an event code, an event
 * type, a length, and a callback function as parameters. It first checks if the
 * event table is NULL and prints an error message if it is. Then it prints the
 * event details and calls the insert_event_at_head function to insert the event
 * at the head of the event table.
 *
 * @param table A pointer to the event table where the event will be registered.
 * @param event The event type to be registered.
 * @param code The event code to be registered.
 * @param type The event type to be registered.
 * @param len The length of the event data.
 * @param callback The callback function to be associated with the event.
 * @return 0 if the event was registered successfully, 1 if the table pointer is
 * NULL.
 */
uint32_t register_event(event_table_t *table, uint8_t event, uint16_t code,
                        uint16_t type, uint16_t len, event_cb_func_t callback) {
  if (table == NULL) {
    LV_LOG_ERROR("register_event, table is NULL\n");
    return 1;
  }

  // printf("register_event: event=%d, code=%d, type=%d, len=%d\n", event, code,
  //        type, len);

  return insert_event_at_head(&table->head, event, code, type, len, callback);
}

/**
 * Unregisters all events from the given event table.
 *
 * This function iterates through the event table, freeing each event node
 * and setting the table's head to NULL.
 *
 * @param table A pointer to the event table to be unregistered.
 * @return 0 if the unregistration was successful, 1 if the table pointer is
 * NULL.
 */
uint32_t unregister_events(event_table_t *table) {
  if (table == NULL) {
    LV_LOG_ERROR("table is NULL\n");
    return 1;
  }
  event_node_t *event_node_ptr = table->head;
  while (event_node_ptr != NULL) {
    event_node_t *temp = event_node_ptr;
    event_node_ptr = event_node_ptr->next;
    LV_MEM_FREE(temp);
  }
  table->head = NULL;
  return 0;
}

void set_current_event_table(event_table_t *table) {
  current_event_table = table;
}

event_table_t *get_current_event_table() { return current_event_table; }

event_table_t *get_global_event_table() {
  return &global_event_table;
}

void print_event_table(event_table_t *table) {
  if (table == NULL) {
    LV_LOG_ERROR("table is NULL\n");
    return;
  }
  LV_LOG_TRACE("%p\n", table);
  event_node_t *event_node_ptr = table->head;
  while (event_node_ptr != NULL) {
    LV_LOG_TRACE("event=%d, code=%d, type=%d, len=%d\n",
                 event_node_ptr->event, event_node_ptr->code,
                 event_node_ptr->type, event_node_ptr->len);
    event_node_ptr = event_node_ptr->next;
  }
}
