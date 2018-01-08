# Skynet Raft实现
-----
## Raft对象
Skyent raft包含了raft server、raft node、raft log等关键对象。

### Raft Server
Raft server对象定义如下：
```c
typedef void* raft_server_t;

typedef struct {
    /* Persistent state: */
    /* the server's best guess of what the current term is
     * starts at zero */
    int current_term;
    /* The candidate the server voted for in its current term,
     * or Nil if it hasn't voted for any.  */
    int voted_for;
    /* the log which is replicated */
    void* log;
    
    /* Volatile state: */
    /* idx of highest log entry known to be committed */
    int commit_idx;
    /* idx of highest log entry applied to state machine */
    int last_applied_idx;
    /* follower/leader/candidate indicator */
    int state;
    /* amount of time left till timeout */
    int timeout_elapsed;

    raft_node_t* nodes;
    int num_nodes;

    int election_timeout;
    int request_timeout;

    /* what this node thinks is the node ID of the current leader, or -1 if
     * there isn't a known current leader. */
    raft_node_t* current_leader;

    /* callbacks */
    raft_cbs_t cb;
    void* udata;

    /* my node ID */
    raft_node_t* node;

    /* the log which has a voting cfg change, otherwise -1 */
    int voting_cfg_change_log_idx;

    /* our membership with the cluster is confirmed (ie. configuration log was
     * committed) */
    int connected;
} raft_server_private_t;
```

其中，
+ raft_node_t：一个raft node实例
+ raft_cbs_t：包含了raft协议相关的几个回调函数

### Raft Node
Raft node表示一个raft节点实例，包含了节点的内部状态，定义如下：
```c
typedef void* raft_node_t;

typedef struct
{
    void* udata;
    int   next_idx;
    int   match_idx;
    int   flags;
    int   id;
} raft_node_private_t;
```

### Raft Callback Functions
raft_cbs_t结构体包含了raft协议相关的回调函数，定义如下：
```c
typedef struct
{
    /** Callback for sending request vote messages */
    func_send_requestvote_f send_requestvote;
    /** Callback for sending appendentries messages */
    func_send_appendentries_f send_appendentries;
    /** Callback for finite state machine application
     * Return 0 on success.
     * Return RAFT_ERR_SHUTDOWN if you want the server to shutdown. */
    func_logentry_event_f applylog;
    /** Callback for persisting vote data
     * For safety reasons this callback MUST flush the change to disk. */
    func_persist_int_f persist_vote;
    /** Callback for persisting term data
     * For safety reasons this callback MUST flush the change to disk. */
    func_persist_int_f persist_term;
    /** Callback for adding an entry to the log
     * For safety reasons this callback MUST flush the change to disk.
     * Return 0 on success.
     * Return RAFT_ERR_SHUTDOWN if you want the server to shutdown. */
    func_logentry_event_f log_offer;
    /** Callback for removing the oldest entry from the log
     * For safety reasons this callback MUST flush the change to disk.
     * @note If memory was malloc'd in log_offer then this should be the right
     *  time to free the memory. */
    func_logentry_event_f log_poll;
    /** Callback for removing the youngest entry from the log
     * For safety reasons this callback MUST flush the change to disk.
     * @note If memory was malloc'd in log_offer then this should be the right
     *  time to free the memory. */
    func_logentry_event_f log_pop;
    /** Callback for determining which node this configuration log entry
     * affects. This call only applies to configuration change log entries.
     * @return the node ID of the node */
    func_logentry_event_f log_get_node_id;
    /** Callback for detecting when a non-voting node has sufficient logs. */
    func_node_has_sufficient_logs_f node_has_sufficient_logs;
    /** Callback for catching debugging log messages
     * This callback is optional */
    func_log_f log;
} raft_cbs_t;
```

### Raft log
[TODO] Raft log表示raft各个节点间传输的日志条目。




## RPC消息
RPC消息表示raft集群进行选举、成员变更、日志追加等操作时，节点间发送的消息。

### 选主消息
选主相关的RPC消息包括：
+ msg_requestvote_t
+ msg_requestvote_response_t

#### msg_requestvote_t
Vote request message:
+ Sent to nodes when a server wants to become leader
+ This message could force a leader/candidate to become a follower

