## Introduction
This file is a small documentation for files that provided by tellydb. Specified file names are default file names.
They can be changed from configuration file. Configuration file is changeable via command argument.

## Database file | `.tellydb`
It consists of file headers and data lines. File headers have 10 bytes size and is as follows:
* `0x1810 + server age (8 byte)`

A data line is as follows:
* `data key + 0x1D + data type + data value + 0x1E`

Data value scheme is defined as:
* The data type cannot be `TELLY_UNSPECIFIED (0x0)`, because this data type is for key-value pairs whose value has not yet been got.
* If data type is `TELLY_NULL (0x1)`, data value will be nothing and the line will consist of `data key + 0x1D + TELLY_NULL + 0x1E`.
* If data type is `TELLY_INT (0x2)`, data value will be represented as binary. For example, data value is `0x01 + 0x00` or `0x0100` to get 256.
* If data type is `TELLY_STR (0x3)`, data value will be a string.
* If data type is `TELLY_BOOL (0x4)`, data value will be `0x00` or `0x01`.

* If data type is `TELLY_LIST (0x6)`, data value will be `list size (n) + list element 1 + list element 2 ... list element n + 0x1E`.

> [!NOTE]
> The list size is a 4-byte value. For example, `32` is represented as `0x00 0xx0 0x00 0x20`.
> A list element is as `element type + element value + 0x1F` and element values ​​are subject to the same rules as data values.
> Additionally, type of a list element should be `TELLY_NULL`, `TELLY_INT`, `TELLY_STR` or `TELLY_BOOL`.

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
