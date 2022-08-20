//! @file events.h
//!
//! Event struct definition and utilities.

#pragma once

#include <foundationdb/fdb_c.h>
#include <stdint.h>

#define EXTENDED_HEADER 0x80
#define MAX_HEADER_SIZE 4
#define MAX_NUM_FRAGMENTS 16777216


//==============================================================================
// Types
//==============================================================================

typedef struct event_t {
  uint64_t  key;
  uint64_t  data_length;
  uint8_t  *data;
} Event;

typedef struct fragmented_event_t {
  uint64_t   key;
  uint32_t   num_fragments;
  uint8_t    header[MAX_HEADER_SIZE];
  uint8_t    header_length;
  uint16_t   payload_length;
  uint8_t  **fragments;
} FragmentedEvent;

//==============================================================================
// Prototypes
//==============================================================================

void fragment_event(Event *event, FragmentedEvent *f_event);

uint8_t build_fragment_header(uint8_t *header, uint32_t num_fragments);

void free_event(Event *event);

void free_fragmented_event(FragmentedEvent *event);
