# syntax=docker/dockerfile:1-labs

FROM alpine AS build
WORKDIR /tellydb
ARG GIT_HASH
ARG GIT_VERSION

RUN apk add --no-cache \
    openssl openssl-dev \
    musl-dev \
    gcc python3 py3-pip \
    gperf git \
    gmp gmp-dev \
    jemalloc jemalloc-dev

RUN pip install --no-cache-dir --break-system-packages meson ninja

COPY . .
RUN mkdir -p build && \
    meson setup build --native-file meson/release.ini && \
    meson compile -C build

FROM alpine
WORKDIR /tellydb
COPY --from=build /tellydb/build/telly /tellydb/telly

RUN apk add --no-cache openssl gmp jemalloc

EXPOSE 6379
CMD ["/tellydb/telly"]
