//! @file events.h
//!
//! Events.

#include <lmdb.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifndef FDB_API_VERSION
#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>
#endif


//==============================================================================
// Prototypes
//==============================================================================

//! Writes mock events into the given array.
//!
//! @param[in] e     The array to write events into.
//! @param[in] n     The number of events to write into an array in memory.
//! @param[in] size  The size of each event, in bytes.
//!
//! @return       Events array.
//! @return NULL  Failure.
FDBKeyValue* load_mock_events(long n, int size);

//! Reads events from LMDB and writes them into an array in memory.
//!
//! @param[in] mdb_file  The LMDB database file to read events from.
//! @param[in] e     The array to write events into.
//! @param[in] n         The number of events to write into the event array.
//! @param[in] size      The size of each event, in bytes.
//!
//! @return       Events array.
//! @return NULL  Failure.
FDBKeyValue* load_lmdb_events(char* mdb_file, long n, int size);

//! Counts the number of digits in the given integer.
//!
//! @param[in] n  The integer.
//!
//! @return  The number of digits.
int count_digits(int n);