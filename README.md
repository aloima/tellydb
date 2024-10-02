# tellydb
A key-value database project for educational purposes.

## Features
+ Follows [RESP2](https://redis.io/docs/latest/develop/reference/protocol-spec/) specification from redis
+ Includes B-Tree for caching
+ Fully configurable via [.tellyconf](./FILE.md)
+ Includes command queue system
+ Supports integer, string, null, boolean, list and hash table types
+ Provides atomicity when saving to the database file

## Usage
Start the server, install a client that you can connect to redis/tellydb server like redis-cli. Start to use!

## Quick Start
To get information about server, use `telly help`.
To get information about available commands, look at [src/commands](./src/commands/)

## License
Licensed under [BSD-3 Clause Clear License](./LICENSE).
