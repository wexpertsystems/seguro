//! @file fdb_timer.h
//!
//!

#pragma once

#include <foundationdb/fdb_c.h>
#include <limits.h>
#include <time.h>


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
