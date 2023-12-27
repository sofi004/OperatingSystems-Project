#include "api.h"

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
int geral_pipe;
int response_pipe;
int request_pipe;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  //tratamento do server_pipe
  //char temp_path[128];
  //strcpy(temp_path, "/p2_base/server/");
  //strcat(temp_path, server_pipe_path);
  //printf("%s\n", temp_path);
  geral_pipe = open(server_pipe_path, O_WRONLY);
  printf("passei o open do pipe geral\n");
  if (geral_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  ssize_t ret = write(geral_pipe, req_pipe_path, strlen(req_pipe_path));
  if (ret < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  ret = write(geral_pipe, resp_pipe_path, strlen(resp_pipe_path));
  if (ret < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  //tratamento do req_pipe_path
  if (unlink(req_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path,
            strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (mkfifo(req_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  request_pipe = open(req_pipe_path, O_WRONLY);
  printf("passei o open do pipe de pedidos\n");


  //tratamento do resp_pipe_path
  printf("*");
  printf("%s", resp_pipe_path);
  printf("*\n");
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path,
            strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (mkfifo(resp_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  response_pipe = open(resp_pipe_path, O_RDONLY);
  printf("passei o open do pipe de respostas\n");


  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  close(geral_pipe);
  close(response_pipe);
  close(request_pipe);
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) { 
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  ssize_t ret0 = write(request_pipe, 3, sizeof(int));
  printf("%ld\n", ret0);
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ssize_t ret1 = write(request_pipe, event_id, sizeof(event_id));
  if (ret1 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ssize_t ret2 = write(request_pipe, num_rows, sizeof(num_rows));
  if (ret2 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ssize_t ret3 = write(request_pipe, num_cols, sizeof(num_cols));
  if (ret3 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}
