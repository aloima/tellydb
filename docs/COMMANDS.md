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

## Database Commands

### BGSAVE
**Syntax**: `BGSAVE`  
**Description**: Saves all data to database file in background using a thread.  
**Since**: `0.1.6`  
**Time complexity**: `O(N) where N is cached key-value pairs`  
**Returns**: `OK`  
**Behavior**:
* Creates a thread to save all data to the database.
* The created thread will be closed after saving.
* If a saving process from [BGSAVE](#bgsave) or [SAVE](#save) is active, throws an error.

---

### DBSIZE
**Syntax**: `DBSIZE`  
**Description**: Returns key count in the database.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Returns**: Integer

---

### LASTSAVE
**Syntax**: `LASTSAVE`  
**Description**: Returns last save time of database as UNIX time.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Returns**: Integer  

---

### SAVE
**Syntax**: `SAVE`  
**Description**: Saves all data to database file.  
**Since**: `0.1.6`  
**Time complexity**: `O(N) where N is cached key-value pairs`  
**Returns**: `OK`  
**Behavior**:
* Waits until saving all data to database file, so **it blocks all client commands.**

---

## Generic Commands

### AGE
**Syntax**: `AGE`  
**Description**: Sends the server age as seconds.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Returns**: Integer

---

### CLIENT
**Syntax**: `CLIENT ID|INFO|SETINFO`  
**Description**: Main command of client(s).  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  

**Subcommands**:

#### ID
**Syntax**: `CLIENT ID`  
**Description**: Returns ID number of client.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: Integer

#### INFO
**Syntax**: `CLIENT INFO`  
**Description**: Returns information about the client.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: String

#### SETINFO
**Syntax**: `CLIENT SETINFO property value`  
**Description**: Returns information about the client.  
**Since**: `0.1.2`  
**Time complexity**: `O(1)`  
**Returns**: `OK`  
**Behavior**:
* If uppercased form of property is not `LIB-NAME` or `LIB-VERSION`, throws an error.

---

### COMMAND
**Syntax**: `COMMAND LIST|COUNT|DOCS`  
**Description**: Gives information about the commands in the server.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  

**Subcommands**:

#### LIST
**Syntax**: `COMMAND LIST`  
**Description**: Returns name list of all commands.  
**Since**: `0.1.0`  
**Time complexity**: `O(N) where N is count of all commands`  
**Returns**: Array includes string

#### COUNT
**Syntax**: `COMMAND COUNT`  
**Description**: Returns count of all commands in the server.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: Integer

#### DOCS
**Syntax**: `COMMAND DOCS`  
**Description**: Returns documentation about multiple commands.  
**Since**: `0.1.0`  
**Time complexity**: `O(N) where N is count of commands to look up`  
**Returns**: Array including each command's information as array  

---

### HELLO
**Syntax**: `HELLO [protover]`  
**Description**: Handshakes with the tellydb server.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
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
**Returns**: Information about the server and connection as string  
**Behavior**:
+ Allowed section names are `server` and `clients`, if it is not specified, includes all of them in return value.

---

### PING
**Syntax**: `PING [value]`  
**Description**: Pings the server and returns a simple/bulk string.  
**Since**: `0.1.2`  
**Time complexity**: `O(1)`  
**Returns**: `PONG` or value

---

### TIME
**Syntax**: `TIME`
**Description**: Returns the current server time as two elements in a array, a Unix timestamp and microseconds already elapsed in the current second.  
**Since**: `0.1.2`  
**Time complexity**: `O(1)`  
**Returns**: Array includes time information  
**Behavior**:
* Calls gettimeofday() method

---

## List Commands

### LINDEX
**Syntax**: `LINDEX key index`  
**Description**: Returns element at the index in the list.  
**Since**: `0.1.4`  
**Time complexity**: `O(N) where N is absolute index number`  
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
**Returns**: A value or null reply  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns null reply.

**Example**:
```shell
HGET user_profile age
```

---

### HLEN
**Syntax**: `HLEN key`  
**Description**: Returns field count information of the hash table.  
**Since**: `0.1.3`  
**Time complexity**: `O(1)`  
**Returns**: An array or null reply  
* First element is allocated field area count except fields from field->next.
* Second element is filled field count using allocated field areas except fields from field->count.
* Third element is all filled field count included fields from field->next.
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns null reply.

**Example**:
```shell
HLEN user_profile
```

---

### HSET
**Syntax**: `HSET key [(field value) ...]`  
**Description**: Sets field(s) of the hash table.  
**Since**: `0.1.3`  
**Time complexity**: `O(N) where N is written field name-value pair count`  
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
**Returns**: A value type or null reply  
**Behavior**:
* If the key is holding a value that is not a hash table, throws an error.
* If the key is not holding a value, returns null reply

**Example**:
```shell
HSET user_profile name "Alice" age 30
```

---

## Key-Value Commands

### DECR
**Syntax**: `APPEND key value`  
**Description**: Appends string to string value. If key is not exist, creates a new one.  
**Since**: `0.1.7`  
**Time complexity**: `O(1)`  
**Returns**: String length after appending  
**Behavior**:
* Throws an error if the key is holding a value that is not string.

**Example**:
```shell
APPEND user_name " Black"
```

---

### DECR
**Syntax**: `DECR key`  
**Description**: Decrements value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: New integer value stored at the key  
**Behavior**:
* If key is not holding a value, value will be set to `0` and will not be decremented.
* Throws an error if the key is holding a value that is not integer.

**Example**:
```shell
DECR user_age
```

---

### EXISTS
**Syntax**: `EXISTS key [key ...]`  
**Description**: Checks if specified keys exist or not.  
**Since**: `0.1.4`  
**Time complexity**: `O(N) where N is key count`  
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
**Returns**: A value or null reply if key is not exist  
**Behavior**:
* If the value is hash table or list, writes only "hash table" or "list", not be written list and hash table values.

**Example**:
```shell
GET user_name
```

---

### INCR
**Syntax**: `INCR key`  
**Description**: Increments value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: New integer value stored at the key  
**Behavior**:
* If key is not holding a value, value will be set to `0` and will not be incremented.
* Throws an error if the key is holding a value that is not integer.

**Example**:
```shell
INCR user_age
```

---

### SET
**Syntax**: `SET key value [NX|XX] [GET]`  
**Description**: Sets value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: `OK` or a value  
**Behavior**:
* If the key is exist, new value will be overwritten.

**Arguments**:
- **NX**: Only set if the key does not already exist.
- **XX**: Only set if the key exists.
- **GET**: Returns the old value if it existed.

**Examples**:
```shell
SET user_name "Alice"
SET user_age 25 NX
SET session_token "abc123" XX GET
```

---

### TYPE
**Syntax**: `TYPE key`  
**Description**: Returns type of the value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: A value type

**Example**:
```shell
TYPE user_name
```
