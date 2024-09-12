# tellydb
A toy key-value database project for educational purposes.

## Features
+ Partially implemented [RESP2](https://redis.io/docs/latest/develop/reference/protocol-spec/) from redis.
+ Includes B-Tree for caching.
+ Fully configurable via [.tellyconf](./FILE.md).
+ Includes transaction/command queue system.

## Usage
Start the server, install a tool that you can connect to redis server like redis-cli. Start to use!

## Quick Start
To get information about server, use `telly help`.
To get information about available commands, look at [src/commands](./src/commands/)

## License
Licensed under [BSD-3 Clause Clear License](./LICENSE).
