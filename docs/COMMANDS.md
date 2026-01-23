# Commands
This document provides a detailed description of all the available commands. Each command's syntax, arguments, and examples are included for clarity.

---

## Table of Contents
1. [Database Commands](#database-commands)
2. [Generic Commands](#generic-commands)
3. [Hashtable Commands](#hashtable-commands)
4. [Key-Value Commands](#key-value-commands)
5. [List Commands](#list-commands)

---

#### General Behavior of Commands
The list of unwritten behavior to commands.
+ When a command requires some permissions and client does not have it, it throws an error message includes needed permission(s) name.

---

## Database Commands

### BGSAVE
**Syntax**: `BGSAVE`  
**Description**: Saves all data to database file in background using a thread.  
**Since**: `0.1.6`  
**Time complexity**: `O(N) where N is cached key-value pair count`  
**Permissions**: `P_SERVER`  
**Returns**: `OK`  
**Behavior**:
* Creates a thread to save all data to the database.
* The created thread will be closed after saving.
* If a saving process from [BGSAVE](#bgsave) or [SAVE](#save) is active, throws an error.

---

### DBSIZE
**Syntax**: `DBSIZE [database]`  
**Description**: Returns key count in the database.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ`  
**Returns**: Integer
**Behavior**:
* If database cannot be found, throws an error.

**Arguments**:
- **database**: The name of the database to be retrieve key count. If not specified, it will be current database of the client

---

### LASTSAVE
**Syntax**: `LASTSAVE`  
**Description**: Returns last save time of database as UNIX time.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Permissions**: `P_SERVER`  
**Returns**: Integer  

---

### SAVE
**Syntax**: `SAVE`  
**Description**: Saves all data to database file.  
**Since**: `0.1.6`  
**Time complexity**: `O(N) where N is cached key-value pair count`  
**Permissions**: `P_SERVER`  
**Returns**: `OK`  
**Behavior**:
* Waits until saving all data to database file, so **it blocks all client commands.**

### SELECT
**Syntax**: `SELECT database`  
**Description**: Selects database which will be used by client.  
**Since**: `0.1.9`  
**Time complexity**: `O(N) where N is total database count`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
* If database cannot be found, throws an error.
* If database cannot be created, throws an error.

**Arguments**:
- **database**: The name of the database to be used by client.

---

## Generic Commands

### AGE
**Syntax**: `AGE`  
**Description**: Sends the server age as seconds.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: Integer

---

### AUTH
**Syntax**: `AUTH password ["ok"]`  
**Description**: Sends the server age as seconds.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`  
**Behaviour**:
* If the client is already using a password, it throws an error without `ok` argument.

---

### CLIENT
**Syntax**: `CLIENT ID|INFO|LOCK|SETINFO|KILL`  
**Description**: Main command of client(s).  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  

**Subcommands**:

#### ID
**Syntax**: `CLIENT ID`  
**Description**: Returns ID number of client.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: Integer

#### INFO
**Syntax**: `CLIENT INFO [<id>]`  
**Description**: Returns information about the client.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: String  
**Behavior**:
* If there is no client which has the specified ID, throws an error.
* If there is no `P_CLIENT` permission with the specified ID, throws the permission error.
* If the specified ID is not integer or out of bounds for `uint32_t`, throws an error.

#### LIST
**Syntax**: `CLIENT LIST`  
**Description**: Lists IDs of the connected clients.  
**Since**: `0.2.0`  
**Time complexity**: `O(1)`  
**Permissions**: `P_CLIENT`  
**Returns**: Array

#### LOCK
**Syntax**: `CLIENT LOCK id`  
**Description**: Locks specified client.  
**Since**: `0.1.8`  
**Time complexity**: `O(1)`  
**Permissions**: `P_CLIENT`  
**Returns**: `OK`  
**Behavior**:
* If specified client is not exist, throws an error.
* If specified client ID value is higher or less than uint32_t bounds, throws an error.
* If specified client ID is already locked, throws an error.

**Arguments**:
- **id**: Client ID, must be unsigned integer value

#### SETINFO
**Syntax**: `CLIENT SETINFO property value`  
**Description**: Returns information about the client.  
**Since**: `0.1.2`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
* If uppercased form of property is not `LIB-NAME` or `LIB-VERSION`, throws an error.

#### KILL
**Syntax**: `CLIENT KILL id`  
**Description**: Kills specified client.  
**Since**: `0.1.8`  
**Time complexity**: `O(1)`  
**Permissions**: `P_CLIENT`  
**Returns**: `OK`  
**Behavior**:
* If specified client is not exist, throws an error.
* If specified client ID value is higher or less than uint32_t bounds, throws an error.

**Arguments**:
- **id**: Client ID, must be unsigned integer value

#### UNLOCK
**Syntax**: `CLIENT UNLOCK id`  
**Description**: Unlocks specified client.  
**Since**: `0.1.8`  
**Time complexity**: `O(1)`  
**Permissions**: `P_CLIENT`  
**Returns**: `OK`
**Behavior**:
* If specified client is not exist, throws an error.
* If specified client ID value is higher or less than uint32_t bounds, throws an error.
* If specified client ID is not locked, throws an error.

---

### COMMAND
**Syntax**: `COMMAND LIST|COUNT|DOCS`  
**Description**: Gives information about the commands in the server.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  

**Subcommands**:

#### LIST
**Syntax**: `COMMAND LIST`  
**Description**: Returns name list of all commands.  
**Since**: `0.1.0`  
**Time complexity**: `O(N) where N is count of all commands`  
**Permissions**: None  
**Returns**: Array

#### COUNT
**Syntax**: `COMMAND COUNT`  
**Description**: Returns count of all commands in the server.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: Integer

#### DOCS
**Syntax**: `COMMAND DOCS`  
**Description**: Returns documentation about multiple commands.  
**Since**: `0.1.0`  
**Time complexity**: `O(N) where N is count of commands to look up`  
**Permissions**: None  
**Returns**: Array including each command's information as array  

---

### DISCARD
**Syntax**: `DISCARD`  
**Description**: Discards the current started transaction block.  
**Since**: `0.2.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
+ If there is no started transaction block, throws an error.

---

### EXEC
**Syntax**: `EXEC`  
**Description**: Executes a transaction block consists of multiple transactions.  
**Since**: `0.2.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
+ If there is no started transaction block, throws an error.

---

### HELLO
**Syntax**: `HELLO [protover]`  
**Description**: Handshakes with the tellydb server.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: Basic information about the server as array for RESP2, map for RESP3  
**Behavior**:
* protover must be 2 or 3, otherwise throws an error.
* Sets protocol using by client as protover. If it is not specified, it will not be changed.
* Default protocol version of clients is 2/RESP2.

---

### INFO
**Syntax**: `INFO [section [section ...]]`  
**Description**: Displays server information.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: Information about the server and connection as string  
**Behavior**:
+ Allowed section names are `server` and `clients`, if it is not specified, includes all of them in return value.

---

### MULTI
**Syntax**: `MULTI`  
**Description**: Creates a transaction block consists of multiple transactions.  
**Since**: `0.2.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
+ If there is a started transaction block already, throws an error.

---

### PING
**Syntax**: `PING [value]`  
**Description**: Pings the server and returns a simple/bulk string.  
**Since**: `0.1.2`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `PONG` or value

---

### PWD
**Syntax**: `PWD ADD|REMOVE|GENERATE`  
**Description**: Allows to manage passwords.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Permissions**: `P_AUTH`  

**Subcommands**:

#### ADD
**Syntax**: `PWD ADD password permissions`  
**Description**: Adds a password.  
**Since**: `0.1.7`  
**Time complexity**: `O(N) where N is permissions length`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
* If current using password by client do not have a permissions in `permissions`, throws an error.
* If there is an invalid permission in `permissions`, throws an error.
* If `password` already exists, throws an error.

**Arguments**:
- **password**: Password value
- **permissions**: Look at [AUTH.md](./AUTH.md). If argument is `all`, it means that all permissions. If not, each character represents a permissions:
  + `r` => `P_READ`
  + `w` => `P_WRITE`
  + `c` => `P_CLIENT`
  + `o` => `P_CONFIG`
  + `a` => `P_AUTH`
  + `s` => `P_SERVER`

#### EDIT
**Syntax**: `PWD EDIT password permissions`  
**Description**: Edits a password permissions.  
**Since**: `0.1.7`  
**Time complexity**: `O(N) where N is permissions length`  
**Permissions**: None  
**Returns**: `OK`  
**Behavior**:
* If current using password by client do not have a permissions in `permissions`, throws an error.
* If there is an invalid permission in `permissions`, throws an error.
* If `password` does not exist, throws an error.

**Arguments**:
- **password**: Password value
- **permissions**: For permissions, look at [AUTH.md](./AUTH.md). Each character represents a permissions:
  + `r` => `P_READ`
  + `w` => `P_WRITE`
  + `c` => `P_CLIENT`
  + `o` => `P_CONFIG`
  + `a` => `P_AUTH`
  + `s` => `P_SERVER`

#### REMOVE
**Syntax**: `PWD REMOVE password`  
**Description**: Removes a password.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`

#### GENERATE
**Syntax**: `PWD GENERATE`  
**Description**: Generates a password value.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: `OK`  

---

### TIME
**Syntax**: `TIME`
**Description**: Returns the current server time.  
**Since**: `0.1.2`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: Array includes an Unix timestamp and microseconds already elapsed in the current second  
**Behavior**:
* Calls gettimeofday() method.

---

## List Commands

### LINDEX
**Syntax**: `LINDEX key index`  
**Description**: Returns element at the index in the list.  
**Since**: `0.1.4`  
**Time complexity**: `O(N) where N is min(absolute index, list size - absolute index - 1) number`  
**Permissions**: `P_READ`  
**Returns**: A value or null reply if the index is not exist  
**Behavior**:
* Index starts from 0; -1 represents the last element.

**Example**:
```shell
LINDEX tasks 1
LINDEX tasks -3
```

---

### LLEN
**Syntax**: `LLEN key`  
**Description**: Returns length of the list.  
**Since**: `0.1.3`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ`  
**Returns**: Integer  
**Behavior**:
* If the key is holding a value that is not a list, throws an error.
* If the key is not holding a value, returns zero.

**Example**:
```shell
LLEN tasks
```

---

### LPOP
**Syntax**: `LPOP key`  
**Description**: Removes and returns first element of the list.  
**Since**: `0.1.3`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: A value or null reply  
**Behavior**:
* If the key is holding a value that is not a list, throws an error.
* If the key is not holding a value, returns null reply.
* If the list has only one element, the list will be deleted from the database.

**Example**:
```shell
LPOP tasks
```

---

### LPUSH
**Syntax**: `LPUSH key element [element ...]`  
**Description**: Pushes element(s) to beginning of the list.  
**Since**: `0.1.3`  
**Time complexity**: `O(N) where N is written element count`  
**Permissions**: `P_WRITE`  
**Returns**: Pushed element count  
**Behavior**:
* If the key is holding a value that is not a list, throws an error.
* If the key is not holding a value, creates a list for the key and pushes element(s).

**Example**:
```shell
LPUSH tasks "Write report" "Send email"
```

---

### RPOP
**Syntax**: `RPOP key`  
**Description**: Removes and returns last element(s) of the list.  
**Since**: `0.1.3`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: A value or null reply  
**Behavior**:
* If the key is holding a value that is not a list, throws an error.
* If the key is not holding a value, returns null reply.
* If the list has only one element, the list will be deleted from the database.

**Example**:
```shell
RPOP tasks
```

---

### RPUSH
**Syntax**: `RPUSH key element [element ...]`  
**Description**: Pushes element(s) to ending of the list.  
**Since**: `0.1.3`  
**Time complexity**: `O(N) where N is written element count`  
**Permissions**: `P_WRITE`  
**Returns**: Pushed element count  
**Behavior**:
* If the key is holding a value that is not a list, throws an error.
* If the key is not holding a value, creates a list for the key and pushes element(s).

**Example**:
```shell
RPUSH tasks "Write report" "Send email"
```

---

## Hashtable Commands

### HDEL
**Syntax**: `HDEL key field [field ...]`  
**Description**: Deletes field(s) of the hash table.  
**Since**: `0.1.5`  
**Time complexity**: `O(N) where N is written field name count`  
**Permissions**: (`P_READ` if need to be sent response) and `P_WRITE`  
**Returns**: Deleted field count  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns `0`.

**Example**:
```shell
HDEL user_profile age
```

---

### HGET
**Syntax**: `HGET key field`  
**Description**: Gets a field from the hash table.  
**Since**: `0.1.3`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ`  
**Returns**: A value or null reply  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns null reply.

**Example**:
```shell
HGET user_profile age
```

---

### HGETALL
**Syntax**: `HGETALL key`  
**Description**: Gets all fields and their values from the hash table.  
**Since**: `0.1.9`  
**Time complexity**: `O(N) where N is hash table size`  
**Permissions**: `P_READ`  
**Returns**: A map/array  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns an empty array/map reply.
* Returns an array for RESP2 protocol, a map for RESP3 protocol.

**Example**:
```shell
HGETALL user_profile
```

---

### HKEYS
**Syntax**: `HKEYS key`  
**Description**: Gets all field names from the hash table.  
**Since**: `0.1.9`  
**Time complexity**: `O(N) where N is written field name-value pair count`  
**Permissions**: `P_READ`  
**Returns**: An array  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns an empty array

**Example**:
```shell
HKEYS user_profile
```

---

### HLEN
**Syntax**: `HLEN key`  
**Description**: Returns field count information of the hash table.  
**Since**: `0.1.3`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ`  
**Returns**: An array or null reply  
* First element is used field count.
* Second element is usable field capacity.

**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns null reply.

**Example**:
```shell
HLEN user_profile
```

---

### HSET
**Syntax**: `HSET key [(field name) (field value) ...]`  
**Description**: Sets field(s) of the hash table.  
**Since**: `0.1.3`  
**Time complexity**: `O(N) where N is written field name-value pair count`  
**Permissions**: `P_WRITE`  
**Returns**: Field count that is set  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, creates a hash table and sets field(s).

**Example**:
```shell
HSET user_profile name "Alice" age 30
```

---

### HTYPE
**Syntax**: `HTYPE key field`  
**Description**: Returns type of the field from hash table.  
**Since**: `0.1.3`  
**Time complexity**: `O(N) where N is written field name-value pair count`  
**Permissions**: `P_READ`  
**Returns**: A value type or null reply  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns null reply

**Example**:
```shell
HTYPE user_profile name
```

---

### HVALS
**Syntax**: `HVALS key`  
**Description**: Gets all field values from the hash table.  
**Since**: `0.1.9`  
**Time complexity**: `O(N) where N is written field name-value pair count`  
**Permissions**: `P_READ`  
**Returns**: An array  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns an empty array

**Example**:
```shell
HVALS user_profile
```

---

## Key-Value Commands

### APPEND
**Syntax**: `APPEND key value`  
**Description**: Appends string to string value. If key is not exist, creates a new one.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: String length after appending  
**Behavior**:
* Throws an error if the key is holding a value that is not string.

**Example**:
```shell
APPEND user_name " Black"
```

---

### DECR
**Syntax**: `DECR [...key(s)]`  
**Description**: Decrements the number stored at each key.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: A map/an array that keys to responses  
**Behavior**:
* If the key is not holding a value, value will be set to `0` and will not be decremented.
* If the key is holding a value that is not integer, response will be `invalid type`.
* If the new key cannot be set, response will be `error`.
* Response will be new value of the key when the key is set successfully.

**Example**:
```shell
DECR user_age
DECR user_count user_money
```

---

### DECRBY
**Syntax**: `DECRBY key value`  
**Description**: Decrements the number stored at key by value.  
**Since**: `0.2.0`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: An integer  
**Behavior**:
* If the key is not holding a value, value will be set to `-value`.
* If the key is holding a value that is not integer, throws an error.
* If the new key cannot be set, throws an error.

**Example**:
```shell
DECRBY user_age 3
```

---

### DEL
**Syntax**: `DEL key [key ...]`  
**Description**: Deletes the specified keys.  
**Since**: `0.1.7`  
**Time complexity**: `O(N) where N is key count`  
**Permissions**: `P_WRITE`  
**Returns**: Deleted key count  

**Example**:
```shell
DEL user_name user_id
```

---

### EXISTS
**Syntax**: `EXISTS key [key ...]`  
**Description**: Checks if specified keys exist or not.  
**Since**: `0.1.4`  
**Time complexity**: `O(N) where N is key count`  
**Permissions**: None  
**Returns**: Array has 2+N elements where N is key count
* First element is existed key count
* Second element is not existed key count
* Other elements represents keys are existed or not key by key. Ordered like usage in the command.

**Example**:
```shell
EXISTS user_name session_token
```

---

### GET
**Syntax**: `GET key`  
**Description**: Gets value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ`  
**Returns**: A value or null reply if key is not exist  
**Behavior**:
* If the value is hash table or list, writes only "hash table" or "list", not be written list and hash table values.

**Example**:
```shell
GET user_name
```

---

### INCR
**Syntax**: `INCR [..key(s)]`  
**Description**: Increments the number stored at each key.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: A map/an array that keys to responses  
**Behavior**:
* If the key is not holding a value, value will be set to `0` and will not be incremented.
* If the key is holding a value that is not integer, response will be `invalid type`.
* If the new key cannot be set, response will be `error`.
* Response will be new value of the key when the key is set successfully.

**Example**:
```shell
INCR user_age
INCR user_count user_money
```

---

### INCRBY
**Syntax**: `INCRBY key value`  
**Description**: Increments the number stored at key by value.  
**Since**: `0.2.0`  
**Time complexity**: `O(1)`  
**Permissions**: `P_READ` and `P_WRITE`  
**Returns**: An integer  
**Behavior**:
* If the key is not holding a value, value will be set to `value`.
* If the key is holding a value that is not integer, throws an error.
* If the new key cannot be set, throws an error.

**Example**:
```shell
INCRBY user_age 3
```

---

### RENAME
**Syntax**: `RENAME old new`  
**Description**: Renames existing key to new key.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Permissions**: `P_WRITE`  
**Returns**: `OK` or null reply if key is ntot exist  
**Behavior**:
* If new key already exists, throws an error.

**Example**:
```shell
RENAME name user_name
```

---

### SET
**Syntax**: `SET key value [NX|XX] [GET] [AS type]`  
**Description**: Sets value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: (`P_READ` if used `GET` argument) and `P_WRITE`  
**Returns**: `OK` or a value or an error message  
**Behavior**:
* If the key is exist, new value will be overwritten.
* If value type is not specified using `AS type` argument, type will be determined by value itself. For example, `boolean` type for `true` value.
* If value type is specified using `AS type` and value does not match the type, throws an error.
* If a memory allocation is failed, throws an error.

**Arguments**:
- **NX**: Only set if the key does not exist.
- **XX**: Only set if the key exists.
- **GET**: Returns the old value if it existed.
- **AS type**: Determines value type, allowed values are:
  * `str` or `string`
  * `number` or `num` or `integer` or `int`
  * `bool` or `boolean`
  * `null`

**Examples**:
```shell
SET user_name "Alice"
SET user_age 25 NX
SET session_token "abc123" XX GET
SET user_id 369 AS str
```

---

### TYPE
**Syntax**: `TYPE key`  
**Description**: Returns type of the value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Permissions**: None  
**Returns**: A value type

**Example**:
```shell
TYPE user_name
```
