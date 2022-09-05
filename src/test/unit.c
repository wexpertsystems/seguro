//! @file test.c
//!
//! Unit tests for Seguro

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../constants.h"
#include "../event.h"


//==============================================================================
// Prototypes
//==============================================================================

//! Test splitting events into one or more fragments.
void test_fragment_event(void);

//! Test fragmentation for a 1-byte event.
void test_fragment_event_trivial(void);

//! Test fragmentation for the largest single-fragment event.
void test_fragment_event_small(void);

//! Test fragmentation for an event with multiple fragments and an odd-sized leftover payload.
void test_fragment_event_large(void);

//! Test fragmented event header construction.
void test_build_header(void);

//==============================================================================
// Functions
//=============================================================================

//! Execute the Seguro unit tests.
//!
//! @param[in] argc  Number of command-line options provided.
//! @param[in] argv  Array of command-line options provided.
//!
//! @return  0  Success.
//! @return -1  Failure.
int main(int argc, char **argv) {

  printf("Starting unit tests...\n");

  // Run tests
  test_fragment_event();
  test_build_header();

  // Success
  printf("\nUnit tests completed successfully.\n");
  return 0;
}

void test_fragment_event(void) {

  printf("\nStarting event fragmentation tests...\n");

  test_fragment_event_trivial();
  test_fragment_event_small();
  test_fragment_event_large();

  printf("Completed event fragmentation tests.\n");
}

void test_fragment_event_trivial(void) {

  FragmentedEvent f_event;

  // Setup event
  uint64_t  key = 123;
  uint8_t   data[1];
  Event     event = { key, 1, data };

  // Fragment event
  fragment_event(&event, &f_event);

  // Confirm correct state
  printf("\ttrivial event fragmentation... ");

  assert(f_event.key == key);
  assert(f_event.num_fragments == 1);
  assert(f_event.header[0] == 0);
  assert(f_event.header_length == 1);
  assert(f_event.payload_length == 1);
  assert(f_event.fragments[0] == data);

  printf(" PASSED\n");
}

void test_fragment_event_small(void) {

  FragmentedEvent f_event;

  // Setup event
  uint64_t  key = 456;
  uint16_t  data_length = OPTIMAL_VALUE_SIZE;
  uint8_t   data[data_length];
  Event     event = { key, data_length, data };

  // Fragment event
  fragment_event(&event, &f_event);

  // Confirm correct state
  printf("\tmax size single fragment... ");

  assert(f_event.key == key);
  assert(f_event.num_fragments == 1);
  assert(f_event.header[0] == 0);
  assert(f_event.header_length == 1);
  assert(f_event.payload_length == data_length);
  assert(f_event.fragments[0] == data);

  printf(" PASSED\n");
}

void test_fragment_event_large(void) {

  FragmentedEvent f_event;

  // Setup event
  uint64_t  key = 789;
  uint16_t  data_length = (3 * OPTIMAL_VALUE_SIZE) + 1;
  uint8_t   data[data_length];
  Event     event = { key, data_length, data };

  uint8_t   num_fragments = 4;

  // Set first byte of each fragment to 1 (all others are 0)
  data[0] = 1;
  for (uint32_t i = 0; i < (data_length - 1); ++i) {
    if (i % OPTIMAL_VALUE_SIZE) {
      data[(i + 1)] = 0;
    } else {
      data[(i + 1)] = 1;
    }
  }

  // Fragment event
  fragment_event(&event, &f_event);

  // Confirm correct state
  printf("\tmultiple fragments w/ leftover... ");

  assert(f_event.key == key);
  assert(f_event.num_fragments == num_fragments);
  assert(f_event.header[0] == (num_fragments - 1));
  assert(f_event.header_length == 1);
  assert(f_event.payload_length == 1);
  assert(f_event.fragments[0][0] == 1);
  assert(f_event.fragments[0][1] == 1);
  assert(f_event.fragments[0][(OPTIMAL_VALUE_SIZE + 1)] == 1);
  assert(f_event.fragments[0][((2 * OPTIMAL_VALUE_SIZE) + 1)] == 1);

  printf(" PASSED\n");
}

void test_build_header(void) {

  uint8_t header[MAX_HEADER_SIZE] = { 0 };
  uint8_t header_length = 0;

  printf("\nStarting event header tests...\n");
  printf("\tbuilding headers... ");

  // 0
  header_length = build_header(header, 0);
  assert(header_length == 1);
  assert(header[0] == 0);

  // 127
  header_length = build_header(header, 127);
  assert(header_length == 1);
  assert(header[0] == 127);

  // 128
  header_length = build_header(header, 128);
  assert(header_length == 2);
  assert(header[0] == (EXTENDED_HEADER & 1));
  assert(header[1] == 128);

  // 255
  header_length = build_header(header, 255);
  assert(header_length == 2);
  assert(header[0] == (EXTENDED_HEADER & 1));
  assert(header[1] == 255);

  // 256
  header_length = build_header(header, 256);
  assert(header_length == 3);
  assert(header[0] == (EXTENDED_HEADER & 2));
  assert(header[1] == 0);
  assert(header[2] == 1);

  // 65535
  header_length = build_header(header, 65535);
  assert(header_length == 3);
  assert(header[0] == (EXTENDED_HEADER & 2));
  assert(header[1] == 255);
  assert(header[2] == 255);

  // 65536
  header_length = build_header(header, 65536);
  assert(header_length == 4);
  assert(header[0] == (EXTENDED_HEADER & 3));
  assert(header[1] == 0);
  assert(header[2] == 0);
  assert(header[3] == 1);

  // 16777215
  header_length = build_header(header, 16777215);
  assert(header_length == 4);
  assert(header[0] == (EXTENDED_HEADER & 3));
  assert(header[1] == 255);
  assert(header[2] == 255);
  assert(header[3] == 255);

  // 16777216
  header_length = build_header(header, 16777216);
  assert(header_length == 4);
  assert(header[0] == (EXTENDED_HEADER & 3));
  assert(header[1] == 0);
  assert(header[2] == 0);
  assert(header[3] == 0);

  printf(" PASSED\n");
  printf("Completed event header tests.\n");
}
