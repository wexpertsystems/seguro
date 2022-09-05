//! @file fdb.c
//!
//! Definitions for functions used to manage interaction with a FoundationDB cluster.

#include <foundationdb/fdb_c.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "constants.h"
#include "fdb.h"

// Approximate maximum number of range clears that fit in a FoundationDB transaction
#define CLEAR_BATCH_SIZE 75000


//==============================================================================
// Variables
//==============================================================================

FDBDatabase *fdb_database;
pthread_t    fdb_network_thread;
uint32_t     fdb_batch_size = 1;

//==============================================================================
// Prototypes
//==============================================================================

//! Network loop function for handling interactions with a FoundationDB cluster in a separate process.
void *network_thread_func(void *arg);

//! Add a limited number of write operations for the fragments of an event to a FoundationDB transaction.
//!
//! @param[in] tx         FoundationDB transaction handle
//! @param[in] event      Fragmented event handle
//! @param[in] start_pos  Starting position in fragments array of event from which to begin writing
//! @param[in] limit      Absolute limit on the number of fragments to write in the transaction
//!
//! @return   Number of event fragments added to transaction
uint32_t add_event_set_transactions(FDBTransaction *tx, FragmentedEvent *event, uint32_t start_pos, uint32_t limit);

//! Add a clear operation for all fragments of an event to a FoundationDB transaction.
//!
//! @param[in] tx         FoundationDB transaction handle
//! @param[in] event      Fragmented event handle
void add_event_clear_transaction(FDBTransaction *tx, FragmentedEvent *event);

//! Check if a FoundationDB API command returned an error. If so, print the error description and exit.
//!
//! @param[in] err  FoundationDB error code
void check_error_bail(fdb_error_t err);

//==============================================================================
// Functions
//==============================================================================

void fdb_init_database(void) {

  const char *cluster_file_path = "/etc/foundationdb/fdb.cluster";

  // Check cluster file attributes, exit if not found
  struct stat cluster_file_buffer;
  uint32_t cluster_file_stat = stat(cluster_file_path, &cluster_file_buffer);
  if (cluster_file_stat != 0) {
    printf("ERROR: no fdb.cluster file found at: %s\n", cluster_file_path);
    exit(1);
  }

  // Ensure correct FDB API version
  printf("Ensuring correct API version...\n");
  check_error_bail(fdb_select_api_version(FDB_API_VERSION));

  // Setup FDB network
  printf("Setting up network...\n");
  check_error_bail(fdb_setup_network());

  // Create the database
  printf("Creating the database...\n");
  check_error_bail(fdb_create_database((char *) cluster_file_path, &fdb_database));
}

void fdb_init_network_thread(void) {

  // Start the network thread
  if (pthread_create(&fdb_network_thread, NULL, network_thread_func, NULL)) {
    perror("pthread_create() error");
    fdb_shutdown_database();
    exit(-1);
  }
}

void fdb_shutdown_database(void) {

  // Destroy the database
  fdb_database_destroy(fdb_database);
}

int fdb_shutdown_network_thread(void) {

  int err;

  // Signal network shutdown
  err = fdb_check_error(fdb_stop_network());
  if (err) return err;

  // Stop the network thread
  if (pthread_join(fdb_network_thread, NULL)) {
    perror("pthread_join() error");
    return -1;
  }

  printf("Successfully shutdown network thread.\n");

  // Success
  return 0;
}

int fdb_set_batch_size(uint32_t batch_size) {

  if (!batch_size) return -1;

  fdb_batch_size = batch_size;
  return 0;
}

int fdb_setup_transaction(FDBTransaction **tx) {

  // Create a new database transaction (actually a snapshot of prospective diffs to apply as a single transaction)
  if (fdb_check_error(fdb_database_create_transaction(fdb_database, tx))) {
    // Failure
    return -1;
  }

  // Success
  return 0;
}

