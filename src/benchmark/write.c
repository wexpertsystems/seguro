//! @file benchmark.c
//!
//! Write benchmark suite for Seguro Phase 2.
//!
//! Documentation links:
//!   https://www.gnu.org/software/libc/manual/html_node/Using-Getopt.html
//!   https://linux.die.net/man/3/getopt_long
//!   https://apple.github.io/foundationdb/benchmarking.html

#include <foundationdb/fdb_c.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "../constants.h"
#include "../event.h"
#include "../fdb.h"
#include "../fdb_timer.h"


//==============================================================================
// Types
//==============================================================================

typedef struct data_config_t {
  uint32_t num_events;
  uint32_t event_size;
} DataConfig;

//==============================================================================
// Prototypes
//==============================================================================

//! Run the default benchmarking suite for Seguro.
void run_benchmarks(void);

//! Run the default event write benchmarks.
//!
//! @param[in] config   Configuration settings for the benchmark test
void run_write_benchmark(DataConfig config);

//! Write an array of events to a FoundationDB cluster and time the process.
//!
//! @param[in] events       Array of events to write
//! @param[in] num_events   Number of events in array
//! @param[in] num_frags    Number of fragments per event
//! @param[in] batch_size   Batch size of writes per FoundationDB transaction
void timed_array_write(FragmentedEvent* events, uint32_t num_events, uint32_t num_frags, uint32_t batch_size);

//! Generate an array of mock events and fragment them.
//!
//! @param[in] events       Handle for an array of events
//! @param[in] f_events     Handle for an array of fragmented events
//! @param[in] num_events   Number of events in array
//! @param[in] size         The size of each event, in bytes
void load_mock_events(Event **events, FragmentedEvent **f_events, uint32_t num_events, uint32_t size);

//! Releases memory allocated for mock events.
//!
//! @param[in] events       Array of raw events to free
//! @param[in] f_events     Array of fragmented events to free
//! @param[in] num_events   Number of events in arrays
void release_events_memory(Event *events, FragmentedEvent *f_events, uint32_t num_events);

//! Print that a fatal error occurred and exit.
void fatal_error(void);

//! Parse a positive integer from a string.
//!
//! @param[in] str  The string to parse.
//!
//! @return     A positive integer
//! @return 0   Failure
uint32_t parse_pos_int(char const *str);

//! Count the number of total fragments in an array of fragmented events.
//!
//!
//! @param[in]  events      Pointer to the array of FragmentedEvent objects.
//! @param[in]  num_events  Number of events in the array.
//!
//! @return  Number of total fragments.
uint32_t total_fragments(FragmentedEvent *events, uint32_t num_events);

//==============================================================================
// Functions
//==============================================================================

//! Execute the Seguro write benchmark suite.
//!
//! @param[in] argc  Number of command-line options provided.
//! @param[in] argv  Array of command-line options provided.
//!
//! @return  0  Success
//! @return -1  Failure (error occurred)
int main(int argc, char **argv) {

  // Run benchmark(s)
  printf("Seguro Phase 2 benchmarks...\n");

  // Initialize FoundationDB database
  fdb_init_database();
  fdb_init_network_thread();

  // Run benchmarks
  run_benchmarks();

  // Clean up FoundationDB database
  printf("Tearing down database...\n");
  fdb_shutdown_network_thread();
  fdb_shutdown_database();

  // Success
  printf("Benchmarks completed.\n");
  return 0;
}

void run_benchmarks(void) {

  printf("Running default benchmarks...\n");

  int num_configs = 4;
  DataConfig configs[] = {
    // n     , size (bytes)
    { 1000   , 1    },
    { 10000  , 10   },
    { 10000  , 100  },
    { 100000 , 1000 },
  };

  for (uint8_t i = 0; i < num_configs; ++i) {
    run_write_benchmark(configs[i]);
  }

  printf("\nDefault benchmarks complete.\n");
}

void run_write_benchmark(DataConfig config) {

  Event           *raw_events;
  FragmentedEvent *events;

  // Size of transaction cannot exceed 10,000,000 bytes (10MB) of "affected data" (e.g. keys + values + ranges for
  // write, keys + ranges for read). Therefore, batch size cannot exceed 1000 with OPTIMAL_VALUE_SIZE of 10,000 bytes
  // (10KB).
  uint32_t batch_sizes[] = { 1, 10, 100, 500, 950 };
  uint32_t num_bs = 5;
  uint32_t num_events = config.num_events;
  uint32_t event_size = config.event_size;
  uint16_t num_fragments = (uint16_t) ceil((double) event_size / (double) OPTIMAL_VALUE_SIZE);

  printf("\nRunning write benchmarks for %u-fragment events...\n\n", num_fragments);

  // Generate mock events
  load_mock_events(&raw_events, &events, num_events, event_size);

  // Array batch writes for each batch size
  for (uint8_t i = 0; i < num_bs; ++i) {
    timed_array_write(events, num_events, num_fragments, batch_sizes[i]);
  }

  // Clean up heap
  release_events_memory(raw_events, events, num_events);

  // Success
  printf("\nWrite benchmarks for %d-fragment events complete.\n", num_fragments);
}

