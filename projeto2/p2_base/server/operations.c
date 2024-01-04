#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/io.h"
#include "eventlist.h"

static struct EventList* event_list = NULL;
static unsigned int state_access_delay_us = 0;

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event_id The ID of the event to get.
/// @param from First node to be searched.
/// @param to Last node to be searched.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event* get_event_with_delay(unsigned int event_id, struct ListNode* from, struct ListNode* to) {
  struct timespec delay = {0, state_access_delay_us * 1000};
  nanosleep(&delay, NULL);  // Should not be removed

  return get_event(event_list, event_id, from, to);
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event* event, size_t row, size_t col) { return (row - 1) * event->cols + col - 1; }

int ems_init(unsigned int delay_us) {
  if (event_list != NULL) {
    fprintf(stderr, "EMS state has already been initialized\n");
    return 1;
  }

  event_list = create_list();
  state_access_delay_us = delay_us;

  return event_list == NULL;
}

int ems_terminate() {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_wrlock(&event_list->rwl) != 0) {
    fprintf(stderr, "Error locking list rwl\n");
    return 1;
  }

  free_list(event_list);
  pthread_rwlock_unlock(&event_list->rwl);
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_wrlock(&event_list->rwl) != 0) {
    fprintf(stderr, "Error locking list rwl\n");
    return 1;
  }

  if (get_event_with_delay(event_id, event_list->head, event_list->tail) != NULL) {
    fprintf(stderr, "Event already exists\n");
    pthread_rwlock_unlock(&event_list->rwl);
    return 1;
  }

  struct Event* event = malloc(sizeof(struct Event));

  if (event == NULL) {
    fprintf(stderr, "Error allocating memory for event\n");
    pthread_rwlock_unlock(&event_list->rwl);
    return 1;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  if (pthread_mutex_init(&event->mutex, NULL) != 0) {
    pthread_rwlock_unlock(&event_list->rwl);
    free(event);
    return 1;
  }
  event->data = calloc(num_rows * num_cols, sizeof(unsigned int));

  if (event->data == NULL) {
    fprintf(stderr, "Error allocating memory for event data\n");
    pthread_rwlock_unlock(&event_list->rwl);
    free(event);
    return 1;
  }

  if (append_to_list(event_list, event) != 0) {
    fprintf(stderr, "Error appending event to list\n");
    pthread_rwlock_unlock(&event_list->rwl);
    free(event->data);
    free(event);
    return 1;
  }

  pthread_rwlock_unlock(&event_list->rwl);
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    fprintf(stderr, "Error locking list rwl\n");
    return 1;
  }

  struct Event* event = get_event_with_delay(event_id, event_list->head, event_list->tail);

  pthread_rwlock_unlock(&event_list->rwl);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    return 1;
  }

  if (pthread_mutex_lock(&event->mutex) != 0) {
    fprintf(stderr, "Error locking mutex\n");
    return 1;
  }

  for (size_t i = 0; i < num_seats; i++) {
    printf("seat xs:%ld  ys:%ld\n", xs[i], ys[i]);
    if (xs[i] <= 0 || xs[i] > event->rows || ys[i] <= 0 || ys[i] > event->cols) {
      fprintf(stderr, "Seat out of bounds\n");
      pthread_mutex_unlock(&event->mutex);
      return 1;
    }
  }

  for (size_t i = 0; i < event->rows * event->cols; i++) {
    for (size_t j = 0; j < num_seats; j++) {
      if (seat_index(event, xs[j], ys[j]) != i) {
        continue;
      }

      if (event->data[i] != 0) {
        fprintf(stderr, "Seat already reserved\n");
        pthread_mutex_unlock(&event->mutex);
        return 1;
      }

      break;
    }
  }

  unsigned int reservation_id = ++event->reservations;

  for (size_t i = 0; i < num_seats; i++) {
    event->data[seat_index(event, xs[i], ys[i])] = reservation_id;
  }

  pthread_mutex_unlock(&event->mutex);
  return 0;
}

int ems_show(int response, unsigned int event_id) {
  ssize_t ret;
  size_t erro = 0;
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    ret = write(response, &erro, sizeof(size_t));
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    fprintf(stderr, "Error locking list rwl\n");
    ret = write(response, &erro, sizeof(size_t));
    return 1;
  }

  struct Event* event = get_event_with_delay(event_id, event_list->head, event_list->tail);
  pthread_rwlock_unlock(&event_list->rwl);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    ret = write(response, &erro, sizeof(size_t));
    return 1;
  }

  if (pthread_mutex_lock(&event->mutex) != 0) {
    fprintf(stderr, "Error locking mutex\n");
    ret = write(response, &erro, sizeof(size_t));
    return 1;
  }
  int seat_index_list[event->rows][event->cols];

  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1; j <= event->cols; j++) {
      seat_index_list[i - 1][j - 1] = event->data[seat_index(event, i, j)];
    }
  }
  ret = write(response, &event->rows, sizeof(size_t));
  ret = write(response, &event->cols, sizeof(size_t));
  ret = write(response, &seat_index_list, sizeof(int)*(event->rows*event->cols));
  printf("fiz o write\n");
  pthread_mutex_unlock(&event->mutex);
  return 0;
}

int ems_list_events(int response) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    fprintf(stderr, "Error locking list rwl\n");
    return 1;
  }

  struct ListNode* to = event_list->tail;
  struct ListNode* current = event_list->head;

  int counter = 0;
  while (1) {
    counter++;
    if (current == to) {
      break;
    }
    current = current->next;
  }
  printf("counter: %d\n", counter);
  ssize_t ret = write(response, &counter, sizeof(counter));
  current = event_list->head;
  if(counter != 0){
    int event_list[counter];
    counter = 0;
    while (1) {
      if (current == NULL) {
        break;
      }
      event_list[counter] = current->event->id;
      printf("counter na lista: %d\n", current->event->id);

      current = current->next;
      counter++;
    }
    ret = write(response, &event_list, sizeof(event_list));
  }
  pthread_rwlock_unlock(&event_list->rwl);
  return 0;
}

void sig_show(){
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    exit(EXIT_FAILURE);
  }

  struct ListNode* current = event_list->head;
  printf("entrei\n");
  while (current != NULL) {
    printf("entrei no while\n");
    unsigned int* event_id = current->event->id;
    printf("Event: %d\n", event_id);
    for (size_t i = 1; i <= current->event->rows; i++) {
      for (size_t j = 1; j <= current->event->cols; j++) {
        unsigned int* seat = current->event->data[seat_index(current->event, i, j)];       
        printf("%d", seat);
        if (j < current->event->cols) {
          printf(" ");
        }
      }
      printf("\n");
    }
    current = current->next;
  } 
  printf("saí\n"); 
}