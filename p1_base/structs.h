#ifndef THREAD_STRUCT_H
#define THREAD_STRUCT_H


struct Thread_struct {
  char fd_name[256];
  int  fd_out;
  int  barrier_line;
  int  index;
  int  max_threads;
};


#endif