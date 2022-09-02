//! @file fdb_timer.c
//!
//!

#include <foundationdb/fdb_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fdb.h"
#include "fdb_timer.h"


//==============================================================================
// Variables
//==============================================================================

// Key for thread-specific FDBTimer.
pthread_key_t timer_key;

//==============================================================================
// Prototypes
//==============================================================================

//! Network loop function for handling interactions with a FoundationDB cluster in a separate, timed process.
void *timed_network_thread_func(void *arg);

//! Callback function for when an asynchronous FoundationDB transaction is applied successfully.
//!
//! @param[in] future   Handle for the FoundationDB future
//! @param[in] t_start  Handle for a clock_t containing the time at which the transaction was launched
void timer_callback(FDBFuture *future, void *t_start);

//! Destructor for the thread-specific FDBTimer.
//!
//! @param[in] raw_ptr  Pointer to the FDBTimer
void timer_destructor(void *raw_ptr);

//! Check if a timed call to the FoundationDB API command returned an error. If so, print the error description.
//!
//! @param[in] err  FoundationDB error code
//!
//! @return   The input FoundationDB error code
fdb_error_t timed_check_error(fdb_error_t err);

//==============================================================================
// Functions
//==============================================================================

void fdb_init_timed_network_thread(uint32_t num_events, uint32_t num_frags, uint32_t batch_size) {

  BenchmarkConfig *config = malloc(sizeof(BenchmarkConfig));
  config->num_events = num_events;
  config->num_frags = num_frags;
  config->batch_size = batch_size;

  // Start the network thread
  if (pthread_create(&fdb_network_thread, NULL, timed_network_thread_func, config)) {
    perror("pthread_create() error");
    fdb_shutdown_database();
    exit(-1);
  }
}

void fdb_init_timed_thread_keys(void) {

  if (pthread_key_create(&timer_key, timer_destructor)) {
    perror("timer_key pthread_key_create() error");
    exit(-1);
  }
}

void fdb_shutdown_timed_thread_keys(void) {

  if (pthread_key_delete(timer_key)) {
    perror("timer_key pthread_key_delete() error");
  }
}

int fdb_send_timed_transaction(FDBTransaction *tx) {

  clock_t *start_t = malloc(sizeof(clock_t));

  // Start timer just before committing transaction
  *start_t = clock();
  FDBFuture *future = fdb_transaction_commit(tx);

  // Register callback
  if (timed_check_error(fdb_future_set_callback(future, &timer_callback, start_t))) goto tx_fail;

  // TODO: Synchornous; need to test asynchronous version
  // Wait for the future to be ready
  if (timed_check_error(fdb_future_block_until_ready(future))) goto tx_fail;

  // Check that the future did not return any errors
  if (timed_check_error(fdb_future_get_error(future))) goto tx_fail;

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

void *timed_network_thread_func(void *arg) {

  pthread_setspecific(timer_key, malloc(sizeof(FDBTimer)));

  FDBTimer *timer = pthread_getspecific(timer_key);
  timer->t_min = (clock_t)INT_MAX;
  timer->t_max = (clock_t)0;
  timer->t_total = 0.0;
  timer->num_events = ((BenchmarkConfig *)arg)->num_events;
  timer->num_frags = ((BenchmarkConfig *)arg)->num_frags;
  timer->batch_size = ((BenchmarkConfig *)arg)->batch_size;

  free(arg);

  printf("Starting network thread...\n");

  if (timed_check_error(fdb_run_network())) exit(-1);

  return timer;
}

void timer_callback(FDBFuture *future, void *t_start) {

  // Output timing stats
  FDBTimer* timer = pthread_getspecific(timer_key);
  clock_t   t_end = clock();
  clock_t   t_diff = (t_end - *((clock_t *)t_start));
  double    total_time = 1000.0 * t_diff / CLOCKS_PER_SEC;

  if (t_diff < timer->t_min) {
    timer->t_min = t_diff;
  }

  if (t_diff > timer->t_max) {
    timer->t_max = t_diff;
  }

  timer->t_total += total_time;

  free(t_start);
}

void timer_destructor(void *raw_ptr) {

  FDBTimer *timer = raw_ptr;

  printf("Thread time to write events:     %.2f s\n", (timer->t_total / 1000.0));
  printf("Average time per event:          %.4f ms\n", (timer->t_total / timer->num_events));
  printf("Max batch time:                  %.4f ms\n", (1000.0 * timer->t_max / CLOCKS_PER_SEC));
  printf("Avg batch time:                  %.4f ms\n",
         (timer->t_total / (timer->num_events * timer->num_frags) * timer->batch_size));
  printf("Min batch time:                  %.4f ms\n", (1000.0 * timer->t_min / CLOCKS_PER_SEC));

  free((void *)timer);
}

fdb_error_t timed_check_error(fdb_error_t err) {

  if (err) {
    printf("fdb error: (%d) %s\n", err, fdb_get_error(err));
  }

  return err;
}
