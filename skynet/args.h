#ifndef ARGS_H__
#define ARGS_H__

#include <stdio.h>
#include <getopt.h>
#include <box/array.h>

#define REQUEST_TIMEOUT     10000
#define ELECTION_TIMEOUT    15000
#define HEARTBEAT_TIMEOUT   5000

typedef struct Address {
    int id;
    const char ip[16];
    int port;
} Address;

typedef struct Args {
    // zlog config
    const char* zlog;
    // self-node's address and ip/port
    Address self;
    // other nodes address
    Array* nodes;
} Args;

// command line definition
static const char *opt_string = "h?";

static const struct option long_opts[] = {
    {"zlog", required_argument, NULL, 0},
    {"self", required_argument, NULL, 0},
    {"node", required_argument, NULL, 0},
    {"help", no_argument, NULL, 'h'},
    {NULL, no_argument, NULL, 0}
};

static const char* long_opts_desc[] = {
    "zlog config file",
    "`id@ip:port`, this node's id, ip and port",
    "`id@ip:port`, other node's id, ip and port",
    "this message",
    NULL
};

Args* args_new();
void args_free(Args*);
Address* address_new();
int parse_args(int, char**, Args*);
void display_args(const Args*);
int check_args(const Args*);
void display_usage(const char*);

#endif