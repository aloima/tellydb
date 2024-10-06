# Specs
This file contains the specs of tellydb.

## Process
tellydb contains a process and a thread:
* Thread manages sent commands by clients and executes them.
* Process accepts clients, receives data sent by client, interprets received data as commands and manages the server.

## Limits
* BTree implementation of tellydb allows size of up to `uint32`. It means that a BTree can store up to `2^32-1` or `4,294,967,295` key-value pairs.
> The amount of bytes represented by `uint64` is ~18 PB, this is too high for memory/caching for today's servers. So, selected `uint32` for limit.
