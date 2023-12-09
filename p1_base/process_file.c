#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "process_file.h"
#include "structs.h"
#include <stdbool.h>

// fazer uma funçao a parte com o while(1) que possa ser chamada com file descriptor que é dado a partir do while
// anterior dos diretorios, no final de cada while interior é para alterar o fd
// no final de percorrer os ficheiros todos acaba o programa

void *process(void *arg)
{

  struct Thread_struct *thread_struct = (struct Thread_struct *)arg;
  bool lock_thread;
  int temp_current_line = 0;
  int max_threads = thread_struct->max_threads;
  int current_line = *(thread_struct->current_line);
  int thread_index = thread_struct->index;
  char fd_name[256];
  strcpy(fd_name, thread_struct->fd_name);
  int fd = open(fd_name, O_RDONLY);
  if (thread_index == current_line % max_threads)
  {
    lock_thread = true;
  }
  while (1)
  {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    fflush(stdout);
    temp_current_line++;
    if(temp_current_line <= current_line){
        char ch;
         while (read(fd, &ch, 1) == 1 && ch != '\n')
         ;
      continue;
    }
    
    switch (get_next(fd))
    {

    case CMD_CREATE:
      if (lock_thread && (parse_create(fd, &event_id, &num_rows, &num_columns) != 0))
      {
        // fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (lock_thread &&(ems_create(event_id, num_rows, num_columns)))
      {
        // fprintf(stderr, "Failed to create event\n");
      }

      break;

    case CMD_RESERVE:
      if(lock_thread){
        num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);
      }
      if (num_coords == 0)
      {
        // fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (lock_thread &&(ems_reserve(event_id, num_coords, xs, ys)))
      {
        // fprintf(stderr, "Failed to reserve seats\n");
      }

      break;

    case CMD_SHOW:
      if (lock_thread &&(parse_show(fd, &event_id) != 0))
      {
        // fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (lock_thread &&(ems_show(event_id, thread_struct->fd_out)))
      {
        // fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      if (lock_thread &&(ems_list_events(thread_struct->fd_out)))
      {
        // fprintf(stderr, "Failed to list events\n");
      }

      break;

    case CMD_WAIT:
      if (lock_thread &&(parse_wait(fd, &delay, NULL) == -1))
      { // thread_id is not implemented
        // fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (lock_thread &&(delay > 0))
      {
        // printf("Waiting...\n");
        ems_wait(delay);
      }

      break;

    case CMD_INVALID:
      // fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      if(lock_thread){
        write(thread_struct->fd_out,
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"
            "  BARRIER\n"
            "  HELP\n",
            sizeof("Available commands:\n"
                   "  CREATE <event_id> <num_rows> <num_columns>\n"
                   "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                   "  SHOW <event_id>\n"
                   "  LIST\n"
                   "  WAIT <delay_ms> [thread_id]\n"
                   "  BARRIER\n"
                   "  HELP\n") -
                1);
      }

      break;

    case CMD_BARRIER: // Not implemented
      *(thread_struct->current_line) = temp_current_line;
      pthread_exit(NULL);
      break;
    case CMD_EMPTY:
      break;

    case EOC:
      *(thread_struct->current_line) = 0;
       pthread_exit(NULL);
    }
  }
  close(fd);
}