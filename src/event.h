//! @file events.h
//!
//! Event struct definitions and declarations for functions which manage events.

#pragma once

#include <foundationdb/fdb_c.h>
#include <stdint.h>

#define EXTENDED_HEADER 0x80
#define MAX_HEADER_SIZE 4


//==============================================================================
// Types
//==============================================================================

typedef struct event_t {
  uint64_t  id;           // Unique, ordered identifier for event
  uint64_t  data_length;  // Length of event data in bytes
  uint8_t  *data;         // Pointer to event data array
} Event;

typedef struct fragmented_event_t {
  uint64_t   id;                      // Unique, ordered identifier for event
  uint32_t   num_fragments;           // Number of fragments into which the event has been split
  uint8_t    header[MAX_HEADER_SIZE]; // Header for first fragment which encodes the number of fragments
  uint8_t    header_length;           // Length of header in bytes
  uint16_t   payload_length;          // Length of data payload of first fragment
  uint8_t  **fragments;               // Array of fragments as pointers into raw event data array
} FragmentedEvent;

//==============================================================================
// Prototypes
//==============================================================================

//! Split an event into one or more fragments. This is necessary for improved performance when writing to a database,
//! or for the database to accept the event at all.
//!
//! @param[in] event    The event to split into one or more fragments
//! @param[in] f_event  Pointer to the fragmented event to setup using the input event
void fragment_event(Event *event, FragmentedEvent *f_event);

//! Create the header for a fragmented event, which stores the number of fragments of which an event is composed.
//!
//! @param[in] header         Pointer to the byte array to which to output the header
//! @param[in] num_fragments  The number of fragments that the header should encode
//!
//! @return   The length of the header in bytes
uint8_t build_header(uint8_t *header, uint32_t num_fragments);

//! Read the total number of fragments for an event from the header.
//!
//! @param[in] header         Handle for the header
//! @param[in] num_fragments  Handle for location to which to write the number of fragments
//!
//! @return   The length of the header in bytes
uint8_t read_header(const uint8_t *header, uint32_t *num_fragments);

//! Deallocate the heap memory used by an event.
//!
//! @param[in] event  The event to deallocate
void free_event(Event *event);

//! Deallocate the heap memory used by a fragmented event.
//!
//! @param[in] event  The fragmented event to deallocate
void free_fragmented_event(FragmentedEvent *event);
