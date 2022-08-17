//! @file events.c
//!
//! Generates or reads events from LMDB, and loads them into memory.

#include <foundationdb/fdb_c.h>
#include <stdint.h>
#include <stdio.h>

#include "event.h"
#include "fragment.h"


//==============================================================================
// Functions
//==============================================================================

void event_set_transaction(FDBTransaction *tx, Event *event) {

  int key_length = get_key_length(event);
  uint8_t key[key_length];
  sprintf((char *)key, "%d", event->key);

  fdb_transaction_set(tx,
                      key,
                      key_length,
                      event->fragments[0].data,
                      event->fragments[0].data_length);
}

void event_clear_transaction(FDBTransaction *tx, Event *event) {

  int key_length = get_key_length(event);
  uint8_t key[key_length];
  sprintf((char *)key, "%d", event->key);

  fdb_transaction_clear(tx, key, key_length);
}

int get_key_length(Event *event) {
  return snprintf(NULL, 0, "%d", event->key);
}
