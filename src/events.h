//! @file events.h
//!
//! Events.

#pragma once

#include <foundationdb/fdb_c.h>


//==============================================================================
// Prototypes
//==============================================================================

//! Writes mock events into the given array.
//!
//! @param[in] num_events  The number of events to write into an array in memory.
//! @param[in] size        The size of each event, in bytes.
//!
//! @return       Events array.
//! @return NULL  Failure.
FDBKeyValue* load_mock_events(long num_events, int size);

//! Counts the number of digits in the given integer.
//!
//! @param[in] n  The integer.
//!
//! @return  The number of digits.
int count_digits(int n);
