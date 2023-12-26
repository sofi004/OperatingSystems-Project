#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "process_file.h"
#include <pthread.h>
#include "structs.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  // Check if there are more than 4 command-line arguments.
  if (argc > 4) {
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

  if (argc == 4) {
    // Check if there are exactly 4 command-line arguments.
    DIR *directory;
    struct dirent *ent;
    // Open the directory specified by the first command-line argument.
    directory = opendir(argv[1]);
    char job_name[MAX_RESERVATION_SIZE];
    // Copy the directory name and append a "/" to it.
    strcpy(job_name, argv[1]);
    strcat(job_name, "/");
    char temp_name[MAX_RESERVATION_SIZE];
    char temp_name_out[MAX_RESERVATION_SIZE];
    char *trimchar;
    int process_counter = 0;
    int status;

    while ((ent = readdir(directory)) != NULL)
    {
      if (strstr(ent->d_name, ".jobs") == NULL) {
        // Skip unwanted files.
        continue;
      }
      int id = fork();
      // Create a child process.
      process_counter++;
      //Wait for a child process to finish
      if (id != 0 && process_counter >= atoi(argv[2])) {
        pid_t idw = wait(&status);
        printf("O processo %d terminou com o estado: %d\n", idw, WEXITSTATUS(status));
        process_counter--;
      }
      // If child process.
      if (id == 0) {
         // Build the path for the .jobs file.
        memset(temp_name, '\0', MAX_RESERVATION_SIZE);
        strcpy(temp_name, job_name);
        strcat(temp_name, ent->d_name);
        // Build the path for the .out file.
        memset(temp_name_out, '\0', MAX_RESERVATION_SIZE);
        strcpy(temp_name_out, job_name);
        strcat(temp_name_out, ent->d_name);
        trimchar = strchr(temp_name_out, '.');
        strcpy(trimchar, ".out");
        // List of threads.
        pthread_t th[atoi(argv[3])];
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, NULL);
        struct Thread_struct thread_struct[atoi(argv[3])];
        int temp_barrier_line = 0;
        // The output file descriptor is shared between threads.
        int fd_out = open(temp_name_out, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR); 
        // We use the while loop so if we find a barrier our threads will all join and be created again.
        while (1){
          for (int i = 0; i < atoi(argv[3]); i++) {
            // Inicialization of the structure as an argument to the thread_function.
            thread_struct[i].shared_lock = &lock;
            thread_struct[i].fd_out = fd_out;
            thread_struct[i].max_threads = atoi(argv[3]);
            strcpy(thread_struct[i].fd_name, temp_name);
            thread_struct[i].barrier_line = temp_barrier_line;
            thread_struct[i].index = i;
            // Create threads.
            pthread_create(&th[i], NULL, &process, (void *)&thread_struct[i]);
          }
          // For loop to join all the threads and storing the return value.
          for (int i = 0; i < atoi(argv[3]); i++)
          {
            pthread_join(th[i], (void **)&temp_barrier_line);            
          }
          /* In case of no barrier, the temp_barrier_line == 0, what means that
          we finished processing our file and we can leave the while loop. */
          if(temp_barrier_line == 0){
            break;
          }
        }
        pthread_mutex_destroy(&lock);
        close(fd_out);
        ems_terminate();
        exit(0);
      }
    }

    for (int i = 0; i < process_counter; i++) {
      // Wait for the remaining child processes to finish.
      pid_t idw = wait(&status);
      printf("O processo %d terminou com o estado: %d\n", idw, WEXITSTATUS(status));
    }

    closedir(directory);
    ems_terminate();
  }
  return 0;
}
