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

//! Initializes a FoundationDB connection, for use with top-level read/write initialization functions.
//!
//! @return       FoundationDB database handle.
//! @return NULL  Failed.
FDBDatabase* fdb_init(void);

//! Executes fdb_run_network() (for use in a separate thread). Exits on error.
void *fdb_init_run_network(void *arg);

//! Shut down the FoundationDB network thread.
//!
//! @param[in] fdb  FoundationDB database handle.
//! @param[in] t    Handle for thread running FoudationDB network connection.
//!
//! @return  0  Success.
//! @return -1  Failure.
int fdb_shutdown(FDBDatabase *fdb, pthread_t *t);

//! Write a single event to FoundationDB in a single transaction.
//!
//! @param[in] fdb    FoundationDB database handle.
//! @param[in] event  Event to write.
//!
//! @return  0  Success.
//! @return -1  Failure.
int write_event(FDBDatabase *fdb, Event *event);

//! Write a several events to FoundationDB in a single transaction.
//!
//! @param[in] fdb          FoundationDB database handle.
//! @param[in] events       Array of events to write.
//! @param[in] batch_size   Number of events to write.
//!
//! @return  0  Success.
//! @return -1  Failure.
int write_event_batch(FDBDatabase *fdb, Event *events, uint32_t batch_size);

//! Remove given events from FoundationDB.
//!
//! @param[in] fdb          FoundationDB database handle.
//! @param[in] events       Array of events to remove.
//! @param[in] batch_size   Number of events in array.
//!
//! @return  0  Success.
//! @return -1  Failure.
int clear_events(FDBDatabase *fdb, Event *events, uint32_t num_events);
