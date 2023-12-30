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
  
  char buffer1[40];
  memset(buffer1, '\0', sizeof(buffer1));

  //leitura do nome da pipe de pedidos
  int name_len = 0;
  ssize_t ret = read(geral, &name_len, sizeof(name_len));
  if (ret == 0) {
      // ret == 0 indicates EOF
      fprintf(stderr, "[INFO]: pipe closed\n");
  } else if (ret == -1) {
    // ret == -1 indicates error
    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  ret = read(geral, &buffer1, name_len);
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

  char buffer[40];
  memset(buffer, '\0', sizeof(buffer));
  //leitura do nome da pipe de respostas
  name_len = 0;
  ssize_t ret1 = read(geral, &name_len, sizeof(name_len));
  ret1 = read(geral, &buffer, name_len);
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
    int event_id;
    int op_code = 0;
    // TODO: Read from pipe
    ssize_t ret = read(request, &op_code, sizeof(op_code));
    switch (op_code) {
        case 3:
            
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
            if (ems_create(event_id, num_rows, num_cols)) 
                fprintf(stderr, "Failed to create event\n");
            
            break;
        case 4:
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
            //int xs_size = 256;
            //ssize_t ret3 = read(request, &xs_size, xs_size);

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
            if (ems_reserve(event_id, num_seats, xs, ys)) 
                fprintf(stderr, "Failed to reserve seats\n");
            break;
        case 5:
            int fd_out;
            ssize_t ret5 = read(request, &event_id, sizeof(event_id));
            if (ret4 < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            ret5 = read(request, &fd_out, sizeof(fd_out));
            if (ret5 < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (ems_show(fd_out, event_id)) 
                fprintf(stderr, "Failed to show event seats\n");
            break;
        case 6:
            int fd_out_list;
            ssize_t ret6 = read(request, &fd_out_list, sizeof(fd_out_list));
            if (ret6 < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (ems_list_events(fd_out)) 
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

  //TODO: Close Server

  ems_terminate();
}