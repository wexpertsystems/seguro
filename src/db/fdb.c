//! @file fdb.c
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
// Variables
//==============================================================================

extern pthread_t fdb_network_thread;

//==============================================================================
// Functions
//==============================================================================

void* fdb_init_run_network(void* arg) {
  printf("Starting network thread...\n\n");
  fdb_error_t err = fdb_run_network();
  if (err != 0) {
    printf("fdb_init_run_network: %s\n", fdb_get_error(err));
    exit(-1);
  }
}

FDBDatabase* fdb_init() {
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
  pthread_create(&fdb_network_thread, NULL, fdb_init_run_network, NULL);

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

int fdb_shutdown(FDBDatabase* fdb, pthread_t *t) {
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
  err = pthread_join(*t, NULL);
  if (err) {
    printf("fdb_init wait for network thread: %d\n", err);
    return -1;
  }

  return 0;
}

FDBKeyValue* read_event(int key) {
  // Create transaction object.
  FDBTransaction *fdb_tx;

  // Create a new database transaction with `fdb_database_create_transaction()`.
  // Create an `FDBFuture` and set it to the return of `fdb_transaction_get()`.
  // Block event loop with `fdb_future_block_until_ready()`, retry if needed.
  // Get the value from the `FDBFuture` with `fdb_future_get_value()`.
  // Destroy the `FDBFuture` with `fdb_future_destroy()`.
  return NULL;
}

FDBKeyValue* read_event_batch(int start, int end) {
  return NULL;
}

int write_event(FDBKeyValue e) {
  return 0;
}

int write_event_batch(FDBKeyValue events[]) {
  return 0;
}
