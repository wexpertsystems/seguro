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

int test_read_one_oversized_event();
int test_write_one_oversized_event();