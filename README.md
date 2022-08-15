# Seguro Phase 2

This is the work-in-progress repo for Phase 2 of the Seguro implementation for the [Urbit](https://urbit.org) project.

More information:
- [Urbit Foundation grant for Seguro](https://urbit.org/grants/seguro-prototype)
- [Seguro Phase 2 proposal](https://github.com/wexpertsystems/seguro/blob/34180d5108b4d03f8389242b5c0bd6181f9e3a62/PROPOSAL.md)

# Installation

## Dependencies:

- [FoundationDB](https://apple.github.io/foundationdb/downloads.html)
- [make](https://www.gnu.org/software/make/)

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
