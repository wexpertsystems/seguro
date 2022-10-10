/// @file fdb.h
///
/// Declarations for functions used to manage interaction with a FoundationDB
/// cluster.
///
/// Documentation links:
///   https://apple.github.io/foundationdb/api-c.html#c.FDBNetworkOption
///   https://apple.github.io/foundationdb/known-limitations.html
///   https://gist.github.com/ThatWilsonNerd/e850e9af5e426d565ea6
///   https://gist.github.com/kirilltitov/5fabd0905c4d5a300d5d00b4647ebc53
///
///

#pragma once

#include <foundationdb/fdb_c.h>
#include <pthread.h>
#include <stdint.h>

#include "event.h"

#define FDB_KEY_TOTAL_LENGTH                                                   \
  (1 + FDB_KEY_EVENT_LENGTH + FDB_KEY_FRAGMENT_LENGTH)
#define FDB_KEY_EVENT_LENGTH 8
#define FDB_KEY_FRAGMENT_LENGTH 4

//==============================================================================
// Variables
//==============================================================================

extern FDBDatabase *fdb_database;
extern pthread_t fdb_network_thread;

//==============================================================================
// Prototypes
//==============================================================================

/// Initialize a connection to a FoundationDB cluster.
void fdb_init_database(void);

/// Initialize an asynchronous helper process for interacting with a
/// FoundationDB cluster.
void fdb_init_network_thread(void);

/// Shutdown the connection to a FoundationDB cluster.
void fdb_shutdown_database(void);

/// Shutdown the asynchronous helper process for interacting with a FoundationDB
/// cluster.
int fdb_shutdown_network_thread(void);

/// Set the maximum batch size of event fragments in a single write transaction.
///
/// @param[in] batch_size  The new maximum batch size (must be greater than 0).
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_set_batch_size(uint32_t batch_size);

/// Setup a handle for a new FoundationDB transaction.
///
/// @param[in] tx  Memory address to write the new transaction handle into.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_setup_transaction(FDBTransaction **tx);

/// Attempt to synchronously apply a FoundationDB write transaction.
///
/// @param[in] tx  Handle for the transaction containing writes/clears.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_send_transaction(FDBTransaction *tx);

/// Write a single batch of fragments from a single event and, on success,
/// updates the position variable to the first fragment not in included in the
/// batch.
///
/// @param[in] event  Handle for the event.
/// @param[in] pos    Position first fragment to write in the fragments array.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_write_batch(FragmentedEvent *event, uint32_t *pos);

/// Write a single event (fragmentation is automatic).
///
/// @param[in] event  Handle for the event to write.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_write_event(Event *event);

/// Write a single fragmented event.
///
/// @param[in] event  Handle for the fragmented event to write.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_write_fragmented_event(FragmentedEvent *event);

/// Write an array of fragmented events.
///
/// @param[in] events      Handle for the array of events to write.
/// @param[in] num_events  Number of events in the array.
///
/// @return  0  Success
/// @return -1  Failure
int fdb_write_fragmented_event_array(FragmentedEvent *f_events,
                                     uint32_t num_events);

/// Write an array of events.
///
/// @param[in] events      Handle for the array of events to write.
/// @param[in] num_events  Number of events in the array.
///
/// @return  0  Success
/// @return -1  Failure
int fdb_write_event_array(Event *events, uint32_t num_events);

/// Read event fragments from the database and combine them into one event.
///
/// @param[in] event  Handle for the event to write to.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_read_event(Event *event);

/// Read an array of events from the database.
///
/// @param[in] events       Handle for the event array.
/// @param[in] num_events   Number of events in array.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_read_event_array(Event *events, uint32_t num_events);

/// Remove a single fragmented event from the database.
///
/// @param[in] event  Handle for the event to remove.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_clear_event(FragmentedEvent *event);

/// Remove an array of fragmented events from the database.
///
/// @param[in] events       Handle for the array of events to remove.
/// @param[in] num_events   Number of events in the array.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_clear_event_array(FragmentedEvent *events, uint32_t num_events);

/// Remove all key-value pairs from the database.
///
/// @return  0  Success.
/// @return -1  Failure.
int fdb_clear_database(void);

/// Build the FoundationDB key for an event fragment.
///
/// @param[in] fdb_key   Pointer to the write location for the FoundationDB key.
/// @param[in] key       The unique event identifier.
/// @param[in] fragment  The fragment number.
void fdb_build_event_key(uint8_t *fdb_key, uint64_t key, uint32_t fragment);

/// Check if a FoundationDB API command returned an error. If so, print the
/// error description.
///
/// @param[in] err  FoundationDB error code.
///
/// @return  The input FoundationDB error code.
fdb_error_t fdb_check_error(fdb_error_t err);
