#include "parser.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "constants.h"

static int read_uint(int fd, unsigned int *value, char *next) {
  char buf[16];
  int i = 0;
  // Read characters until a non-digit character is encountered.
  while (1) {
    if (read(fd, buf + i, 1) == 0) {
      *next = '\0';
      break;
    }
    *next = buf[i];
    if (buf[i] > '9' || buf[i] < '0') {
      buf[i] = '\0';
      break;
    }
    i++;
  }
  unsigned long ul = strtoul(buf, NULL, 10);
  // Check for overflow.
  if (ul > UINT_MAX) {
    return 1;
  }
  *value = (unsigned int)ul;
  return 0;
}

// Clean up characters until a newline is encountered.
void cleanup(int fd) {
  char ch;
  while (read(fd, &ch, 1) == 1 && ch != '\n')
    ;
}

// Get the next command from the file descriptor.
enum Command get_next(int fd) {
  char buf[16];
  if (read(fd, buf, 1) != 1) {
    // End of Commands.
    return EOC; 
  }

  switch (buf[0]) {
    case 'C':
      // Read the next characters and check for the CREATE command.
      if (read(fd, buf + 1, 6) != 6 || strncmp(buf, "CREATE ", 7) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      return CMD_CREATE;

    case 'R':
      // Read the next characters and check for the RESERVE command.
      if (read(fd, buf + 1, 7) != 7 || strncmp(buf, "RESERVE ", 8) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      return CMD_RESERVE;

    case 'S':
      // Read the next characters and check for the SHOW command.
      if (read(fd, buf + 1, 4) != 4 || strncmp(buf, "SHOW ", 5) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }

      return CMD_SHOW;

    case 'L':
      // Read the next characters and check for the LIST command.
      if (read(fd, buf + 1, 3) != 3 || strncmp(buf, "LIST", 4) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      if (read(fd, buf + 4, 1) != 0 && buf[4] != '\n') {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      return CMD_LIST_EVENTS;

    case 'B':
      // Read the next characters and check for the BARRIER command.
      if (read(fd, buf + 1, 6) != 6 || strncmp(buf, "BARRIER", 7) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      if (read(fd, buf + 7, 1) != 0 && buf[7] != '\n') {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      return CMD_BARRIER;

    case 'W':
      // Read the next characters and check for the WAIT command.
      if (read(fd, buf + 1, 4) != 4 || strncmp(buf, "WAIT ", 5) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      return CMD_WAIT;

    case 'H':
      // Read the next characters and check for the HELP command.
      if (read(fd, buf + 1, 3) != 3 || strncmp(buf, "HELP", 4) != 0) {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      if (read(fd, buf + 4, 1) != 0 && buf[4] != '\n') {
        cleanup(fd);
        // Invalid Command.
        return CMD_INVALID;
      }
      return CMD_HELP;

    case '#':
      cleanup(fd);
      return CMD_EMPTY;

    case '\n':
      return CMD_EMPTY;

    default:
      cleanup(fd);
      return CMD_INVALID;
    
  }
}

int parse_create(int fd, unsigned int *event_id, size_t *num_rows, size_t *num_cols) {
  char ch;
  // Read event_id and check for space.
  if (read_uint(fd, event_id, &ch) != 0 || ch != ' ') {
    cleanup(fd);
    // Error: Invalid input.
    return 1;
  }

  unsigned int u_num_rows;
  // Read num_rows and check for space.
  if (read_uint(fd, &u_num_rows, &ch) != 0 || ch != ' ') {
    cleanup(fd);
    // Error: Invalid input.
    return 1;
  }
  *num_rows = (size_t)u_num_rows;
  // Read num_cols and check for newline.
  unsigned int u_num_cols;
  if (read_uint(fd, &u_num_cols, &ch) != 0 || (ch != '\n' && ch != '\0')) {
    cleanup(fd);
    // Error: Invalid input.
    return 1;
  }
  *num_cols = (size_t)u_num_cols;

  return 0;
}

size_t parse_reserve(int fd, size_t max, unsigned int *event_id, size_t *xs, size_t *ys) {
  char ch;
  // Read event_id and check for space.
  if (read_uint(fd, event_id, &ch) != 0 || ch != ' ') {
    cleanup(fd);
    return 0;
  }
  // Check for the existence of a '['.
  if (read(fd, &ch, 1) != 1 || ch != '[') {
    cleanup(fd);
    return 0;
  }
  size_t num_coords = 0;
  while (num_coords < max) {
    // Check for the existence of a '('.
    if (read(fd, &ch, 1) != 1 || ch != '(') {
      cleanup(fd);
      return 0;
    }
    unsigned int x;
    // Read x coordinate and check for ','.
    if (read_uint(fd, &x, &ch) != 0 || ch != ',') {
      cleanup(fd);
      return 0;
    }
    xs[num_coords] = (size_t)x;
    unsigned int y;
    // Read y coordinate and check for ')'
    if (read_uint(fd, &y, &ch) != 0 || ch != ')') {
      cleanup(fd);
      return 0;
    }
    ys[num_coords] = (size_t)y;
    num_coords++;
    if (read(fd, &ch, 1) != 1 || (ch != ' ' && ch != ']')) {
      cleanup(fd);
      return 0;
    }
    // Break if ']' is encountered.
    if (ch == ']') {
      break;
    }
  }
  if (num_coords == max) {
    cleanup(fd);
    return 0;
  }
  if (read(fd, &ch, 1) != 1 || (ch != '\n' && ch != '\0')) {
    cleanup(fd);
    return 0;
  }
  return num_coords;
}

int parse_show(int fd, unsigned int *event_id) {
  char ch;
  // Read event_id and check for newline.
  if (read_uint(fd, event_id, &ch) != 0 || (ch != '\n' && ch != '\0')) {
    cleanup(fd);
    // Error: Invalid input.
    return 1;
  }
  return 0;
}

int parse_wait(int fd, unsigned int *delay, unsigned int *thread_id) {
  char ch;
  // Read delay and check if it is valid.
  if (read_uint(fd, delay, &ch) != 0) {
    cleanup(fd);
    // Error: Invalid input.
    return -1;
  }
  // Check if a space exists after delay.
  if (ch == ' ') {
    // Check if thread_id is provided.
    if (thread_id == NULL) {
      cleanup(fd);
      return 0;
    }
    // Read thread_id and check for newline.
    if (read_uint(fd, thread_id, &ch) != 0 || (ch != '\n' && ch != '\0')) {
      cleanup(fd);
      // Error: Invalid input.
      return -1; 
    }
    // Success: thread_id provided.
    return 1;
  } else if (ch == '\n' || ch == '\0') {
    // Success: No thread_id provided
    return 0;
  } else {
    cleanup(fd);
    // Error: Invalid input.
    return -1;
  }
}
