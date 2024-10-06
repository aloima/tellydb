# tellydb
A key-value database project for educational purposes.

## Features
+ Follows [RESP2](https://redis.io/docs/latest/develop/reference/protocol-spec/) specification from redis, so all redis clients is compatible
+ Includes B-Tree for caching
+ Fully configurable via [.tellyconf](./docs/FILE.md)
+ Includes command queue system
+ Supports integer, string, null, boolean, list and hash table types
+ Provides atomicity when saving to the database file and thread handling

> Look at [docs/SPECS.md](./docs/SPECS.md) for more technical information

## Usage
Start the server, install a client that you can connect to redis/tellydb server like redis-cli. Start to use!

## Quick Start
Compile using `make`, start the server, install a client that allows to connect tellydb server, connect and use the commands.
To list the commands, look at [src/commands](./src/commands/), to get information about cli commands, use `telly help`.

## License
Licensed under [BSD-3 Clause Clear License](./LICENSE).
