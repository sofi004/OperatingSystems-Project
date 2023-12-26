#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "eventlist.h"

// Declare a global variable for the event list.
static struct EventList* event_list = NULL;
// Declare a global variable for the delay.
static unsigned int state_access_delay_ms = 0;

/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event_id The ID of the event to get.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event* get_event_with_delay(unsigned int event_id) {
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL);  // Should not be removed
  return get_event(event_list, event_id);
}

/// Gets the seat with the given index from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event Event to get the seat from.
/// @param index Index of the seat to get.
/// @return Pointer to the seat.
static unsigned int* get_seat_with_delay(struct Event* event, size_t index) {
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL);  // Should not be removed
  return &event->data[index];
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event* event, size_t row, size_t col) { return (row - 1) * event->cols + col - 1; }

// Initialize the EMS state with a delay.
int ems_init(unsigned int delay_ms) {
  // Check if the EMS state has already been initialized.
  if (event_list != NULL) {
    fprintf(stderr, "EMS state has already been initialized\n");
    return 1;
  }
  // Create a new event list and set the delay.
  event_list = create_list();
  state_access_delay_ms = delay_ms;
  // Check if the event list creation was successful.
  return event_list == NULL;
}

// Terminate the EMS state
int ems_terminate() {
  // Check if the EMS state has been initialized.
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }
  // Free the event list and set the global variable to NULL.
  free_list(event_list);
  return 0;
}

// Create a new event.
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // Check if the EMS state has been initialized
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }
  // Lock the event list for writing and check if the event already exists.
  pthread_rwlock_wrlock(&event_list->list_lock);
  if (get_event_with_delay(event_id) != NULL) {
    fprintf(stderr, "Event already exists\n");
    pthread_rwlock_unlock(&event_list->list_lock);
    return 1;
  }
  // Allocate memory for the new event.
  struct Event* event = malloc(sizeof(struct Event));
  if (event == NULL) {
    fprintf(stderr, "Error allocating memory for event\n");
    pthread_rwlock_unlock(&event_list->list_lock);
    return 1;
  }
  pthread_mutex_t event_lock;
  pthread_mutex_init(&event_lock, NULL);
  event->event_lock = event_lock;
  pthread_mutex_lock(&event->event_lock);
  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  event->data = malloc(num_rows * num_cols * sizeof(unsigned int));

  // Check if memory allocation for event data was successful.
  if (event->data == NULL) {
    fprintf(stderr, "Error allocating memory for event data\n");
    free(event);
    pthread_mutex_unlock(&event->event_lock);
    pthread_rwlock_unlock(&event_list->list_lock);
    return 1;
  }

  for (size_t i = 0; i < num_rows * num_cols; i++) {
    event->data[i] = 0;
  }

  // Append the new event to the event list.
  if (append_to_list(event_list, event) != 0) {
    fprintf(stderr, "Error appending event to list\n");
    free(event->data);
    free(event);
    pthread_mutex_unlock(&event->event_lock);
    pthread_rwlock_unlock(&event_list->list_lock);
    return 1;
  }
  pthread_mutex_unlock(&event->event_lock);
  pthread_rwlock_unlock(&event_list->list_lock);
  return 0;
}

// Swap two size_t values.
void swap(size_t* a, size_t* b) {
    size_t temp = *a;
    *a = *b;
    *b = temp;
}

// Perform bubble sort on x coordinates.
void bubbleSortx(size_t arr[], size_t arr2[], size_t n) {
    for (size_t i = 0; i < n-1; i++) {
        for (size_t j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                swap(&arr[j], &arr[j+1]);
                swap(&arr2[j], &arr2[j+1]);
            }
        }
    }
}

