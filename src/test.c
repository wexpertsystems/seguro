//! @file test.c
//!
//! Seguro tests.

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "seguro.h"

//==============================================================================
// Functions
//==============================================================================

int test_read_one_event();
int test_write_one_event();

int test_read_one_fragmented_event();
int test_write_one_fragmented_event();

int test_read_event_batch(int min_key, int max_key);
int test_write_event_batch(int min_key, int max_key);

int test_read_fragmented_event_batch(int min_key, int max_key);
int test_write_fragmented_event_batch(event_t events[]);