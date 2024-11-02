# Commands
This document provides a detailed description of all the available commands in TellyDB, an in-memory key-value database. Each command's syntax, arguments, and examples are included for clarity.

---

## Table of Contents
1. [Database Commands](#database-commands)
2. [Key-Value Commands](#key-value-commands)
3. [List Commands](#list-commands)
4. [Hash Commands](#hash-commands)

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
* If a saving process from [BGSAVE](#bgsave) or [SAVE](#save) is active, gives an error.

---

### DBSIZE
**Syntax**: `DBSIZE`  
**Description**: Returns key count in the database.  
**Since**: `0.1.6`  
**Time complexity**: `O(1)`  
**Returns**: An integer

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

## Key-Value Commands

### DECR
**Syntax**: `DECR key`  
**Description**: Decrements value.  
**Since**: `0.1.0`  
**Time complexity**: `O(1)`  
**Returns**: An integer, new value stored at the key  
**Behavior**:
* If key is not holding a value, value will be set to 0 and command stops.
* Gives an error if the key is holding a value that is not an integer.

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
**Returns**: An array has 2+N elements where N is key count
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
**Returns**: An integer, new value stored at the key  
**Behavior**:
* If key is not holding a value, value will be set to 0 and command stops.
* Gives an error if the key is holding a value that is not an integer.

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

---

## List Commands

### LINDEX
**Syntax**: `LINDEX key index`  
**Description**: Returns element at the index in the list.  
**Since**: `0.1.4`  
**Time complexity**: `O(N) where N is absolute index number`  
**Returns**: A value or null reply if the index is not exist
**Behavior**:
- Index starts from 0; -1 represents the last element.

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
**Returns**: An integer  
**Behavior**:
* If the key is holding a value that is not a list, gives an error.
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
* If the key is holding a value that is not a list, gives an error.
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
* If the key is holding a value that is not a list, gives an error.
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
* If the key is holding a value that is not a list, gives an error.
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
* If the key is holding a value that is not a list, gives an error.
* If the key is not holding a value, creates a list for the key and pushes element(s).

**Example**:
```shell
RPUSH tasks "Write report" "Send email"
```

---

## Hash Commands

### HDEL
**Syntax**: `HDEL key field [field ...]`  
**Description**: Deletes field(s) of the hash table.  
**Since**: `0.1.5`  
**Time complexity**: `O(N) where N is written field name count`  
**Returns**: Deleted field count  
**Behavior**:
* If the key is holding a value that is not a hash table, gives an error.
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
* If the key is holding a value that is not a hash table, gives an error.
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
* If the key is holding a value that is not a hash table, gives an error.
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
* If the key is holding a value that is not a hash table, gives an error.
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
* If the key is holding a value that is not a hash table, gives an error.
* If the key is not holding a value, returns null reply

**Example**:
```shell
HSET user_profile name "Alice" age 30
```
