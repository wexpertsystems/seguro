//! @file benchmark.c
//!
//! Benchmarking suite for Seguro Phase 2.

#include <foundationdb/fdb_c.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "../event.h"
#include "../fdb.h"


//==============================================================================
// Prototypes
//==============================================================================

//! Run the default benchmarking suite for Seguro.
//!
//! @param[in] fdb  FoundationDB handle.
//!
//! @return   Error code or 0 if no error.
int run_default_benchmarks(FDBDatabase *fdb);

//! Run a custom benchmark test on Seguro.
//!
//! @param[in] fdb          FoundationDB handle.
//! @param[in] num_events   The number of events to test writing.
//! @param[in] event_size   The size of each event, in bytes.
//! @param[in] batch_size   The number of events to write per FDB transaction.
//!
//! @return   Error code or 0 if no error.
int run_custom_benchmark(FDBDatabase *fdb, uint32_t num_events, uint32_t event_size, uint32_t batch_size);

//! Run a single benchmark test, timing the results and printing them to console.
//!
//! @param[in] fdb          FoundationDB handle.
//! @param[in] events       Array of events to write to FDB.
//! @param[in] num_events   The number of events in the array.
//!
//! @return   Error code or 0 if no error.
int timed_benchmark(FDBDatabase *fdb, FragmentedEvent *events, uint32_t num_events);

//! Generate an array of mock events.
//!
//! @param[in] num_events  The number of events to generate.
//! @param[in] size        The size of each event, in bytes.
//!
//! @return       Array of events.
//! @return NULL  Failure.
Event *load_mock_events(uint32_t num_events, uint64_t size);

//! Releases memory allocated for mock events.
//!
//! @param[in] events       Array of raw events to free.
//! @param[in] events       Array of fragmented events to free.
//! @param[in] num_events   The number of events in the arrays.
//!
//! @return   Error code or 0 if no error.
int release_events_memory(Event *events, FragmentedEvent *f_events, uint32_t num_events);

//! Parse a positive integer from a string.
//!
//! @param[in] str  The string to parse.
//!
//! @return     A positive integer.
//! @return 0   Failure.
uint32_t parse_pos_int(char const *str);

//==============================================================================
// Functions
//==============================================================================

//! Execute the Seguro benchmark suite. Takes several command-line arguments which can be used to specify a custom
//! benchmark.
//!
//! @param[in] argc  Number of command-line options provided.
//! @param[in] argv  Array of command-line options provided.
//!
//! @return  0  Success.
//! @return -1  Failure.
int main(int argc, char **argv) {

  // Variables to parse command line options using getopt
  int opt;
  int longindex;
  static struct option longopts[] = {
    // name         has_arg            flag   val
    { "custom",     no_argument,       NULL, 'c' },
    { "num-events", required_argument, NULL, 'n' },
    { "event-size", required_argument, NULL, 'e' },
    { "batch-size", required_argument, NULL, 'b' },
    {NULL,          0,                 NULL, 0}
  };

  // Variables for custom benchmark

  // Run custom benchmark?
  bool custom_benchmark = false;
  // Number of events to generate
  uint32_t num_events = 10000;
  // Event size in bytes (10KB by default)
  uint32_t event_size = 10000;
  // Event batch size (10 by default)
  uint32_t batch_size = 10;

  // Parse options
  while ((opt = getopt_long(argc, argv, "-cn:e:b:", longopts, &longindex)) != EOF) {
    switch (opt) {
      case 'c':
        custom_benchmark = true;
        break;
      case 'n':
        num_events = parse_pos_int(optarg);
        if (!num_events) {
          printf("%s: 'num-events' expects a positive integer argument.\n", argv[0]);
          exit(1);
        }
        break;
      case 'e':
        event_size = parse_pos_int(optarg);
        if (!event_size) {
          printf("%s: 'event-size' expects a positive integer argument.\n", argv[0]);
          exit(1);
        }
        break;
      case 'b':
        batch_size = parse_pos_int(optarg);
        if (!batch_size) {
          printf("%s: 'batch-size' expects a positive integer argument.\n", argv[0]);
          exit(1);
        }
        break;
      case 1:
        printf ("%s: unexpected argument '%s'\n", argv[0], optarg);
      default: // '?', ':', 0
        exit(1);
    }
  }

  // Validate input arguments
  if (num_events % batch_size) {
    printf ("%s: num-events (%u) not divisable by batch_size (%u)\n", argv[0], num_events, batch_size);
    exit(1);
  }

  // Run benchmark(s)

  printf("Seguro Phase 2 benchmarks...\n");

  // Initialize FoundationDB database
  FDBDatabase *fdb = fdb_init();

  if (custom_benchmark) {
    printf("Running custom benchmark...\n");
    return run_custom_benchmark(fdb, num_events, event_size, batch_size);
  } else {
    printf("Running default suite...\n");
    return run_default_benchmarks(fdb);
  }

  // Clean up FoundationDB database
  printf("Tearing down database...\n");
  fdb_shutdown(fdb, &fdb_network_thread);

  // Success
  printf("Benchmarks completed.\n");
  return 0;
}

