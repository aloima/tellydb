#pragma once

#include "client.h"
#include "../utils/utils.h" // IWYU pragma: export

#include <stddef.h>

#include <unistd.h>

#include <openssl/ssl.h>

int _read(Client *client, char *buf, const size_t nbytes);
int _write(Client *client, char *buf, const size_t nbytes);

#define WRITE_OK_MESSAGE(client, message) _write((client),    RDT_SSTRING message "\r\n", sizeof(message) + 2)
#define WRITE_ERROR_MESSAGE(client, message) _write((client), RDT_ERROR   message "\r\n", sizeof(message) + 2)

#if defined(__linux__)
  #include <sys/epoll.h>

  typedef struct epoll_event event_t;

  #define GET_EVENT_FD(event) (event).data.fd
  #define WAIT_EVENTS(eventfd, events, count) epoll_wait((eventfd), (events), (count), -1)
  #define GET_EVENT_DATA(event) (event).data.ptr
  #define IS_CONNECTION_CLOSED(event) ((event).events & (EPOLLRDHUP | EPOLLHUP))
  #define ADD_TO_MULTIPLEXING(eventfd, connfd, event) epoll_ctl((eventfd), EPOLL_CTL_ADD, (connfd), &(event))

  #define PREPARE_EVENT(event, client, connfd) do { \
    (void) connfd; \
    (event).events = (EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP); \
    (event).data.ptr = (client); \
  } while (0)

  #define CREATE_EVENTFD() epoll_create1(0)

  #define CREATE_EVENT(event, sockfd) do { \
    (event).events = EPOLLIN; \
    (event).data.fd = (sockfd); \
  } while (0)

  #define ADD_EVENT(eventfd, sockfd, event) epoll_ctl((eventfd), EPOLL_CTL_ADD, (sockfd), &(event))

  #define REMOVE_EVENT(eventfd, connfd) epoll_ctl((eventfd), EPOLL_CTL_DEL, (connfd), NULL)
  #define PREPARE_REMOVING_EVENT(ev, connfd) (void) ev, (void) connfd
#elif defined(__APPLE__)
  #include <sys/event.h>
  #include <sys/time.h>
  #include <fcntl.h>

  typedef struct kevent event_t;

  #define GET_EVENT_FD(event) (event).ident
  #define WAIT_EVENTS(eventfd, events, count) kevent((eventfd), NULL, 0, (events), (count), NULL)
  #define GET_EVENT_DATA(event) (event).udata
  #define IS_CONNECTION_CLOSED(event) ((event).flags & EV_EOF)
  #define ADD_TO_MULTIPLEXING(eventfd, connfd, event) kevent((eventfd), &(event), 1, NULL, 0, NULL)

  #define PREPARE_EVENT(event, client, connfd) \
    EV_SET(&(event), (connfd), EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, (client))

  #define CREATE_EVENTFD() kqueue()
  #define CREATE_EVENT(event, sockfd) EV_SET(&(event), (sockfd), EVFILT_READ, EV_ADD, 0, 0, NULL)
  #define ADD_EVENT(eventfd, sockfd, event) kevent((eventfd), &(event), 1, NULL, 0, NULL)

  #define REMOVE_EVENT(eventfd, connfd) kevent((eventfd), &ev, 1, NULL, 0, NULL)
  #define PREPARE_REMOVING_EVENT(ev, connfd) EV_SET(&(ev), (connfd), EVFILT_READ, EV_DELETE, 0, 0, NULL)
#endif
