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


int main(int argc, char *argv[])
{
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 4)
  {
    char *endptr;
    unsigned long int delay = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0' || delay > UINT_MAX)
    {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms))
  {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  if (argc == 4)
  {
    DIR *directory;
    struct dirent *ent;
    directory = opendir(argv[1]);
    char job_name[256];
    strcpy(job_name, argv[1]);
    strcat(job_name, "/");
    char temp_name[256];
    char temp_name_out[256];
    char *trimchar;
    int process_counter = 0;
    int status;

    while ((ent = readdir(directory)) != NULL)
    {
      if (strstr(ent->d_name, ".jobs") == NULL)
      { // verifica se nao estamos a ler algum ficheiro indesejado
        continue;
      }
      int id = fork();
      process_counter++;
      if (id != 0 && process_counter >= atoi(argv[2])) // Espera por algum processo acabe
      {
        pid_t idw = wait(&status);
        printf("O processo %d terminou com o estado: %d\n", idw, WEXITSTATUS(status));
        process_counter--;
      }
      if (id == 0)
      { // processamento do ficheiro
        memset(temp_name, '\0', 256);
        strcpy(temp_name, job_name);
        strcat(temp_name, ent->d_name);

        memset(temp_name_out, '\0', 256);
        strcpy(temp_name_out, job_name);
        strcat(temp_name_out, ent->d_name);
        trimchar = strchr(temp_name_out, '.');
        strcpy(trimchar, ".out");

        pthread_t th[(int)*argv[3]];
        struct Thread_struct thread_struct;
        thread_struct.current_line = 0;
        strcpy(thread_struct.fd_name, temp_name);
        thread_struct.fd_out = open(temp_name_out, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

        for(int i = 0; i < (int)*argv[3]; i++){
      
          pthread_create(&th[i], NULL, &process, (void *)&thread_struct);
        }
        for(int i = 0; i < (int)*argv[3]; i++){
      
          pthread_join(th[i], NULL);
        }
        close(thread_struct.fd_out);
        exit(0);
      }
    }

    for (int i = 0; i < process_counter; i++)
    {
      pid_t idw = wait(&status);
      printf("O processo %d terminou com o estado: %d\n", idw, WEXITSTATUS(status));
    }

    closedir(directory);
    ems_terminate();
    // quando o while acabar quero chamar o ems terminate
  }
  return 0;
}
