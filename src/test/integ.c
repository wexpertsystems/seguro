//! @file integ.c
//!
//! Integration tests for Seguro

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../constants.h"
#include "../event.h"
#include "../fdb.h"


//==============================================================================
// Prototypes
//==============================================================================

//! Test that data can be written to a FoundationDB cluster through the C API.
void test_write_to_fdb(void);

//! Test that data can be cleared from a FoundationDB cluster through the C API.
void test_clear_from_fdb(void);

//! Test that a single event can be cleared from a FoundationDB cluster in its entirety.
void test_clear_event(void);

//! Test that an array of events can be cleared from a FoundationDB cluster in their entirety.
void test_clear_event_array(void);

//! Test that all data can be cleared from a FoundationDB cluster in a single transaction.
void test_clear_database(void);

//! Test that a portion of an event the size of one batch can be written to a FoundationDB cluster.
void test_write_batch(void);

//! Test that an event can be written to a FoundationDB cluster in its entirety.
void test_write_event(void);

//! Test that an array of events can be written to a FoundationDB cluster in their entirety.
void test_write_event_array(void);

//! Test that an event can be read from a FoundationDB cluster in its entirety.
void test_read_event(void);

//! Generate random, fake data for simulating events.
//!
//! @param[in] size   Number of bytes of data to generate
//!
//! @return   Handle to array of generated data
uint8_t *generate_dummy_data(uint64_t size);

//! Count the number of keys stored in the FoundationDB cluster referenced by a FDBTransaction.
//!
//! @param[in] tx   Handle to a FoundationDB transaction
//!
//! @return   Number of keys stored in the FoundationDB cluster
uint32_t count_keys_in_database(FDBTransaction *tx);

//! Count the number of keys stored for a particular event in the FoundationDB cluster referenced by a FDBTransaction.
//!
//! @param[in] tx   Handle to a FoundationDB transaction
//!
//! @return   Number of event keys stored in the FoundationDB cluster
uint32_t count_event_fragments_in_database(FDBTransaction *tx, uint64_t event_id);

//! Gracefully fail a test by cleaning up before exiting.
void fail_test(void);

//==============================================================================
// Functions
//=============================================================================

//! Execute the Seguro integration tests.
//!
//! @param[in] argc  Number of command-line options provided
//! @param[in] argv  Array of command-line options provided
//!
//! @return  0  Success
//! @return -1  Failure
int main(int argc, char **argv) {

  printf("Starting integration tests...\n");

  // Initialize FoundationDB database
  fdb_init_database();
  fdb_init_network_thread();

  // Run integration tests
  // TODO: Could fail tests more gracefully, using calls to 'fail_test()' instead of asserts
  test_write_to_fdb();
  test_clear_from_fdb();
  test_clear_event();
  test_clear_event_array();
  test_clear_database();
  test_write_batch();
  test_write_event();
  test_write_event_array();
  test_read_event();

  // Success
  printf("\nIntegration tests completed successfully.\n");

  // Clean up FoundationDB database
  fdb_shutdown_network_thread();
  fdb_shutdown_database();

  return 0;
}

void test_write_to_fdb(void) {

  FDBFuture      *future;
  FDBTransaction *tx;
  uint8_t         num_tests = 5;
  uint8_t         dummy_size = 10;
  uint8_t         dummy_keys[num_tests];
  uint8_t        *dummy_data[num_tests];

  printf("\nStarting simple FDB write test...\n");

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // Add write operations to transaction for dummy keys with dummy data
  for (uint8_t i = 0; i < num_tests; ++i) {
    dummy_keys[i] = i;
    dummy_data[i] = generate_dummy_data(dummy_size);

    fdb_transaction_set(tx, (dummy_keys + i), 1, dummy_data[i], dummy_size);
  }

  // Apply transaction to database
  if (fdb_send_transaction(tx)) fail_test();

  // Check that each key exists in the database and that it has the correct value data
  for (uint8_t i = 0; i < num_tests; ++i) {
    fdb_bool_t     has_value;
    const uint8_t *value;
    int32_t        value_length;

    future = fdb_transaction_get(tx, (dummy_keys + i), 1, 0);

    if (fdb_check_error(fdb_future_block_until_ready(future))) fail_test();
    if (fdb_check_error(fdb_future_get_error(future))) fail_test();
    if (fdb_check_error(fdb_future_get_value(future, &has_value, &value, &value_length))) fail_test();

    assert(has_value);
    assert(value_length == dummy_size);
    assert(!memcmp((const char *)dummy_data[i], (const char *)value, dummy_size));

    fdb_future_destroy(future);
  }

  // Release the dummy data memory
  for (uint8_t i = 0; i < num_tests; ++i) {
    free((void *)dummy_data[i]);
  }

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Success
  printf("Simple FDB write test PASSED\n");
}

