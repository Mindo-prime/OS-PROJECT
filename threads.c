#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <ctype.h>

void* thread1(void* arg) {
    char c1, c2;

    printf("Please input two alphabetic characters.\n");
    printf("First Character: "); 
    scanf("%c", &c1);
    printf("Second Character: ");
    scanf(" %c", &c2);

    if(c1<c2) 
        for(char i = c1; i <= c2; i++) 
            printf("%c ", i); 
    else 
        for(char i = c2; i <= c1; i++)
            printf("%c ", i);
    printf("\n");
}

void* thread2(void* args) {
    printf("Thread 2 ID: %lu\n", pthread_self());
    printf("This statement indicated that thread 2 is executing.\n");
    printf("Thread 2 will now finish execution\n");
}

void* thread3(void* args) {
    int a, b;

    printf("Please input 2 integers.");
    scanf("%c %c", &a, &b);

    int sum = a + b;
    printf("The sum of the two integers is: %d", sum);

    float average = sum/2;
    printf("The average of the two integers is: %.1f", average);

    int product = a*b;
    printf("The product of the two integers is %d", product);
}

void testLoop() {
    for (int i = 0; i < __INT_MAX__; i++) {}
}

int main() {
    // Define CPU affinity set
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset); // Initialize the CPU set
    CPU_SET(0, &cpuset); // Assign execution to CPU 0

    pthread_t t1, t2, t3;
    pthread_attr_t attr1,attr2; // Thread attributes
    struct sched_param param;
    int ret;

    pthread_attr_init(&attr1); // Initialize thread attributes
    ret = pthread_attr_setschedpolicy(&attr1, SCHED_FIFO);
    if (ret != 0) {
        perror("Thread 1: Unable to set scheduling policy (SCHED_FIFO)");
    }
    param.sched_priority = 1; // Example priority; adjust as needed

    ret = pthread_attr_setschedparam(&attr1, &param);
    if (ret != 0) {
        perror("Thread 1: Unable to set scheduling parameter");
    }
    pthread_attr_setinheritsched(&attr1, PTHREAD_EXPLICIT_SCHED);

    pthread_attr_init(&attr2); // Initialize thread attributes
    ret = pthread_attr_setschedpolicy(&attr2, SCHED_RR);
    if (ret != 0) {
        perror("Thread 1: Unable to set scheduling policy (SCHED_FIFO)");
    }
    param.sched_priority = 1; // Example priority; adjust as needed

    ret = pthread_attr_setschedparam(&attr2, &param);
    if (ret != 0) {
        perror("Thread 1: Unable to set scheduling parameter");
    }
    pthread_attr_setinheritsched(&attr2, PTHREAD_EXPLICIT_SCHED);


    pthread_attr_setaffinity_np(&attr1, sizeof(cpu_set_t), &cpuset); // Set thread CPU affinity
    pthread_attr_setaffinity_np(&attr2, sizeof(cpu_set_t), &cpuset); // Set thread CPU affinity

    pthread_create(&t1, &attr1, thread1, NULL);
    pthread_create(&t2, &attr1, thread2, NULL);
    pthread_create(&t3, &attr1, thread3, NULL);

    // pthread_create(&t1, &attr1, thread1, NULL);
    // pthread_create(&t2, &attr1, thread2, NULL);
    // pthread_create(&t3, &attr1, thread3, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
}