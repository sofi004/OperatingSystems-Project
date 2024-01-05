#ifndef CLIENT_STRUCT_H
#define CLIENT_STRUCT_H
#include "constants.h"
#include <pthread.h>

struct Path_list {
  char resp_pipe_path[BUFFER_SIZE];
  char req_pipe_path[BUFFER_SIZE];
  int session_id;
};

struct Client_struct {
  int counter;
  struct Path_list path_list[MAX_SESSION_COUNT];
  pthread_mutex_t head_lock;
  pthread_mutex_t tail_lock;
  pthread_cond_t signal_condition;
  pthread_cond_t add_condition;
};


#endif