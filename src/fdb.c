//! @file fdb.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <foundationdb/fdb_c.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "fdb.h"


//==============================================================================
// Variables
//==============================================================================

pthread_t fdb_network_thread;

//==============================================================================
// Functions
//==============================================================================

void *fdb_init_run_network(void *arg) {

  printf("Starting network thread...\n\n");

  fdb_error_t err = fdb_run_network();
  if (err != 0) {
    printf("fdb_init_run_network: %s\n", fdb_get_error(err));
    exit(-1);
  }

  return NULL;
}

FDBDatabase *fdb_init(void) {

  const char *cluster_file_path = "/etc/foundationdb/fdb.cluster";
  FDBFuture *fdb_future = NULL;
  
  // Check cluster file attributes, exit if not found
  struct stat cluster_file_buffer;
  uint32_t cluster_file_stat = stat(cluster_file_path, &cluster_file_buffer);
  if (cluster_file_stat != 0) {
    printf("ERROR: no fdb.cluster file found at: %s\n", cluster_file_path);
    return NULL;
  }
  
  // Ensure correct FDB API version
  printf("Ensuring correct API version...\n");
  fdb_error_t err;
  err = fdb_select_api_version(FDB_API_VERSION);
  if (err != 0) {
    printf("fdb_init select api version: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Setup FDB network
  printf("Setting up network...\n");
  err = fdb_setup_network();
  if (err != 0) {
    printf("fdb_init failed to setup fdb network: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Start the network thread
  pthread_create(&fdb_network_thread, NULL, fdb_init_run_network, NULL);

  // Create the database
  printf("Creating the database...\n");
  FDBDatabase *fdb;
  err = fdb_create_database((char *) cluster_file_path, &fdb);
  if (err != 0) {
    printf("fdb_init create database: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // Success
  return fdb;

  // FDB initialization error goto.
  fdb_init_error:
  if (fdb_future) {
    fdb_future_destroy(fdb_future);
  }

  // Failure
  return NULL;
}

int fdb_shutdown(FDBDatabase *fdb, pthread_t *t) {

  fdb_error_t err;

  // Destroy the database
  fdb_database_destroy(fdb);

  // Signal network shutdown
  err = fdb_stop_network();
  if (err != 0) {
    printf("fdb_init stop network: %s\n", fdb_get_error(err));
    return -1;
  }

  // Stop the network thread
  err = pthread_join(*t, NULL);
  if (err) {
    printf("fdb_init wait for network thread: %d\n", err);
    return -1;
  }

  // Success
  return 0;
}

int write_event(FDBDatabase *fdb, Event *event) {
  return write_event_batch(fdb, event, 1);
}

int write_event_batch(FDBDatabase *fdb, Event *events, uint32_t batch_size) {

  FDBTransaction *tx;
  fdb_error_t err;

  // Create a new database "transaction" (actually a snapshot of diffs to apply as a single transaction)
  err = fdb_database_create_transaction(fdb, &tx);
  if (err != 0) {
    printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
    return -1;
  }

  // Write the key/value pair for each event as part of the transaction
  for (uint32_t i = 0; i < batch_size; ++i) {
    event_set_transaction(tx, (events + i));
  }

  // Commit event batch transaction
  FDBFuture *future;
  future = fdb_transaction_commit(tx);

  // TODO: Synchornous; need to test asynchronous version
  // Wait for the future to be ready
  err = fdb_future_block_until_ready(future);
  if (err != 0) {
    printf("fdb_future_block_until_ready error:\n%s", fdb_get_error(err));
    return -1;
  }

  // Check that the future did not return any errors
  err = fdb_future_get_error(future);
  if (err != 0) {
    printf("fdb_future_error:\n%s\n", fdb_get_error(err));
    return -1;
  }

  // Destroy the future
  fdb_future_destroy(future);

  return 0;
}

int clear_events(FDBDatabase *fdb, Event *events, uint32_t num_events) {

  FDBTransaction *tx;
  fdb_error_t err;

  // Create a new database "transaction" (actually a snapshot of diffs to apply as a single transaction)
  err = fdb_database_create_transaction(fdb, &tx);
  if (err != 0) {
    printf("fdb_database_create_transaction error:\n%s", fdb_get_error(err));
    return -1;
  }

  // Remove the key for each event
  for (uint32_t i = 0; i < num_events; ++i) {
    event_clear_transaction(tx, (events + i));
  }

  // Commit batched clear transaction
  FDBFuture *future;
  future = fdb_transaction_commit(tx);

  // TODO: Can clears be async?
  // Wait for the future to be ready
  err = fdb_future_block_until_ready(future);
  if (err != 0) {
    printf("fdb_future_block_until_ready error:\n%s", fdb_get_error(err));
    return -1;
  }

  // Check that the future did not return any errors
  err = fdb_future_get_error(future);
  if (err != 0) {
    printf("fdb_future_error:\n%s\n", fdb_get_error(err));
    return -1;
  }

  // Destroy the future.
  fdb_future_destroy(future);

  return 0;
}
