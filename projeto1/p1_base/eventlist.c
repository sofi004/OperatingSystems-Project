#include "eventlist.h"
#include <stdlib.h>

struct EventList* create_list() {
  // Allocate memory for a new EventList.
  struct EventList* list = (struct EventList*)malloc(sizeof(struct EventList));
  // Check if the memory allocation was successful.
  if (!list) return NULL; 
  // Initialize the head and tail pointers of the list to NULL.
  list->head = NULL;
  list->tail = NULL;
  // Declare and initialize a read-write lock for the list.
  pthread_rwlock_t list_lock;
  pthread_rwlock_init(&list_lock, NULL);
  list->list_lock = list_lock;
  // Return the EventList created and initialized.
  return list;
}

int append_to_list(struct EventList* list, struct Event* event) {
  // Check if the input list is valid.
  if (!list) return 1;
  // Allocate memory for a ListNode
  struct ListNode* new_node = (struct ListNode*)malloc(sizeof(struct ListNode));
  // Check if the memory allocation for the new node was successful.
  if (!new_node){
    return 1;
  }
  // Assign the provided event to the new node and set its next pointer to NULL.
  new_node->event = event;
  new_node->next = NULL;
  // If the list is empty, the new node becomes the head and the tail.
  if (list->head == NULL) {
    list->head = new_node;
    list->tail = new_node;
  // If the list is not empty, teh node is added to the end of the list, and the tail is updated.
  } else {
    list->tail->next = new_node;
    list->tail = new_node;
  }
  return 0;
}

static void free_event(struct Event* event) {
  // Check if the input event is valid.
  if (!event) return;
  pthread_mutex_destroy(&event->event_lock);
  // Free the memory allocated for the event's data and the event itself.
  free(event->data);
  free(event);
}

void free_list(struct EventList* list) {
  // Check if the input list is valid.
  if (!list) return;
  // Declare a pointer to a ListNode and initialize it with the head of the list.
  struct ListNode* current = list->head;
  while (current) {
    // Save the current node in a temporary variable and move the current pointer to the next node.
    struct ListNode* temp = current;
    current = current->next;
    free_event(temp->event);
    free(temp);
  }
  pthread_rwlock_destroy(&list->list_lock);
  free(list);
}

struct Event* get_event(struct EventList* list, unsigned int event_id) {
  // Check if the input list is valid.
  if (!list) return NULL;
  // Declare a pointer to a ListNode and initialize it with the head of the list.
  struct ListNode* current = list->head;
  while (current) {
    struct Event* event = current->event;
    // Check if the ID of the current event matches the target ID.
    if (event->id == event_id) {
      // Return the event.
      return event;
    }
    // Move to the next node in the list.
    current = current->next;
  }
  // Return NULL if the target event is not found.
  return NULL;
}
