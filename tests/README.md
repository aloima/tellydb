# Testing
For testing, you need Python +3.10 and pip.

> [!CAUTION]
> Used redis-py library is specialized for redis, so tests are mostly broken,
> even tellydb is compatible with [Redis Serialization Protocol](https://redis.io/docs/latest/develop/reference/protocol-spec/).
> Library will be changed as soon as possible.

Install required packages:
```
pip install -r requirements.txt
```

Run tests individually or all:
```
pytest
pytest generic
pytest generic/test_ping.py
```