void test_clear_from_fdb(void) {

  FDBFuture      *future;
  FDBTransaction *tx;
  uint8_t         num_tests = 5;
  uint8_t         dummy_keys[num_tests];

  printf("\nStarting simple FDB clear test...\n");

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is not empty before test
  assert(count_keys_in_database(tx) != 0);

  // Add clear operations to the transaction for each key from the previous test
  for (uint8_t i = 0; i < num_tests; ++i) {
    dummy_keys[i] = i;
    fdb_transaction_clear(tx, (dummy_keys + i), 1);
  }

  // Apply transaction to database
  if (fdb_send_transaction(tx)) fail_test();

  // Check that each key no longer exists in the database
  for (uint8_t i = 0; i < num_tests; ++i) {
    fdb_bool_t     has_value;
    const uint8_t *value;
    int32_t        value_length;

    future = fdb_transaction_get(tx, (dummy_keys + i), 1, 0);

    if (fdb_check_error(fdb_future_block_until_ready(future))) fail_test();
    if (fdb_check_error(fdb_future_get_error(future))) fail_test();
    if (fdb_check_error(fdb_future_get_value(future, &has_value, &value, &value_length))) fail_test();

    assert(!has_value);

    fdb_future_destroy(future);
  }

  // Check that the database is completely empty
  assert(count_keys_in_database(tx) == 0);

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Success
  printf("Simple FDB clear test PASSED\n");
}

void test_clear_event(void) {

  FDBTransaction *tx;
  FragmentedEvent dummy_event;
  uint64_t        event_id = 42;
  uint32_t        num_fragments = 10;
  uint16_t        dummy_size = 10000;
  uint8_t        *dummy_data[num_fragments];

  printf("\nStarting fdb_clear_event() test...\n");

  // Setup dummy fragmented event
  dummy_event.id = event_id;
  dummy_event.num_fragments = num_fragments;

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // Manually add keys for a dummy event to the database
  for (uint32_t i = 0; i < num_fragments; ++i) {
    uint8_t key[FDB_KEY_TOTAL_LENGTH];

    fdb_build_event_key(key, event_id, i);

    dummy_data[i] = generate_dummy_data(dummy_size);

    fdb_transaction_set(tx, key, FDB_KEY_TOTAL_LENGTH, dummy_data[i], dummy_size);
  }

  if (fdb_send_transaction(tx)) fail_test();

  // Verify that the event is in the database
  assert(count_event_fragments_in_database(tx, event_id) == num_fragments);
  assert(count_keys_in_database(tx) == num_fragments);

  // fdb_clear_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Attempt to remove the event from the database
  fdb_clear_event(&dummy_event);

  // Need a new transaction handle to read from the database
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that no event fragments still exist in the database
  assert(count_event_fragments_in_database(tx, event_id) == 0);
  assert(count_keys_in_database(tx) == 0);

  // Release the dummy data memory
  for (uint8_t i = 0; i < num_fragments; ++i) {
    free((void *)dummy_data[i]);
  }

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Success
  printf("fdb_clear_event() test PASSED\n");
}

