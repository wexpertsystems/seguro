//! @file events.c
//!
//! Generates or reads events from LMDB, and loads them into memory.

#include <stdint.h>

#include "events.h"


//==============================================================================
// Functions
//==============================================================================

int count_digits(uint32_t n) {

  if (n == 0) {
    return 1;
  }

  int count = 0;
  while (n != 0) {
    n = n / 10;
    ++count;
  }

  return count;
}