// Perform bubble sort on y coordinates.
void bubbleSorty(size_t arr[], size_t arr2[], size_t n) {
    for (size_t i = 0; i < n-1; i++) {
        for (size_t j = 0; j < n-i-1; j++) {
            if (arr2[j] > arr2[j+1]) {
                swap(&arr[j], &arr[j+1]);
                swap(&arr2[j], &arr2[j+1]);
            }
        }
    }
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  // Sort the seats to be reserved.
  bubbleSorty(xs, ys, num_seats);
  bubbleSortx(xs, ys, num_seats);
  // Check if the EMS state has been initialized.
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }
  // Lock the event list for reading.
  pthread_rwlock_rdlock(&event_list->list_lock);
  // Get the event.
  struct Event* event = get_event_with_delay(event_id);
  // Check if the event was not found.
  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    pthread_rwlock_unlock(&event_list->list_lock);
    return 1;
  }
  pthread_mutex_lock(&event->event_lock);
  // Increment the reservation ID
  unsigned int reservation_id = ++event->reservations;
  size_t i = 0;
  for (; i < num_seats; i++) {
    size_t row = xs[i];
    size_t col = ys[i];
    // Check if the seat coordinates are valid.
    if (row <= 0 || row > event->rows || col <= 0 || col > event->cols) {
      fprintf(stderr, "Invalid seat\n");
      break;    
    }
    // Check if the seat is already reserved.
    if (*get_seat_with_delay(event, seat_index(event, row, col)) != 0) {
      fprintf(stderr, "Seat already reserved\n");
      break;
    }
    // Reserve the seat.
    *get_seat_with_delay(event, seat_index(event, row, col)) = reservation_id;
  }

  // If the reservation was not successful.
  if (i < num_seats) {
    // Decrement the reservation ID.
    event->reservations--;
    for (size_t j = 0; j < i; j++) {
      *get_seat_with_delay(event, seat_index(event, xs[j], ys[j])) = 0;
    }
    // Unlock the locks. 
    pthread_rwlock_unlock(&event_list->list_lock);
    pthread_mutex_unlock(&event->event_lock);
    // Return 1 indicating that there was an error.
    return 1;
  }
  pthread_mutex_unlock(&event->event_lock);
  pthread_rwlock_unlock(&event_list->list_lock);
  return 0;
}

int ems_show(unsigned int event_id, int fd1) {
  // Check if the EMS state has been initialized.
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }
  // Lock the event list for reading.
  pthread_rwlock_rdlock(&event_list->list_lock);
  // Get the event.
  struct Event* event = get_event_with_delay(event_id);
  pthread_rwlock_unlock(&event_list->list_lock);

  // Check if the event was not found.
  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    return 1;
  }
  // Iterate through the rows and columns of the event and write seat information in the .out file.
  pthread_mutex_lock(&event->event_lock);
  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1; j <= event->cols; j++) {
      unsigned int* seat = get_seat_with_delay(event, seat_index(event, i, j));
      char  c[2]= {'0' + (char) *seat};
      write(fd1, c, sizeof(char));
      if (j < event->cols) {
        write(fd1, " ", 1);
      }
    }
    write(fd1, "\n", 1);
  }
  pthread_mutex_unlock(&event->event_lock);
  return 0;
}

int ems_list_events(int fd1) {
  // Check if the EMS state has been initialized.
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }
  pthread_rwlock_rdlock(&event_list->list_lock);
  // Check if there are no events.
  if (event_list->head == NULL) {
    write(fd1, "No events\n", sizeof("No events\n") - 1);
    pthread_rwlock_unlock(&event_list->list_lock);
    return 0;
  }
  // Iterate through the events and write their IDs to the .out file.
  struct ListNode* current = event_list->head;
  while (current != NULL) {
    unsigned int* event_id = &(current->event)->id;
    char  c[20];
    int length = sprintf(c, "%u", *event_id);
    write(fd1, "Event: ", sizeof("Event: ") - 1);
    write(fd1, c, (size_t)length);
    write(fd1, "\n", 1);
    current = current->next;
  }
  pthread_rwlock_unlock(&event_list->list_lock);
  return 0;
}

void ems_wait(unsigned int delay_ms) {
  // Simulate a delay in milliseconds.
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}
