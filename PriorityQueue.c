#include "PriorityQueue.h"
#include <stdio.h>

//This will be based on a min binary-heap
//size would also be the index of the last element (1 based indexing)
struct PqNode* createPqNode(int data, int priority) {
    struct PqNode* newPqNode = (struct PqNode*)malloc(sizeof(struct PqNode));
    if (newPqNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newPqNode->data = data;
    newPqNode->priority = priority;
    
    return newPqNode;
}

PriorityQueue* createPriorityQueue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    if (pq == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    pq->size = 0;
    pq->capacity = capacity;

    //I ignore index 0 for simpler artithmetic 
    pq->array = (struct PqNode*)malloc((capacity+1) * sizeof(struct PqNode));
    
    return pq;
}

int isEmpty(PriorityQueue* pq) {
    return pq->size == 0;
}

void siftUp(PriorityQueue* pq, int index) {
  int parentIndex = index / 2;
  while (index >1 && pq->array[index].priority < pq->array[parentIndex].priority){
    struct PqNode temp = pq->array[index];
    pq->array[index] = pq->array[parentIndex];
    pq->array[parentIndex] = temp;
    index = parentIndex;
    parentIndex = index /2;
  }
}

void siftDown(PriorityQueue* pq, int index) {
    int leftChild = index*2;
    int rightChild = index*2 +1;
    while ( (leftChild<=pq->size && pq->array[index].priority > pq->array[leftChild].priority) 
    || (rightChild<=pq->size && pq->array[index].priority > pq->array[rightChild].priority)){
      int targetSwapIndex = leftChild;
      if (rightChild<=pq->size && pq->array[leftChild].priority > pq->array[rightChild].priority)
        targetSwapIndex = rightChild;
      struct PqNode temp = pq->array[index];
      pq->array[index] = pq->array[targetSwapIndex];
      pq->array[targetSwapIndex] = temp;
      index = targetSwapIndex;
      leftChild = index *2;
      rightChild = index*2+1;
    }
  }

    


void enqueue(PriorityQueue* pq, int data, int priority) {
    if (pq->capacity==pq->size){
        pq->array = (struct PqNode*)realloc(pq->array, (pq->capacity * 2 + 1) * sizeof(struct PqNode));
        pq->capacity *=2;
    }
    pq->size++;
    pq->array[pq->size] = *createPqNode(data,priority);
    siftUp(pq,pq->size);
}


int dequeue(PriorityQueue* pq) {
    int ans = pq->array[1].data;
    pq->array[1] = pq->array[pq->size];
    siftDown(pq,1);
    pq->size--;
    return ans;
}

int peekPQ(PriorityQueue* pq){
    return pq->array[1].data;
}


void freePQ(PriorityQueue* pq) {
    free(pq->array);
    free(pq);
}

/*int main(void){
    PriorityQueue* pq = createPriorityQueue(3);
    enqueue(pq,5,5);
    printf("peeking: %d  size: %d capacity: %d\n",peekPQ(pq),pq->size,pq->capacity);
    enqueue(pq,2,2);
    printf("peeking: %d  size: %d capacity: %d\n",peekPQ(pq),pq->size,pq->capacity);
    enqueue(pq,3,3);
    printf("peeking: %d  size: %d capacity: %d\n",peekPQ(pq),pq->size,pq->capacity);
    enqueue(pq,6,6);
    printf("peeking: %d  size: %d capacity: %d\n",peekPQ(pq),pq->size,pq->capacity);
    enqueue(pq,1,1);
    enqueue(pq,4,4);
    while(!isEmpty(pq)){
        printf("popping: %d  size: %d capacity: %d\n",dequeue(pq),pq->size,pq->capacity);
    }
    freePQ(pq);


}*/

