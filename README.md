# tellydb
An in-memory key-value database project for educational purposes.

## Features
+ Follows [RESP2/RESP3](https://redis.io/docs/latest/develop/reference/protocol-spec/) specification from redis, so all redis clients are compatible
+ Includes B-Tree for caching
+ Fully configurable via [.tellyconf](./docs/FILE.md)
+ Includes command queue system using a thread
+ Supports integer, string, null, boolean, list and hash table types
+ Provides atomicity when saving to the database file
+ Provides saving to the database file using a background thread
+ Provides authorization system with permissions using passwords
+ Uses Direct I/O for logging and database files
+ Uses pipelining for combine multiple commands sent by same clients
+ Supports multiple databases
+ Data persists on one-file

> Look at:  
> [docs/SPECS.md](./docs/SPECS.md) for more technical information,  
> [docs/FILE.md](./docs/FILE.md) for information about provided files by tellydb,  
> [docs/COMMANDS.md](./docs/COMMANDS.md) for information about commands,  
> [docs/AUTH.md](./docs/AUTH.md) for information about authorization.

## Quick Start
* Install a client that allows to connect tellydb or redis server
* Install `OpenSSL` library and its development headers
* Install `pkg-config` for compilation flags of libraries
* Compile using `make`
* Look at default configuration file using `telly default-config`
* If you want to change configuration values, create a configuration file using `telly create-config`
* Start the server using `telly`

To get information about cli commands, use `telly help`.

## Benchmark Results
Look at [utils/README.md](./utils/README.md).

## License
Licensed under [BSD-3 Clause Clear License](./LICENSE).