实现如下：
```c
typedef struct
{
    /** currentTerm, to force other leader/candidate to step down */
    int term;
    /** candidate requesting vote */
    int candidate_id;
    /** index of candidate's last log entry */
    int last_log_idx;
    /** term of candidate's last log entry */
    int last_log_term;
} msg_requestvote_t;
```

#### msg_requestvote_response_t
Vote request response message:
+ Indicates if node has accepted the server's vote request

实现如下：
```c
typedef struct
{
    /** currentTerm, for candidate to update itself */
    int term;
    /** true means candidate received vote */
    int vote_granted;
} msg_requestvote_response_t;
```




## 初始化
初始化包括了：
1. 创建raft server节点
2. 添加其它节点到raft server中

### 创建raft server
raft_new()创建一个raft server节点，实现如下：
```c
raft_server_t* raft_new() {
    raft_server_private_t* me = calloc(1, sizeof(raft_server_private_t));
    me->current_term = 0;
    me->voted_for = -1;
    me->timeout_elapsed = 0;
    me->request_timeout = 200;
    me->election_timeout = 1000;
    me->log = log_new();
    me->voting_cfg_change_log_idx = -1;
    raft_set_state((raft_server_t*)me, RAFT_STATE_FOLLOWER);
    me->current_leader = NULL;
    return (raft_server_t*)me;
}
```

### 添加raft node
raft_add_node()将raft node添加到raft集群中，包括自身节点。raft_add_node()实现如下：
```c
raft_node_t* raft_add_node(raft_server_t* me_, void* udata, int id, int is_self)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;

    /* set to voting if node already exists */
    raft_node_t* node = raft_get_node(me_, id);
    if (node)
    {
        if (!raft_node_is_voting(node))
        {
            raft_node_set_voting(node, 1);
            return node;
        }
        else
            /* we shouldn't add a node twice */
            return NULL;
    }

    me->num_nodes++;
    me->nodes = realloc(me->nodes, sizeof(void*) * me->num_nodes);
    me->nodes[me->num_nodes - 1] = raft_node_new(udata, id);
    if (is_self)
        me->node = me->nodes[me->num_nodes - 1];
    return me->nodes[me->num_nodes - 1];
}
```

### raft_periodic()
完成初始化之后，raft周期性调用raft_periodic()开始选主、日志复制等等操作。

raft_periodic()实现如下：
```c
int raft_periodic(raft_server_t* me_, int msec_since_last_period)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;

    me->timeout_elapsed += msec_since_last_period;

    /* Only one voting node means it's safe for us to become the leader */
    if (1 == raft_get_num_voting_nodes(me_) &&
        raft_node_is_voting(raft_get_my_node((void*)me)) &&
        !raft_is_leader(me_))
        raft_become_leader(me_);

    if (me->state == RAFT_STATE_LEADER) {
        if (me->request_timeout <= me->timeout_elapsed)
            raft_send_appendentries_all(me_);
    } else if (me->election_timeout <= me->timeout_elapsed) {
        if (1 < raft_get_num_voting_nodes(me_) &&
            raft_node_is_voting(raft_get_my_node(me_)))
            raft_election_start(me_);
    }

    if (me->last_applied_idx < me->commit_idx)
        return raft_apply_entry(me_);
    return 0;
}
```

raft_periodic()中：
1. 当raft集群中只有一个node(即本节点)时，将本节点的角色转换为Leader
2. 当本节点的角色是Leader时，如果超过了`request_timeout`，则发送日志复制消息给其它节点
3. 如果本节点的角色不是Leader，并且超过了`election_timeout`，则开始选举，即调用raft_election_start()，要求集群中的其它节点选举自己为Leader
4. 当`last_applied_idx`小于`commit_idx`，调用raft_apply_entry()



## 选主
Raft server首先会进行选主的初始化操作，即server会增加其term，把状态改成candidate，然后选举自己为主，并把选主的RPC并行地发送给集群中其他的server，根据返回的RPC的情况的不同，做不同的处理：
1. 该server被选为leader
2. 其他的server选为leader
3. 一段时间后，没有server被选为leader

针对情况一，该server被选为leader，当前仅当在大多数的server投票给该server时。当其被选为主时，会马上发送心跳消息给其他的server，来表明其已经是leader，防止发生新的选举。

针对情况二，其他的server被选为leader，它会收到leader发送的心跳信息，此时，该server应该转为follower，然后退出选举。

