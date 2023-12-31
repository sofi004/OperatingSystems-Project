#ifndef CLIENT_STRUCT_H
#define CLIENT_STRUCT_H
#include "constants.h"
#include <pthread.h>

struct Path_list {
  char resp_pipe_path[40];
  char req_pipe_path[40];
};

struct Client_struct {
  int counter;
  struct Path_list path_list[MAX_SESSION_COUNT];
  pthread_mutex_t shared_lock;
};


#endif