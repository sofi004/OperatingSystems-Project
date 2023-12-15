#ifndef THREAD_STRUCT_H
#define THREAD_STRUCT_H

#include <pthread.h>

struct Thread_struct {
  char fd_name[MAX_RESERVATION_SIZE];
  int  fd_out;
  int  barrier_line;
  int  index;
  int  max_threads;
  pthread_mutex_t *shared_lock;
};

#endif