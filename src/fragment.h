//! @file events.h
//!
//! Fragment struct definition and utilities.

#pragma once

#include <stdint.h>


//==============================================================================
// Types
//==============================================================================

typedef struct fragment_t {
  uint16_t  data_length;
  uint8_t  *data;
} Fragment;
