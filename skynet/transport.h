#ifndef BH_TRANSPORT_H__
#define BH_TRANSPORT_H__

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include "logger.h"
#include "args.h"

#define IPLEN       16
#define ROLE_SERVER 101
#define ROLE_CLIENT 102

typedef struct MsgHeader {
    char type;
    int length;
} MsgHeader;

typedef struct Transport
{
    // socket prop
    evutil_socket_t fd;
    int role;
    int id;
    char ip[IPLEN];
    int port;
    struct sockaddr_in sin;
    // event prop
    struct event_base* ev_base;
    struct event* ev_server;
    struct event ev_timer;
    // event callback
    event_callback_fn read_cb;
    event_callback_fn timer_cb;
} Transport;

// constructor
Transport* transport_server_init(int, const char*, int);
Transport* transport_client_init(int, evutil_socket_t, const char*, int);
void transport_free(Transport*);

// io
int transport_server_run(Transport*);
int transport_send(Transport*, const char*, int);

// setup callback
void transport_set_read_cb(Transport*, event_callback_fn);
void transport_set_timer_cb(Transport*, event_callback_fn);

#endif