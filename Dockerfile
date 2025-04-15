# syntax=docker/dockerfile:1-labs

FROM alpine AS build
WORKDIR /tellydb
ARG GIT_HASH
ARG GIT_VERSION

RUN apk add --no-cache openssl openssl-dev musl-dev gcc make pkgconfig

COPY --parents src headers Makefile ./
RUN make telly

FROM alpine
COPY --from=build /tellydb/telly /telly
EXPOSE 6379

RUN apk add --no-cache openssl
CMD ["./telly"]
