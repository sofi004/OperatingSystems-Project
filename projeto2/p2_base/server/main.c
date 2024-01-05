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

volatile sig_atomic_t flag = 0;
int session_counter = 0;

static void sig_handler(int sig) {
  /*
  static int count = 0;

  // UNSAFE: This handler uses non-async-signal-safe functions (printf(),
  // exit();)
  if (sig == SIGUSR1) {
    // In some systems, after the handler call the signal gets reverted
    // to SIG_DFL (the default action associated with the signal).
    // So we set the signal handler back to our function after each trap.
    //
    if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
      exit(EXIT_FAILURE);
    }
    //aqui temos que fazer o chamamento do ems_show para cada um dos eventos.
    count++;
    fprintf(stderr, "Caught SIGUSR1(%d)\n", count);
    return; // Resume execution at point of interruption
  }
  */
  flag = 1;
}

int main(int argc, char* argv[]) {
  /*
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    exit(EXIT_FAILURE);
  }
  */
 struct sigaction S1;
 S1.sa_sigaction = sig_handler;
 sigaction(SIGUSR1, &S1, NULL);

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  //TODO: Intialize server, create worker threads
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
  pthread_mutex_t head_lock;
  pthread_mutex_t tail_lock;
  pthread_cond_t signal_condition;
  pthread_cond_t add_condition;
  struct Client_struct client_list;
  //substituir counter por tail e head para resolver problema de buffer cheio
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
  printf("na main: %p\n", &client_list.signal_condition);
  printf("na main: %p\n", &client_list.head_lock);
  pthread_t th[MAX_SESSION_COUNT];
  for( int i = 0; i < MAX_SESSION_COUNT; i++){
    memset(client_list.path_list[i].req_pipe_path, '\0', sizeof(client_list.path_list[i].req_pipe_path));
    memset(client_list.path_list[i].resp_pipe_path, '\0', sizeof(client_list.path_list[i].resp_pipe_path));
    if(pthread_create(&th[i], NULL, &client_thread, (void *)&client_list) != 0){
        fprintf(stderr, "Falha ao criar a thread\n");
        exit(EXIT_FAILURE);
    }
  }
  int list_index = 0;

  while(1) {
    if(flag == 1){
      printf("ola\n");
      sig_show();
      flag = 0;
    }
    if(client_list.path_list[list_index].req_pipe_path[0] != '\0'){
      continue;
    }
    int op_code = 0;
    ssize_t ret = read(geral, &op_code, sizeof(op_code));
    if (ret == 0) {
        // ret == 0 indicates EOF
        //fprintf(stderr, "[INFO]: pipe closed\n");
    } else if (ret == -1) {
        // ret == -1 indicates error
        if(errno != EINTR){
          fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
        }
    }


    switch (op_code) {
        case 0:
          break;
        case SETUP_CLIENT:
          //tratamento do request pipe
          int name_len = 0;
          pthread_mutex_lock(&client_list.tail_lock);
          ret = read(geral, &name_len, sizeof(name_len));
          if (ret == 0) {
            // ret == 0 indicates EOF
            fprintf(stderr, "[INFO]: pipe closed\n");
          } else if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
          }
          ret = read(geral, &client_list.path_list[tail].req_pipe_path, name_len);
          if (ret == 0) {
              // ret == 0 indicates EOF
              fprintf(stderr, "[INFO]: pipe closed\n");
          } else if (ret == -1) {
              // ret == -1 indicates error
              fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
              exit(EXIT_FAILURE);
          }
          //tratamento do response pipe
          ret = read(geral, &name_len, sizeof(name_len));
          if (ret == 0) {
            // ret == 0 indicates EOF
            fprintf(stderr, "[INFO]: pipe closed\n");
          } else if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
          }
          ret = read(geral, &client_list.path_list[tail].resp_pipe_path, name_len);
          if (ret == 0) {
              // ret == 0 indicates EOF
              fprintf(stderr, "[INFO]: pipe closed\n");
          } else if (ret == -1) {
              // ret == -1 indicates error
              fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
              exit(EXIT_FAILURE);
          }
          client_list.path_list[tail].session_id = session_counter;
          session_counter++;
          tail++;
          if(tail >= MAX_SESSION_COUNT){
            tail = 0;
          }
          printf("meti o signal\n");
          pthread_cond_signal(&client_list.signal_condition);
          printf("passei o signal\n");
          if (((client_list.counter == 0) && tail == (MAX_SESSION_COUNT - 1)) || (client_list.counter - tail == 1)){
            printf("head: %d  tail: %d\n", client_list.counter, tail);
            if(pthread_cond_wait(&client_list.add_condition, &client_list.tail_lock)){
                printf("deu asneira no wait\n");
            }
          }

          pthread_mutex_unlock(&client_list.tail_lock);

            break;
        default:
            fprintf(stderr, "Unknown op_code: %d\n", op_code);
            break;  
    }
  }

  //TODO: Close Server

  ems_terminate();
}