void test_clear_event_array(void) {

  Event           *mock_events;
  FragmentedEvent *mock_f_events;
  FDBTransaction  *tx;
  uint32_t         num_events = 10;
  uint8_t          data_size = 10;

  printf("\nStarting fdb_clear_event_array() test...\n");

  // Setup events
  mock_events = malloc(sizeof(Event) * num_events);
  mock_f_events = malloc(sizeof(FragmentedEvent) * num_events);

  for (uint8_t i = 0; i < num_events; ++i) {
    // Want deterministic keys that are unordered and don't collide
    mock_events[i].id = (((lrint(pow(10.0, i))) + i) % 90000);
    mock_events[i].data_length = data_size;
    mock_events[i].data = generate_dummy_data(data_size);

    fragment_event((mock_events + i), (mock_f_events + i));
  }

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // Manually add keys for a dummy event to the database
  for (uint32_t i = 0; i < num_events; ++i) {
    uint8_t key[FDB_KEY_TOTAL_LENGTH];
    fdb_build_event_key(key, mock_f_events[i].id, 0);

    fdb_transaction_set(tx, key, FDB_KEY_TOTAL_LENGTH, mock_f_events[i].fragments[0], mock_f_events[i].payload_length);
  }

  if (fdb_send_transaction(tx)) fail_test();

  // Verify that the events are in the database
  for (uint32_t i = 0; i < num_events; ++i) {
    assert(count_event_fragments_in_database(tx, mock_f_events[i].id) == 1);
  }

  // fdb_clear_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Attempt to remove the event from the database
  fdb_clear_event_array(mock_f_events, num_events);

  // Need a new transaction handle to read from the database
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that no event fragments still exist in the database
  assert(count_keys_in_database(tx) == 0);

  // Release the dummy data memory
  for (uint8_t i = 0; i < num_events; ++i) {
    free_event(mock_events + i);
  }
  free((void *)mock_f_events);
  free((void *)mock_events);

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Success
  printf("fdb_clear_event_array() test PASSED\n");
}

void test_clear_database(void) {

  Event           *mock_events;
  FragmentedEvent *mock_f_events;
  FDBTransaction  *tx;
  uint32_t         num_events = 5;
  uint32_t         num_fragments = 5;
  uint32_t         data_size = (num_fragments * OPTIMAL_VALUE_SIZE);

  printf("\nStarting fdb_clear_database() test...\n");

  // Setup events
  mock_events = malloc(sizeof(Event) * num_events);
  mock_f_events = malloc(sizeof(FragmentedEvent) * num_events);

  for (uint8_t i = 0; i < num_events; ++i) {
    mock_events[i].id = i;
    mock_events[i].data_length = data_size;
    mock_events[i].data = generate_dummy_data(data_size);

    fragment_event((mock_events + i), (mock_f_events + i));
  }

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // Manually add keys for a dummy event to the database
  for (uint32_t i = 0; i < num_events; ++i) {
    uint8_t key[FDB_KEY_TOTAL_LENGTH];

    for (uint8_t j = 0; j < num_fragments; ++j) {
      fdb_build_event_key(key, mock_f_events[i].id, j);

      fdb_transaction_set(tx, key, FDB_KEY_TOTAL_LENGTH, mock_f_events[i].fragments[j], OPTIMAL_VALUE_SIZE);
    }
  }

  if (fdb_send_transaction(tx)) fail_test();

  // Verify that the events are in the database
  assert(count_keys_in_database(tx) == (num_events * num_fragments));

  // fdb_clear_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Attempt to remove the event from the database
  fdb_clear_database();

  // Need a new transaction handle to read from the database
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that no event fragments still exist in the database
  assert(count_keys_in_database(tx) == 0);

  // Release the dummy data memory
  for (uint8_t i = 0; i < num_events; ++i) {
    free_event(mock_events + i);
  }
  free((void *)mock_f_events);
  free((void *)mock_events);

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Success
  printf("fdb_clear_database() test PASSED\n");
}

void test_write_batch(void) {

  FDBTransaction  *tx;
  Event            mock_event;
  FragmentedEvent  mock_f_event;
  uint64_t         event_id = 42;
  uint32_t         num_fragments = 3;
  uint32_t         fragment_pos = 0;
  uint32_t         data_size = (num_fragments * OPTIMAL_VALUE_SIZE);
  uint8_t          batch_count = 0;

  printf("\nStarting fdb_write_batch() test...\n");

  // Setup FoundationDB batch settings
  fdb_set_batch_size(1);

  // Setup event
  mock_event.id = event_id;
  mock_event.data_length = data_size;
  mock_event.data = generate_dummy_data(data_size);

  fragment_event(&mock_event, &mock_f_event);

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // fdb_write_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Write event one batch at a time, counting batches
  while (fragment_pos != num_fragments) {
    fdb_write_batch(&mock_f_event, &fragment_pos);
    ++batch_count;
  }

  // Verify that the event was written one fragment at a time
  assert(batch_count = num_fragments);

  // Need a new transaction handle to read from the database
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that the events are in the database
  assert(count_keys_in_database(tx) == num_fragments);
  assert(count_event_fragments_in_database(tx, event_id) == num_fragments);

  // Release the dummy data memory
  free((void *)mock_event.data);

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Clear the database
  fdb_clear_database();

  // Success
  printf("fdb_write_batch() test PASSED\n");
}

