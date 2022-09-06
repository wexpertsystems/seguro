//! @file benchmark.c
//!
//! Benchmarking suite for Seguro Phase 2.
//!
//! Documentation links:
//!   https://www.gnu.org/software/libc/manual/html_node/Using-Getopt.html
//!   https://linux.die.net/man/3/getopt_long
//!   https://apple.github.io/foundationdb/benchmarking.html

#include <foundationdb/fdb_c.h>
#include <getopt.h>
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

//! Run a custom benchmark test on Seguro.
//!
//! @param[in] num_events   The number of events to test writing
//! @param[in] event_size   The size of each event, in bytes
//! @param[in] batch_size   The number of events to write per FDB transaction
void run_custom_write_benchmark(uint32_t num_events, uint32_t event_size, uint32_t batch_size);

//! Run the default benchmarking suite for Seguro.
void run_default_benchmarks(void);

//! Run the default event write benchmarks.
//!
//! @param[in] config   Configuration settings for the benchmark test
void run_default_write_benchmark(DataConfig config);

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

//==============================================================================
// Functions
//==============================================================================

//! Execute the Seguro benchmark suite. Takes several command-line arguments which can be used to specify a custom
//! benchmark.
//!
//! @param[in] argc  Number of command-line options provided.
//! @param[in] argv  Array of command-line options provided.
//!
//! @return  0  Success
//! @return  1  Failure (bad input)
//! @return -1  Failure (error occurred)
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
  fdb_init_database();
  fdb_init_network_thread();

  if (custom_benchmark) {
    run_custom_write_benchmark(num_events, event_size, batch_size);
  } else {
    run_default_benchmarks();
  }

  // Clean up FoundationDB database
  printf("Tearing down database...\n");
  fdb_shutdown_network_thread();
  fdb_shutdown_database();

  // Success
  printf("Benchmarks completed.\n");
  return 0;
}

void run_custom_write_benchmark(uint32_t num_events, uint32_t event_size, uint32_t batch_size) {

  printf("Running custom benchmark...\n");

  Event           *raw_events;
  FragmentedEvent *events;
  int              err;

  // Generate mock events
  printf("Generating %u mock events...\n", num_events);
  load_mock_events(&raw_events, &events, num_events, event_size);

  // Run the custom write benchmark
  fdb_set_batch_size(batch_size);
  err = fdb_write_event_array(events, num_events);
  if (err) fatal_error();

  // Clean up the FoundationDB cluster
  err = fdb_timed_clear_database(num_events, events[0].num_fragments);
  if (err) fatal_error();

  // Clean up heap
  release_events_memory(raw_events, events, num_events);

  // Success
  printf("Custom benchmark complete.\n");
}

void run_default_benchmarks(void) {

  printf("Running default benchmarks...\n");

  // Limit amount of data written to local cluster at one time to 10 GB
  DataConfig configs[5] = {
    { 100000, 1 * OPTIMAL_VALUE_SIZE },
    { 100000, 10 * OPTIMAL_VALUE_SIZE },
    { 10000, 100 * OPTIMAL_VALUE_SIZE },
    { 4000, 250 * OPTIMAL_VALUE_SIZE },
    { 1000, 1000 * OPTIMAL_VALUE_SIZE }
  };

  for (uint8_t i = 0; i < 1; ++i) {
    run_default_write_benchmark(configs[i]);
  }

  printf("\nDefault benchmarks complete.\n");
}

void run_default_write_benchmark(DataConfig config) {

  Event           *raw_events;
  FragmentedEvent *events;

  // Size of transaction cannot exceed 10,000,000 bytes (10MB) of "affected data" (e.g. keys + values + ranges for
  // write, keys + ranges for read). Therefore, batch size cannot exceed 1000 with OPTIMAL_VALUE_SIZE of 10,000 bytes
  // (10KB).
  uint32_t batch_sizes[5] = { 100, 10, 100, 500, 900 };
  uint32_t num_events = config.num_events;
  uint32_t event_size = config.event_size;
  uint16_t num_fragments = (event_size / OPTIMAL_VALUE_SIZE);

  printf("\nRunning write benchmarks for %d-fragment events...\n", num_fragments);

  // Generate mock events
  load_mock_events(&raw_events, &events, num_events, event_size);

  // Array batch writes for each batch size
  for (uint8_t i = 0; i < 1; ++i) {
    timed_array_write(events, num_events, num_fragments, batch_sizes[i]);
  }

  // Clean up heap
  release_events_memory(raw_events, events, num_events);

  // Success
  printf("Write benchmarks for %d-fragment events complete.\n", num_fragments);
}

void timed_array_write(FragmentedEvent* events, uint32_t num_events, uint32_t num_frags, uint32_t batch_size) {

  time_t start, end;
  clock_t c_start, c_end;

//  // Data limited to 10 GB, 10 KB per fragment => 1,000,000 fragments
//  // 1,000,000 / 10,000 = 100 => 100 progress bar ticks => each tick is 1%
//  uint32_t progress_bar_fragments = 10000;
//  uint32_t progress_bar_increment = (progress_bar_fragments / events[0].num_fragments);

  printf("Running batch size %d benchmark...\n", batch_size);

  fdb_set_batch_size(batch_size);

  // Write array of events in batches
  time(&start);
  c_start = clock();
//  // Write array of events in batches, and print a bar as a visual indicator of progress
//  for (uint32_t i = 0; i < num_events; i += progress_bar_increment) {
//    err = fdb_write_event_array((events + i), progress_bar_increment);
//    if (err) fatal_error();
//
//    printf(".");
//    fflush(stdout);
//  }
//  printf("\n");
  fdb_timed_write_event_array(events, num_events);
  c_end = clock();
  time(&end);

  // Print timing results
  printf("Wall clock time to write events: %.2f s\n", difftime(end, start));
  printf("Total CPU time to write events:  %.2f s\n", (((double)(c_end - c_start)) / CLOCKS_PER_SEC));

  // Clean up the FoundationDB cluster
  fdb_timed_clear_database(num_events, num_frags);

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
  printf("Fatal error during benchmarks\n");
  exit(1);
}

uint32_t parse_pos_int(char const *str) {

  int32_t parsed_num = atoi(str);
  if (parsed_num < 1) {
    return 0;
  }

  return (uint32_t)parsed_num;
}
