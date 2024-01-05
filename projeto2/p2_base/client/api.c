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
#include "common/io.h"

// Global variables
int geral_pipe;
int response_pipe;
int request_pipe;
int session_id;

int ems_setup(char const *req_pipe_path, char const *resp_pipe_path, char const *server_pipe_path) {
  // Check and create the request pipe
  if (unlink(req_pipe_path) != 0 && errno != ENOENT)
  {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path,
            strerror(errno));
    return EXIT_FAILURE;
  }
  // Check and create the response pipe
  if (mkfifo(req_pipe_path, 0640) != 0)
  {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT)
  {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path,
            strerror(errno));
    return EXIT_FAILURE;
  }
  if (mkfifo(resp_pipe_path, 0640) != 0)
  {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // Open the general pipe for writing
  geral_pipe = open(server_pipe_path, O_WRONLY);
  if (geral_pipe == -1)
  {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  // Send setup information to the server
  char buffer[2];
  memset(buffer, '\0', sizeof(buffer));
  strcpy(buffer, "1");
  size_t len = 2;
  size_t written = 0;
  ssize_t ret;
  while (written < len) {
      ret = write(geral_pipe, buffer + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }

  // Send request and response pipe paths to the server
  char buffer1[BUFFER_SIZE];
  memset(buffer1, '\0', sizeof(buffer1));
  strcpy(buffer1, req_pipe_path);
  len = BUFFER_SIZE;
  written = 0;
  while (written < len) {
      ret = write(geral_pipe, buffer1 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }

  char buffer2[BUFFER_SIZE];
  memset(buffer2, '\0', sizeof(buffer2));
  strcpy(buffer2, resp_pipe_path);
  len = BUFFER_SIZE;
  written = 0;
  while (written < len) {
      ret = write(geral_pipe, buffer2 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }
  // Open request and response pipes
  request_pipe = open(req_pipe_path, O_WRONLY);
  if (request_pipe == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  response_pipe = open(resp_pipe_path, O_RDONLY);
  if (response_pipe == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  // Read the session ID from the response pipe
  ret = read(response_pipe, &session_id, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  return 0;
}

int ems_quit(void) {
  // Send quit signal to the server
  char buffer0[2];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, "2");
  int len = 2;
  int written = 0;
  ssize_t ret;
  while (written < len) {
      ret = write(request_pipe, buffer0 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }
  // Close pipes and reset session information
  close(geral_pipe);
  close(response_pipe);
  close(request_pipe);
  session_id = -1;
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // Send create event request to the server
  int erro;
  char buffer0[2];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, "3");
  int len = 2;
  int written = 0;
  ssize_t ret;
  while (written < len) {
      ret = write(request_pipe, buffer0 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }

  ret = write(request_pipe, &session_id, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  ret = write(request_pipe, &event_id, sizeof(event_id));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  ret = write(request_pipe, &num_rows, sizeof(num_rows));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  ret = write(request_pipe, &num_cols, sizeof(num_cols));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  // Read the response from the server
  ret = read(response_pipe, &erro, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if (erro == 1)
  {
    return EXIT_FAILURE;
  }
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys)
{
  int erro;
  // Send reserve seats request to the server
  char buffer0[2];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, "4");
  int len = 2;
  int written = 0;
  ssize_t ret;
  while (written < len) {
      ret = write(request_pipe, buffer0 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }
  ret = write(request_pipe, &session_id, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  ret = write(request_pipe, &event_id, sizeof(event_id));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  ret = write(request_pipe, &num_seats, sizeof(num_seats));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  ret = write(request_pipe, xs, MAX_JOB_FILE_NAME_SIZE);
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  ret = write(request_pipe, ys, MAX_JOB_FILE_NAME_SIZE);
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  // Read the response from the server
  ret = read(response_pipe, &erro, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if (erro == 1)
  {
    return EXIT_FAILURE;
  }
  return 0;
}

int ems_show(int out_fd, unsigned int event_id){
  int error;
  size_t num_rows, num_cols;
  // Send show event request to the server
  char buffer0[2];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, "5");
  int len = 2;
  int written = 0;
  ssize_t ret;
  while (written < len) {
      ret = write(request_pipe, buffer0 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }
  ret = write(request_pipe, &session_id, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  ret = write(request_pipe, &event_id, sizeof(event_id));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  // Read the response from the server and display seating arrangement
  ret = read(response_pipe, &error, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if (error == 1)
  {
    return EXIT_FAILURE;
  }
  ret = read(response_pipe, &num_rows, sizeof(size_t));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  ret = read(response_pipe, &num_cols, sizeof(size_t));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  int seat_index_list[num_rows][num_cols];
  ret = read(response_pipe, &seat_index_list, sizeof(int) * (num_rows * num_cols));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  for (size_t i = 0; i < num_rows; i++)
  {
    for (size_t j = 0; j < num_cols; j++)
    {
      char buffer[16];
      sprintf(buffer, "%u", seat_index_list[i][j]);

      if (print_str(out_fd, buffer))
      {
        perror("Error writing to file descriptor");
        return 1;
      }

      if (j < num_cols)
      {
        if (print_str(out_fd, " "))
        {
          perror("Error writing to file descriptor");
          return EXIT_FAILURE;
        }
      }
    }

    if (print_str(out_fd, "\n"))
    {
      perror("Error writing to file descriptor");
      return EXIT_FAILURE;
    }
  }

  return 0;
}

int ems_list_events(int out_fd){
  int event_counter, error;
  // Send list events request to the server
  char buffer0[2];
  memset(buffer0, '\0', sizeof(buffer0));
  strcpy(buffer0, "6");
  int len = 2;
  int written = 0;
  ssize_t ret;
  while (written < len) {
      ret = write(request_pipe, buffer0 + written, len - written);
      if (ret < 0) {
          fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      written += ret;
  }
  ret = write(request_pipe, &session_id, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  // Read the response from the server and display event list
  ret = read(response_pipe, &error, sizeof(int));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if (ret == 1)
  {
    return EXIT_FAILURE;
  }
  ret = read(response_pipe, &event_counter, sizeof(event_counter));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  if (event_counter == 0)
  {
    char buff[] = "No events\n";
    if (print_str(out_fd, buff))
    {
      perror("Error writing to file descriptor");
      return EXIT_FAILURE;
    }
    return 0;
  }
  int event_list[event_counter];
  ret = read(response_pipe, &event_list, sizeof(event_list));
  if (ret == -1) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
  }
  for (int i = 0; i < event_counter; i++)
  {
    char buffer[25];
    memset(buffer, '\0', 25);
    sprintf(buffer, "Event: %d\n", event_list[i]);
    len = strlen(buffer);
    int done = 0;
    while (len > 0) {
        int bytes_written = write(out_fd, buffer + done, len);

        if (bytes_written < 0){
          fprintf(stderr, "write error: %s\n", strerror(errno));
          return -1;
        }

        /* might not have managed to write all, len becomes what remains */
        len -= bytes_written;
        done += bytes_written;
    }
  }
  return 0;
}
