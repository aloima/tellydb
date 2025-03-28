FROM ubuntu
WORKDIR /tellydb

COPY . .
RUN apt-get update
RUN apt-get -y install libssl-dev gcc make git pkg-config
RUN make telly

EXPOSE 6379

CMD ["./telly"]
