## Introduction
This file is a small documentation for files that provided by tellydb. Specified file names are default file names.
They can be changed from configuration file. Configuration file can be specified as command argument.

## Database file | `.tellydb`
It consists of lines and a line is as follows:
`data key + 0x1D + data type + data value + 0x1E`

+ If data type is `TELLY_NULL (0x1)`, data value will be nothing and the line will consist of `data key + 0x1D + TELLY_NULL + 0x1E`.
+ If data type is `TELLY_INT (0x2)`, data value will be represented as binary. For example, data value is `0x01 + 0x00` or `0x0100` to get 256.
+ If data type is `TELLY_STR (0x3)`, data value will be a string.
+ If data type is `TELLY_BOOL (0x4)`, data value will be `0x00` or `0x01`.

## Configuration file | `.tellyconf`
It consists of lines that are comments and values. A comment line is as follows:
```
#any text, no need a blank character after # character.
```

A value line is as follows:
```
KEY=VALUE
```
