#ifndef PriorityQueeue_H
#define PriorityQueue_H

#include <stdlib.h>

struct PqNode {
    int data;
    int priority;
    struct PqNode* next;
};
typedef struct {
    struct PqNode* head;
    int size;
}  PriorityQueue;

void enqueue(PriorityQueue* pq, int data, int priority);
int dequeue(PriorityQueue* pq);
void displayPQ(struct PqNode* head);
int isEmpty(struct PqNode* head);
struct PqNode* createPqNode(int data, int priority);

#endif