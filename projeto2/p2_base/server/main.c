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

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

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
  
  char buffer1[4];
  memset(buffer1, '\0', sizeof(buffer1));

  //leitura do nome da pipe de pedidos
  ssize_t ret = read(geral, &buffer1, 4);
  if (ret == 0) {
      // ret == 0 indicates EOF
      fprintf(stderr, "[INFO]: pipe closed\n");
  } else if (ret == -1) {
      // ret == -1 indicates error
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  printf("%s\n", buffer1);
  int request = open(buffer1,O_RDONLY);
  printf("passei o open dos pedidos\n");

  char buffer[5];
  memset(buffer, '\0', sizeof(buffer));
  //leitura do nome da pipe de respostas
  ssize_t ret1 = read(geral, &buffer, 5);
  if (ret1 == 0) {
      // ret == 0 indicates EOF
      fprintf(stderr, "[INFO]: pipe closed\n");
  } else if (ret1 == -1) {
      // ret == -1 indicates error
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  printf("%s\n", buffer);
  int response = open(buffer, O_WRONLY);
  printf("passei o segundo open, das respostas\n");


  while (1) {
    sleep(1);
    int op_code;
    char buffer[2];
    memset(buffer, '\0', sizeof(buffer));
    // TODO: Read from pipe
    ssize_t ret = read(request, buffer, sizeof(buffer));
    printf("ret :%ld\n", ret);
    op_code = atoi(buffer);
    printf("op_code: %d\n", op_code);
    switch (op_code) {
        case 3:
            int event_id;
            size_t num_rows;
            size_t num_cols;
            ssize_t ret0 = read(request, &event_id, sizeof(int));
            printf("event_id: %d\n", event_id);
            if (ret0 == 0) {
                // ret == 0 indicates EOF
                fprintf(stderr, "[INFO]: pipe closed\n");
            } else if (ret0 == -1) {
                // ret == -1 indicates error
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            ssize_t ret1 = read(request, &num_rows, sizeof(size_t));
            printf("%ld\n", num_rows);
            if (ret1 == 0) {
                // ret == 0 indicates EOF
                fprintf(stderr, "[INFO]: pipe closed\n");
            } else if (ret1 == -1) {
                // ret == -1 indicates error
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            ssize_t ret2 = read(request, &num_cols, sizeof(size_t));
            printf("%ld\n", num_cols);
            if (ret2 == 0) {
                // ret == 0 indicates EOF
                fprintf(stderr, "[INFO]: pipe closed\n");
            } else if (ret2 == -1) {
                // ret == -1 indicates error
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (ems_create(event_id, num_rows, num_cols)) 
                fprintf(stderr, "Failed to create event\n");
            
            break;
        default:
            fprintf(stderr, "Unknown op_code: %d\n", op_code);
            break;
      }



 
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  ems_terminate();
}