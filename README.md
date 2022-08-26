# Seguro Phase 2

This is the work-in-progress repo for Phase 2 of the Seguro implementation for the [Urbit](https://urbit.org) project.

More information:
- [Urbit Foundation grant for Seguro](https://urbit.org/grants/seguro-prototype)
- [Seguro Phase 2 proposal](https://github.com/wexpertsystems/seguro/blob/34180d5108b4d03f8389242b5c0bd6181f9e3a62/PROPOSAL.md)

# Installation

## Dependencies:

- [FoundationDB](https://github.com/apple/foundationdb/releases)
- [make](https://www.gnu.org/software/make/)

## Configuration

After installing the clients and server packages of FoundationDB, the default server should be reconfigured for local
testing. This can be done using the `fdbcli` utility on Linux and MacOS:
```shell
fdbcli
configure single ssd
```
This will set the default local FoundationDB cluster to store a single copy of data on disk.

# Usage

## Run the benchmark suite

The following command will run the default benchmarking suite:
```
make benchmark
```

The benchmarking suite can also be used to run custom benchmarks by passing in custom parameters. Unfortunately, there's
no good way to pass command arguments through `make`. Therefore, custom benchmarks need to be called like this:
```
# Custom Seguro benchmark with 1000 events of size 10000 bytes (1KB) written in batches of 100

make
./bin/seguro-benchmark -c -n 1000 -e 10000 -b 100

# OR

make
./bin/seguro-benchmark --custom --num-events 1000 --event-size 10000 --batch-size 100
```

NOTE: The `-c` option is required to run a custom benchmark; the other options do nothing unless `-c` or `--custom` is
present.

# Troubleshooting

The state of the local FoundationDB cluster can be monitored using the `fdbcli` utility. It's self-documented, but
additional information can be found in the [official FoundationDB documentation](https://apple.github.io/foundationdb).

## Soft Reset

If the test or benchmark suites suffer a fatal error, the local FoundationDB may be stranded in an unclean state. It may
be necessary to manually drop keys from the cluster to ensure tests/benchmarks accurately measure
correctness/performance:
```shell
fdbcli
writemode on
clearrange "" \xFF
```
The above series of commands will drop all keys from the FoundationDB cluster.

## Hard Reset

If the FoundationDB cluster becomes so broken during development that a hard reset is required, the following commands
will return the cluster to its initial state after a fresh installation:
```shell
sudo systemctl stop foundationdb
sudo rm -rf /var/lib/foundationdb/data
sudo systemctl start foundationdb
fdbcli
configure new single ssd
```
