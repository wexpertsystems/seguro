//! @file benchmark.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "event.h"
#include "db/fdb.h"

//==============================================================================
// Variables
//==============================================================================

extern char *optarg;
extern pthread_t fdb_network_thread;

//==============================================================================
// Prototypes
//==============================================================================

//! Runs the benchmark for writing one event per transaction to FoundationDB.
//!
//! @param[in] fdb  FoundationDB database object.
//! @param[in] e    Events array to write into the database.
//! @param[in] num_events    Number of events to write into the database. 
//!
//! @return 0   Success.
//! @return -1  Failure.
int run_single_write_benchmark(FDBDatabase* fdb, 
                               Event* events, 
                               int num_events);
 
//! Runs the benchmark for writing a batch of events per transaction to 
//! FoundationDB.
//!
//! @param[in] fdb         FoundationDB database object.
//! @param[in] events      Events array to write into the database.
//! @param[in] num_events  Number of events to write into the database. 
//! @param[in] batch_size  Number of events to write for each batch transaction.
//!
//! @return 0   Success.
//! @return -1  Failure.
int run_batch_write_benchmark(FDBDatabase* fdb, 
                              Event* events, 
                              int num_events, 
                              int batch_size);

//! Releases memory allocated for the given events array.
//!
//! @param[in] events  Event array.
//! @param[in] num_events       Array length.
//!
//! @return 0  Success.
int release_events_memory(Event *events, int num_events);

//==============================================================================
// Functions
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
  int intindex;
  static struct option options[] = {
    // name         has_arg            flag         val
    { "num-events", required_argument, no_argument, 'n' },
    { "event-size", required_argument, no_argument, 's' },
    { "batch-size", required_argument, no_argument, 'b' },
    { "mdb-file"  , required_argument, no_argument, 'f' },
    {NULL, 0, NULL, 0},
  };

  // Number of events to generate (or load from file).
  int num_events = 10000;
  // Event size (in bytes, 10KB by default).
  int event_size = 10000;
  // Batch size (in number of events, 10 by default).
  int batch_size = 10;
  // LMDB file to load events from.
  char *mdb_file = NULL;

  // Parse options.
  while ((opt = getopt_int(argc, argv, "n:s:b:f:", options, &intindex)) != EOF) {
    switch (opt) {
      case 'n':
        // Parse int integer from string.
        errno = 0;
        num_events = strtol(optarg, (char **) NULL, 10);
        const bool range_err = errno == ERANGE;
        if (range_err) {
          printf("ERROR: events must be a valid int integer.\n");
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

  // Validate options.
  if (num_events % batch_size != 0) {
    printf("ERROR: total events modulo batch size must equal 0.\n");
    exit(1);
  }

  // Print startup information.
  printf("Seguro Phase 2\n\n");
  printf("Running benchmarks...\n\n");

  // Event array.
  Event *events;

  // Load events from LMDB, or generate mock ones.
  if (mdb_file) { 
    printf("Loading %ld events from LMDB database file: %s", num_events, mdb_file); 
    events = load_lmdb_events(mdb_file, num_events, event_size);
  } 
  else {
    printf("Generating %ld mock events...\n", num_events);
    events = load_mock_events(num_events, event_size);
  }

  // Initialize a FoundationDB database object.
  FDBDatabase *fdb = fdb_init();

  // Run the single event per tx write benchmark.
  run_single_write_benchmark(fdb, events, num_events);

  // Run the batch event write benchmark.
  run_batch_write_benchmark(fdb, events, num_events, batch_size);

  // Release the events array memory.
  release_events_memory(events, num_events);

  // Success.
  printf("Benchmark completed.\n");
  return 0;
}

int release_events_memory(Event *events, int num_events) {
  // Release key/value array pairs.
  for (int i = 0; i < num_events; i++) {
    free((void *) events[i].key);
    free((void *) events[i].value);
  }

  // Release events.
  free((void *) events);

  // Success.
  return 0;
}

int run_single_write_benchmark(FDBDatabase* fdb, Event* events, int num_events) {
  printf("Writing one event per tx...\n");

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // Write the events, one per transaction.
  int err = write_event_batches(fdb, events, num_events, 1);
  if (0 != err) {
    printf("ERROR: Failed writing the events, one per transaction.\n");
    return -1;
  }

  // Stop the timer.
  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);
  double avg_ms = (cpu_time_used * 1000) / num_events;

  // Print.
  printf("Average single event per tx write time: %f\n\n", avg_ms);

  // Success.
  return 0;
}

int run_batch_write_benchmark(FDBDatabase* fdb, 
                              Event* events, 
                              int num_events, 
                              int batch_size) {
  printf("Writing batches of %d events per tx...\n", batch_size);

  // Start the timer.
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // Write the events in batches.
  int err = write_event_batches(fdb, events, num_events, batch_size);
  if (0 != err) {
    printf("ERROR: Failed writing event batches.\n");
    return -1;
  }

  // Stop the timer.
  end = clock();
  cpu_time_used = ((double) (end - start) / CLOCKS_PER_SEC);
  double avg_ms = (cpu_time_used * 1000) / num_events;

  // Print.
  printf("Average event write time with batches of %d: %f\n\n", batch_size, avg_ms);

  // Success.
  return 0;
}