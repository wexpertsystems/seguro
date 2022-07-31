//! @file benchmark.c
//!
//! Seguro benchmarking tool.

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "seguro.h"
#include "test.h"

//==============================================================================
// External variables.
//==============================================================================

//! CLI options.
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
    // name         has_arg            flag         val
    { "num-events", required_argument, no_argument, 'n' },
    { "event-size", required_argument, no_argument, 's' },
    { "batch-size", required_argument, no_argument, 'b' },
    { "mdb-file"  , required_argument, no_argument, 'f' },
    {NULL, 0, NULL, 0},
  };

  // Number of events to generate (or load from file).
  long num_events = 10000;
  // Event size (in bytes, 10KB by default).
  int event_size = 10000;
  // Batch size (in number of events, 10 by default).
  int batch_size = 10;
  // LMDB file to load events from.
  char *mdb_file = NULL;

  // Parse options.
  while ((opt = getopt_long(argc, argv, "n:s:b:f:", options, &longindex)) != EOF) {
    switch (opt) {
      case 'n':
        // Parse long integer from string.
        errno = 0;
        num_events = strtol(optarg, (char **) NULL, 10);
        const bool range_err = errno == ERANGE;
        if (range_err) {
          printf("ERROR: events must be a valid long integer.\n");
        }
        break;
      case 's':
        event_size = atoi(optarg);
        break;
      case 'b':
        batch_size = atoi(optarg);
        break;
      case 'f':
        mdb_file = optarg;
        break;
    }
  }

  // Print startup information.
  printf("Seguro Phase 2\n\n");
  printf("Running benchmarks...\n\n");

  // Start the benchmark with events loaded from LMDB. 
  int err;
  if (mdb_file) { 
    printf("Loading %ld events from LMDB database file: %s", num_events, mdb_file); 
    err = run_benchmark_lmdb(mdb_file, batch_size);
  } 
  // Start the benchmark with mock events.
  else {
    printf("Generating %ld mock events...\n", num_events);
    err = run_benchmark_mock(num_events, event_size, batch_size);
  }

  if (err != 0) {
    printf("Benchmark failed.\n");
    return -1;
  }

  // Success.
  printf("Benchmark completed.\n");
  return 0;
}