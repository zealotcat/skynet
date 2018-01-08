#include "args.h"

Args* args_new() {
    Args* args = malloc(sizeof(Args));
    bzero(args, sizeof(Args));

    if (array_new(&args->nodes) != CC_OK) {
        free(args);
        return NULL;
    }
    return args;
}

void args_free(Args* args) {
    array_destroy_free(args->nodes);
    free(args);
}

Address* address_new() {
    Address* address = malloc(sizeof(Address));
    bzero(address, sizeof(Address));
    return address;
}

int parse_args(int argc, char* argv[], Args* args) {
    int long_index = 0;
    int opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
    int ret = 0;

    while(opt != -1) {
        if (opt == '?'|| opt == 'h' || strcmp(long_opts[long_index].name, "help") == 0) {
            goto __parse_failed;
        }

        if (strcmp(long_opts[long_index].name, "zlog") == 0) {
            args->zlog = optarg;
        }

        if (strcmp(long_opts[long_index].name, "self") == 0) {
            ret = sscanf(optarg, "%d@%[^:]:%d", &(args->self.id), (char*)(args->self.ip), &(args->self.port));
            if (ret != 3) {
                fprintf(stderr, "self address is invalid: %s, valid syntax is `id@ip:port`\n", optarg);
                goto __parse_failed;
            }
        }

        if (strcmp(long_opts[long_index].name, "node") == 0) {
            Address* address = address_new();
            ret = sscanf(optarg, "%d@%[^:]:%d", &(address->id), (char*)(address->ip), &(address->port));
            if (ret != 3) {
                fprintf(stderr, "node address is invalid: %s, valid syntax is `id@ip:port`\n", optarg);
                goto __parse_failed;
            }
            array_add(args->nodes, address);
        }

        opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
    }

    if (check_args(args) != 0)
        goto __parse_failed;

    return 0;
__parse_failed:
    display_usage(argv[0]);
    return -1;
}

int check_args(const Args* args) {
    if (args == NULL) {
        fprintf(stderr, "call parse_args() first\n");
        return -1;
    }

    if (args->zlog == NULL) {
        fprintf(stderr, "zlog config file must not be empty\n");
        return -1;
    }

    if (args->self.ip[0] == 0  || args->self.port <= 0) {
        fprintf(stderr, "must specify self address\n");
        return -1;
    }

    if (array_size(args->nodes) <= 1) {
        fprintf(stderr, "raft cluster nodes must be greater than 3\n");
        return -1;
    }

    return 0;
}

void display_args(const Args* args) {
    fprintf(stderr, "Argument list:\n");
    fprintf(stderr, "  zlog config  : %s\n", args->zlog);
    fprintf(stderr, "  self address : <%s:%d>\n", args->self.ip, args->self.port);
    
    for (int i = 0; i < array_size(args->nodes); i++) {
        Address* ip;
        array_get_at(args->nodes, i, (void**)&ip);
        fprintf(stderr, "  nodes address : <%s:%d>\n", ip->ip, ip->port);
    }
}

void display_usage(const char* app_name) {
    fprintf(stderr, "[Usage] %s options:\n", app_name);

    int args = (int)(sizeof(long_opts)/sizeof(long_opts[0]));
    for (int i = 0; i < args; i++) {
        if (long_opts[i].name == NULL) { 
            break; 
        }
        
        fprintf(stderr, "  --%-20s: ", long_opts[i].name);

        if (long_opts[i].has_arg == required_argument) {
            fprintf(stderr, "[required argument]");
        } else if (long_opts[i].has_arg == optional_argument) {
            fprintf(stderr, "[optional argument]");
        } else {
            fprintf(stderr, "[no argument]");
        }

        fprintf(stderr, " %s\n", long_opts_desc[i]);
    }
}