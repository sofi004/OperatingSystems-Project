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

int main(int argc, char* argv[]) {
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
  int geral = open(argv[1], O_RDONLY);
    if (geral == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  pthread_mutex_t shared_lock;
  pthread_mutex_init(&shared_lock, NULL);
  struct Client_struct client_list;
  pthread_t th[MAX_SESSION_COUNT];
  for( int i = 0; i < MAX_SESSION_COUNT; i++){
    client_list.counter = 0;
    client_list.shared_lock = shared_lock;
    memset(client_list.path_list[i].req_pipe_path, '\0', sizeof(client_list.path_list[i].req_pipe_path));
    memset(client_list.path_list[i].resp_pipe_path, '\0', sizeof(client_list.path_list[i].resp_pipe_path));
    if(pthread_create(&th[i], NULL, &client_thread, (void *)&client_list) != 0){
        fprintf(stderr, "Falha ao criar a thread\n");
        exit(EXIT_FAILURE);
    }
  }
  int list_index = 0;
  while(1) {
    sleep(1);
    if(client_list.path_list[list_index].req_pipe_path[0] != '\0'){
      continue;
    }
    int op_code = 0;
    ssize_t ret = read(geral, &op_code, sizeof(op_code));
    if (ret == 0) {
        // ret == 0 indicates EOF
        fprintf(stderr, "[INFO]: pipe closed\n");
    } else if (ret == -1) {
        // ret == -1 indicates error
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    switch (op_code) {
        case 0:
          break;
        case 1:
          //tratamento do request pipe
          int name_len = 0;
          ssize_t ret = read(geral, &name_len, sizeof(name_len));
          if (ret == 0) {
            // ret == 0 indicates EOF
            fprintf(stderr, "[INFO]: pipe closed\n");
          } else if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
          }
          ret = read(geral, &client_list.path_list[list_index].req_pipe_path, name_len);
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
          ret = read(geral, &client_list.path_list[list_index].resp_pipe_path, name_len);
          if (ret == 0) {
              // ret == 0 indicates EOF
              fprintf(stderr, "[INFO]: pipe closed\n");
          } else if (ret == -1) {
              // ret == -1 indicates error
              fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
              exit(EXIT_FAILURE);
          }
          list_index++;
          if(list_index > MAX_SESSION_COUNT){
            list_index = 0;
          }
            break;
        default:
            fprintf(stderr, "Unknown op_code: %d\n", op_code);
            break;  
    }
  }

  //TODO: Close Server

  ems_terminate();
}