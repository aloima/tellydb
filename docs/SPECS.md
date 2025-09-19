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

## Databases
* Each database stores the data in a HashMap.
* The hashmap uses **djb2 hash algorithm** and all hashed values of key-value pairs will be stored in, so there is no re-hashing.
* If a hash collision occurs, **linear probing scheme** will be executed.
* The max size of this hashmap is `uint64_t`. It means that a database can store up to `2^64-1` or `18,446,744,073,709,551,615` key-value pairs.

## Limits
* The max size of a list is `uint32`. It means that a list can store up to `2^32-1` or `4,294,967,295` list nodes.
* The max size of a hash table is `uint32`. It means that a hash table can store up to `2^32-1` or `4,294,967,295` field-value pairs.
> This limit is valid for all data. It means that summation of fv->next's count and allocated fv's count cannot exceed `2^32-1`.

* The max length of a key is `1 GB - 1 byte` or `1,073,741,823`.
* The max length of a string-typed value is `1 GB - 1 byte` or `1,073,741,823`.
* The bounds of an integer value are `[-2^1024-1, 2^1024-1]` and represented by 128 byte maximum.
* There is no bound for a double value, it is represented by 128 byte maximum and its precision limit is 128 bits.

> [!TIP]
> The amount of bytes represented by `uint64` is ~18 PB, this is too high for memory/caching for today's servers. So, selected `uint32` for the most limits.
