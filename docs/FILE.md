## Introduction
This file is a small documentation for files that provided by tellydb. Specified file names are default file names.
They can be changed from configuration file. Configuration file is changeable via command argument.

## Database file | `.tellydb`
It consists of file headers, authorization part and data lines.
* `file headers + authorization part + data lines`

File headers have 10 bytes size and is as follows:
* `0x1810 + server age (8 byte)`

### String length specifier
In this file, a structure named string length specifier for strings is defined as follows:
> A string length is maximum `2^30-1` and represented by 30 bit (6 bit + 3 byte).  
> A string length specifier is minimum 1 byte, maximum 4 byte. First two bits of first byte represents additional byte count.  
> For example, construction of string length using `0b(10|100010) 0x07 0x09` data is as follows:
>  
> A: 0b(10|100010)  
> B: 0x07 = 0b00000111  
> C: 0x09 = 0b00001001
>  
> Value of two bits before `|` is `0b10` or `2`. This shows that existence of additional two bytes (B and C).  
> The six bits after `|` and additional bytes represents string length as reversed.  
> `C + B + (Bits after | in A)` or `0b00001001_00000111_100010` is `147938`.

### Authorization part
Authorization part consists of passwords and their permissions and as follows:
* `password count byte count (1 byte, n) + password count (n byte) + passwords`

A password is as follows:
* `derived password (48 byte) + password permissions (1 byte)`

For permissions, look at [AUTH.md](./AUTH.md).

### Data lines
A data line is as follows:
* `string length specifier + data key + data type + data value`
* Data key is a string.

Data value scheme is defined as:
> [!NOTE]
> All content of data value that stores a number (list size, byte count, number etc.) is [little-endian](https://en.wikipedia.org/wiki/Endianness).

* For `TELLY_NULL (0x00)` type, data value is nothing and the line consists of `data key + 0x1D + TELLY_NULL`.
* For `TELLY_NUM (0x01)` type, data value is `byte count (1 byte) + number`. For example, data value is `0x02 + (0x00 + 0x01)` or `0x020001` to get 256.
* For `TELLY_STR (0x02)` type, data value is `string length specifier + string data`.
* For `TELLY_BOOL (0x03)` type, data value is `0x00` or `0x01`.

* For `TELLY_HASHTABLE (0x04)` type, data value is `hash table allocated size (n) + hash table element 1 + hash table element 2 ... hash table element n + 0x17`.

> [!IMPORTANT]
> The hash table **allocated** size is a 4-byte value. For example, `32` is represented as `0x20 0x00 0x00 0x00`.  
> A hash table element is `element type + string length specifier + element key + element value`.  
> Element values ​​are data values, so their rules are same as data value rules.  
> Additionally, type of a hash table element should be `TELLY_NULL`, `TELLY_NUM`, `TELLY_STR` or `TELLY_BOOL`.


* For `TELLY_LIST (0x05)` type, data value is `list size (n) + list element 1 + list element 2 ... list element n`.

> [!IMPORTANT]
> The list size is a 4-byte value. For example, `32` is represented as `0x20 0x00 0x00 0x00`.  
> A list element is `element type + element value` and element values ​​are data values, so their rules are same as data value rules.  
> Additionally, type of a list element should be `TELLY_NULL`, `TELLY_NUM`, `TELLY_STR` or `TELLY_BOOL`.

## Configuration file | `.tellyconf`
It consists of lines that are comments and values. A comment line is as follows:
```
#any text, no need a blank character after # character.
```

A value line is as follows:
```
KEY=VALUE
```

Maximum key length and maximum value length is 48.