void test_write_event(void) {

  FDBTransaction  *tx;
  Event            mock_event;
  FragmentedEvent  mock_f_event;
  uint64_t         event_id = 42;
  uint32_t         num_fragments = 3;
  uint32_t         data_size = (num_fragments * OPTIMAL_VALUE_SIZE);

  printf("\nStarting fdb_write_event() test...\n");

  // Setup FoundationDB batch settings
  fdb_set_batch_size(1);

  // Setup event
  mock_event.id = event_id;
  mock_event.data_length = data_size;
  mock_event.data = generate_dummy_data(data_size);

  fragment_event(&mock_event, &mock_f_event);

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // fdb_write_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Attempt to write event to FoundationDB cluster
  fdb_write_event(&mock_f_event);

  // Need a new transaction handle to read from the database
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that the events are in the database
  assert(count_keys_in_database(tx) == num_fragments);
  assert(count_event_fragments_in_database(tx, event_id) == num_fragments);

  // Release the dummy data memory
  free((void *)mock_event.data);

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Clear the database
  fdb_clear_database();

  // Success
  printf("fdb_write_event() test PASSED\n");
}

void test_write_event_array(void) {

  FDBTransaction  *tx;
  Event           *mock_events;
  FragmentedEvent *mock_f_events;
  uint32_t         num_events = 4;

  printf("\nStarting fdb_write_event_array() test...\n");

  // Setup FoundationDB batch settings
  fdb_set_batch_size(100);

  // Setup events
  mock_events = malloc(sizeof(Event) * num_events);
  mock_f_events = malloc(sizeof(FragmentedEvent) * num_events);

  for (uint8_t i = 0; i < num_events; ++i) {
    // Want to test mixing events, splitting events in the middle, and multiple fragments for a single event
    uint32_t data_size = ((lrint(pow(10.0, i))) * OPTIMAL_VALUE_SIZE);

    mock_events[i].id = i;
    mock_events[i].data_length = data_size;
    mock_events[i].data = generate_dummy_data(data_size);

    fragment_event((mock_events + i), (mock_f_events + i));
  }

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // fdb_write_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Attempt to write events to FoundationDB cluster
  fdb_write_event_array(mock_f_events, num_events);

  // Need a new transaction handle to read from the database
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that the events are in the database
  for (uint8_t i = 0; i < num_events; ++i) {
    assert(count_event_fragments_in_database(tx, mock_f_events[i].id) == mock_f_events[i].num_fragments);
  }

  // Release the dummy data memory
  for (uint8_t i = 0; i < num_events; ++i) {
    free_event(mock_events + i);
  }
  free((void *)mock_f_events);
  free((void *)mock_events);

  // Release the transaction handle
  fdb_transaction_destroy(tx);

  // Clear the database
  fdb_clear_database();

  // Success
  printf("fdb_write_event_array() test PASSED\n");
}

void test_read_event(void) {

  FDBTransaction  *tx;
  Event            mock_event, return_event;
  FragmentedEvent  mock_f_event;
  uint64_t         event_id = 42;
  uint32_t         data_size = (3 * OPTIMAL_VALUE_SIZE);

  printf("\nStarting fdb_read_event() test...\n");

  // Setup FoundationDB batch settings
  fdb_set_batch_size(10);

  // Setup event
  mock_event.id = event_id;
  mock_event.data_length = data_size;
  mock_event.data = generate_dummy_data(data_size);

  fragment_event(&mock_event, &mock_f_event);

  return_event.id = event_id;

  // Setup transaction handle
  if (fdb_check_error(fdb_setup_transaction(&tx))) fail_test();

  // Verify that database is empty before test
  assert(count_keys_in_database(tx) == 0);

  // fdb_write_event() uses its own transaction, so we need to discard ours
  fdb_transaction_destroy(tx);

  // Write event to FoundationDB cluster
  fdb_write_event(&mock_f_event);

  // Attempt to read event back from FoundationDB cluster
  fdb_read_event(&return_event);

  // Verify that output data matches input data
  assert(mock_event.data_length == return_event.data_length);
  assert(!memcmp(mock_event.data, return_event.data, data_size));

  // Release the dummy data memory
  free((void *)mock_event.data);
  free((void *)return_event.data);

  // Clear the database
  fdb_clear_database();

  // Success
  printf("fdb_read_event() test PASSED\n");
}

