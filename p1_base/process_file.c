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

void *process(void *arg) {
  struct Thread_struct *thread_struct = (struct Thread_struct *)arg;
  bool lock_thread;
  size_t temp_current_line = 0;
  pthread_mutex_t *show_lock = thread_struct->shared_lock;
  // Save the structure values.
  int max_threads = thread_struct->max_threads;
  int barrier_line = thread_struct->barrier_line;
  int thread_index = thread_struct->index;
  char fd_name[MAX_RESERVATION_SIZE];
  strcpy(fd_name, thread_struct->fd_name);
  // Handle invalid file descriptor.
  int fd = open(fd_name, O_RDONLY);
  if(fd == -1){
    fprintf(stderr, "invalid file descriptor\n");
  }

  // Loop to process commands from the file descriptor.
  while (1) {
    unsigned int event_id, delay, thread_id;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    fflush(stdout);
    // Check if the current thread should execute the command.
    if (thread_index == (int)temp_current_line % max_threads) {
      lock_thread = true;
    } else{
      lock_thread = false;
    }
    
    temp_current_line++;
    // Skip lines up to the barrier_line.
    if((int)temp_current_line <= barrier_line) {
      char ch;
         while (read(fd, &ch, 1) == 1 && ch != '\n');
      continue;
    }    
    switch (get_next(fd)) {
    
    // Process the command.
    case CMD_CREATE:
      // Parse the CREATE command.
      if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }
      if (lock_thread &&(ems_create(event_id, num_rows, num_columns))) {
        fprintf(stderr, "Failed to create event\n");
      }
      break;

    case CMD_RESERVE:
      // Parse the RESERVE command.
      num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);
      if (num_coords == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }
      if (lock_thread &&(ems_reserve(event_id, num_coords, xs, ys))) {
        fprintf(stderr, "Failed to reserve seats\n");
      }
      break;

    case CMD_SHOW:
      // Parse the SHOW command.
      if(parse_show(fd, &event_id) != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (lock_thread) {
        pthread_mutex_lock(show_lock);
        if(ems_show(event_id, thread_struct->fd_out)){
          fprintf(stderr, "Failed to show event\n");
        }
        pthread_mutex_unlock(show_lock);
      }
      break;

    case CMD_LIST_EVENTS:
      if (lock_thread){
        pthread_mutex_lock(show_lock);
        if(ems_list_events(thread_struct->fd_out)){
        fprintf(stderr, "Failed to list events\n");
        }
        pthread_mutex_unlock(show_lock);
      }
      break;

    case CMD_WAIT:
      // Parse the WAIT command.
      int return_parse = parse_wait(fd, &delay, &thread_id);
      if (return_parse == -1) { 
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      } else if((delay > 0) && (return_parse == 0)) {
        ems_wait(delay);
      } else if((delay > 0) && (thread_index == (int)thread_id)) {
        ems_wait(delay);
      } else if(lock_thread && ((int)thread_id > max_threads || (int)thread_id < 0)) {
        fprintf(stderr, "Invalid thread index\n");
      }
      break;

    case CMD_INVALID:
      // Handle an invalid command.
      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      if(lock_thread){
        pthread_mutex_lock(show_lock);
        // Write the help message to the .out file.
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
                   "  HELP\n") - 1);
        pthread_mutex_unlock(show_lock);
      }
      break;

    case CMD_BARRIER:
      // Close the file descriptor.
      close(fd);
      // Exit the thread with the current line number.
      pthread_exit((void *)temp_current_line);
      break;

    case CMD_EMPTY:
      // Handle an empty command.
      break;

    case EOC:
      // Close the file descriptor.
      close(fd);
      // Reset the line number.
      temp_current_line = 0;
      // Exit the thread.
      pthread_exit((void *)temp_current_line);
      break;
    }
  }
  close(fd);
}
