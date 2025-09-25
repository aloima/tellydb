# syntax=docker/dockerfile:1-labs

FROM alpine AS build
WORKDIR /tellydb
ARG GIT_HASH
ARG GIT_VERSION

RUN apk add --no-cache \
    openssl openssl-dev \
    musl-dev \
    gcc make cmake \
    gperf git \
    gmp gmp-dev \
    jemalloc jemalloc-dev \
    hiredis hiredis-dev

COPY . .
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make telly

FROM alpine
WORKDIR /tellydb
COPY --from=build /tellydb/build/telly /tellydb/telly

RUN apk add --no-cache openssl gmp jemalloc

EXPOSE 6379
CMD ["/tellydb/telly"]
