/// @file benchmark.c
///
/// Write benchmark suite for Seguro Phase 2.
///
/// Documentation links:
///   https://www.gnu.org/software/libc/manual/html_node/Using-Getopt.html
///   https://linux.die.net/man/3/getopt_long
///   https://apple.github.io/foundationdb/benchmarking.html

#include <foundationdb/fdb_c.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

/// Run the default benchmarking suite for Seguro.
void run_benchmarks(void);

/// Run the default event write benchmarks.
///
/// @param[in] config   Configuration settings for the benchmark test.
void run_write_benchmark(DataConfig config);

/// Run the default asynchronous write benchmarks.
///
/// @param[in] config   Configuration settings for the benchmark test.
void run_write_benchmark_async(DataConfig config);

/// Write an array of events to a FoundationDB cluster and time the process.
///
/// TODO: Implement for dynamic number of fragments per event.
///
/// @param[in] events       Array of events to write.
/// @param[in] num_events   Number of events in array.
/// @param[in] num_frags    Number of fragments per event.
/// @param[in] batch_size   Batch size of writes per FoundationDB transaction.
void timed_array_write(FragmentedEvent *events, uint32_t num_events,
                       uint32_t num_frags, uint32_t batch_size);

/// Write an array of events to a FoundationDB cluster and time the process.
///
/// TODO: Implement for dynamic number of fragments per event.
///
/// @param[in] events       Array of events to write.
/// @param[in] num_events   Number of events in array.
/// @param[in] num_frags    Number of fragments per event.
/// @param[in] batch_size   Batch size of writes per FoundationDB transaction.
void timed_array_write_async(FragmentedEvent *events, uint32_t num_events,
                             uint32_t num_frags, uint32_t batch_size);

/// Generate an array of mock events and fragment them.
///
/// @param[in] events       Handle for an array of events.
/// @param[in] f_events     Handle for an array of fragmented events.
/// @param[in] num_events   Number of events in array.
/// @param[in] size         The size of each event, in bytes.
void load_mock_events(Event **events, FragmentedEvent **f_events,
                      uint32_t num_events, uint32_t size);

/// Releases memory allocated for mock events.
///
/// @param[in] events       Array of raw events to free.
/// @param[in] f_events     Array of fragmented events to free.
/// @param[in] num_events   Number of events in arrays.
void release_events_memory(Event *events, FragmentedEvent *f_events,
                           uint32_t num_events);

/// Print that a fatal error occurred and exit.
void fatal_error(void);

/// Parse a positive integer from a string.
///
/// @param[in] str  The string to parse..
///
/// @return     A positive integer.
/// @return 0   Failure.
uint32_t parse_pos_int(char const *str);

//==============================================================================
// Functions
//==============================================================================

/// Execute the Seguro write benchmark suite.
///
/// @param[in] argc  Number of command-line options provided.
/// @param[in] argv  Array of command-line options provided.
///
/// @return  0  Success
/// @return -1  Failure (error occurred)
int main(int argc, char **argv) {
  // Initialize FoundationDB database
  fdb_init_database();
  fdb_init_network_thread();

  // Run benchmarks
  run_benchmarks();

  // Clean up FoundationDB database
  fdb_shutdown_network_thread();
  fdb_shutdown_database();

  // Success
  return 0;
}

void run_benchmarks(void) {
  int num_configs = 2;
  DataConfig configs[] = {
      // n     , size (bytes)
      {1000, 500},
      {1000, 1000},
  };

  for (uint8_t i = 0; i < num_configs; ++i) {
    run_write_benchmark(configs[i]);
  }

  for (uint8_t i = 0; i < num_configs; ++i) {
    run_write_benchmark_async(configs[i]);
  }
}

void run_write_benchmark(DataConfig config) {
  Event *raw_events;
  FragmentedEvent *events;

  // Size of transaction cannot exceed 10,000,000 bytes (10MB) of "affected
  // data" (e.g. keys + values + ranges for write, keys + ranges for read).
  // Therefore, batch size cannot exceed 1000 with OPTIMAL_VALUE_SIZE of 10,000
  // bytes (10KB).
  uint32_t batch_sizes[] = {1, 5, 10};
  uint32_t num_bs = 3;
  uint32_t num_events = config.num_events;
  uint32_t event_size = config.event_size;
  uint16_t num_fragments =
      (uint16_t)ceil((double)event_size / (double)OPTIMAL_VALUE_SIZE);

  // Generate mock events
  load_mock_events(&raw_events, &events, num_events, event_size);

  // Array batch writes for each batch size
  for (uint8_t i = 0; i < num_bs; ++i) {
    printf("\n");
    printf("    events  %u\n", num_events);
    printf("event size  %u bytes\n", event_size);
    printf("batch size  %u\n", batch_sizes[i]);
    printf(" fragments  %u\n", num_fragments);
    printf("    method  synchronous\n");
    timed_array_write(events, num_events, num_fragments, batch_sizes[i]);
  }

  // Clean up heap
  release_events_memory(raw_events, events, num_events);
}

