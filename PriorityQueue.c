#include "PriorityQueue.h"
#include <stdio.h>
struct PqNode* createPqNode(int data, int priority) {
    struct PqNode* newPqNode = (struct PqNode*)malloc(sizeof(struct PqNode));
    if (newPqNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newPqNode->data = data;
    newPqNode->priority = priority;
    newPqNode->next = NULL;
    return newPqNode;
}

int isEmpty(struct PqNode* head) {
    return head == NULL;
}


void enqueue(PriorityQueue* pq, int data, int priority) {
    struct PqNode** head = &pq->head;
    struct PqNode* newPqNode = createPqNode(data, priority);
    
    if (*head == NULL) {
        *head = newPqNode;
    } else if (newPqNode->priority < (*head)->priority) {
        newPqNode->next = *head;
        *head = newPqNode;
    } else {
        struct PqNode* current = *head;
        while (current->next != NULL && current->next->priority <= newPqNode->priority) {
            current = current->next;
        }
        newPqNode->next = current->next;
        current->next = newPqNode;
    }
    pq->size++;
}


int dequeue(PriorityQueue* pq) {
    struct PqNode** head = &pq->head;
    if (isEmpty(*head)) {
        printf("Queue is empty\n");
        return -1;
    }
    
    struct PqNode* temp = *head;
    int data = temp->data;
    *head = (*head)->next;
    free(temp);
    pq->size--;
    return data;
}

void displayPQ(struct PqNode* head) {
    struct PqNode* current = head;
    if (current == NULL) {
        printf("Queue is empty\n");
        return;
    }
    
    printf("Priority Queue elements: ");
    while (current != NULL) {
        printf("%d (p%d) ", current->data, current->priority);
        current = current->next;
    }
    printf("\n");
}


void freeQueue(struct PqNode** head) {
    struct PqNode* current = *head;
    while (current != NULL) {
        struct PqNode* temp = current;
        current = current->next;
        free(temp);
    }
    *head = NULL;
}