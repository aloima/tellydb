# tellydb
An in-memory key-value database project.

## Features
+ Follows [RESP2/RESP3](https://redis.io/docs/latest/develop/reference/protocol-spec/) from redis, so all redis clients are compatible
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

## Installation
### Install from GitHub Releases:
* Download the [latest release file](https://github.com/aloima/tellydb/releases/latest/download/telly)
* Start the server using `./telly`

### Compile on Local Machine:
+ Clone the repository
+ Install `OpenSSL` library and its development headers
+ Install `jemalloc` library and its development headers
+ Install `gmp` library and its development headers
+ Install `cmake` to generate compile files
+ Create build directory using `mkdir build` and enter `cd build`
+ Generate compilation files using `cmake ..`
  - Compile inside the directory using `make telly`, then start the server using `./telly`
  - Install your local machine using `sudo make install`, then start the server using `telly`

### Install via Docker (WIP):
Pull docker image:
```sh
docker pull aloima/tellydb
```

Run docker container to start the server:
```sh
docker run -d -p 6379:6379 --name telly aloima/tellydb
```

To get information about cli commands, use `./telly help`.

## Benchmark Results
Look at [utils/README.md](./utils/README.md).

## License
Licensed under [BSD-3 Clause Clear License](./LICENSE).