uint8_t *generate_dummy_data(uint64_t size) {

  uint8_t *result = malloc(sizeof(uint8_t) * size);

  // Seed the random number generator
  srand(clock());

  // Generate random byte data
  for (uint64_t i = 0; i < size; ++i) {
    result[i] = rand() % 256;
  }

  // Success
  return result;
}

/*
 * Found on the FDB forums:
 *
 * "The fdb_transaction_get_range() operation returns data one batch at a time, meaning that you are supposed to check
 *  the value of out_more to know whether you need to call it again to get more keys for the range. The
 *  FDB_STREAMING_MODE_WANT_ALL streaming mode does not mean 'in a single batch'; it should be understood as 'in as few
 *  batches as possible'. The other streaming modes are for cases where the user plans to inspect data as it arrives and
 *  stop iterating on some end condition, possibly well before the end of the range.
 *
 *  Consider the hypothetical case of 5000 keys in a given range. It's possible that FDB will only return 3000 keys:
 *  what it considers to be the ideal batch size for this get request. In this case, the user would need to check if
 *  out_more is non-zero after the call, and call the fdb_transaction_get_range again using the
 *  FDB_KEYSEL_FIRST_GREATER_THAN macro on the last key of the previous batch. The user would need to repeat this until
 *  one such transaction returned an out_more value of 0 - or more specifically, until one such transaction failed to
 *  set out_more to 1."
 */
uint32_t count_keys_in_database(FDBTransaction *tx) {

  FDBFuture         *future;
  const FDBKeyValue *out_kv;
  fdb_bool_t         out_more;
  uint32_t           out_total = 0;
  int32_t            out_count;
  uint8_t            range_start_key = 0;
  uint8_t            range_end_key = 0xFF;

  // Loop until FoundationDB says there is no more data
  do {
    out_more = 0;
    future = fdb_transaction_get_range(tx,
                                       &range_start_key, 0, 0, (out_total + 1),
                                       &range_end_key, 1, 0, 1,
                                       0, 0, FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);

    if (fdb_check_error(fdb_future_block_until_ready(future))) fail_test();
    if (fdb_check_error(fdb_future_get_error(future))) fail_test();
    if (fdb_check_error(fdb_future_get_keyvalue_array(future, &out_kv, &out_count, &out_more))) fail_test();

    out_total += out_count;

    fdb_future_destroy(future);
  } while (out_more);

  return (uint32_t)out_total;
}

uint32_t count_event_fragments_in_database(FDBTransaction *tx, uint64_t event_id) {

  FDBFuture         *future;
  const FDBKeyValue *out_kv;
  fdb_bool_t         out_more;
  uint32_t           out_total = 0;
  int32_t            out_count;
  uint8_t            range_start_key[FDB_KEY_TOTAL_LENGTH];
  uint8_t            range_end_key[FDB_KEY_TOTAL_LENGTH];

  fdb_build_event_key(range_start_key, event_id, 0);
  fdb_build_event_key(range_end_key, (event_id + 1), 0);

  // Loop until FoundationDB says there is no more data
  do {
    out_more = 0;
    future = fdb_transaction_get_range(tx,
                                       range_start_key, FDB_KEY_TOTAL_LENGTH, 1, out_total,
                                       range_end_key, FDB_KEY_TOTAL_LENGTH, 0, 1,
                                       0, 0, FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);

    if (fdb_check_error(fdb_future_block_until_ready(future))) fail_test();
    if (fdb_check_error(fdb_future_get_error(future))) fail_test();
    if (fdb_check_error(fdb_future_get_keyvalue_array(future, &out_kv, &out_count, &out_more))) fail_test();

    out_total += out_count;

    fdb_future_destroy(future);
  } while (out_more);

  return (uint32_t)out_total;
}

void fail_test(void) {

  fdb_shutdown_network_thread();
  fdb_shutdown_database();

  printf("test FAILED\n");

  exit(-1);
}
