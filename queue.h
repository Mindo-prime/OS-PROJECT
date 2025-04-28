#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>


typedef struct node {
    int value;
    struct node* next;
} node;


typedef struct {
    node* tail;
    int size;
} queue;


void init_queue(queue* q);
void push(queue* q, int value);
int pop(queue* q);
int peek(queue* q);

#endif 