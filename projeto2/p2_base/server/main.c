#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "client_thread.h"
#include "common/client_struct.h"

// Declaration of the global flag variable for signal handling
volatile sig_atomic_t flag = 0;

// Signal handler function for SIGUSR1
static void sig_handler() {
  flag = 1;
}

int main(int argc, char* argv[]) {
  // Signal handling setup
  struct sigaction S1;
  S1.sa_sigaction = sig_handler;
  sigaction(SIGUSR1, &S1, NULL);
  
  // Check for correct command-line arguments
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  // Variables for delay configuration
  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  // Parse delay from command-line arguments if provided
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }
  // Initialize the EMS with the specified delay
  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  /// Open the server FIFO pipe for reading and writing
  if (unlink(argv[1]) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (mkfifo(argv[1], 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
  }
  int geral = open(argv[1], O_RDWR);
    if (geral == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  // Initialize mutexes, conditions, and client list for thread management
  pthread_mutex_t head_lock;
  pthread_mutex_t tail_lock;
  pthread_cond_t signal_condition;
  pthread_cond_t add_condition;
  struct Client_struct client_list;
  // Initialize client list
  client_list.head_lock = head_lock;
  client_list.tail_lock = tail_lock;
  client_list.counter = 0;
  int tail = 0;
  client_list.signal_condition = signal_condition;
  client_list.add_condition = add_condition;
  pthread_cond_init(&client_list.signal_condition, NULL);
  pthread_cond_init(&client_list.add_condition, NULL);
  pthread_mutex_init(&client_list.head_lock, NULL);
  pthread_mutex_init(&client_list.tail_lock, NULL);
  // Create an array of thread identifiers
  pthread_t th[MAX_SESSION_COUNT];
  // Create and start client threads
  for( int i = 0; i < MAX_SESSION_COUNT; i++){
    // Initialize paths for each thread
    memset(client_list.path_list[i].req_pipe_path, '\0', sizeof(client_list.path_list[i].req_pipe_path));
    memset(client_list.path_list[i].resp_pipe_path, '\0', sizeof(client_list.path_list[i].resp_pipe_path));
    // Create client thread
    if(pthread_create(&th[i], NULL, &client_thread, (void *)&client_list) != 0){
        fprintf(stderr, "Falha ao criar a thread\n");
        exit(EXIT_FAILURE);
    }
  }
  // Main loop to handle server operations
  int list_index = 0;

  while(1) {
    // Check for the flag set by the signal handler
    if(flag == 1){
      // Handle signal for showing events
      sig_show();
      flag = 0;
    }
    // Check if the current client path is available
    if(client_list.path_list[list_index].req_pipe_path[0] != '\0'){
      continue;
    }
    // Read the operation code from the server FIFO pipe
    char op_code[2];
    ssize_t ret = read(geral, &op_code, sizeof(op_code));
    if (ret < 0) {
      // Handle read errors (excluding EINTR)
      if(errno != EINTR){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
    op_code[ret] = '\0';
    int int_op_code = atoi(op_code);
    // Process the operation based on the operation code
    switch (int_op_code) {
        case 0:
          // No operation (placeholder)
          break;
        case SETUP_CLIENT:
          // Set up client paths
          pthread_mutex_lock(&client_list.tail_lock);
          ret = read(geral, &client_list.path_list[tail].req_pipe_path, BUFFER_SIZE);
          if (ret == -1) {
              // ret == -1 indicates error
              fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
              exit(EXIT_FAILURE);
          }
          ret = read(geral, &client_list.path_list[tail].resp_pipe_path, BUFFER_SIZE);
          if (ret == -1) {
              // ret == -1 indicates error
              fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
              exit(EXIT_FAILURE);
          }
          // Update tail index and signal the client thread
          tail++;
          if(tail >= MAX_SESSION_COUNT){
            tail = 0;
          }
          pthread_cond_signal(&client_list.signal_condition);
          // Wait if the client list is full
          if (((client_list.counter == 0) && tail == (MAX_SESSION_COUNT - 1)) || (client_list.counter - tail == 1)){
            if(pthread_cond_wait(&client_list.add_condition, &client_list.tail_lock)){
              printf("Failed to wait for a thread to finish\n");
            }
          }
            // Unlock the tail lock
            pthread_mutex_unlock(&client_list.tail_lock);
            break;
        default:
            // Handle unknown operation codes
            fprintf(stderr, "Unknown op_code: %d\n", int_op_code);
            break;  
    }
  }
  // Terminate the EMS before exiting the program
  ems_terminate();
}