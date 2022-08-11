//! @file test.c
//!
//! FoundationDB tests.

//==============================================================================
// Functions
//==============================================================================

// TODO
int test_read_one_event();
int test_write_one_event();

int test_read_one_fragmented_event();
int test_write_one_fragmented_event();

int test_read_event_batch(int min_key, int max_key);
int test_write_event_batch(int min_key, int max_key);

int test_read_fragmented_event_batch(int min_key, int max_key);
int test_write_fragmented_event_batch(FDBKeyValue events[]);
