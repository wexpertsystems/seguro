# Seguro

This is the work-in-progress repo for Phase 2 of the Seguro implementation for 
the [Urbit](https://urbit.org) project.

More information:
- [Seguro Prototype Proposal](https://urbit.org/grants/seguro-prototype)
- [Seguro MVP Proposal](https://urbit.org/grants/seguro-phase-2)

# Installation

## Dependencies:

- [FoundationDB](https://github.com/apple/foundationdb/releases)
- [make](https://www.gnu.org/software/make/)

## Configuration

After installing the clients and server packages of FoundationDB, the default 
server should be reconfigured for local testing. This can be done using the 
`fdbcli` utility on Linux and MacOS:
```shell
fdbcli
configure single ssd
```
This will set the default local FoundationDB cluster to store a single copy of 
data on disk.

# Usage

## Run tests

The following command will run all Seguro tests:
```shell
make test
```

Alternatively, the unit or integration tests can be run separately from each other:
```shell
make test-unit
make test-integ
```

## Run benchmarks

The following command will run all Seguro benchmarks:
```shell
make benchmark
```

# Troubleshooting

The state of the local FoundationDB cluster can be monitored using the `fdbcli` utility. It's self-documented, but
additional information can be found in the [official FoundationDB documentation](https://apple.github.io/foundationdb).

However, be warned that the `fdbcli` can be finicky, and can lie. Some examples:
- Complaining that there is no memory available with 12GB memory available
- Complaining that there is no space on the drive available when there is 50GB space available
- Errors during benchmark tests while the `status` command shows nothing wrong
- No errors during benchmark tests while the `status` command shows memory and storage warnings

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
