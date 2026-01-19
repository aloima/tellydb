# Testing
For testing, you need Python +3.14 and pip.

> [!CAUTION]
> Used [tellypy](https://github.com/aloima/tellypy) library, it is not yet published.  
> So, use with manual downloading or via github link in requirements.txt

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
