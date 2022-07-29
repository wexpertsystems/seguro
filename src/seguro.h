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

#define FDB_API_VERSION 610
#include <foundationdb/fdb_c.h>

#define CLUSTER_NAME    "fdb.cluster"
#define DB_NAME         "DB"
#define MAX_VALUE_SIZE  10000
#define MAX_RETRIES     5


//==============================================================================
// Types
//==============================================================================

//==============================================================================
// External variables.
//==============================================================================

//! Events to be used for the benchmark.
extern FDBKeyValue events[];

//==============================================================================
// Functions
//==============================================================================

//! Runs read and write benchmarks with mock events.
//!
//! @return 0  Success.
//! @return 1  Failure.
int run_benchmark_mock(int num_events, int event_size, int batch_size) {
  // Error keeper.
  int err;

  // Generate mock events and load them into our global `events` array.
  err = _load_mock_events(events, num_events, event_size);

  // Initialize FoundationDB.
  FDBDatabase *fdb;
  err = _fdb_init(fdb);
  if (err != 0) {
    exit(err);
  }

  // Run the batch write benchmark (one tx per batch of events written).
  // if (err != 0) {
  //   err = _run_write_event_batch_benchmark(batch_size);
  // }
  // else {
  //   exit(err);
  // }

  // Run the single write benchmark (one tx per event written).
  if (err != 0) {
    err = _run_write_event_benchmark(fdb, num_events);
  }
  else {
    exit(err);
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
  clock_t start, end;
  double cpu_time_used;

  start = clock();

  // Do stuff here.

  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);

  // Success.
  return 0;
}

//! Runs the write benchmark. Prints results.
//!
//! @param[in] num_events  The number of events to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _run_write_event_benchmark(FDBDatabase* fdb, int num_events) {
  clock_t start, end;
  double cpu_time_used;

  // Start the timer.
  start = clock();

  // Iterate through events and write each to the database.
  for (int i = 0; i < num_events; i++) {

    // Error.
    int err;

    // Create a new database transaction with `fdb_database_create_transaction()`.
    FDBTransaction *tx;
    err = fdb_database_create_transaction(fdb, &tx);

    // Get the key/value pair from the events array.
    char *key = events[i].key;
    int key_length = events[i].key_length;
    char *value = events[i].value;
    int value_length = events[i].value_length;

    // Create a transaction with a write of a single key/value pair.
    if (err != 0) {
      exit(err);
    }
    fdb_transaction_set(tx, key, key_length, value, value_length);

    // Commit the transaction.
    FDBFuture *future;
    future = fdb_transaction_commit(tx);

    // Wait for the future to be ready.
    err = fdb_future_block_until_ready(future);
    if (err != 0) {
      exit(err);
    }

    // Check that the future did not return any errors.
    err = fdb_future_get_error(future);
    if (err != 0) {
      exit(err);
    }

    // Destroy the future.
    fdb_future_destroy(future);

    // Success.
    return 0;
  }

  // Stop the timer.
  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);

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

//! Executes fdb_run_network() (for use in a separate pthread).
//! Exits on error.
void* _fdb_init_run_network(void* arg) {
  fdb_error_t err = fdb_run_network();
  if (err != 0) {
    fprintf(stderr, "_fdb_init_run_network: %s\n", fdb_get_error(err));
    exit(-1);
  }
}

//! Initializes a FoundationDB connection, for use with top-level read/write 
//! initialization functions fdb_read_init() and fdb_write_init().
//!
//! @return  0  Success.
//! @return -1  Failure.
int _fdb_init(FDBDatabase* fdb) {
  const char *linux_cluster_path = "/etc/foundationdb/fdb.cluster";
  FDBFuture *fdb_future = NULL;
  
  // Check cluster file attributes, exit if not found.
  struct stat cluster_file_buffer;
  uint32_t cluster_file_stat = stat(linux_cluster_path, &cluster_file_buffer);
  if (cluster_file_stat != 0) {
    fprintf(stderr, "ERROR: no fdb.cluster file found at: %s\n", linux_cluster_path);
    return -1;
  }
  
  // Ensure correct FDB API version.
  fdb_error_t err;
  err = fdb_select_api_version(FDB_API_VERSION);
  if (err != 0) {
    fprintf(stderr, "fdb_init select api version: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Setup FDB network.
  err = fdb_setup_network();
  if (err != 0) {
    fprintf(stderr, "fdb_init failed to setup fdb network: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }
  pthread_t fdb_network_thread;
  pthread_create(&fdb_network_thread, NULL, _fdb_init_run_network, NULL);

  // Create the database.
  err = fdb_create_database((char *) linux_cluster_path, &fdb);
  if (err != 0) {
    fprintf(stderr, "fdb_init create database: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Stop the network.
  err = fdb_stop_network();
  if (err != 0) {
    fprintf(stderr, "fdb_init stop network: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }
  err = pthread_join(fdb_network_thread, NULL);
  if (err != 0) {
    fprintf(stderr, "fdb_init wait for network thread: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Success.
  return 0;

  // FDB initialization error goto.
  fdb_init_error:
    if (fdb_future) {
      fdb_future_destroy(fdb_future);
    }
    return -1;
}

//! Writes mock events into the given array.
//!
//! @param[in] events      The event array to write events into.
//! @param[in] n           The number of events to write into the event array.
//! @param[in] event_size  The size of each event, in bytes.
//!
//! @return 0  Success. 
int _load_mock_events(FDBKeyValue events[], int num_events, int event_size) {
  // Seed the random number generator.
  srand(time(0));

  // Write random key/values into the given array.
  for (int i = 1; i <= num_events; i++) {
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
    events[i].key = key;
    events[i].key_length = key_length;
    events[i].value = value;
    events[i].value_length = event_size;
  }

  return 0;
}

//! Reads events from an LMDB database and writes them into the given array.
//!
//! @param[in] event_size  The size of each event, in bytes.
//! @param[in] events      The event array to write events into.
//!
//! @return 0  Success. 
int _load_lmdb_events(FDBKeyValue events[]) {
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