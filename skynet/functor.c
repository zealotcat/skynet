#include "functor.h"

// raft callback functions
void raft_log_cb(raft_server_t* raft, raft_node_t* node, void *udata, const char *buf) {
    info("raft: %s", buf);
}

int raft_send_requestvote_cb(raft_server_t* raft, void* user_data, raft_node_t* node, msg_requestvote_t* msg) {
    Transport* tran = (Transport*)raft_node_get_udata(node);
    Serializer* se = (Serializer*)raft_get_udata(raft);
    int myid = raft_get_nodeid(raft);
    
    int ret = serialize_requestvote(se, myid, msg);
    if (ret != 0) {
        error("serialize_requestvote() failed");
        return -1;
    }

    char* s = serializer_to_string(se);
    debug("send: %s", s);
    ret = transport_send(tran, s, strlen(s));
    if (ret <= 0) {
        error("transport_send() failed");
        return -1;
    }
    return 0;
}

int raft_send_appendentries_cb(raft_server_t* raft, void* user_data, raft_node_t* node, msg_appendentries_t* msg) {
    Transport* tran = (Transport*)raft_node_get_udata(node);
    Serializer* se = (Serializer*)raft_get_udata(raft);
    int myid = raft_get_nodeid(raft);
    
    int ret = serialize_appendentries(se, myid, msg);
    if (ret != 0) {
        error("serialize_requestvote() failed");
        return -1;
    }

    char* s = serializer_to_string(se);
    debug("send: %s", s);
    ret = transport_send(tran, s, strlen(s));
    if (ret <= 0) {
        error("transport_send() failed");
        return -1;
    }

    return 0;
}

// libevent callback functions
void timeout_cb(int fd, short event, void *arg) {
    struct event* timeout_event = (struct event*)arg;

    info("timeout, elaspsed: %d", raft_get_timeout_elapsed(raft_server));
    raft_periodic(raft_server, HEARTBEAT_TIMEOUT);
    
    // reset timer
    struct timeval tv = {HEARTBEAT_TIMEOUT/1000, 0};
    evtimer_add(timeout_event, &tv);
}

void udp_read_cb(int fd, short events, void *arg) {
    char buffer[4096];
    int ret;
    ev_socklen_t size = sizeof(struct sockaddr);
    struct sockaddr_in cli_sa;
 
    memset(buffer, 0, sizeof(buffer));
    ret = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_sa, &size);
    if (ret <= 0) {
        error("recvfrom() failed, err: %s", strerror(ret));
        return;
    } 

    info("recv: %s", buffer);
    handle_rpc_message(fd, buffer, ret, (struct sockaddr*)&cli_sa, size);
}

// raft rpc message handler
int handle_rpc_message(int fd, char* buffer, int len, struct sockaddr* caddr, ev_socklen_t size) {
    Serializer* se = (Serializer*)raft_get_udata(raft_server);
    int ret = deserialize_message(se, buffer, len);
    if (ret != 0) {
        error("deserialize_message() failed");
        return -1;
    }
    
    const char* message_type = json_string_value(json_object_get(se->root, "message_type"));
    if (strcmp(message_type, "request_vote") == 0) {
        ret = handle_rpc_requestvote_message(fd, caddr, size);
    } else if (strcmp(message_type, "request_vote_response") == 0) {
        ret = handle_rpc_requestvote_response_message(fd, caddr, size);
    } else if (strcmp(message_type, "append_entries") == 0) {
        ret = handle_rpc_appendentries_message(fd, caddr, size);
    }
    return 0;
}

int handle_rpc_requestvote_message(int fd, struct sockaddr* caddr, ev_socklen_t size) {
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;
    Serializer* se = (Serializer*)raft_get_udata(raft_server);

    deserialize_requestvote(se, &rv);
    raft_recv_requestvote(raft_server, NULL, &rv, &rvr);
    serialize_requestvote_response(se, raft_get_nodeid(raft_server), &rvr);

    char* m = serializer_to_string(se);
    debug("send: %s", m);
    if (sendto(fd, m, strlen(m), 0, caddr, size) <= 0)
    {
        perror("sendto() failed");
        return -1;
    }
    return 0;
}

int handle_rpc_requestvote_response_message(int fd, struct sockaddr* client_addr, ev_socklen_t addr_size) {
    msg_requestvote_response_t rvr;
    Serializer* se = (Serializer*)raft_get_udata(raft_server);
    int node_id = json_integer_value(json_object_get(se->root, "myid"));
    raft_node_t* node = raft_get_node(raft_server, node_id);
    deserialize_requestvote_response(se, &rvr);
    raft_recv_requestvote_response(raft_server, node, &rvr);
    return 0;
}

int handle_rpc_appendentries_message(int fd, struct sockaddr* caddr, ev_socklen_t size) {
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;
    Serializer* se = (Serializer*)raft_get_udata(raft_server);
    
    deserialize_appendentries(se, &ae);
    raft_recv_appendentries(raft_server, NULL, &ae, &aer);
    serialize_appendentries_response(se, raft_get_nodeid(raft_server), &aer);

    char* m = serializer_to_string(se);
    debug("send: %s", m);
    if (sendto(fd, m, strlen(m), 0, caddr, size) <= 0)
    {
        perror("sendto() failed");
        return -1;
    }
    return 0;
}

int handle_rpc_appendentries_response_message(int fd, struct sockaddr* client_addr, ev_socklen_t addr_size) {
    msg_appendentries_response_t aer;
    Serializer* se = (Serializer*)raft_get_udata(raft_server);
    int node_id = json_integer_value(json_object_get(se->root, "myid"));
    raft_node_t* node = raft_get_node(raft_server, node_id);
    deserialize_appendentries_response(se, &aer);
    raft_recv_appendentries_response(raft_server, node, &aer);
    return 0;
}