void timed_array_write(FragmentedEvent* events, uint32_t num_events, uint32_t num_frags, uint32_t batch_size) {

  time_t   start, end;
  clock_t  c_start, c_end;
  uint32_t progress_bar_increment = ((num_events * num_frags) / 100);

  printf("Running batch size %d benchmark...\n", batch_size);

  fdb_set_batch_size(batch_size);

  // Write array of events in batches, and print a bar as a visual indicator of progress
  time(&start);
  c_start = clock();

  for (uint32_t i = 0; i < 100; ++i) {
    // printf("[");
    // for (uint32_t j = 0; j <= i; ++j) {
    //   printf(".");
    // }
    // for (uint32_t j = (i + 1); j < 100; ++j) {
    //   printf(" ");
    // }
    // printf("]");
    // printf("\r");

    // fflush(stdout);

    // if (fdb_timed_write_event_array((events + (i * progress_bar_increment)), progress_bar_increment)) fatal_error();
  }
  printf("\n");

  int error = fdb_timed_write_event_array_async(events, num_events);
  if (error) fatal_error();

  c_end = clock();
  time(&end);

  // Print timing results
  printf("Wall clock time to write events: %.2f s\n", difftime(end, start));
  printf("Total CPU time to write events:  %.2f s\n", (((double)(c_end - c_start)) / CLOCKS_PER_SEC));

  // Clean up the FoundationDB cluster
  if (fdb_clear_timed_database(num_events, num_frags)) fatal_error();

  // Success
  printf("Batch size %d benchmark complete.\n", batch_size);
}

void load_mock_events(Event **events, FragmentedEvent **f_events, uint32_t num_events, uint32_t size) {

  printf("Generating %u %u-byte mock events...\n", num_events, size);

  // Seed the random number generator
  srand(time(0));

  // Allocate memory for events
  *events = (Event *) malloc(sizeof(Event) * num_events);

  // Write random key/values into the given array.
  for (uint32_t i = 0; i < num_events; ++i) {
    // Generate random byte data
    uint8_t *data = (uint8_t *) malloc(sizeof(uint8_t) * size);
    for (uint64_t j = 0; j < size; ++j) {
      data[j] = rand() % 256;
    }

    // Generate Event from data
    (*events)[i].id = i;
    (*events)[i].data_length = size;
    (*events)[i].data = data;
  }

  printf("Generating mock events complete.\n");
  printf("Fragmenting mock events...\n");

  // Fragment events
  *f_events = (FragmentedEvent *) malloc(sizeof(FragmentedEvent) * num_events);
  for (uint32_t i = 0; i < num_events; ++i) {
    fragment_event((*events + i), (*f_events + i));
  }

  // Success
  printf("Fragmented mock events.\n");
}

void load_lmdb_events(Event **events, FragmentedEvent **f_events, uint32_t num_events, uint32_t size) {
  printf("Loading %u %u-byte events from LMDB...\n", num_events, size);

  // Allocate memory for events
  *events = (Event *) malloc(sizeof(Event) * num_events);

  // TODO

  printf("Loading events from LMDB complete.\n");
  printf("Fragmenting mock events...\n");

  // Fragment events
  *f_events = (FragmentedEvent *) malloc(sizeof(FragmentedEvent) * num_events);
  for (uint32_t i = 0; i < num_events; ++i) {
    fragment_event((*events + i), (*f_events + i));
  }

  // Success
  printf("Fragmented events from LMDB.\n");
}

void release_events_memory(Event *events, FragmentedEvent *f_events, uint32_t num_events) {

  // Release fragment pointers
  for (uint32_t i = 0; i < num_events; ++i) {
    free_fragmented_event(f_events + i);
  }

  // Release fragmented events array
  free((void *) f_events);

  // Release event data
  for (uint32_t i = 0; i < num_events; ++i) {
    free_event(events + i);
  }

  // Release events array
  free((void *) events);
}

void fatal_error(void) {
  fprintf(stderr, "Fatal error during benchmarks\n");
  exit(1);
}

uint32_t parse_pos_int(char const *str) {

  int32_t parsed_num = atoi(str);
  if (parsed_num < 1) {
    return 0;
  }

  return (uint32_t)parsed_num;
}

uint32_t total_fragments(FragmentedEvent *events, uint32_t num_events) {
  uint32_t sum = 0;
  for (uint32_t i = 0; i < num_events; i++) {
    sum += events[i].num_fragments;
  }
  return sum;
}