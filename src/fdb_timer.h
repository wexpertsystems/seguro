//! @file fdb_timer.h
//!
//!

#pragma once

#include <foundationdb/fdb_c.h>
#include <limits.h>
#include <time.h>


//==============================================================================
// Variables
//==============================================================================

extern uint32_t fdb_batch_size;

//==============================================================================
// Types
//==============================================================================

typedef struct benchmark_config_t {
  uint32_t num_events;
  uint32_t num_frags;
  uint32_t batch_size;
} BenchmarkConfig;

typedef struct fdb_timer_t {
  clock_t  t_min;
  clock_t  t_max;
  double   t_total;
  uint32_t num_events;
  uint32_t num_frags;
  uint32_t batch_size;
} FDBTimer;

//==============================================================================
// Prototypes
//==============================================================================

//! Initialize a timed asynchronous helper process for interacting with a FoundationDB cluster.
//!
//! @param[in] num_events   Total number of events which plan to be written (for per-event times)
//! @param[in] num_frags    Number of fragments per event
//! @param[in] batch_size   Maximum size of a single batch (for pet-batch times)
void fdb_init_timed_network_thread(uint32_t num_events, uint32_t num_frags, uint32_t batch_size);

//! Initialize thread-specific data keys for timing FoundationDB writes.
void fdb_init_timed_thread_keys(void);

//! Shutdown thread-specific data keys for timing FoundationDB writes.
void fdb_shutdown_timed_thread_keys(void);

//! Attempt to synchronously apply a FoundationDB write transaction, and time it.
//!
//! @param[in] tx   Handle for the transaction containing writes/clears
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_send_timed_transaction(FDBTransaction *tx);

//! Write an array of fragmented events and timed the process.
//!
//! @param[in] events       Handle for the array of events to write
//! @param[in] num_events   Number of events in the array
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_timed_write_event_array(FragmentedEvent *events, uint32_t num_events);

//==============================================================================
// External Prototypes
//==============================================================================

//! Add a limited number of write operations for the fragments of an event to a FoundationDB transaction.
//!
//! @param[in] tx         FoundationDB transaction handle
//! @param[in] event      Fragmented event handle
//! @param[in] start_pos  Starting position in fragments array of event from which to begin writing
//! @param[in] limit      Absolute limit on the number of fragments to write in the transaction
//!
//! @return   Number of event fragments added to transaction
uint32_t add_event_set_transactions(FDBTransaction *tx, FragmentedEvent *event, uint32_t start_pos, uint32_t limit);

//! Check if a FoundationDB API command returned an error. If so, print the error description and exit.
//!
//! @param[in] err  FoundationDB error code
void check_error_bail(fdb_error_t err);
