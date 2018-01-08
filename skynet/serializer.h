#ifndef BH_SERIALIZER_H__
#define BH_SERIALIZER_H__

#include <strings.h>
#include <jansson/jansson.h>
#include <raft/raft.h>

typedef struct Serializer
{
    json_t* root;
} Serializer;

Serializer* serializer_init();
void serializer_free(Serializer*);

// helper
char* serializer_to_string(Serializer*);

// serialize/deserialize raft rpc message
int deserialize_message(Serializer*, const char*, int);

int serialize_requestvote(Serializer*, int, const msg_requestvote_t*);
int deserialize_requestvote(Serializer*, msg_requestvote_t*);
int serialize_requestvote_response(Serializer*, int, const msg_requestvote_response_t*);
int deserialize_requestvote_response(Serializer*, msg_requestvote_response_t*);
int serialize_appendentries(Serializer*, int, const msg_appendentries_t*);
int deserialize_appendentries(Serializer*, msg_appendentries_t*);
int serialize_appendentries_response(Serializer*, int, const msg_appendentries_response_t*);
int deserialize_appendentries_response(Serializer*, msg_appendentries_response_t*);

#endif