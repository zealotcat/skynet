#include "transport.h"

Transport* transport_server_init(int id, const char* ip, int port) {
    Transport* tran = malloc(sizeof(Transport));
    
    bzero(tran, sizeof(Transport));
    
    tran->id = id;
    tran->role = ROLE_SERVER;
    tran->fd = socket(AF_INET, SOCK_DGRAM, 0);
    tran->sin.sin_family = AF_INET;
    tran->sin.sin_addr.s_addr = inet_addr(ip);
    tran->sin.sin_port = htons(port);
    strncpy(tran->ip, ip, IPLEN);
    tran->port = port;

    int flag = 1;
    if (setsockopt(tran->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        perror("setsockopt()");
        free(tran);
        return NULL;
    }

    if (bind(tran->fd, (struct sockaddr *) &(tran->sin), sizeof(tran->sin))) {
        perror("bind()");
        free(tran);
        return NULL;
    }
    return tran;
}

Transport* transport_client_init(int client_id, evutil_socket_t server_fd, const char* client_ip, int client_port) {
    Transport* tran = malloc(sizeof(Transport));
    
    bzero(tran, sizeof(Transport));
    
    tran->id = client_id;
    tran->role = ROLE_CLIENT;
    tran->fd = server_fd;
    tran->sin.sin_family = AF_INET;
    tran->sin.sin_addr.s_addr = inet_addr(client_ip);
    tran->sin.sin_port = htons(client_port);
    strncpy(tran->ip, client_ip, IPLEN);
    tran->port = client_port;

    return tran;
}

void transport_set_read_cb(Transport* tran, event_callback_fn cb) {
    tran->read_cb = cb;
}

void transport_set_timer_cb(Transport* tran, event_callback_fn cb) {
    tran->timer_cb = cb;
}

void transport_free(Transport* tran) {
    evtimer_del(&tran->ev_timer);
    if (tran->ev_server) event_free(tran->ev_server);
    if (tran->ev_base) event_base_free(tran->ev_base);
    if (tran) free(tran);
}

int transport_server_run(Transport* tran) {
    if (tran->role == ROLE_CLIENT) {
        error("this is a client transport");
        return -1;
    }

    if (tran->read_cb == NULL || tran->timer_cb == NULL) {
        error("read callback function and timer callback function must not be null");
        return -1;
    }

    // add socket event
    tran->ev_base = event_base_new();
    if (tran->ev_base == NULL) {
        error("create event_base failed");
        return -1;
    }

    tran->ev_server = event_new(tran->ev_base, tran->fd, EV_READ|EV_PERSIST, tran->read_cb, tran->ev_base);
    if (tran->ev_server == NULL) {
        error("create socket server event failed");
        return -1;
    }

    if (event_add(tran->ev_server, NULL) != 0) {
        error("add socket server event failed");
        return -1;
    }

    // add timer event
    struct timeval tv = {HEARTBEAT_TIMEOUT/1000, 0};
    evtimer_assign(&tran->ev_timer, tran->ev_base, tran->timer_cb, &tran->ev_timer);
    if (evtimer_add(&tran->ev_timer, &tv) != 0) {
        error("add timer event failed");
        return -1;
    }

    // event loop run
    if (event_base_dispatch(tran->ev_base) != 0) {
        error("event loop dispacth failed");
        return -1;
    }
    return 0;
}

int transport_send(Transport* tran, const char* buffer, int length) {
    int ret = sendto(tran->fd, buffer, length, 0, (const struct sockaddr *)(&(tran->sin)), sizeof(tran->sin));
    return ret;
}