针对情况三，一段时间后，没有server被选为leader，这种情况发生在没有server获得了大多数的server的投票情况下，此时，应该随机选择一个超时时间发起新一轮的选举。

在讨论raft_periodic()时，我们知道当本节点角色不是Leader，并且超过了`election_timeout`，则开始选举，即调用raft_election_start()，要求集群中的其它节点选举自己为Leader。

### raft_election_start()
实现如下：
```c
void raft_election_start(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    raft_become_candidate(me_);
}
```

开始选举前，节点的状态必须是Candidate。

### raft_become_candidate()
实现如下：
```c
void raft_become_candidate(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int i;

    raft_set_current_term(me_, raft_get_current_term(me_) + 1);
    for (i = 0; i < me->num_nodes; i++)
        raft_node_vote_for_me(me->nodes[i], 0);
    raft_vote(me_, me->node);
    me->current_leader = NULL;
    raft_set_state(me_, RAFT_STATE_CANDIDATE);

    /* We need a random factor here to prevent simultaneous candidates.
     * If the randomness is always positive it's possible that a fast node
     * would deadlock the cluster by always gaining a headstart. To prevent
     * this, we allow a negative randomness as a potential handicap. */
    me->timeout_elapsed = me->election_timeout - 2 * (rand() % me->election_timeout);

    for (i = 0; i < me->num_nodes; i++)
        if (me->node != me->nodes[i] && raft_node_is_voting(me->nodes[i]))
            raft_send_requestvote(me_, me->nodes[i]);
}

void raft_node_vote_for_me(raft_node_t* me_, const int vote)
{
    raft_node_private_t* me = (raft_node_private_t*)me_;
    if (vote)
        me->flags |= RAFT_NODE_VOTED_FOR_ME;
    else
        me->flags &= ~RAFT_NODE_VOTED_FOR_ME;
}
```

本节点发送选主消息给集群中其它的节点，即依次对raft node调用raft_send_requestvote()。

### raft_send_requestvote()
实现如下：
```c
int raft_send_requestvote(raft_server_t* me_, raft_node_t* node)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    msg_requestvote_t rv;

    rv.term = me->current_term;
    rv.last_log_idx = raft_get_current_idx(me_);
    rv.last_log_term = raft_get_last_log_term(me_);
    rv.candidate_id = raft_get_nodeid(me_);
    me->cb.send_requestvote(me_, me->udata, node, &rv);
    return 0;
}
```

raft_send_requestvote()调用raft_cbs_t中的send_requestvote回调函数发送选主消息。

Raft集群的其它节点接收到msg_requestvote_t消息后，调用raft_recv_requestvote()处理选主消息。

### raft_recv_requestvote()
Receive a requestvote message. 

实现如下：
```c
int raft_recv_requestvote(raft_server_t* me_,
                          raft_node_t* node,
                          msg_requestvote_t* vr,
                          msg_requestvote_response_t *r)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;

    if (!node)
        node = raft_get_node(me_, vr->candidate_id);

    if (raft_get_current_term(me_) < vr->term) {
        raft_set_current_term(me_, vr->term);
        raft_become_follower(me_);
    }

    if (__should_grant_vote(me, vr)) {
        raft_vote_for_nodeid(me_, vr->candidate_id);
        r->vote_granted = 1;

        /* there must be in an election. */
        me->current_leader = NULL;
        me->timeout_elapsed = 0;
    } else {
        /* It's possible the candidate node has been removed from the cluster but
         * hasn't received the appendentries that confirms the removal. Therefore
         * the node is partitioned and still thinks its part of the cluster. It
         * will eventually send a requestvote. This is error response tells the
         * node that it might be removed. */
        if (!node) {
            r->vote_granted = RAFT_REQUESTVOTE_ERR_UNKNOWN_NODE;
            goto done;
        }
        else
            r->vote_granted = 0;
    }
done:
    r->term = raft_get_current_term(me_);
    return 0;
}
```

raft_recv_requestvote()关键步骤如下：
1. 如果本节点的term小于发送选主消息的节点的term，则转换本节点的状态为Follower，并更新term
2. 调用\_\_should_grant_vote()，判断是否应该选择发送选主消息的节点为Leader

