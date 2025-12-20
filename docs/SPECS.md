# Specifications
This file contains the specifications of tellydb.

## Process
tellydb contains a main thread (process), a transaction thread, I/O threads and an optional background saving thread.

### Main Thread
1. The thread initializing everything.
2. After initializing, the thread runs an infinite event loop, continuously polling for I/O requests. (`epoll()`/`kqueue()`)
3. Operations of this loop are accepting clients and enqueuing I/O jobs. (`accept()`)
4. Once the close signal, deinitializes everything and ends the loop.

### I/O Threads
tellydb spawns `max(processor count - 1, 1)` I/O threads. The architecture operates as follows:
1. Each I/O thread runs an infinite loop, continuously polling for incoming tasks.
2. Any thread can enqueue an I/O job when need into a thread-safe queue.
3. An available I/O thread dequeues a task from the queue.
4. The I/O thread executes the task.
5. The result of the task is not captured by the calling thread externally.

I/O threads responsibilities as follows:
* Reading raw data from clients and deserializing them into commands (`read()`)
* Writing transaction responses to calling client (`write()`)
* Terminating clients (`close()`)

### Transaction Thread
tellydb spawn a transaction thread. The architecture operates as follows:
1. The thread runs an infinite loop, continuously polling for incoming transactions.
2. Any thread can enqueue an transaction, all transactions are enqueued by an successfull read-and-parse I/O job.
3. The thread dequeues and executes transactions sequentially.
4. The response of the transaction is written to buffer of the client.
5. A write I/O job is enqueued into I/O job queue.

### Background-Saving Thread
Background saving thread can be spawned via [`BGSAVE` command](./COMMANDS.md#BGSAVE).
1. All data currently in the memory is started for saving to the persistent database file.
2. Once saving process is complete, the thread terminates automatically.
3. If a background-saving thread is already in progress, [`BGSAVE` command](./COMMANDS.md#BGSAVE) throws an error.

## Authorization
To get information, look at [AUTH.md](./AUTH.md).

## Databases
tellydb handles database as follows:
* Database will be taken from persistent database file, then be saved to cache.
* When closing the server, database will be taken from cache and written to persistent database file.

* There may be databases more than one, look at [SELECT command](./COMMANDS.md#SELECT).
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
