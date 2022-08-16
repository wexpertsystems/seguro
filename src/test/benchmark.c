//! @file benchmark.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <foundationdb/fdb_c.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "../events.h"
#include "../fdb.h"

//==============================================================================
// Prototypes
//==============================================================================

//! Run the default benchmarking suite for Seguro.
//!
//! @return   Error code or 0 if no error.
int run_default_benchmarks(void);

//! Run a custom benchmark test on Seguro.
//!
//! @param[in] num_events   The number of events to test writing.
//! @param[in] event_size   The size of each event, in bytes.
//! @param[in] batch_size   The number of events to write per FDB transaction.
//!
//! @return   Error code or 0 if no error.
int run_custom_benchmark(uint32_t num_events, uint32_t event_size, uint32_t batch_size);

//! Run a single benchmark test, timing the results and printing them to console.
//!
//! @param[in] fdb          FoundationDB handle.
//! @param[in] events       Array of events to write to FDB.
//! @param[in] num_events   The number of events in the array.
//! @param[in] batch_size   The number of events to write per FDB transaction.
//!
//! @return   Error code or 0 if no error.
int timed_benchmark(FDBDatabase *fdb, FDBKeyValue *events, uint32_t num_events, uint32_t batch_size);

//! Generate an array of mock events.
//!
//! @param[in] num_events  The number of events to generate.
//! @param[in] size        The size of each event, in bytes.
//!
//! @return       Array of events.
//! @return NULL  Failure.
FDBKeyValue *load_mock_events(uint32_t num_events, uint32_t size);

//! Releases memory allocated for an array of events.
//!
//! @param[in] events       Array of events to free.
//! @param[in] num_events   The number of events in the array.
//!
//! @return   Error code or 0 if no error.
int release_events_memory(FDBKeyValue *events, uint32_t num_events);

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

  if (custom_benchmark) {
    printf("Running custom benchmark...\n");
    return run_custom_benchmark(num_events, event_size, batch_size);
  } else {
    printf("Running default suite...\n");
    return run_default_benchmarks();
  }

  printf("Benchmarks completed.\n");

  // Success
  return 0;
}

int run_default_benchmarks(void) {

  // Default benchmark suite event settings

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
  FDBKeyValue *events = load_mock_events(num_events, event_size);

  // Initialize FoundationDB database
  FDBDatabase *fdb = fdb_init();

  // Run the write benchmarks
  timed_benchmark(fdb, events, num_events, 1);
  timed_benchmark(fdb, events, num_events, 10);
  timed_benchmark(fdb, events, num_events, 100);
  timed_benchmark(fdb, events, num_events, 200);
  timed_benchmark(fdb, events, num_events, 300);
  timed_benchmark(fdb, events, num_events, 450);
  timed_benchmark(fdb, events, num_events, 600);
  timed_benchmark(fdb, events, num_events, 900);

  // Clean up heap
  release_events_memory(events, num_events);

  // Success
  return 0;
}

int run_custom_benchmark(uint32_t num_events, uint32_t event_size, uint32_t batch_size) {

  // Generate mock events
  printf("Generating %u mock events...\n", num_events);
  FDBKeyValue *events = load_mock_events(num_events, event_size);

  // Initialize FoundationDB database
  FDBDatabase *fdb = fdb_init();

  // Run the custom write benchmark
  timed_benchmark(fdb, events, num_events, batch_size);

  // Clean up heap
  release_events_memory(events, num_events);

  // Success
  return 0;
}

int timed_benchmark(FDBDatabase* fdb, FDBKeyValue* events, uint32_t num_events, uint32_t batch_size) {

  // Setup timers
  clock_t t_start, t_end, b_start, b_end, b_diff;
  clock_t b_max = 0;
  clock_t b_min = (clock_t)INT_MAX;
  double total_time;
  int err;

  printf("Writing %u events in batches of %u...\n", num_events, batch_size);

  // Start timer
  t_start = clock();

  // Perform batched writes
  for(uint32_t i = 0; i < num_events; i += batch_size) {
    // Time each batch write
    b_start = clock();

    err = write_event_batch(fdb, (events + i), batch_size);
    if (err != 0) goto fatal_error;

    // Record batch time if new min or new max set
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
  printf("Max batch time:             %.4f ms\n", (1000.0 * b_max / CLOCKS_PER_SEC));
  printf("Avg batch time:             %.4f ms\n", (1000.0 * total_time / (num_events / batch_size)));
  printf("Min batch time:             %.4f ms\n", (1000.0 * b_min / CLOCKS_PER_SEC));

  // Clean up database
  printf("Cleaning events from database...\n\n");

  err = clear_events(fdb, events, num_events);
  if (err != 0) goto fatal_error;

  // Success
  return 0;

  // Failure
  fatal_error:
  printf("Fatal error during write benchmark\n");
  exit(1);
}

FDBKeyValue *load_mock_events(uint32_t num_events, uint32_t size) {

  // Seed the random number generator
  srand(time(0));

  // Allocate memory for events
  FDBKeyValue *events = (FDBKeyValue *) malloc(sizeof(FDBKeyValue) * num_events);

  // Write random key/values into the given array.
  for (uint32_t i = 0; i < num_events; i++) {
    // Generate key (unique event ID)
    int key_length = count_digits(i);
    uint8_t *key = (uint8_t *) malloc(sizeof(uint8_t) * key_length);
    sprintf((char *)key, "%d", i);

    // Generate a value (random bytes)
    uint8_t *value = (uint8_t *) malloc(sizeof(uint8_t) * size);
    for (uint32_t j = 0; j < size; j++) {
      value[j] = rand() % 256;
    }

    // Set event properties
    events[i].key = key;
    events[i].key_length = key_length;
    events[i].value = value;
    events[i].value_length = size;
  }

  printf("Loaded %u events into memory.\n\n", num_events);

  return events;
}

int release_events_memory(FDBKeyValue *events, uint32_t num_events) {

  // Release event key/value pairs
  for (uint32_t i = 0; i < num_events; i++) {
    free((void *) events[i].key);
    free((void *) events[i].value);
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
