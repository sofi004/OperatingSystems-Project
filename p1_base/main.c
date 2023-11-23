#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "process_file.h"


int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if(argc == 2){
    
    DIR *directory;
    struct dirent *ent;
    directory = opendir(argv[1]);
    char * file_name;
    char job_name[6] = "/";
    while ((ent = readdir (directory)) != NULL){
      file_name = strcat(job_name, ent->d_name);
      int fd = open(file_name, O_RDONLY); //usar o concat de paths
      process(fd);
    }
    ems_terminate();
    // quando o while acabar quero chamar o ems terminate
  }
  if (argc > 2) {
    char *endptr;
    unsigned long int delay = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
}
