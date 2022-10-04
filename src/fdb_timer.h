//! @file fdb_timer.h
//!
//! Additions/modifications to fdb.h for performing timed benchmarks.

#pragma once

#include <foundationdb/fdb_c.h>
#include <limits.h>
#include <time.h>


//==============================================================================
// Types
//==============================================================================

typedef struct benchmark_settings_t {
  uint32_t num_events;
  uint32_t num_frags;
  uint32_t batch_size;
} BenchmarkSettings;

typedef struct fdb_timer_t {
  clock_t  t_min;
  clock_t  t_max;
  double   t_total;
} FDBTimer;

typedef struct fdb_callback_data_t {
  clock_t        *start_t;
  FDBTransaction *tx;
  uint32_t       *txs_processing;
  FDBTimer       *timer;
  uint32_t       num_events;
  uint32_t       num_frags;
  uint32_t       batch_size;
} FDBCallbackData;

//==============================================================================
// Prototypes
//==============================================================================

//! Attempt to synchronously apply a FoundationDB write transaction, and time it.
//!
//! @param[in] tx                 Handle for the transaction containing writes/clears
//! @param[in] callback_function  Callback function for FDB to call upon tx commit success.
//! @param[in] callback_param     Optional callback parameters for FDB to pass to the callback function.
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_send_timed_transaction(FDBTransaction *tx, FDBCallback callback_function, void *callback_param);

//! Write an array of fragmented events and time the process.
//!
//! @param[in] events       Handle for the array of events to write
//! @param[in] num_events   Number of events in the array
//!
//! @return  0  Success
//! @return -1  Failure
int fdb_timed_write_event_array(FragmentedEvent *events, uint32_t num_events);

//! Asynchronously write an array of fragmented events and time the process.
//!
//! @param[in] events          Handle for the array of events to write
//! @param[in] num_events      Number of events in the array
//!
//! @return  0  Success.
//! @return -1  Failure.
int fdb_timed_write_event_array_async(FragmentedEvent *events, uint32_t num_events);

int fdb_clear_timed_database(uint32_t num_events, uint32_t num_fragments);
int fdb_clear_timed_database_async(uint32_t num_events, uint32_t num_fragments);

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
