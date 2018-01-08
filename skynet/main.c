#include <stdlib.h>
#include <stdio.h>
#include <raft/raft.h>
#include "logger.h"
#include "args.h"
#include "functor.h"

extern void* raft_server;

int main(int argc, char* argv[]) {
    // parse command line
    Args* args = args_new();
    if (args == NULL || parse_args(argc, argv, args) != 0) {
        fprintf(stderr, "parse command line failed\n");
        return -1;
    }
    display_args(args);

    // init zlog
    int ret = dzlog_init(args->zlog, "skynet");
    if (ret != 0) {
        fprintf(stderr, "zlog load configuration file `%s` failed\n", args->zlog);
        args_free(args); 
        return -1;
    }

    info("skynet bootstrap ...");

    // create serializer instance
    Serializer* se = serializer_init();
    
    // create server transport component
    Transport* tran = transport_server_init(args->self.id, args->self.ip, args->self.port);
    if (tran == NULL) {
        error("create transport server failed");
        goto __main_exit;
    }

    transport_set_read_cb(tran, udp_read_cb);
    transport_set_timer_cb(tran, timeout_cb);

    // create a raft server
    raft_server = raft_new();
    if (raft_server == NULL) {
        error("create raft server failed, exit!");
        goto __main_exit;
    }

    // set callback function
    raft_cbs_t raft_funcs = {
        .send_requestvote = raft_send_requestvote_cb,
        .send_appendentries = raft_send_appendentries_cb,
        .log = raft_log_cb
    };

    raft_set_callbacks(raft_server, &raft_funcs, se);

    // add myself into cluster
    raft_add_node(raft_server, NULL, args->self.id, 1);

    // set election && request timeout
    raft_set_request_timeout(raft_server, REQUEST_TIMEOUT);
    raft_set_election_timeout(raft_server, ELECTION_TIMEOUT);

    // add other nodes
    for (int i = 0; i < array_size(args->nodes); i++) {
        Address* ip;
        array_get_at(args->nodes, i, (void**)&ip);
        Transport* t = transport_client_init(ip->id, tran->fd, ip->ip, ip->port);
        raft_add_node(raft_server, t, ip->id, 0);  
    }

    // event dispatch 
    ret = transport_server_run(tran);
    if (ret != 0) {
        error("transport server bootstrap failed");
        return -1;
    }

    // exit
__main_exit:
    serializer_free(se);
    transport_free(tran);
    zlog_fini();
    args_free(args);
    return 0;
}



