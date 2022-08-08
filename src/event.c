//! @file event.c
//!
//! Generates mock events, or reads events from LMDB, and loads them into memory.

#include <lmdb.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "event.h"


//==============================================================================
// Functions
//==============================================================================

//! @n (1) Allocates memory for a new events array.
//! @n (2) Generates random key/values and writes them into the array.
Event* load_mock_events(int num_events, int size) {
  Event *events = (Event *) malloc(sizeof(Event) * num_events); // (1)

  srand(time(0));
  for (int i = 0; i < num_events; i++) {
    char *value = (char *) malloc(sizeof(char) * size);
    for (int j = 0; j < size; j++) {
      // 0-255 (all valid bytes).
      int b = rand() % 256;
      value[j] = b;
    }
    events[i].key = i;
    events[i].value = value;
    events[i].value_length = size;
  } // (3)

  printf("Loaded %ld events into memory.\n\n", num_events);
  return events;
}

//! @n (1) ...
Event* load_lmdb_events(char* mdb_file, int num_events, int size) {
  return NULL;
}

int count_digits(int n) {
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