//! @file fdb.h
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#pragma once

#include <foundationdb/fdb_c.h>
#include <lmdb.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define CLUSTER_NAME    "fdb.cluster"
#define DB_NAME         "DB"
#define MAX_VALUE_SIZE  10000
#define MAX_RETRIES     5


//==============================================================================
// Variables
//==============================================================================

extern pthread_t fdb_network_thread;

//==============================================================================
// Prototypes
//==============================================================================

//! Initializes a FoundationDB connection, for use with top-level read/write 
//! initialization functions fdb_read_init() and fdb_write_init().
//!
//! @return       Pointer to the FDBDatabase object.
//! @return NULL  Failed.
FDBDatabase* fdb_init(void);

//! Executes fdb_run_network() (for use in a separate thread).
//! Exits on error.
void* fdb_init_run_network(void* arg);

//! Shuts down the FoundationDB network.
//!
//! @return  0  Success.
//! @return -1  Failure.
int fdb_shutdown(FDBDatabase *fdb, pthread_t *t);

//! Reads an event from FoundationDB.
//!
//! @param[in] key  Event identifier.
//!
//! @return       Pointer to the event.
//! @return NULL  Failed.
FDBKeyValue* read_event(FDBDatabase *fdb, int key);

//! Asynchronously reads a batch of events from the FoundationDB cluster, 
//! specified by the given key range.
//!
//! @return       Pointer to the event array.
//! @return NULL  Failed to read the event batch.
FDBKeyValue* read_event_batch(FDBDatabase *fdb, int start, int end);

//! Asynchronously writes a single event to the FoundationDB cluster with a 
//! single FoundationDB transaction.
//!
//! @param[in] e  Event to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int write_event(FDBDatabase *fdb, FDBKeyValue *e);

//! Asynchronously writes a batch of events with a single FoundationDB 
//! transaction.
//!
//! @param[in] e  Events to write.
//!
//! @return 0  Success.
//! @return 1  Failure.
int write_event_batch(FDBDatabase *fdb, FDBKeyValue* e);
