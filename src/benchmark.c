//! @file benchmark.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "events.h"
#include "db/fdb.h"

//==============================================================================
// Variables
//==============================================================================

extern char *optarg;
extern pthread_t fdb_network_thread;

//==============================================================================
// Prototypes
//==============================================================================

//! Runs the benchmark for writing one event per transaction to FoundationDB.
//!
//! @param[in] fdb  FoundationDB database object.
//! @param[in] e    Events array to write into the database.
//! @param[in] n    Number of events to write into the database. 
//!
//! @return 0   Success.
//! @return -1  Failure.
int run_single_write_benchmark(FDBDatabase* fdb, FDBKeyValue* e, long n);
 
//! Runs the benchmark for writing a batch of events per transaction to 
//! FoundationDB.
//!
//! @param[in] fdb  FoundationDB database object.
//! @param[in] e    Events array to write into the database.
//! @param[in] n    Number of events to write into the database. 
//!
//! @return 0   Success.
//! @return -1  Failure.
int run_batch_write_benchmark(FDBDatabase* fdb, FDBKeyValue* e, long n, int b);

//==============================================================================
// Functions
//==============================================================================

//! Accepts arguments from the command-line and executes the Seguro benchmark.
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
  static struct option options[] = {
    // name         has_arg            flag         val
    { "num-events", required_argument, no_argument, 'n' },
    { "event-size", required_argument, no_argument, 's' },
    { "batch-size", required_argument, no_argument, 'b' },
    { "mdb-file"  , required_argument, no_argument, 'f' },
    {NULL, 0, NULL, 0},
  };

  // Number of events to generate (or load from file).
  long num_events = 10000;
  // Event size (in bytes, 10KB by default).
  int event_size = 10000;
  // Batch size (in number of events, 10 by default).
  int batch_size = 10;
  // LMDB file to load events from.
  char *mdb_file = NULL;

  // Parse options.
  while ((opt = getopt_long(argc, argv, "n:s:b:f:", options, &longindex)) != EOF) {
    switch (opt) {
      case 'n':
        // Parse long integer from string.
        errno = 0;
        num_events = strtol(optarg, (char **) NULL, 10);
        const bool range_err = errno == ERANGE;
        if (range_err) {
          printf("ERROR: events must be a valid long integer.\n");
        }
        break;
      case 's':
        event_size = atoi(optarg);
        break;
      case 'b':
        batch_size = atoi(optarg);
        break;
      case 'f':
        mdb_file = optarg;
        break;
    }
  }

  // Validate options.
  if (num_events % batch_size != 0) {
    printf("ERROR: total events modulo batch size must equal 0.\n");
    exit(1);
  }

  // Print startup information.
  printf("Seguro Phase 2\n\n");
  printf("Running benchmarks...\n\n");

  // Event array.
  FDBKeyValue *events;

  // Load events from LMDB, or generate mock ones.
  if (mdb_file) { 
    printf("Loading %ld events from LMDB database file: %s", num_events, mdb_file); 
    events = load_lmdb_events(mdb_file, num_events, event_size);
  } 
  else {
    printf("Generating %ld mock events...\n", num_events);
    events = load_mock_events(num_events, event_size);
  }

  // Initialize a FoundationDB database object.
  FDBDatabase *fdb = fdb_init();

  // Run the single event per tx write benchmark.
  run_single_write_benchmark(fdb, events, num_events);

  // Run the batch event write benchmark.
  // run_batch_write_benchmark(fdb, events, num_events, batch_size);

  // Success.
  printf("Benchmark completed.\n");
  return 0;
}

int run_single_write_benchmark(FDBDatabase* fdb, FDBKeyValue* e, long n) {
  printf("Writing one event per tx...\n");

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // Prepare a transaction object.
  FDBTransaction *tx;
  fdb_error_t err;

  // Iterate through events and write each to the database.
  for (int i = 0; i < n; i++) {
    // Create a new database transaction.
    err = fdb_database_create_transaction(fdb, &tx);
    if (err != 0) {
      printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
      return -1;
    }

    // Get the key/value pair from the events array.
    const uint8_t *key = e[i].key;
    int key_length = e[i].key_length;
    const uint8_t *value = e[i].value;
    int value_length = e[i].value_length;

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
  double avg_ms = (cpu_time_used * 1000) / n;

  // Print.
  printf("Average single event per tx write time: %f\n\n", avg_ms);

  // Success.
  return 0;
}

int run_batch_write_benchmark(FDBDatabase* fdb, FDBKeyValue* e, long n, int b) {
  printf("Writing batches of %d events per tx...\n", b);

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // Prepare a transaction object.
  FDBTransaction *tx;
  fdb_error_t err;

  // Iterate through events and write each to the database.
  for (int i = 0; i < n; i++) {
    // Create a new database transaction.
    err = fdb_database_create_transaction(fdb, &tx);
    if (err != 0) {
      printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
      return -1;
    }

    // Get a key/value pair from the events array.
    const char *key = e[i].key;
    int key_length = e[i].key_length;
    const char *value = e[i].value;
    int value_length = e[i].value_length;

    // Create a transaction with a write of a single key/value pair.
    fdb_transaction_set(tx, key, key_length, value, value_length);

    // Commit the transaction when a batch is ready.
    if (i % b == 0) {
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
  }

  // Stop the timer.
  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);
  double avg_ms = (cpu_time_used * 1000) / n;

  // Print.
  printf("Average event write time with batches of %d: %f\n\n", b, avg_ms);

  // Success.
  return 0;
}