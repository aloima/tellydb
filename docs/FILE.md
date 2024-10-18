## Introduction
This file is a small documentation for files that provided by tellydb. Specified file names are default file names.
They can be changed from configuration file. Configuration file is changeable via command argument.

## Database file | `.tellydb`
It consists of file headers and data lines. File headers have 10 bytes size and is as follows:
* `0x1810 + server age (8 byte)`

A data line is as follows:
* `data key + 0x1D + data type + data value + 0x1E`

Data value scheme is defined as:
> [!NOTE]
> All content of data value that stores a number (list size, byte count, number etc.) is [little-endian](https://en.wikipedia.org/wiki/Endianness).

* For `TELLY_NULL (0x00)` type, data value is nothing and the line consists of `data key + 0x1D + TELLY_NULL`.
* For `TELLY_NUM (0x01)` type, data value is `byte count (1 byte) + number`. For example, data value is `0x02 + (0x00 + 0x01)` or `0x02001` to get 256.
* For `TELLY_STR (0x02)` type, data value is a string.
* For `TELLY_BOOL (0x03)` type, data value is `0x00` or `0x01`.

* For `TELLY_LIST (0x05)`, data value is `list size (n) + list element 1 + list element 2 ... list element n`.

> [!NOTE]
> The list size is a 4-byte value. For example, `32` is represented as `0x20 0xx0 0x00 0x00`.
> A list element is as `element type + element value + 0x1F` and element values ​​are subject to the same rules as data values.
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
