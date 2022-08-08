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
#include "fragment.h"


//==============================================================================
// Functions
//==============================================================================

//! @n (1) Set the FragmentedEvent object's integer attributes.
//! @n (2) Allocate memory for fragments, and generate them.
int fragment_event(Event event, FragmentedEvent* fe) {
  int num_fragments = get_num_fragments(event);
  fe->event_id = event.key;
  fe->num_fragments = num_fragments;

  // TODO: implement this.
  FDBKeyValue* fragments = (FDBKeyValue *) \
    malloc(sizeof(FDBKeyValue) * num_fragments);
  for (int i = 0; i < num_fragments; i++) {
    // Generate and set key (`"<event id>:<fragment id>"`).
    sprintf(fragments[i].key, "%d:%d", event.key, 1);
    // Set key length.
    fragments[i].key_length = strlen(fragments[i].key);
    // Generate and set value (`<fragment header><fragment payload>`).
    int header = 0;
    int payload = 0;
    sprintf(fragments[i].value, "%d%d", header, payload);
    // Set value length.
    fragments[i].value_length = strlen(fragments[i].value);
  } // (2)

  return 0;
}

//! @n (1) Check if the default header size is sufficient.
//! @n (2) If not, calculate the appropriate custom size.
int get_header_size(Event event) {
  int default_header_size = 4;
  int default_max_fragments = 127;
  int default_free_value_space = MAX_VALUE_SIZE - default_header_size;
  int num_fragments = (int) ceil((double) event.value_length / \
    (double) default_free_value_space);
  if (num_fragments < default_max_fragments) {
    return default_header_size;
  } // (1)

  int num_bits = (int) log2(num_fragments) + 1;
  int num_bytes = (int) ceil((double) num_bits / 8.0) + 1;
  int custom_size = 2 * num_bytes + 2;
  return custom_size; // (2)
}

int get_num_fragments(Event event) {
  int header_size = get_header_size(event);
  int free_value_space = MAX_VALUE_SIZE - header_size;
  int num_fragments = (int) ceil((double) event.value_length / \
    (double) free_value_space);
  return num_fragments;
}