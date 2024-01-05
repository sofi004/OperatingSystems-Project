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
#include "common/client_struct.h"
#include "client_thread.h"

void *client_thread(void *arg) {
    // Signal blocking setup for SIGUSR1
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
  // Extract the client structure from the argument
  struct Client_struct *client_struct = (struct Client_struct *)arg;
  // Infinite loop for handling client requests
  while (1){
    // Lock the head mutex and wait for a signal
    pthread_mutex_lock(&client_struct->head_lock);
    if(pthread_cond_wait(&client_struct->signal_condition, &client_struct->head_lock)){
        printf("Failed to wait in a thread\n");
    }
    // Check if the client path is available
    if(client_struct->path_list[client_struct->counter].req_pipe_path[0] != '\0'){
      // Open request and response pipes
      int request = open(client_struct->path_list[client_struct->counter].req_pipe_path,O_RDONLY);
      memset(client_struct->path_list[client_struct->counter].req_pipe_path, '\0', sizeof(client_struct->path_list[client_struct->counter].req_pipe_path));
      int response = open(client_struct->path_list[client_struct->counter].resp_pipe_path, O_WRONLY);
      memset(client_struct->path_list[client_struct->counter].resp_pipe_path, '\0', sizeof(client_struct->path_list[client_struct->counter].resp_pipe_path));
      // Increment the counter and handle wraparound
      client_struct->counter++;
      if(client_struct->counter >= MAX_SESSION_COUNT){
        client_struct->counter = 0;
      }
      // Write the counter to the response pipe
      ssize_t ret = write(response, &client_struct->counter, sizeof(int));
      if (ret == -1) {
        // ret == -1 indicates error
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
        }
      // Unlock the head mutex
      pthread_mutex_unlock(&client_struct->head_lock);
      // Process client requests from the request pipe
      bool finish_file = false;
      while (1) {
        if(finish_file){
          break;
        }
        // Read operation code and session ID from the request pipe
        int event_id;
        char op_code [2];
        ret = read(request, &op_code, sizeof(op_code));
        if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        op_code[ret] = '\0';
        // Handle pipe closure and read errors
        if (ret == 0) {
            fprintf(stderr, "[INFO]: pipe closed\n");
        } else if (ret == -1) {
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        int session_id;
        ret = read(request, &session_id, sizeof(int));
        if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Convert operation code to integer
        int int_op_code = atoi(op_code);
        // Process the client request based on the operation code
        switch (int_op_code) {
            case QUIT_CLIENT:
                // Close pipes and finish processing the file
                close(response);
                close(request);
                finish_file = true;
                break;
            case CREATE_EVENT:
                // Read additional parameters for creating an event
                size_t num_rows;
                size_t num_cols;
                ssize_t ret3 = read(request, &event_id, sizeof(event_id));
                if (ret3 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret3 = read(request, &num_rows, sizeof(num_rows));
                if (ret3 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret3 = read(request, &num_cols, sizeof(num_cols));
                if (ret3 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                // Call the EMS function to create the event
                if (ems_create(event_id, num_rows, num_cols, response)) 
                    fprintf(stderr, "Failed to create event\n");
                
                break;
            case RESERVE_SEATS:
                // Read additional parameters for reserving seats
                size_t num_seats;
                size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
                ssize_t ret4 = read(request, &event_id, sizeof(event_id));
                if (ret4 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret4 = read(request, &num_seats, sizeof(num_seats));
                if (ret4 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                ret4 = read(request, &xs, 256);
                if (ret4 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret4 = read(request, &ys, 256);
                if (ret4 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                // Call the EMS function to reserve seats
                if (ems_reserve(event_id, num_seats, xs, ys, response)) 
                    fprintf(stderr, "Failed to reserve seats\n");
                break;
            case SHOW_EVENT:
                // Read additional parameter for showing event seats
                ssize_t ret5 = read(request, &event_id, sizeof(event_id));
                if (ret5 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                // Call the EMS function to show event seats
                if (ems_show(response, event_id)) 
                    fprintf(stderr, "Failed to show event seats\n");

                break;
            case LIST_EVENTS:
                // Call the EMS function to list events
                if (ems_list_events(response)) 
                    fprintf(stderr, "Failed to list events\n");
              break;
            case 0:
                // No operation (placeholder)
                break;
            default:
                // Handle unknown operation codes
                fprintf(stderr, "Unknown op_code: %d\n", int_op_code);
                break;
          }
    
        
    }
    // Signal the producer thread that a file is processed
    if(pthread_cond_signal(&client_struct->add_condition)){
        printf("Failed to signal\n");
    }
    }else{
      // Unlock the head mutex if the client path is not available
      pthread_mutex_unlock(&client_struct->head_lock);
    }
  }
}