#### \_\_should_grant_vote()
判断是否应该选择消息发送节点为主节点，实现如下：
```c
static int __should_grant_vote(raft_server_private_t* me, msg_requestvote_t* vr)
{
    /* TODO: 4.2.3 Raft Dissertation:
     * if a server receives a RequestVote request within the minimum election
     * timeout of hearing from a current leader, it does not update its term or
     * grant its vote */
    if (!raft_node_is_voting(raft_get_my_node((void*)me)))
        return 0;
    if (vr->term < raft_get_current_term((void*)me))
        return 0;
    /* TODO: if voted for is candiate return 1 (if below checks pass) */
    if (raft_already_voted((void*)me))
        return 0;

    /* Below we check if log is more up-to-date... */
    int current_idx = raft_get_current_idx((void*)me);

    /* Our log is definitely not more up-to-date if it's empty! */
    if (0 == current_idx)
        return 1;

    raft_entry_t* e = raft_get_entry_from_idx((void*)me, current_idx);
    if (e->term < vr->last_log_term)
        return 1;

    if (vr->last_log_term == e->term && current_idx <= vr->last_log_idx)
        return 1;

    return 0;
}
```

### raft_recv_requestvote_response()
接收选主消息的响应，实现如下：
```c
int raft_recv_requestvote_response(raft_server_t* me_,
                                   raft_node_t* node,
                                   msg_requestvote_response_t* r)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    if (!raft_is_candidate(me_)) {
        return 0;
    } else if (raft_get_current_term(me_) < r->term) {
        raft_set_current_term(me_, r->term);
        raft_become_follower(me_);
        return 0;
    } else if (raft_get_current_term(me_) != r->term) {
        /* The node who voted for us would have obtained our term.
         * Therefore this is an old message we should ignore.
         * This happens if the network is pretty choppy. */
        return 0;
    }

    switch (r->vote_granted)
    {
        case RAFT_REQUESTVOTE_ERR_GRANTED:
            if (node)
                raft_node_vote_for_me(node, 1);
            int votes = raft_get_nvotes_for_me(me_);
            if (raft_votes_is_majority(raft_get_num_voting_nodes(me_), votes))
                raft_become_leader(me_);
            break;
        case RAFT_REQUESTVOTE_ERR_NOT_GRANTED:
            break;
        case RAFT_REQUESTVOTE_ERR_UNKNOWN_NODE:
            if (raft_node_is_voting(raft_get_my_node(me_)) &&
                me->connected == RAFT_NODE_STATUS_DISCONNECTING)
                return RAFT_ERR_SHUTDOWN;
            break;
        default:
            assert(0);
    }
    return 0;
}
```

其中：
1. 当本节点的term小于发送选主结果消息的节点时，本节点转换状态为Follower
2. 当本节点接收到超过raft集群中节点半数以上的`同意本节点为Leader`的选主响应消息时，本节点转换状态为Leader

### raft_become_follower()
代码如下：
```c
void raft_become_follower(raft_server_t* me_)
{
    raft_set_state(me_, RAFT_STATE_FOLLOWER);
}
```

### raft_become_leader()
raft_become_leader()将节点的状态转换为Leader，代码如下：
```c
void raft_become_leader(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int i;

    raft_set_state(me_, RAFT_STATE_LEADER);
    for (i = 0; i < me->num_nodes; i++)
    {
        if (me->node == me->nodes[i])
            continue;

        raft_node_t* node = me->nodes[i];
        raft_node_set_next_idx(node, raft_get_current_idx(me_) + 1);
        raft_node_set_match_idx(node, 0);
        raft_send_appendentries(me_, node);
    }
}
```

当本节点被选举为Leader后，开始发送日志复制消息。

### raft_send_appendentries()
发送日志复制消息，实现如下：
```c
int raft_send_appendentries(raft_server_t* me_, raft_node_t* node)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;

    msg_appendentries_t ae = {};
    ae.term = me->current_term;
    ae.leader_commit = raft_get_commit_idx(me_);
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;

    int next_idx = raft_node_get_next_idx(node);

    ae.entries = raft_get_entries_from_idx(me_, next_idx, &ae.n_entries);

    /* previous log is the log just before the new logs */
    if (1 < next_idx)
    {
        raft_entry_t* prev_ety = raft_get_entry_from_idx(me_, next_idx - 1);
        ae.prev_log_idx = next_idx - 1;
        if (prev_ety)
            ae.prev_log_term = prev_ety->term;
    }

    me->cb.send_appendentries(me_, me->udata, node, &ae);
    return 0;
}
```








