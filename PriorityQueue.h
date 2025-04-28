#ifndef PriorityQueeue_H
#define PriorityQueue_H

#include <stdlib.h>

struct PqNode {
    int data;
    int priority;
};
typedef struct {
    int size;
    int capacity;
    struct PqNode* array;
}  PriorityQueue;

void enqueue(PriorityQueue* pq, int data, int priority);
int dequeue(PriorityQueue* pq);
int peekPQ(PriorityQueue* pq);
int isEmpty(PriorityQueue* pq);
void freePQ(PriorityQueue* pq);
struct PqNode* createPqNode(int data, int priority);
PriorityQueue* createPriorityQueue(int capacity);

#endif