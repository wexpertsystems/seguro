/// @file events.c
///
/// Definitions for functions which manage events.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "event.h"

//==============================================================================
// Functions
//==============================================================================

void fragment_event(Event *event, FragmentedEvent *f_event) {
  uint32_t num_fragments;
  uint16_t payload_length;
  uint8_t header_length;

  // Split event into as many optimally sized fragments as possible, and always
  // put the oddly-sized fragment at the front. Thus, every fragment after the
  // first one will be exactly OPTIMAL_VALUE_SIZE bytes long, whereas the
  // payload of the first fragment may be as small as 1 byte or as large as
  // OPTIMAL_VALUE_SIZE bytes.
  num_fragments = (event->data_length / OPTIMAL_VALUE_SIZE);
  payload_length = (event->data_length % OPTIMAL_VALUE_SIZE);

  // Tuning opportunities here (e.g. if X < 1000, payload of 1st fragment =
  // (OPTIMAL_VALUE_SIZE + X))
  if (!payload_length) {
    payload_length = OPTIMAL_VALUE_SIZE;
  } else {
    ++num_fragments;
  }

  // Each fragment data array is just a pointer to an index in the existing raw
  // event data array
  uint8_t **fragments = (uint8_t **)malloc(sizeof(uint8_t *) * num_fragments);
  fragments[0] = event->data;
  for (uint32_t i = 1; i < num_fragments; ++i) {
    fragments[i] =
        (event->data + payload_length + ((i - 1) * OPTIMAL_VALUE_SIZE));
  }

  // Header encodes number of ADDITIONAL fragments
  header_length = build_header(f_event->header, num_fragments - 1);

  // Setup remaining members of fragmented event
  f_event->id = event->id;
  f_event->num_fragments = num_fragments;
  f_event->header_length = header_length;
  f_event->payload_length = payload_length;
  f_event->fragments = fragments;
}

uint8_t build_header(uint8_t *header, uint32_t num_fragments) {
  // HEADER   MAX # FRAGS   MAX EVENT SIZE
  // 1 byte   128                1280000 bytes (  1.28 MB)
  // 2 byte   256                2560000 bytes (  2.56 MB)
  // 3 byte   65536            655360000 bytes (655.36 MB)
  // 4 byte   16777216      167772160000 bytes (~168 GB)
  // ...
  //
  // Thus, the header size is determined by the number of fragments necessary to
  // store the event (which is indirectly tied to the OPTIMAL_VALUE_SIZE macro
  // [10,000 bytes for FDB]). We can also set an upper limit using the
  // MAX_HEADER_SIZE macro for event sizes we expect to never be reached (e.g.
  // maximum header size of 4 if we never expect to see an event larger than
  // ~168GB).
  //
  if (num_fragments < 128) {
    header[0] = ((uint8_t *)&num_fragments)[0];
    return 1;
  }

  for (uint8_t i = 1; i < (MAX_HEADER_SIZE - 1); ++i) {
    if (num_fragments < ((uint32_t)(1 << (i * 8)))) {
      header[0] = (EXTENDED_HEADER | i);
      memcpy((header + 1), &num_fragments, i);
      return (i + 1);
    }
  }

  // Technically this is wrong if num_fragments >= 2^24
  header[0] = (EXTENDED_HEADER | (MAX_HEADER_SIZE - 1));
  memcpy((header + 1), &num_fragments, (MAX_HEADER_SIZE - 1));
  return MAX_HEADER_SIZE;
}

uint8_t read_header(const uint8_t *header, uint32_t *num_fragments) {
  if (header[0] & EXTENDED_HEADER) {
    uint8_t header_bytes = (header[0] ^ EXTENDED_HEADER);
    memcpy((uint8_t *)num_fragments, (header + 1), header_bytes);

    return (header_bytes + 1);
  }

  *num_fragments = header[0];
  return 1;
}

void free_event(Event *event) { free((void *)event->data); }

void free_fragmented_event(FragmentedEvent *event) {
  free((void *)event->fragments);
}
