#include <stdlib.h>
#include "queue.h"

void init_queue(queue* q) {
    q->tail = NULL;
    q->size = 0;
}

void push(queue* q, int value) {
    node* new_node = (node*) malloc(sizeof(node));
if (new_node == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
}
new_node->value = value;

    if (q->tail == NULL) {
        q->tail = new_node;
        q->tail->next = new_node;
    } else {
        new_node->next = q->tail->next;
        q->tail->next = new_node;
        q->tail = new_node;
    }
    q->size++;
}

int pop(queue* q) {
    if (q->size == 0) {
        return -1;
    }
    int value = q->tail->next->value;
    node* temp = q->tail->next;
    if (q->size == 1) {
        q->tail = NULL;
    } else {
        q->tail->next = q->tail->next->next;
    }
    q->size--;
    free(temp);
    return value;
}

int peek(queue* q) {
    if (q->size == 0) {
        return -1;
    }
    return q->tail->next->value;
}

