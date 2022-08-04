//! @file fragment.c
//!
//! Splits large events (which exceed the maximum size limit of the database) 
//! into multiple fragments for storage in FoundationDB key/value pairs.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "db/fdb.h"
#include "event.h"


//==============================================================================
// Functions
//==============================================================================

FDBKeyValue* fragment_event(Event event) {
  if (event.value_length < MAX_VALUE_SIZE) {
    // Create a FDBKeyValue and return a pointer to it.
  }

  return NULL;
}

int get_header_size(Event event) {
  // Check if the default header size is sufficient.
  int default_size = 4;
  int max_default_fragments = 127;
  int usable_default_value_space = MAX_VALUE_SIZE - default_size;
  int num_fragments = (int) ceil((double) event.value_length / \
    (double) usable_default_value_space);
  if (num_fragments < max_default_fragments) {
    return default_size;
  }

  // If not, calculate the custom size.
  int num_bits = (int) log2(num_fragments) + 1;
  int num_bytes = (int) ceil((double) num_bits / 8.0) + 1;
  int custom_size = 2 * num_bytes + 2;

  return default_size;
}

int get_num_fragments(Event event) {
  int header_size = get_header_size(event);
  int usable_value_space = MAX_VALUE_SIZE - header_size;
  int num_fragments = (int) ceil((double) event.value_length / \
    (double) usable_value_space);
  return num_fragments;
}