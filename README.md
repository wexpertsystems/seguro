# Seguro Phase 2

Dependencies:

- Linux
- FoundationDB installation (`sudo apt-get install foundationdb -y` works fine)

## Run the benchmark

```
gcc benchmark.c -o seguro-benchmark -lfdb_c -llmdb
./seguro-benchmark
```

## Run the tests

```
gcc test.c -o seguro-tests -lfdb_c
./seguro-tests
```
