//! @file benchmark.c
//!
//! Seguro benchmarking tool.

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>

#include "seguro.h"

//==============================================================================
// External variables.
//==============================================================================

extern char *optarg;

//==============================================================================
// Functions.
//==============================================================================

//! Accepts arguments from the command-line and executes the Seguro benchmark.
//!
//! @param[in] argc  Number of command-line options provided.
//! @param[in] argv  Array of command-line options provided.
//!
//! @return  0  Success.
//! @return -1  Failure.
int main(int argc, char** argv) {
  // Options.
  int opt;
  int longindex;
  static struct option options[] = {
    // name       has_arg            flag         val
    { "events",   required_argument, no_argument, 'e' },
    { "mdb-file", required_argument, no_argument, 'f' },
    {NULL, 0, NULL, 0},
  };

  // Number of events to generate (or load from file).
  long events = (long) 10000;
  // LMDB file to load events from.
  char* mdb_file = NULL;

  // Parse options.
  while ((opt = getopt_long(argc, argv, "e:f:", options, &longindex)) != EOF) {
    switch (opt) {
      case 'e':
        // Parse long integer from string.
        errno = 0;
        events = strtol(optarg, (char **) NULL, 10);
        const bool range_err = errno == ERANGE;
        if (range_err) {
          printf("ERROR: events must be a valid long integer.\n");
        }
        break;
      case 'f':
        mdb_file = optarg;
        break;
    }
  }

  // Print startup information.
  printf("Seguro Phase 2\n\n");
  printf("Running benchmarks...\n");

  // Start the benchmark with events loaded from LMDB. 
  if (mdb_file) { 
    printf("Loading %ld events from LMDB database file: %s", events, mdb_file); 
  } 
  // Start the benchmark with mock events.
  else {
    printf("Generating %ld mock events...\n", events);
    int n = 100000;
    event_t events[n];
    _load_mock_events(events, n, 10000);
  }

  return 0;
}