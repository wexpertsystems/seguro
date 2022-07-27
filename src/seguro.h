//! @file seguro.h
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <lmdb.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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

//! Event object.
typedef struct event {
  //! A non-negative integer formatted as an ASCII string.
  char* key;
  //! A byte-array.
  char* value;
} event_t;

//==============================================================================
// External variables.
//==============================================================================

//! Events to be used for the benchmark.
extern event_t events[];

//==============================================================================
// Functions
//==============================================================================

//! Reads a single event from the FoundationDB cluster, specified by the given 
//! key.
//! TODO: asynchronous or not?
//!
//! @return NULL  Failed to read event.
//! @return       Pointer to the event.
event_t* _read_event(int key) {
  return NULL;
}

//! Reads a batch of events from the FoundationDB cluster, specified by the 
//! given key range.
//! TODO: asynchronous or not?
//!
//! @return NULL  Failed to read the event batch.
//! @return       Pointer to the event array.
event_t* _read_event_batch(int start, int end) {
  return NULL;
}

//! Writes a single event to the FoundationDB cluster, asynchronously.
//!
//! @param[in] e  Event to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _write_event_async(event_t e) {
  return 0;
}

//! Writes a batch of events to the FoundationDB cluster, synchronously.
//!
//! @param[in] events  Events to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int _write_event_batch(event_t events[]) {
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
  const char* linux_cluster_path = "/etc/foundationdb/fdb.cluster";
  FDBFuture* fdb_future = NULL;
  
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
int _load_mock_events(event_t events[], int n, int event_size) {
  // Seed the random number generator.
  srand(time(0));

  // Write random key/values into the given array.
  for (int i = 0; i < n; i++) {
    // Generate a key.
    char key[50];
    sprintf(key, "%d", i);

    // Generate a value.
    char value[event_size]; 
    for (int j = 0; j < event_size; j++) {
      // 0-255 (all valid bytes).
      value[j] = rand() % 256;
    }

    // Write the key and value into a slot in the array.
    events[i].key = key;
    events[i].value = value;
  }

  return 0;
}

//! Reads events from an LMDB database and writes them into the given array.
//!
//! @param[in] event_size  The size of each event, in bytes.
//! @param[in] events      The event array to write events into.
//!
//! @return 0  Success. 
int _load_lmdb_events(event_t events[]) {
  return 0;
}