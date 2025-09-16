# Specifications
This file contains the specifications of tellydb.

## Process
tellydb contains a process and a thread:
* Thread manages sent commands by clients and executes them.
* Process accepts clients, receives data sent by client, interprets received data as commands and manages the server.

tellydb handles received data as follows:
* Data will be saved to cache.
* When closing the server, data will be taken from cache and written to database file.

## Authorization
To get information, look at [AUTH.md](./AUTH.md).

## B-Tree
* BTree implementation of tellydb allows size of up to `uint32`. It means that a BTree can store up to `2^32-1` or `4,294,967,295` values.
* BTree indexes are numbers, not keys. So, keys are converted to numbers by **djb2 hash algorithm**.
* If a hash collision occurs, **linear probing scheme** will be executed.

## Limits
* The max size of a list is `uint32`. It means that a list can store up to `2^32-1` or `4,294,967,295` list nodes.
* The max size of a hash table is `uint32`. It means that a hash table can store up to `2^32-1` or `4,294,967,295` field-value pairs.
> This limit is valid for all data. It means that summation of fv->next's count and allocated fv's count cannot exceed `2^32-1`.

* The max length of a key is `1 GB - 1 byte` or `1,073,741,823`.
* The max length of a string-typed value is `1 GB - 1 byte` or `1,073,741,823`.
* The bounds of an integer value are `[-2^1024-1, 2^1024-1]` and represented by 128 byte maximum.
* There is no bound for a double value, it is represented by 128 byte maximum and its precision limit is 128 bits.

> [!TIP]
> The amount of bytes represented by `uint64` is ~18 PB, this is too high for memory/caching for today's servers. So, selected `uint32` for most limits.