void run_write_benchmark_async(DataConfig config) {
  Event *raw_events;
  FragmentedEvent *events;

  // Size of transaction cannot exceed 10,000,000 bytes (10MB) of "affected
  // data" (e.g. keys + values + ranges for write, keys + ranges for read).
  // Therefore, batch size cannot exceed 1000 with OPTIMAL_VALUE_SIZE of 10,000
  // bytes (10KB).
  uint32_t batch_sizes[] = {1, 5, 10};
  uint32_t num_bs = 3;
  uint32_t num_events = config.num_events;
  uint32_t event_size = config.event_size;
  uint16_t num_fragments =
      (uint16_t)ceil((double)event_size / (double)OPTIMAL_VALUE_SIZE);

  // Generate mock events
  load_mock_events(&raw_events, &events, num_events, event_size);

  // Array batch writes for each batch size
  for (uint8_t i = 0; i < num_bs; ++i) {
    printf("\n");
    printf("    events  %u\n", num_events);
    printf("event size  %u bytes\n", event_size);
    printf("batch size  %u\n", batch_sizes[i]);
    printf(" fragments  %u\n", num_fragments);
    printf("    method  asynchronous\n");
    timed_array_write_async(events, num_events, num_fragments, batch_sizes[i]);
  }

  // Clean up heap
  release_events_memory(raw_events, events, num_events);
}

void timed_array_write(FragmentedEvent *events, uint32_t num_events,
                       uint32_t num_frags, uint32_t batch_size) {
  clock_t c_start, c_end;
  // uint32_t progress_bar_increment = ((num_events * num_frags) / 100);

  fdb_set_batch_size(batch_size);

  // Write array of events in batches, and print a bar as a visual indicator of
  // progress
  c_start = clock();
  int error = fdb_timed_write_event_array(events, num_events);
  if (error)
    fatal_error();

  c_end = clock();

  // Print timing results
  printf("  cpu time  %12f ms\n",
         (((double)(c_end - c_start)) / CLOCKS_PER_SEC) * 1000.0);

  // Clean up the FoundationDB cluster
  if (fdb_clear_timed_database(num_events, num_frags))
    fatal_error();
}

void timed_array_write_async(FragmentedEvent *events, uint32_t num_events,
                             uint32_t num_frags, uint32_t batch_size) {
  clock_t c_start, c_end;
  // uint32_t progress_bar_increment = ((num_events * num_frags) / 100);

  fdb_set_batch_size(batch_size);

  // Write array of events in batches, and print a bar as a visual indicator of
  // progress
  c_start = clock();
  int error = fdb_timed_write_event_array_async(events, num_events);
  if (error)
    fatal_error();

  c_end = clock();

  // Print timing results
  printf("  cpu time  %12f ms\n",
         (((double)(c_end - c_start)) / CLOCKS_PER_SEC) * 1000.0);

  // Clean up the FoundationDB cluster
  if (fdb_clear_timed_database_async(num_events, num_frags))
    fatal_error();
}

void load_mock_events(Event **events, FragmentedEvent **f_events,
                      uint32_t num_events, uint32_t size) {
  // Seed the random number generator
  srand(time(0));

  // Allocate memory for events
  *events = (Event *)malloc(sizeof(Event) * num_events);

  // Write random key/values into the given array.
  for (uint32_t i = 0; i < num_events; ++i) {
    // Generate random byte data
    uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t) * size);
    for (uint64_t j = 0; j < size; ++j) {
      data[j] = rand() % 256;
    }

    // Generate Event from data
    (*events)[i].id = i;
    (*events)[i].data_length = size;
    (*events)[i].data = data;
  }

  // Fragment events
  *f_events = (FragmentedEvent *)malloc(sizeof(FragmentedEvent) * num_events);
  for (uint32_t i = 0; i < num_events; ++i) {
    fragment_event((*events + i), (*f_events + i));
  }
}

void load_lmdb_events(Event **events, FragmentedEvent **f_events,
                      uint32_t num_events, uint32_t size) {
  // Allocate memory for events
  *events = (Event *)malloc(sizeof(Event) * num_events);

  // TODO

  // Fragment events
  *f_events = (FragmentedEvent *)malloc(sizeof(FragmentedEvent) * num_events);
  for (uint32_t i = 0; i < num_events; ++i) {
    fragment_event((*events + i), (*f_events + i));
  }
}

void release_events_memory(Event *events, FragmentedEvent *f_events,
                           uint32_t num_events) {
  // Release fragment pointers
  for (uint32_t i = 0; i < num_events; ++i) {
    free_fragmented_event(f_events + i);
  }

  // Release fragmented events array
  free((void *)f_events);

  // Release event data
  for (uint32_t i = 0; i < num_events; ++i) {
    free_event(events + i);
  }

  // Release events array
  free((void *)events);
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