int fdb_send_transaction(FDBTransaction *tx) {

  // Commit event batch transaction
  FDBFuture *future = fdb_transaction_commit(tx);

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

int fdb_write_batch(FragmentedEvent *event, uint32_t *pos) {

  FDBTransaction *tx;
  uint32_t        num_out;

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // Add write events to transaction
  num_out = add_event_set_transactions(tx, event, *pos, fdb_batch_size);

  // Attempt to apply the transaction
  if (fdb_check_error(fdb_send_transaction(tx))) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);
  *pos += num_out;

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_write_event(FragmentedEvent *event) {

  FDBTransaction *tx;
  uint32_t        i = 0;

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // Write event fragments in maximal batches
  while (i < event->num_fragments) {
    i += add_event_set_transactions(tx, event, i, fdb_batch_size);

    if (fdb_check_error(fdb_send_transaction(tx))) goto tx_fail;
  }

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_write_event_array(FragmentedEvent *events, uint32_t num_events) {

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
      if (fdb_check_error(fdb_send_transaction(tx))) goto tx_fail;

      batch_filled = 0;
    }
  }

  // Catch the final, non-full batch
  if (fdb_check_error(fdb_send_transaction(tx))) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_clear_event(FragmentedEvent *event) {

  FDBTransaction *tx;

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // Add a clear operation for the event
  add_event_clear_transaction(tx, event);

  // Attempt to apply the transaction
  if (fdb_send_transaction(tx)) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_clear_event_array(FragmentedEvent *events, uint32_t num_events) {

  FDBTransaction *tx;

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // Add a clear operation for the each event, attempt to apply full batches
  for (uint32_t i = 0; i < num_events; ++i) {

    add_event_clear_transaction(tx, (events + i));

    if (!(i % CLEAR_BATCH_SIZE)) {
      if (fdb_send_transaction(tx)) goto tx_fail;
    }
  }

  // Catch the final, non-full batch
  if (fdb_send_transaction(tx)) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

int fdb_clear_database(void) {

  FDBTransaction *tx;
  uint8_t  start_key[1] = { 0 };
  uint8_t  end_key[1] = { 0xFF };

  // Initialize transaction
  if (fdb_check_error(fdb_setup_transaction(&tx))) goto tx_fail;

  // Add clear operation to transaction
  fdb_transaction_clear_range(tx, start_key, 1, end_key, 1);

  // Catch the final, non-full batch
  if (fdb_send_transaction(tx)) goto tx_fail;

  // Clean up the transaction
  fdb_transaction_destroy(tx);

  // Success
  return 0;

  // Failure
  tx_fail:
  return -1;
}

void fdb_build_event_key(uint8_t *fdb_key, uint64_t key, uint32_t fragment) {
  // FoundationDB has a rule that keys beginning with 0xff access a special key-space, so need to prepend a null byte
  fdb_key[0] = 0;

  for (uint8_t i = 0; i < FDB_KEY_EVENT_LENGTH; ++i) {
    fdb_key[(FDB_KEY_EVENT_LENGTH - i)] = ((uint8_t *)(& key))[i];
  }
  for (uint8_t i = 0; i < FDB_KEY_FRAGMENT_LENGTH; ++i) {
    fdb_key[(FDB_KEY_TOTAL_LENGTH - (i + 1))] = ((uint8_t *)(& fragment))[i];
  }
//  memcpy((fdb_key + 1), &key, FDB_KEY_EVENT_LENGTH);
//  memcpy((fdb_key + FDB_KEY_EVENT_LENGTH + 1), &fragment, FDB_KEY_FRAGMENT_LENGTH);
}

fdb_error_t fdb_check_error(fdb_error_t err) {
  if (err) {
    printf("fdb error: (%d) %s\n", err, fdb_get_error(err));
  }

  return err;
}

void *network_thread_func(void *arg) {

  printf("Starting network thread...\n");

  if (fdb_check_error(fdb_run_network())) return NULL;

  return NULL;
}

uint32_t add_event_set_transactions(FDBTransaction *tx, FragmentedEvent *event, uint32_t start_pos, uint32_t limit) {

  // Determine the number of fragments that are going to be written
  uint32_t max_pos = (start_pos + limit);
  uint32_t end_pos = (max_pos < event->num_fragments) ? max_pos : event->num_fragments;
  uint32_t num_kvp = end_pos - start_pos;
  uint8_t  key[FDB_KEY_TOTAL_LENGTH] = { 0 };

  // Special rules for first fragment
  if (!start_pos) {
    // First fragment contains header and has an irregularly sized payload
    uint32_t value_length = event->header_length + event->payload_length;
    uint8_t  value[value_length];

    memcpy(value, event->header, event->header_length);
    memcpy((value + event->header_length), event->fragments[0], event->payload_length);

    fdb_build_event_key(key, event->key, 0);

    fdb_transaction_set(tx,
                        key,
                        FDB_KEY_TOTAL_LENGTH,
                        value,
                        value_length);

    ++start_pos;
  }

  for (uint32_t i = start_pos; i < end_pos; ++i) {
    // Setup key for event fragment
    fdb_build_event_key(key, event->key, i);

    // Add write operation to transaction
    fdb_transaction_set(tx,
                        key,
                        FDB_KEY_TOTAL_LENGTH,
                        event->fragments[i],
                        OPTIMAL_VALUE_SIZE);
  }

  return num_kvp;
}

void add_event_clear_transaction(FDBTransaction *tx, FragmentedEvent *event) {

  uint8_t  range_start_key[FDB_KEY_TOTAL_LENGTH] = { 0 };
  uint8_t  range_end_key[FDB_KEY_TOTAL_LENGTH] = { 0 };

  // Setup start key for range
  fdb_build_event_key(range_start_key, event->key, 0);

  // Setup end key for range
  fdb_build_event_key(range_end_key, event->key, event->num_fragments);

  // Add clear operation to transaction
  fdb_transaction_clear_range(tx,
                              range_start_key,
                              FDB_KEY_TOTAL_LENGTH,
                              range_end_key,
                              FDB_KEY_TOTAL_LENGTH);
}

void check_error_bail(fdb_error_t err) {
  if (err) {
    printf("fdb error: (%d) %s\n", err, fdb_get_error(err));
    exit(-1);
  }
}
