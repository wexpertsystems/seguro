//! @file seguro.h
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <lmdb.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

#define CLUSTER_NAME    "fdb.cluster"
#define DB_NAME         "DB"
#define MAX_VALUE_SIZE  10000
#define MAX_RETRIES     5


//==============================================================================
// Types
//==============================================================================

//==============================================================================
// Global variables.
//==============================================================================

//! Event array to be used for the benchmark.
static FDBKeyValue *events;

//! FoundationDB network thread.
pthread_t fdb_network_thread;

//==============================================================================
// Function Declarations
//==============================================================================

int _count_digits(int n);
FDBDatabase* _fdb_init();
int _fdb_shutdown();
FDBKeyValue* _load_mock_events(FDBKeyValue* events, long num_events, int event_size);
int _run_write_event_benchmark(FDBDatabase* fdb, long num_events);

//==============================================================================
// Functions
//==============================================================================

//! Runs read and write benchmarks with mock events.
//!
//! @return 0  Success.
//! @return 1  Failure.
int run_benchmark_mock(long num_events, int event_size, int batch_size) {
  // Error keeper.
  fdb_error_t err;

  // Generate mock events and load them into our global `events` array.
  events = _load_mock_events(events, num_events, event_size);

  // Initialize FoundationDB.
  printf("Initializing FoundationDB...\n");
  FDBDatabase *fdb = _fdb_init();

  // Run the single write benchmark (one tx per event written).
  // err = _run_write_event_benchmark(fdb, num_events);
  // if (err != 0) {
  //   printf("Write event benchmark failed.\n");
  //   return -1;
  // }
  printf("Running single write benchmark...\n");

  // Start the timer.
  clock_t start, end;
  start = clock();
  double cpu_time_used;

  // Prepare a transaction object.
  FDBTransaction *tx;

  // Iterate through events and write each to the database.
  for (int i = 0; i < num_events; i++) {
    // Create a new database transaction.
    err = fdb_database_create_transaction(fdb, &tx);
    if (err != 0) {
      printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
      return -1;
    }

    // Get the key/value pair from the events array.
    const char *key = events[i].key;
    int key_length = events[i].key_length;
    const char *value = events[i].value;
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
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC * 1000; // ms

  // Calculate performance.
  double avg_write_time = cpu_time_used / (double) num_events;

  // Print.
  printf("Wrote %ld events to the database.\n", num_events);
  printf("Average single event per tx write time: %f\n", avg_write_time);

  // Shutdown the database.
  err = _fdb_shutdown(fdb);
  if (err != 0) {
    printf("Shutdown failed.\n");
    return -1;
  }

  // Success.
  return 0;
}

//! Runs read and write benchmarks with events from LMDB.
//!
//! @return 0  Success.
//! @return 1  Failure.
int run_benchmark_lmdb(char* mdb_file, int batch_size) {
  return 0;
}

//! Runs the batch write benchmark. Prints results.
//!
//! @param[in] batch_size  The number of events to write per transaction.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _run_write_event_batch_benchmark(int batch_size) {
  return 0;
}

//! Runs the write benchmark. Prints results.
//!
//! @param[in] num_events  The number of events to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _run_write_event_benchmark(FDBDatabase* fdb, long num_events) {
  printf("Running single write benchmark...\n");

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;

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
    const char *key = events[i].key;
    int key_length = events[i].key_length;
    const char *value = events[i].value;
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

  // Print.
  printf("Wrote %ld events to the database.", num_events);

  // Success.
  return 0;
}

//! Synchronously reads a single event from the FoundationDB cluster, specified 
//! by the given key.
//!
//! @return NULL  Failed to read event.
//! @return       Pointer to the event.
FDBKeyValue* _read_event(int key) {
  // Create transaction object.
  FDBTransaction *fdb_tx;

  // Create a new database transaction with `fdb_database_create_transaction()`.
  // Create an `FDBFuture` and set it to the return of `fdb_transaction_get()`.
  // Block event loop with `fdb_future_block_until_ready()`, retry if needed.
  // Get the value from the `FDBFuture` with `fdb_future_get_value()`.
  // Destroy the `FDBFuture` with `fdb_future_destroy()`.
  return NULL;
}

//! Asynchronously reads a batch of events from the FoundationDB cluster, 
//! specified by the given key range.
//!
//! @return NULL  Failed to read the event batch.
//! @return       Pointer to the event array.
FDBKeyValue* _read_event_batch(int start, int end) {
  return NULL;
}

//! Asynchronously writes a single event to the FoundationDB cluster with a 
//! single FoundationDB transaction.
//!
//! @param[in] e  Event to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _write_event(FDBKeyValue e) {
  return 0;
}

