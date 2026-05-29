# tellydb
An in-memory key-value database project.

## Features
* To make it work by all redis clients, follows [RESP2/RESP3](https://redis.io/docs/latest/develop/reference/protocol-spec)
* Fully configurable via [.tellyconf](./docs/FILE.md)
* **Persistency**
  * Holds all databases inside [a file](./docs/FILE.md#database-file--tellydb)
  * Provides saving database into file using [an external thread](./docs/SPECS.md#background-saving-thread) asynchrously
  * Atomicity on execution of saving process
* **Database**
  * Provides many data types such as hash table, list, and double
  * Includes key expiry
  * Multiple databases
* **Architecture**
  * Executes commands in [a single thread](./docs/SPECS.md#transaction-thread) sequentially
  * Uses [multiple threads](./docs/SPECS.md#io-threads) for handling I/O requests such as reading, and writing
  * Allows pipelining to combine multiple commands from a single client simultaneously
* **Permissions**
  * Relies independent passwords to authorize clients
  * Includes [precise permissions](./docs/AUTH.md#permissions) for commands such as database reading, and managing clients

The project is documented as follows:
| File                                   | Scope                          |
| :------------------------------------- | :----------------------------- |
| [docs/SPECS.md](./docs/SPECS.md)       | architecture of the project    |
| [docs/FILE.md](./docs/FILE.md)         | provided files by tellydb      |
| [docs/COMMANDS.md](./docs/COMMANDS.md) | commands and their information |
| [docs/AUTH.md](./docs/AUTH.md)         | authorization logic            |

## Installation

### Install via script
> [!IMPORTANT]
> It downloads dependencies and makes Docker operations, so it needs to be used via `sudo`.

```bash
curl -o- https://raw.githubusercontent.com/aloima/tellydb/master/install.sh | sudo bash
```


### Install from GitHub Releases:
* Download the [latest release file](https://github.com/aloima/tellydb/releases/latest)
* Give permission to make file executable using `chmod a+x ./telly`
* Start the server using `./telly`

### Compile on Local Machine:
+ Clone the repository
+ Install `OpenSSL` library and its development headers
+ Install `jemalloc` library and its development headers
+ Install `gmp` library and its development headers
+ Install `meson` and `ninja` to configure and build the project
+ Install `gperf` to generate perfect hash method
+ Configure the build directory using `meson setup build`
  - Compile the project using `meson compile -C build`, then start the server using `./build/telly`
  - Install to your local machine using `sudo meson install -C build`, then start the server using `telly`
  - Run the test suite using `meson test -C build --verbose`

### Install via Docker:
Pull docker image:
```sh
docker pull aloima/tellydb        # for master branch
docker pull aloima/tellydb:1.0.0  # for specific release
```

Run docker container to start the server:
```sh
docker run -d -p 6379:6379 --name telly aloima/tellydb
```

To get information about cli commands, use `./telly help`.

## License
Licensed under [BSD-3 Clause Clear License](./LICENSE).
