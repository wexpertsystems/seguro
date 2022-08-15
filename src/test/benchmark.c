//! @file benchmark.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <errno.h>
#include <foundationdb/fdb_c.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../events.h"
#include "../fdb.h"

//==============================================================================
// Prototypes
//==============================================================================

int run_default_benchmarks(void);

int run_custom_benchmark(long num_events, int event_size, int batch_size);

//! Runs the benchmark for writing one event per transaction to FoundationDB.
//!
//! @param[in] fdb  FoundationDB database object.
//! @param[in] e    Events array to write into the database.
//! @param[in] num_events    Number of events to write into the database. 
//!
//! @return 0   Success.
//! @return -1  Failure.
int run_single_write_benchmark(FDBDatabase* fdb, FDBKeyValue* events, long num_events);
 
//! Runs the benchmark for writing a batch of events per transaction to 
//! FoundationDB.
//!
//! @param[in] fdb         FoundationDB database object.
//! @param[in] events      Events array to write into the database.
//! @param[in] num_events  Number of events to write into the database. 
//!
//! @return 0   Success.
//! @return -1  Failure.
int run_batch_write_benchmark(FDBDatabase* fdb, FDBKeyValue* e, long num_events, int b);

//! Releases memory allocated for the given events array.
//!
//! @param[in] events  FDBKeyValue array.
//! @param[in] num_events       Array length.
//!
//! @return 0  Success.
int release_events_memory(FDBKeyValue *events, long num_events);

unsigned int parse_pos_int(char const *str);

//==============================================================================
// Functions
//==============================================================================

//! Execute the Seguro benchmark suite. Takes several command-line arguments
//! which can be used to specify a custom benchmark.
//!
//! @param[in] argc  Number of command-line options provided.
//! @param[in] argv  Array of command-line options provided.
//!
//! @return  0  Success.
//! @return -1  Failure.
int main(int argc, char** argv) {
  // Options.
  int opt;
  int longindex;
  static struct option longopts[] = {
    // name         has_arg            flag   val
    { "custom",     no_argument,       NULL,  'c' },
    { "num-events", required_argument, NULL,  'n' },
    { "event-size", required_argument, NULL,  'e' },
    { "batch-size", required_argument, NULL,  'b' },
    {NULL, 0, NULL, 0},
  };

  // Variables for custom benchmark
  // Run custom benchmark?
  bool custom_benchmark = false;
  // Number of events to generate.
  unsigned int num_events = 10000;
  // Event size in bytes (10KB by default).
  unsigned int event_size = 10000;
  // Event batch size (10 by default).
  unsigned int batch_size = 10;

  // Parse options.
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

  // Print startup information.
  printf("Seguro Phase 2 benchmarks...\n");

  if (custom_benchmark) {
    printf("Running custom benchmark...\n");
    return run_custom_benchmark(num_events, event_size, batch_size);
  } else {
    printf("Running default suite...\n");
    return run_default_benchmarks();
  }
}

int run_default_benchmarks(void) {

  long num_events = 10000;
  int event_size = 10000;

  // Event array.
  FDBKeyValue *events;

  // Generate mock events.
  printf("Generating %ld mock events...\n", num_events);
  events = load_mock_events(num_events, event_size);

  // Initialize a FoundationDB database object.
  FDBDatabase *fdb = fdb_init();

  // Run the write benchmarks.
  run_single_write_benchmark(fdb, events, num_events);
  run_batch_write_benchmark(fdb, events, num_events, 10);
  run_batch_write_benchmark(fdb, events, num_events, 100);
  run_batch_write_benchmark(fdb, events, num_events, 1000);

  // Release the events array memory.
  release_events_memory(events, num_events);

  // Success.
  printf("Benchmark completed.\n");
  return 0;
}

int run_custom_benchmark(long num_events, int event_size, int batch_size) {
  // Event array.
  FDBKeyValue *events;

  // Generate mock events.
  printf("Generating %ld mock events...\n", num_events);
  events = load_mock_events(num_events, event_size);

  // Initialize a FoundationDB database object.
  FDBDatabase *fdb = fdb_init();

  // Run the benchmark.
  run_batch_write_benchmark(fdb, events, num_events, batch_size);

  // Release the events array memory.
  release_events_memory(events, num_events);

  // Success.
  printf("Benchmark completed.\n");
  return 0;
}

