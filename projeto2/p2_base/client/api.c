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
#include "common/constants.h"
int geral_pipe;
int response_pipe;
int request_pipe;
int session_id;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  if (unlink(req_pipe_path) != 0 && errno != ENOENT) {
          fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path,
          strerror(errno));
          return EXIT_FAILURE;
  }
  if (mkfifo(req_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT) {
          fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path,
          strerror(errno));
          return EXIT_FAILURE;
  }
  if (mkfifo(resp_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  geral_pipe = open(server_pipe_path, O_WRONLY);
  printf("passei o open do pipe geral\n");
  if (geral_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  char op_code[2];
  memset(op_code, '\0', sizeof(op_code));
  strcpy(op_code, "1");
  printf("antes do write do setup\n");
  ssize_t ret = write(geral_pipe, op_code, strlen(op_code) + 1);
  printf("depois do write do setup\n");
  //mandar o nome pro servidor
  char buffer0[BUFFER_SIZE];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, req_pipe_path);
  int done = 0;
  int len = BUFFER_SIZE;
   while (len > 0) {
      ssize_t bytes_written = write(geral_pipe, buffer0 + done, len);

      if (bytes_written < 0){
         fprintf(stderr, "write error: %s\n", strerror(errno));
         return EXIT_FAILURE;
      }

      len -= bytes_written;
      done += bytes_written;
   }
  //mandar o nome pro servidor
  char buffer1[BUFFER_SIZE];
  memset(buffer1, '\0', sizeof(buffer1));
  strcpy(buffer1, resp_pipe_path);
  done = 0;
  len = BUFFER_SIZE;
  while (len > 0) {
    ssize_t bytes_written = write(geral_pipe, buffer1 + done, len);

    if (bytes_written < 0){
        fprintf(stderr, "write error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

      len -= bytes_written;
      done += bytes_written;
   }


  request_pipe = open(req_pipe_path, O_WRONLY);
  printf("passei o open do pipe de pedidos e o id é %d\n", request_pipe);

  response_pipe = open(resp_pipe_path, O_RDONLY);
  printf("passei o open do pipe de respostas\n");

  ret = read(response_pipe, &session_id, sizeof(int));
  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  char op_code[2];
  memset(op_code, '\0', sizeof(op_code));
  strcpy(op_code, "2");
  ssize_t ret0 = write(request_pipe, op_code, strlen(op_code) + 1);
  close(geral_pipe);
  close(response_pipe);
  close(request_pipe);
  session_id = -1;
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) { 
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  int erro;
  char op_code[2];
  memset(op_code, '\0', sizeof(op_code));
  strcpy(op_code, "3");
  ssize_t ret0 = write(request_pipe, op_code, strlen(op_code) + 1);
  ret0 = write(request_pipe, &session_id, sizeof(int));
  printf("%ld no create\n", ret0);
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ret0 = write(request_pipe, &event_id, sizeof(event_id));
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  ret0 = write(request_pipe, &num_rows, sizeof(num_rows));
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ret0 = write(request_pipe, &num_cols, sizeof(num_cols));
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ret0 = read(response_pipe, &erro, sizeof(int));
  if(erro == 1){
    return EXIT_FAILURE;
  }
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  int erro;
  char op_code[2];
  memset(op_code, '\0', sizeof(op_code));
  strcpy(op_code, "4");
  ssize_t ret0 = write(request_pipe, op_code, strlen(op_code) + 1);
  ret0 = write(request_pipe, &session_id, sizeof(int));
  printf("%ld no reserve\n", ret0);
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ssize_t ret1 = write(request_pipe, &event_id, sizeof(event_id));
  if (ret1 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  ssize_t ret2 = write(request_pipe, &num_seats, sizeof(num_seats));
  if (ret2 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ssize_t ret3 = write(request_pipe, xs, 256);
  if (ret3 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ret3 = write(request_pipe, ys, 256);
  if (ret3 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  ret3 = read(response_pipe, &erro, sizeof(int));
  if(erro == 1){
    return EXIT_FAILURE;
  }
  return 0;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  int error;
  size_t num_rows, num_cols;
  char op_code[2];
  memset(op_code, '\0', sizeof(op_code));
  strcpy(op_code, "5");
  ssize_t ret0 = write(request_pipe, op_code, strlen(op_code) + 1);
  ret0 = write(request_pipe, &session_id, sizeof(int));
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  ssize_t ret1 = write(request_pipe, &event_id, sizeof(event_id));
  if (ret1 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  printf("cheguei ao read\n");
  //read  1 if there is an error
  ret1 = read(response_pipe, &error, sizeof(int));
  if (ret1 < 0) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  if(error == 1){
    return EXIT_FAILURE;
  }
  ret1 = read(response_pipe, &num_rows, sizeof(size_t));
  printf("entrei no if\n");
  ret1 = read(response_pipe, &num_cols, sizeof(size_t));
  if (ret1 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  printf("num_rows: %d, num_cols: %d\n", num_rows, num_cols);
  int seat_index_list[num_rows][num_cols];
  ret1 = read(response_pipe, &seat_index_list, sizeof(int)*(num_rows*num_cols));
  if (ret1 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  for (size_t i = 0; i < num_rows; i++) {
      for (size_t j = 0; j < num_cols; j++) {
      char buffer[16];
      sprintf(buffer, "%u", seat_index_list[i][j]);

      if (print_str(out_fd, buffer)) {
          perror("Error writing to file descriptor");
          return 1;
      }

      if (j < num_cols) {
          if (print_str(out_fd, " ")) {
          perror("Error writing to file descriptor");
          return EXIT_FAILURE;
          }
      }
      }

      if (print_str(out_fd, "\n")) {
      perror("Error writing to file descriptor");
      return EXIT_FAILURE;
      }
  }

  return 0;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  int event_counter, error;
  char op_code[2];
  memset(op_code, '\0', sizeof(op_code));
  strcpy(op_code, "6");
  ssize_t ret0 = write(request_pipe, op_code, strlen(op_code) + 1);
  ret0 = write(request_pipe, &session_id, sizeof(int));
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  ret0 = read(response_pipe, &error, sizeof(int));
  if(ret0 == 1){
    return EXIT_FAILURE;
  }
  ret0 = read(response_pipe, &event_counter, sizeof(event_counter));
  if (ret0 < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if(event_counter == 0){
    char buff[] = "No events\n";
    if (print_str(out_fd, buff)) {
      perror("Error writing to file descriptor");
      return EXIT_FAILURE;
    }
    return 0;
  }
  int event_list[event_counter];
  ret0 = read(response_pipe, &event_list, sizeof(event_list));
  for (int i = 0; i < event_counter; i++) {
    char buffer[25];
    memset(buffer, '\0', 25);
    sprintf(buffer, "Event: %d\n", event_list[i]);
    ret0 = write(out_fd, buffer, strlen(buffer));
  }
  return 0;
}
