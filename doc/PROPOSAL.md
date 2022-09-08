# Seguro

A production wrapper around FoundationDB, ready for integration with `vere`.

## Background

The results of Phase 1 of the Seguro project demonstrate that FoundationDB is
the best choice of distributed databases to utilize for the project. This is
Phase 2 of the Seguro project, which implements an MVP event reader/writer
module using FoundationDB as the underlying data store.

See below for additional context:

- [Seguro Prototype (Phase 1) Grant](https://urbit.org/grants/seguro-prototype)
- [Seguro Prototype (Phase 1) Repository](https://github.com/wexpertsystems/seguro)
- [Seguro & Armada Paper](https://gist.github.com/wexpert/0485a722185d5ee70742570036faf32f)

## Objectives

Phase 2 of the Seguro project can be considered as "all work up to actual
integration with `vere`". Limiting Phase 2 to this is desirable because it i.)
keeps the grant's scope of work at a reasonable level, and ii.) it allows more
time for the following three ancillary runtime projects/considerations to
achieve resolution:

1. Asynchronous reads in `vere`
2. Merging/releasing event log truncation
   [urbit/urbit#7501](https://github.com/urbit/urbit/pull/5701)
3. A decision on whether or not to add another process to the overall Urbit
   runtime architecture

Each of these three workstreams need to be resolved before Seguro can be
integrated with `vere`.

### Event Fragmentation Scheme

FoundationDB supports a maximum value size of 100KB and recommends limiting
value size to only 10KB for best performance. Since our data model requires
arbitrarily sized byte buffer values for events, an event fragmentation
algorithm is required to support events with larger payloads.

Events which exceed the size limit of a single database value must be fragmented
so that such events are stored across multiple database key/value pairs. To
achieve event fragmentation, key/value pairs will be formatted like so:

#### Keys/Values

|           | encoding                                                                           | format                         |
|-----------|------------------------------------------------------------------------------------|--------------------------------|
| **key**   | `0x00` &#x7c; 64-bit event id (big-endian) &#x7c; 32-bit fragment id (big-endian)  | `0x00112233445566778899AABBCC` |
| **value** | raw bytes starting at index `(i * <optimal fragment size>)` for fragment w/ id `i` | `0xdeadbeef...`                |

Notes:
- `|` = concatenation
- The event ID and fragment ID are both contiguous, non-negative integers
- The optimal fragment size, as documented above, will almost certainly be
  `10 KB` = `10,000 bytes`
- The first fragment (fragment `0`) will have a slightly different encoding for
  the value, as documented below

#### Fragment 0

The value of the first fragment of an event will be split into two parts that
are concatenated: the "header" and the "payload".

#### Fragment 0 Header

The fragment header is used to record the total number of fragments for the
event. This serves two functions: it allows the read algorithm to pre-allocate
an appropriate amount of memory on the heap for an event after reading the
first batch out of FDB, and `memcpy` data from FDB as it arrives in batches; it
allows the read algorithm to verify that the number of fragments returned by FDB
is the number of fragments expected.

The fragment header is a raw byte array with the following format:

**128 fragments or less**

| byte | meaning               | format          | example     |
|------|-----------------------|-----------------|-------------|
| 0    | *remaining* fragments | `<# fragments>` | `0111 1111` |

**129 fragments or more**

| byte | meaning                      | format                                             | example      |
|------|------------------------------|----------------------------------------------------|--------------|
| 0    | additional bytes             | `0x80` &#x7c;&#x7c; `<bytes to store # fragments>` | `1000 0010 ` |
| 1..N | bytes encoding `# fragments` | `<# fragments>`                                    | `0001 0110 ` |

Note: `||` = bitwise OR

Let's see how the 257th fragment of an example event which requires 257
fragments to be stored would look:

| byte | bucket        | format                                 | example     |
|------|---------------|----------------------------------------|-------------|
| 0    | byte-width    | overflow toggle bit, bucket byte-width | `1000 0010` |
| 1    | `# fragments` | remaining fragments (little endian)    | `0000 0000` |
| 2    | `# fragments` |                                        | `0000 0001` |

Remember: the header encodes the number of **remaining** fragments. The total
number of fragments is the number stored in the header plus 1, because there is
always guaranteed to be a first fragment, so we can exclude it from counting.

#### Fragment 0 Payload

The `fragment payload` is the raw bytes of the event, as encoded by `vere`.
The likelihood of an event being equally divisible by the optimal FDB value size
is very low. Naturally, this means that most fragmented events will have one
fragment with an odd size. To keep value sizes as close to consistent as
possible, we make the first fragment be the oddly sized one.

For example, if an event has size `10,001 bytes`, then the event will be split
into `2` fragments. The first fragment will have size `2 bytes` (`1 byte` header +
`1 byte` payload) and the second fragment will have size `10,000 bytes`
(assuming the default FDB value size of `10 KB`).

### Writes

Single transaction batch writes and multiple, simultaneous, asynchronous writes
must be supported. Additional requirements for writes include:

- A benchmark suite for determining which batch/value sizes work best
- A brief analysis of pros/cons of integrating the two, based on whether the
  overall architecture is in- or out-of-process

### Reads

Single transaction and streaming batch reads must be supported.

### Tests

Tests ensuring multi-fragment events are written and read correctly and without
corruption will be included in this work.

## Future Work

1. Integration with `vere`
2. Release
3. Automatic failover

## Milestones

Phase 2 - Implement a working FoundationDB event read/write layer that is as
ready as possible for interfacing with the Urbit runtime.

Deliverables:

1. Implementation using the
   [FoundationDB C API](https://apple.github.io/foundationdb/api-c.html)
2. Event fragmentation scheme, including correctness tests
3. Single and batch reads
4. Writes

   1. Single transaction batch writes
   2. Multiple, simultaneous transaction writes (race a bunch of futures)
   3. Benchmark determining best performance write strategy
   4. Brief analysis of implementation pros/cons based on in- or out-of-process
      architecture

5. Linux compatibility (macOS, Windows can be supported later)

Payment: 5 stars

Expected Completion: October 2022
