//! @file seguro.c
//!
//! Reads and writes events into and out of a FoundationDB cluster.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define FDB_API_VERSION 610
#include <fdb_c.h>

#define CLUSTER_NAME    "fdb.cluster"
#define DB_NAME         "DB"
#define MAX_VALUE_SIZE  10000
#define MAX_RETRIES     5

//==============================================================================
// Functions
//==============================================================================