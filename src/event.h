//! @file events.h
//!
//! Event struct definition and utilities.

#pragma once

#include <foundationdb/fdb_c.h>
#include <stdint.h>

#include "fragment.h"


//==============================================================================
// Types
//==============================================================================

typedef struct event_t {
  // TODO: uint64_t for key?
  uint32_t  key;
  uint32_t  num_fragments;
  Fragment *fragments;
} Event;

//==============================================================================
// Prototypes
//==============================================================================

void event_set_transaction(FDBTransaction *tx, Event *event);

void event_clear_transaction(FDBTransaction *tx, Event *event);

//! Computes the length of the FoundationDB key for an event.
//!
//! @param[in] n  The event.
//!
//! @return  The length of the key.
int get_key_length(Event *event);
