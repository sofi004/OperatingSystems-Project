#ifndef CLIENT_STRUCT_H
#define CLIENT_STRUCT_H

struct Client_struct {
  int *counter;
  char resp_pipe_path;
  char req_pipe_path;
  pthread_mutex_t *shared_lock;
};

#endif