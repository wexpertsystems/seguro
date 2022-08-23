//! @file fdb.h
//!
//! Declarations for functions used to manage interaction with a FoundationDB cluster.

#pragma once

#include <foundationdb/fdb_c.h>
#include <pthread.h>
#include <stdint.h>

#include "event.h"


//==============================================================================
// Variables
//==============================================================================

// Handle for FoundationDB network thread which handles asynchronous reading/writing to/from the database.
extern pthread_t fdb_network_thread;

//==============================================================================
// Prototypes
//==============================================================================

//! Set the maximum batch size of events/event fragments writes in a single transaction.
//!
//! @param[in] batch_size   The new maximum batch size (must be greater than 0)
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_set_batch_size(uint32_t batch_size);

//! Executes fdb_run_network() (for use in a separate thread). Exits on error.
void *fdb_init_run_network(void *arg);

//! Initializes a FoundationDB connection, for use with top-level read/write initialization functions.
//!
//! @return       FoundationDB database handle
//! @return NULL  Failed
FDBDatabase* fdb_init(void);

//! Shut down the FoundationDB network thread.
//!
//! @param[in] fdb  FoundationDB database handle
//! @param[in] t    Handle for thread running FoudationDB network connection
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_shutdown(FDBDatabase *fdb, pthread_t *t);

//! Setup a new FoundationDB transaction.
//!
//! @param[in] fdb  FoundationDB database handle
//! @param[in] tx   Pointer to address into which to place the handle for the new transaction
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_setup_transaction(FDBDatabase *fdb, FDBTransaction **tx);

//! Attempt to synchronously apply a FoundationDB write transaction.
//!
//! @param[in] fdb  FoundationDB database handle
//! @param[in] tx   Handle for the transaction containing writes/clears
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_send_transaction(FDBDatabase *fdb, FDBTransaction *tx);

//! Write a single batch of fragments from a single event.
//!
//! @param[in] fdb    FoundationDB database handle
//! @param[in] event  Handle for the event whose fragments to write
//! @param[in] pos    Position of first fragment to write in the fragments array
//!
//! @return  0  Success; pos will be updated to the position of the first fragment not included in the batch
//! @return -1  Failure
int fdb_write_batch(FDBDatabase *fdb, FragmentedEvent *event, uint32_t *pos);

//! Write a single fragmented event.
//!
//! @param[in] fdb    FoundationDB database handle
//! @param[in] event  Handle for the event to write
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_write_event(FDBDatabase *fdb, FragmentedEvent *event);

//! Write an array of fragmented events.
//!
//! @param[in] fdb          FoundationDB database handle
//! @param[in] events       Handle for the array of events to write
//! @param[in] num_events   Number of events in the array
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_write_event_array(FDBDatabase *fdb, FragmentedEvent *events, uint32_t num_events);

//! Remove a single fragmented event from the database.
//!
//! @param[in] fdb    FoundationDB database handle
//! @param[in] event  Handle for the event to remove
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_clear_event(FDBDatabase *fdb, FragmentedEvent *event);

//! Remove an array of fragmented events from the database.
//!
//! @param[in] fdb          FoundationDB database handle
//! @param[in] events       Handle for the array of events to remove
//! @param[in] num_events   Number of events in the array
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_clear_event_array(FDBDatabase *fdb, FragmentedEvent *events, uint32_t num_events);
