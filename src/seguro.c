//! @file seguro.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>

#define FDB_API_VERSION 610
#include <fdb_c.h>

#define CLUSTER_NAME    "fdb.cluster"
#define DB_NAME         "DB"
#define MAX_VALUE_SIZE  10000
#define MAX_RETRIES     5

//==============================================================================
// Functions
//==============================================================================

//! Executes fdb_run_network() (for use in a separate pthread).
//! Exits on error.
void _fdb_init_run_network() {
  fdb_error_t err = fdb_run_network();
  if (0 != err){
    fprintf(stderr, "_fdb_init_run_network: %s\n", fdb_get_error(err));
    exit(-1);
  }
}

//! Initializes a FoundationDB connection, for use with top-level read/write 
//! initialization functions fdb_read_init() and fdb_write_init().
//!
//! @return  0  Success.
//! @return -1  Failure.
void _fdb_init() {
  char* linux_cluster_path = "/etc/foundationdb/fdb.cluster";

  FDBDatabase* fdb = NULL;
  FDBFuture* fdb_future = NULL;
  
  // Check cluster file attributes, exit if not found.
  struct stat cluster_file_buffer;
  uint32_t cluster_file_stat = stat((char *) linux_cluster_path, &cluster_file_buffer);
  if (0 != cluster_file_stat){
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
  pthread_create(&fdb_network_thread, NULL, (void *) _fdb_init_run_network, NULL);

  // Create the database.
  fdb_future = fdb_create_database((char *) linux_cluster_path, &fdb);
  err = fdb_future_block_until_ready(fdb_future);
  if (err != 0){
    fprintf(stderr, "fdb_init create database: %s\n", fdb_get_error(err));
    goto fdb_init_error;
  }

  // FDB initialization error goto.
  fdb_init_error:
    if (fdb_future) {
      fdb_future_destroy(fdb_future);
    }
    return -1;
}