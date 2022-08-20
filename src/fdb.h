//! @file fdb.h
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#pragma once

#include <foundationdb/fdb_c.h>
#include <pthread.h>
#include <stdint.h>

#include "event.h"


//==============================================================================
// Variables
//==============================================================================

extern pthread_t fdb_network_thread;

//==============================================================================
// Prototypes
//==============================================================================

int fdb_set_batch_size(uint32_t batch_size);

//! Executes fdb_run_network() (for use in a separate thread). Exits on error.
void *fdb_init_run_network(void *arg);

//! Initializes a FoundationDB connection, for use with top-level read/write initialization functions.
//!
//! @return       FoundationDB database handle.
//! @return NULL  Failed.
FDBDatabase* fdb_init(void);

//! Shut down the FoundationDB network thread.
//!
//! @param[in] fdb  FoundationDB database handle.
//! @param[in] t    Handle for thread running FoudationDB network connection.
//!
//! @return  0  Success.
//! @return -1  Failure.
int fdb_shutdown(FDBDatabase *fdb, pthread_t *t);

int fdb_setup_transaction(FDBDatabase *fdb, FDBTransaction **tx);

int fdb_send_transaction(FDBDatabase *fdb, FDBTransaction *tx);

int fdb_write_batch(FDBDatabase *fdb, FragmentedEvent *event, uint32_t *pos);

int fdb_write_event(FDBDatabase *fdb, FragmentedEvent *event);

int fdb_write_event_array(FDBDatabase *fdb, FragmentedEvent *events, uint32_t num_events);

int fdb_clear_event(FDBDatabase *fdb, FragmentedEvent *event);

int fdb_clear_event_array(FDBDatabase *fdb, FragmentedEvent *events, uint32_t num_events);

//! Write a several events to FoundationDB in a single transaction.
//!
//! @param[in] fdb          FoundationDB database handle.
//! @param[in] events       Array of events to write.
//! @param[in] batch_size   Number of events to write.
//!
//! @return  0  Success.
//! @return -1  Failure.
