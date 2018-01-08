#ifndef SKYNET_FUNCTOR_H__
#define SKYNET_FUNCTOR_H__

#include "transport.h"
#include "serializer.h"
#include "logger.h"
#include "args.h"

void* raft_server;

// libevent callback
void timeout_cb(int, short, void*);
void udp_read_cb(int, short, void*);

// raft callback
void raft_log_cb(raft_server_t*, raft_node_t*, void*, const char*);
int raft_send_requestvote_cb(raft_server_t*, void*, raft_node_t*, msg_requestvote_t*);
int raft_send_appendentries_cb(raft_server_t*, void*, raft_node_t*, msg_appendentries_t*);

// raft rpc message handler
int handle_rpc_message(int, char*, int, struct sockaddr*, ev_socklen_t);
int handle_rpc_requestvote_message(int, struct sockaddr*, ev_socklen_t);
int handle_rpc_requestvote_response_message(int, struct sockaddr*, ev_socklen_t);
int handle_rpc_appendentries_message(int, struct sockaddr*, ev_socklen_t);
int handle_rpc_appendentries_response_message(int, struct sockaddr*, ev_socklen_t);

#endif