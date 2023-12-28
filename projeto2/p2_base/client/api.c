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
  if (unlink(req_pipe_path) != 0 && errno != ENOENT) {
          fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path,
          strerror(errno));
          exit(EXIT_FAILURE);
  }
  if (mkfifo(req_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT) {
          fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path,
          strerror(errno));
          exit(EXIT_FAILURE);
  }
  if (mkfifo(resp_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  geral_pipe = open(server_pipe_path, O_WRONLY);
  printf("passei o open do pipe geral\n");
  if (geral_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  //mandar o nome pro servidor
  char buffer0[4];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, req_pipe_path);
  int len = strlen(buffer0);
   int done = 0;
   while (len > 0) {
      ssize_t bytes_written = write(geral_pipe, buffer0 + done, len);

      if (bytes_written < 0){
         fprintf(stderr, "write error: %s\n", strerror(errno));
         return -1;
      }

      len -= bytes_written;
      done += bytes_written;
   }
  //tratamento do req_pipe_path

  request_pipe = open(req_pipe_path, O_WRONLY);
  printf("passei o open do pipe de pedidos e o id é %d\n", request_pipe);

  //mandar o nome pro servidor
  char buffer1[5];
  memset(buffer1, '\0', sizeof(buffer1));
  strcpy(buffer1, resp_pipe_path);
  len = strlen(buffer1);
  done = 0;
  while (len > 0) {
    ssize_t bytes_written = write(geral_pipe, buffer1 + done, len);

    if (bytes_written < 0){
        fprintf(stderr, "write error: %s\n", strerror(errno));
        return -1;
    }

      len -= bytes_written;
      done += bytes_written;
   }



  //tratamento do resp_pipe_path

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
  int numero = 3;
  char buffer[2];
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%d",numero);
  ssize_t ret0 = write(request_pipe, buffer, sizeof(buffer));
  printf("%ld\n", ret0);
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%d",event_id);
  ssize_t ret1 = write(request_pipe, buffer, sizeof(buffer));
  if (ret1 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%ld",num_rows);
  ssize_t ret2 = write(request_pipe, buffer, sizeof(buffer));
  if (ret2 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%ld",num_cols);
  ssize_t ret3 = write(request_pipe, buffer, sizeof(buffer));
  if (ret3 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  return 0;
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
