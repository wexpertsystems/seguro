//! @file fdb_timer.c
//!
//!

#include <foundationdb/fdb_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#include "fdb.h"
#include "fdb_timer.h"


//==============================================================================
// Variables
//==============================================================================

//
extern       uint32_t fdb_batch_size;

// Key for thread-specific FDBTimer.
thread_local FDBTimer timer = { (clock_t)INT_MAX, (clock_t)0, 0.0 };

//==============================================================================
// Prototypes
//==============================================================================

//! Callback function for when an asynchronous FoundationDB transaction is applied successfully.
//!
//! @param[in] future   Handle for the FoundationDB future
//! @param[in] t_start  Handle for a clock_t containing the time at which the transaction was launched
void write_callback(FDBFuture *future, void *t_start);

//! Callback function
//!
//! @param[in] future     Handle for the FoundationDB future
//! @param[in] settings
void clear_callback(FDBFuture *future, void *settings);

void reset_timer(void);

//==============================================================================
// Functions
//==============================================================================

int fdb_send_timed_transaction(FDBTransaction *tx, FDBCallback callback_function, void *callback_param) {

  // Commit transaction
  FDBFuture *future = fdb_transaction_commit(tx);

  // Register callback
  if (fdb_check_error(fdb_future_set_callback(future, callback_function, callback_param))) goto tx_fail;

  // TODO: Synchornous; need to test asynchronous version
  // Wait for the future to be ready
  if (fdb_check_error(fdb_future_block_until_ready(future))) goto tx_fail;

  // Check that the future did not return any errors
  if (fdb_check_error(fdb_future_get_error(future))) goto tx_fail;

  // Destroy the future
  fdb_future_destroy(future);

  // Delete existing transaction object and create a new one
  fdb_transaction_reset(tx);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_timed_write_event_array(FragmentedEvent *events, uint32_t num_events) {

  clock_t        *start_t = malloc(sizeof(clock_t));
  FDBTransaction *tx;
  uint32_t        batch_filled = 0;
  uint32_t        frag_pos = 0;
  uint32_t        i = 0;

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // For each event
  while (i < num_events) {

    // Add as many unwritten fragments from the current event as possible (method differs slightly depending on whether
    // there are already other fragments in the batch)
    if (!batch_filled) {
      batch_filled = add_event_set_transactions(tx, (events + i), frag_pos, fdb_batch_size);
      frag_pos += batch_filled;
    } else {
      uint32_t num_kvp = add_event_set_transactions(tx, (events + i), frag_pos, (fdb_batch_size - batch_filled));
      batch_filled += num_kvp;
      frag_pos += num_kvp;
    }

    // Increment event counter when all fragments from an event have been written
    if (frag_pos == events[i].num_fragments) {
      i += 1;
      frag_pos = 0;
    }

    // Attempt to apply transaction when batch is filled
    if (batch_filled == fdb_batch_size) {
      // Start timer just before committing transaction
      *start_t = clock();

      if (fdb_check_error(fdb_send_timed_transaction(tx, (FDBCallback)&write_callback, (void *)start_t))) goto tx_fail;

      batch_filled = 0;
    }
  }

  // Catch the final, non-full batch
  *start_t = clock();
  if (fdb_check_error(fdb_send_timed_transaction(tx, (FDBCallback)&write_callback, (void *)start_t))) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Clean up the timer
  free((void *)start_t);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_timed_clear_database(uint32_t num_events, uint32_t num_fragments) {

  BenchmarkSettings *settings = malloc(sizeof(BenchmarkSettings));
  FDBTransaction    *tx;
  uint8_t            start_key[1] = { 0 };
  uint8_t            end_key[1] = { 0xFF };

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // Add clear operation to transaction
  fdb_transaction_clear_range(tx, start_key, 1, end_key, 1);

  // Setup settings
  settings->num_events = num_events;
  settings->num_frags = num_fragments;
  settings->batch_size = fdb_batch_size;

  // Catch the final, non-full batch
  if (fdb_send_timed_transaction(tx, (FDBCallback)&clear_callback, (void *)settings)) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Clean up settings
  free((void *)settings);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

void write_callback(FDBFuture *future, void *param) {

  // Output timing stats
  clock_t t_start = *((clock_t *)param);
  clock_t t_end = clock();
  clock_t t_diff = (t_end - t_start);
  double  total_time = 1000.0 * t_diff / CLOCKS_PER_SEC;

  if (t_diff < timer.t_min) {
    timer.t_min = t_diff;
  }

  if (t_diff > timer.t_max) {
    timer.t_max = t_diff;
  }

  timer.t_total += total_time;
}

void clear_callback(FDBFuture *future, void *param) {

  BenchmarkSettings *settings = (BenchmarkSettings *)param;

  printf("Thread time to write events:     %.2f s\n", (timer.t_total / 1000.0));
  printf("Average time per event:          %.4f ms\n", (timer.t_total / settings->num_events));
  printf("Max batch time:                  %.4f ms\n", (1000.0 * timer.t_max / CLOCKS_PER_SEC));
  printf("Avg batch time:                  %.4f ms\n",
         (timer.t_total / (settings->num_events * settings->num_frags) * settings->batch_size));
  printf("Min batch time:                  %.4f ms\n", (1000.0 * timer.t_min / CLOCKS_PER_SEC));

  reset_timer();
}

void reset_timer(void) {

  timer.t_min = (clock_t)INT_MAX;
  timer.t_max = (clock_t)0;
  timer.t_total = 0.0;
}