int release_events_memory(FDBKeyValue *events, long num_events) {
  // Release key/value array pairs.
  for (int i = 0; i < num_events; i++) {
    free((void *) events[i].key);
    free((void *) events[i].value);
  }

  // Release events.
  free((void *) events);

  // Success.
  return 0;
}

int run_single_write_benchmark(FDBDatabase* fdb, FDBKeyValue* events, long num_events) {
  printf("Writing one event per tx...\n");

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // Prepare a transaction object.
  FDBTransaction *tx;
  fdb_error_t err;

  // Iterate through events and write each to the database.
  for (int i = 0; i < num_events; i++) {
    // Create a new database transaction.
    err = fdb_database_create_transaction(fdb, &tx);
    if (err != 0) {
      printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
      return -1;
    }

    // Get the key/value pair from the events array.
    uint8_t const *key = events[i].key;
    int key_length = events[i].key_length;
    uint8_t const *value = events[i].value;
    int value_length = events[i].value_length;

    // Create a transaction with a write of a single key/value pair.
    fdb_transaction_set(tx, key, key_length, value, value_length);

    // Commit the transaction.
    FDBFuture *future;
    future = fdb_transaction_commit(tx);

    // Wait for the future to be ready.
    err = fdb_future_block_until_ready(future);
    if (err != 0) {
      printf("fdb_future_block_until_ready error:\n%s", fdb_get_error(err));
      return -1;
    }

    // Check that the future did not return any errors.
    err = fdb_future_get_error(future);
    if (err != 0) {
      return -1;
    }

    // Destroy the future.
    fdb_future_destroy(future);
  }

  // Stop the timer.
  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);
  double avg_ms = (cpu_time_used * 1000) / num_events;

  // Print.
  printf("Average single event per tx write time: %f\n\n", avg_ms);

  // Success.
  return 0;
}

int run_batch_write_benchmark(FDBDatabase* fdb, FDBKeyValue* events, long num_events, int b) {
  printf("Writing batches of %d events per tx...\n", b);

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // Prepare a transaction object.
  FDBTransaction *tx;
  fdb_error_t err;

  // Iterate through events and write each to the database.
  for (int i = 0; i < num_events; i++) {
    // Create a new database transaction.
    err = fdb_database_create_transaction(fdb, &tx);
    if (err != 0) {
      printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
      return -1;
    }

    // Get the key/value pair from the events array.
    uint8_t const *key = events[i].key;
    int key_length = events[i].key_length;
    uint8_t const *value = events[i].value;
    int value_length = events[i].value_length;

    // Create a transaction with a write of a single key/value pair.
    fdb_transaction_set(tx, key, key_length, value, value_length);

    // Commit the transaction when a batch is ready.
    if (((i % b) == 0) || ((i + 1) == num_events)) {
      FDBFuture *future;
      future = fdb_transaction_commit(tx);
      // Wait for the future to be ready.
      err = fdb_future_block_until_ready(future);
      if (err != 0) {
        printf("fdb_future_block_until_ready error:\n%s", fdb_get_error(err));
        return -1;
      }

      // Check that the future did not return any errors.
      err = fdb_future_get_error(future);
      if (err != 0) {
        printf("fdb_future_error:\n%s\n", fdb_get_error(err));
        return -1;
      }

      // Destroy the future.
      fdb_future_destroy(future);
    }
  }

  // Stop the timer.
  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);
  double avg_ms = (cpu_time_used * 1000) / num_events;

  // Print.
  printf("Average event write time with batches of %d: %f\n\n", b, avg_ms);

  // Success.
  return 0;
}

unsigned int parse_pos_int(char const *str) {
  int parsed_num = atoi(str);
  if (parsed_num < 1) {
    return 0;
  }

  return (unsigned int)parsed_num;
}
