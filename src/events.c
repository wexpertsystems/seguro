//! @file events.c
//!
//! Generates or reads events from LMDB, and loads them into memory.

#include <lmdb.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "events.h"


//==============================================================================
// Functions
//==============================================================================

//! @n (1) Allocates memory for a new array.
//! @n (2) Keys and values are uint8_t[].
FDBKeyValue* load_mock_events(long n, int size) {
  // Seed the random number generator.
  srand(time(0));

  // Allocate memory for events.
  FDBKeyValue *events = (FDBKeyValue *) malloc(sizeof(FDBKeyValue) * (n));

  // Write random key/values into the given array.
  for (int i = 0; i < n; i++) {
    // Generate a key.
    int key_length = count_digits(i);
    char *key = (char *) malloc(sizeof(char) * key_length);
    sprintf(key, "%d", i);

    // Generate a value.
    char *value = (char *) malloc(sizeof(char) * size);
    for (int j = 0; j < size; j++) {
      // 0-255 (all valid bytes).
      int b = rand() % 256;
      value[j] = b;
      int c = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[random () % 52];
      value[j] = c;
    }

    // Write the key and value into a slot in the array.
    events[i].key = key;
    events[i].key_length = key_length;
    events[i].value = value;
    events[i].value_length = size;
  }

  // Print results.
  printf("Loaded %ld events into memory.\n", n);

  // Success.
  return events;
}

//! @n (1) ...
FDBKeyValue* load_lmdb_events(char* mdb_file, long n, int size) {
  return NULL;
}

//! @n (1) Base 10.
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