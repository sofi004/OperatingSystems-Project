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
    
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

  struct Client_struct *client_struct = (struct Client_struct *)arg;
  while (1){
    if(pthread_mutex_lock(&client_struct->head_lock)){
        printf("deu asneira\n");
    }
    printf("estou preso\n");
    printf("na thread: %p\n", &client_struct->signal_condition);
    printf("na thread: %p\n", &client_struct->head_lock);

    printf("%s\n", client_struct->path_list[client_struct->counter].req_pipe_path);
    if(pthread_cond_wait(&client_struct->signal_condition, &client_struct->head_lock)){
        printf("deu asneira no wait\n");
    }

    printf("passei o wait\n");

    if(client_struct->path_list[client_struct->counter].req_pipe_path[0] != '\0'){
      printf("estou a chegar aqui\n");
      int request = open(client_struct->path_list[client_struct->counter].req_pipe_path,O_RDONLY);
      printf("passei o open dos pedidos\n");
      memset(client_struct->path_list[client_struct->counter].req_pipe_path, '\0', sizeof(client_struct->path_list[client_struct->counter].req_pipe_path));

      int response = open(client_struct->path_list[client_struct->counter].resp_pipe_path, O_WRONLY);
      memset(client_struct->path_list[client_struct->counter].resp_pipe_path, '\0', sizeof(client_struct->path_list[client_struct->counter].resp_pipe_path));
      printf("passei o segundo open, das respostas\n");
      client_struct->counter++;
      if(client_struct->counter >= MAX_SESSION_COUNT){
        client_struct->counter = 0;
      }
      printf("session_id: %d", client_struct->path_list[client_struct->counter].session_id);
      ssize_t ret = write(response, &client_struct->path_list[client_struct->counter].session_id, sizeof(int));
      pthread_mutex_unlock(&client_struct->head_lock);

      bool finish_file = false;
      while (1) {
        if(finish_file){
          break;
        }
        int event_id;
        int op_code = 0;
        // TODO: Read from pipe
        printf("antes do op_code: %d, request: %d\n", op_code, request);
        ssize_t ret = read(request, &op_code, sizeof(op_code));
        if(ret == -1){
            printf("error reading\n");
        }else if(ret == 0){
            printf("doesn't have anything to read\n");
        }
        printf("depois do op_code: %d\n", op_code);
        if (ret == 0) {
            // ret == 0 indicates EOF
            fprintf(stderr, "[INFO]: pipe closed\n");
        } else if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        switch (op_code) {
            case QUIT_CLIENT:
                close(response);
                close(request);
                finish_file = true;
                break;
            case CREATE_EVENT:
                size_t num_rows;
                size_t num_cols;
                ssize_t ret3 = read(request, &event_id, sizeof(event_id));
                printf("event_id: %d %ld\n", event_id, ret3);
                if (ret3 == 0) {
                    // ret == 0 indicates EOF
                    fprintf(stderr, "[INFO]: pipe closed\n");
                } else if (ret3 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret3 = read(request, &num_rows, sizeof(num_rows));
                if (ret3 == 0) {
                    // ret == 0 indicates EOF
                    fprintf(stderr, "[INFO]: pipe closed\n");
                } else if (ret3 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret3 = read(request, &num_cols, sizeof(num_cols));
                if (ret3 == 0) {
                    // ret == 0 indicates EOF
                    fprintf(stderr, "[INFO]: pipe closed\n");
                } else if (ret3 == -1) {
                    // ret == -1 indicates error
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if (ems_create(event_id, num_rows, num_cols, response)) 
                    fprintf(stderr, "Failed to create event\n");
                
                break;
            case RESERVE_SEATS:
                size_t num_seats;
                size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
                ssize_t ret4 = read(request, &event_id, sizeof(event_id));
                if (ret4 < 0) {
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret4 = read(request, &num_seats, sizeof(num_seats));
                if (ret4 < 0) {
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                ret4 = read(request, &xs, 256);
                if (ret4 < 0) {
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                ret4 = read(request, &ys, 256);
                if (ret4 < 0) {
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if (ems_reserve(event_id, num_seats, xs, ys, response)) 
                    fprintf(stderr, "Failed to reserve seats\n");
                break;
            case SHOW_EVENT:
                ssize_t ret5 = read(request, &event_id, sizeof(event_id));
                if (ret5 < 0) {
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                if (ems_show(response, event_id)) 
                    fprintf(stderr, "Failed to show event seats\n");
                break;
            case LIST_EVENTS:
                if (ems_list_events(response)) 
                    fprintf(stderr, "Failed to list events\n");
              break;
            case 0:
                break;
            default:
                fprintf(stderr, "Unknown op_code: %d\n", op_code);
                break;
          }
    
        //TODO: Write new client to the producer-consumer buffer
      }
        if(pthread_cond_signal(&client_struct->add_condition)){
            printf("deu asneira no wait\n");
        }
    }else{
      pthread_mutex_unlock(&client_struct->head_lock);
    }

  }
}