//! Asynchronously writes a batch of events with a single FoundationDB 
//! transaction.
//!
//! @param[in] events  Events to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _write_event_batch(FDBKeyValue events[]) {
  return 0;
}

//! Executes fdb_run_network() (for use in a separate thread).
//! Exits on error.
void* _fdb_init_run_network(void* arg) {
  printf("Starting network thread...\n");
  fdb_error_t err = fdb_run_network();
  if (err != 0) {
    printf("_fdb_init_run_network: %s\n", fdb_get_error(err));
    exit(-1);
  }
}

//! Initializes a FoundationDB connection, for use with top-level read/write 
//! initialization functions fdb_read_init() and fdb_write_init().
//!
//! @return       Pointer to the FDBDatabase object.
//! @return NULL  Failed.
FDBDatabase* _fdb_init() {
  const char *cluster_file_path = "/etc/foundationdb/fdb.cluster";
  FDBFuture *fdb_future = NULL;
  
  // Check cluster file attributes, exit if not found.
  struct stat cluster_file_buffer;
  uint32_t cluster_file_stat = stat(cluster_file_path, &cluster_file_buffer);
  if (cluster_file_stat != 0) {
    printf("ERROR: no fdb.cluster file found at: %s\n", cluster_file_path);
    return NULL;
  }
  
  // Ensure correct FDB API version.
  printf("Ensuring correct API version...\n");
  fdb_error_t err;
  err = fdb_select_api_version(FDB_API_VERSION);
  if (err != 0) {
    printf("fdb_init select api version: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Setup FDB network.
  printf("Setting up network...\n");
  err = fdb_setup_network();
  if (err != 0) {
    printf("fdb_init failed to setup fdb network: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Start the network thread.
  pthread_create(&fdb_network_thread, NULL, _fdb_init_run_network, NULL);

  // Create the database.
  printf("Creating the database...\n");
  FDBDatabase *fdb;
  err = fdb_create_database((char *) cluster_file_path, &fdb);
  if (err != 0) {
    printf("fdb_init create database: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Success.
  return fdb;

  // FDB initialization error goto.
  fdb_init_error:
    if (fdb_future) {
      fdb_future_destroy(fdb_future);
    }
    return NULL;
}

//! Shuts down the FoundationDB network.
//!
//! @return  0  Success.
//! @return -1  Failure.
int _fdb_shutdown(FDBDatabase* fdb) {
  fdb_error_t err;

  // Destroy the database.
  fdb_database_destroy(fdb);

  // Signal network shutdown.
  err = fdb_stop_network();
  if (err != 0) {
    printf("fdb_init stop network: %s\n", fdb_get_error(err));
    return -1;
  }

  // Stop the network thread.
  err = pthread_join(fdb_network_thread, NULL);
  if (err) {
    printf("fdb_init wait for network thread: %d\n", err);
    return -1;
  }

  return 0;
}

//! Writes mock events into the given array.
//!
//! @param[in] events      The event array to write events into.
//! @param[in] n           The number of events to write into the event array.
//! @param[in] event_size  The size of each event, in bytes.
//!
//! @return  Pointer to the events array.
FDBKeyValue* _load_mock_events(FDBKeyValue* events, long num_events, int event_size) {
  // Seed the random number generator.
  srand(time(0));

  // Allocate memory for events.
  events = (FDBKeyValue *) malloc(sizeof(FDBKeyValue) * (num_events));

  // Write random key/values into the given array.
  for (int i = 0; i < num_events; i++) {
    // Generate a key.
    int key_length = _count_digits(i);
    char key[key_length];
    sprintf(key, "%d", i);

    // Generate a value.
    char value[event_size]; 
    for (int j = 0; j < event_size; j++) {
      // 0-255 (all valid bytes).
      int b = rand() % 256;
      value[j] = b;
    }

    // Write the key and value into a slot in the array.
    events[i].key = (uint8_t *) key;
    events[i].key_length = key_length;
    events[i].value = (uint8_t *) value;
    events[i].value_length = event_size;
  }

  // Print results.
  printf("Loaded %ld events into memory.\n", num_events);

  // Success.
  return events;
}

//! Reads events from an LMDB database and writes them into the given array.
//!
//! @param[in] events      The event array to write events into.
//! @param[in] event_size  The size of each event, in bytes.
//!
//! @return  Pointer to the events array.
FDBKeyValue* _load_lmdb_events(FDBKeyValue* events, int event_size) {
  return 0;
}

//! Counts the number of digits in the given integer.
//!
//! @param[in] n  The integer.
//!
//! @return  The number of digits.
int _count_digits(int n) {
  if (n == 0) {
    return 1;
  }
  int count = 0;
  while (n != 0) {
    n = n / 10;
    ++count;
  }
  return count;
}