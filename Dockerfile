# syntax=docker/dockerfile:1-labs

FROM alpine AS build
WORKDIR /tellydb

COPY --parents .git src headers Makefile ./

RUN apk add --no-cache openssl openssl-dev musl-dev gcc make git pkgconfig
RUN make telly

FROM alpine
COPY --from=build /tellydb/telly /telly
EXPOSE 6379

RUN apk add --no-cache openssl
CMD ["./telly"]
