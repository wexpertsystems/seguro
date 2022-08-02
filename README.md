# Seguro Phase 2

Dependencies:

- Linux
- FoundationDB installation (`sudo apt-get install foundationdb -y` works fine)

## Run the benchmark

```
gcc seguro.c -o seguro -lfdb_c -llmdb
./seguro
```

## Run the tests

```
gcc tests.c -o seguro-tests -lfdb_c
./seguro-tests
```