int run_default_benchmarks(FDBDatabase *fdb) {

  // Size of transaction cannot exceed 10,000,000 bytes (10MB) of "affected data" (e.g. keys + values + ranges for
  // write, keys + ranges for read).
  //
  // In addition, to keep timing stats consistent between tests, each batch should be identical in size for each
  // benchmark.
  //
  // Therefore, we want a large number of events, divisable by as many batch sizes as possible (all less than 1000).
  // 9000 appears to be a good compromise.
  //
  uint32_t num_events = 9000;
  uint32_t event_size = 10000;

  // Generate mock events
  printf("Generating %u mock events...\n", num_events);
  Event *raw_events = load_mock_events(num_events, event_size);

  // Fragment events
  FragmentedEvent events[num_events];
  for (uint32_t i = 0; i < num_events; ++i) {
    fragment_event((raw_events + i), (events + i));
  }

  // Run the write benchmarks
  timed_benchmark(fdb, events, num_events);
//  timed_benchmark(fdb, events, num_events, 1);
//  timed_benchmark(fdb, events, num_events, 10);
//  timed_benchmark(fdb, events, num_events, 100);
//  timed_benchmark(fdb, events, num_events, 200);
//  timed_benchmark(fdb, events, num_events, 300);
//  timed_benchmark(fdb, events, num_events, 450);
//  timed_benchmark(fdb, events, num_events, 600);
//  timed_benchmark(fdb, events, num_events, 900);

  // Clean up heap
  release_events_memory(raw_events, events, num_events);

  // Success
  return 0;
}

int run_custom_benchmark(FDBDatabase *fdb, uint32_t num_events, uint32_t event_size, uint32_t batch_size) {

  // Generate mock events
  printf("Generating %u mock events...\n", num_events);
  Event *raw_events = load_mock_events(num_events, event_size);

  // Fragment events
  FragmentedEvent events[num_events];
  for (uint32_t i = 0; i < num_events; ++i) {
    fragment_event((raw_events + i), (events + i));
  }

  // Run the custom write benchmark
  fdb_set_batch_size(batch_size);
  timed_benchmark(fdb, events, num_events);

  // Clean up heap
  release_events_memory(raw_events, events, num_events);

  // Success
  return 0;
}

int timed_benchmark(FDBDatabase* fdb, FragmentedEvent* events, uint32_t num_events) {

  //
  int err;

  // Setup timers
  clock_t t_start, t_end, b_start, b_end, b_diff;
  clock_t b_max = 0;
  clock_t b_min = (clock_t)INT_MAX;
  double total_time;

  printf("Writing %u events in batches of %u...\n", num_events, 1);

  // Start timer
  t_start = clock();

  for(uint32_t i = 0; i < num_events; ++i) {
    // Time each write
    b_start = clock();

    err = fdb_write_event(fdb, (events + i));
    if (err != 0) goto fatal_error;

    // Record time if new min or new max set
    b_end = clock();
    b_diff = b_end - b_start;
    if (b_diff > b_max) {
      b_max = b_diff;
    } else if (b_diff < b_min) {
      b_min = b_diff;
    }
  }

  // Stop the timer
  t_end = clock();

  // Compute timing stats
  total_time = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;

  printf("Total time to write events: %.2f s\n", total_time);
  printf("Average time per event:     %.4f ms\n", (1000.0 * total_time / num_events));
  printf("Max time:                   %.4f ms\n", (1000.0 * b_max / CLOCKS_PER_SEC));
//  printf("Avg time:                   %.4f ms\n", (1000.0 * total_time / (num_events / batch_size)));
  printf("Min time:                   %.4f ms\n", (1000.0 * b_min / CLOCKS_PER_SEC));

  // Clean up database
  printf("Cleaning events from database...\n\n");

  err = fdb_clear_event_array(fdb, events, num_events);
  if (err != 0) goto fatal_error;

  // Success
  return 0;

  // Failure
  fatal_error:
  printf("Fatal error during write benchmark\n");
  exit(1);
}

Event *load_mock_events(uint32_t num_events, uint64_t size) {

  // Seed the random number generator
  srand(time(0));

  // Allocate memory for events
  Event *events = (Event *) malloc(sizeof(Event) * num_events);

  // Write random key/values into the given array.
  for (uint32_t i = 0; i < num_events; ++i) {
    // Generate random byte data
    uint8_t *data = (uint8_t *) malloc(sizeof(uint8_t) * size);
    for (uint64_t j = 0; j < size; ++j) {
      data[j] = rand() % 256;
    }

    // Generate Event from data
    events[i].key = i;
    events[i].data_length = size;
    events[i].data = data;
  }

  // Success
  printf("Loaded %u events into memory.\n\n", num_events);
  return events;
}

int release_events_memory(Event *events, FragmentedEvent *f_events, uint32_t num_events) {

  // Release fragment pointers
  for (uint32_t i = 0; i < num_events; ++i) {
    free_fragmented_event(f_events + i);
  }

  // Release event data
  for (uint32_t i = 0; i < num_events; ++i) {
    free_event(events + i);
  }

  // Release events array
  free((void *) events);

  // Success
  return 0;
}

uint32_t parse_pos_int(char const *str) {

  int32_t parsed_num = atoi(str);
  if (parsed_num < 1) {
    return 0;
  }

  return (uint32_t)parsed_num;
}
