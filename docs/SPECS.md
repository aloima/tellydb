# Specs
This file contains the specs of tellydb.

## Process
tellydb contains a process and a thread:
* Thread manages sent commands by clients and executes them.
* Process accepts clients, receives data sent by client, interprets received data as commands and manages the server.

## Limits
* BTree implementation of tellydb allows size of up to `uint32`. It means that a BTree can store up to `2^32-1` or `4,294,967,295` key-value pairs.
* The max size of a list is `uint64`. It means that a list can store up to `2^32-1` or `4,294,967,295` list nodes.
* The max length of a key is `1 GB` or `1,073,741,824`.
* The max length of a string-typed value is `1 GB` or `1,073,741,824`.
* The limit of a number value is `[-9,223,372,036,854,775,808, 9,223,372,036,854,775,807]` or `[-2^63, 2^63-1]` and represented by 8 bytes.

> [!TIP]
> The amount of bytes represented by `uint64` is ~18 PB, this is too high for memory/caching for today's servers. So, selected `uint32` for most limits.
