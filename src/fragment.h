//! @file fragment.h
//!
//! Splits large events (which exceed the maximum size limit of the database) 
//! into multiple fragments for storage in FoundationDB key/value pairs.

#ifndef FRAGMENT_H
#define FRAGMENT_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "event.h"

//==============================================================================
// Types
//==============================================================================

typedef struct fragmented_event_t {
  uint8_t      event_id;
  FDBKeyValue* fragments;
  uint8_t      num_fragments;
} FragmentedEvent;

//==============================================================================
// Prototypes
//==============================================================================

//! Fragments a raw event into an array of key/value fragments which 
//! are ready to be inserted into FoundationDB.
//!
//! @param[in] event  The raw event to fragment.
//!
//! @return       Fragmented event object.
//! @return NULL  Failure.
int fragment_event(Event event, FragmentedEvent* fe);

//! Returns the number of bytes required for the given event's fragment 
//! header.
//!
//! @param[in] event  The event.
//!
//! @return    The number of bytes required for fragment header.
//! @return 0  Error.
int get_header_size(Event event);

//! Returns the number of fragments required to split the given event.
//!
//! @param[in] event  The event.
//!
//! @return    The number of fragments required to split the event.
//! @return 0  Error.
int get_num_fragments(Event event);

#endif