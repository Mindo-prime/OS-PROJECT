#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    double start_time, end_time;
    double start_cpu_time, end_cpu_time;
} time_metrics;

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6; // Convert to milliseconds
}

double get_cpu_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

void initialize_time(time_metrics* t) {
    t->start_time = get_time_ms();
    t->start_cpu_time = get_cpu_time_ms();
}

void end_time(time_metrics* t) {
    t->end_time = get_time_ms();
    t->end_cpu_time = get_cpu_time_ms();
}

void* thread1(void* arg) {
    time_metrics* t = (time_metrics*) malloc(sizeof(time_metrics));
    initialize_time(t);
    char c1, c2;

    printf("Please input two alphabetic characters: ");
    scanf("%c %c", &c1, &c2);

    if(c1<c2) 
        for(char i = c1; i <= c2; i++) 
            printf("%c ", i); 
    else 
        for(char i = c2; i <= c1; i++)
            printf("%c ", i);
    printf("\n");
    end_time(t);
    return (void*) t;
}

void* thread2(void* args) {
    time_metrics* t = (time_metrics*) malloc(sizeof(time_metrics));
    initialize_time(t);
    printf("Thread 2 ID: %lu\n", pthread_self());
    printf("This statement indicated that thread 2 is executing.\n");
    printf("Thread 2 will now finish execution\n");
    end_time(t);
    return (void*) t;
}

void* thread3(void* args) {
    time_metrics* t = (time_metrics*) malloc(sizeof(time_metrics));
    initialize_time(t);
    int a, b;

    printf("Please input 2 integers: ");
    scanf("%d %d", &a, &b);

    int sum = a + b;
    printf("The sum of the two integers is: %d\n", sum);

    float average = sum/2;
    printf("The average of the two integers is: %.1f\n", average);

    int product = a*b;
    printf("The product of the two integers is %d\n", product);
    end_time(t);
    return (void*) t;
}

int main() {
    // Define CPU affinity set
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset); // Initialize the CPU set
    CPU_SET(0, &cpuset); // Assign execution to CPU 0

    pthread_t t1, t2, t3;
    pthread_attr_t attr, attr2; // Thread attributes
    struct sched_param param, param2;
    
    pthread_attr_init(&attr); // Initialize thread attributes
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset); // Set thread CPU affinity
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    pthread_attr_init(&attr2);
    pthread_attr_setschedpolicy(&attr2, SCHED_RR);
    
    param.sched_priority = 10;
    pthread_attr_setschedparam(&attr, &param);
    time_metrics *time1, *time2, *time3;

    pthread_create(&t2, &attr, thread2, NULL);
    pthread_create(&t1, &attr, thread1, NULL);
    pthread_create(&t3, &attr, thread3, NULL);

    pthread_join(t1, (void**) &time1);
    pthread_join(t2, (void**) &time2);
    pthread_join(t3, (void**) &time3);
    
    double tt1 = time1->end_time - time1->start_time;
    double tt2 = time2->end_time - time2->start_time;
    double tt3 = time3->end_time - time3->start_time;

    double et1 = time1->end_cpu_time - time1->start_cpu_time;
    double et2 = time2->end_cpu_time - time2->start_cpu_time;
    double et3 = time3->end_cpu_time - time3->start_cpu_time;

    double wt1 = tt1 - et1;
    double wt2 = tt2 - et2;
    double wt3 = tt3 - et3;

    printf("\n");
    printf("Turnaround time of thread 1: %.2f\n", tt1);
    printf("Execution time of thread 1: %.2f\n", et1);
    printf("Waiting time of thread 1: %.2f\n", wt1);
    printf("\n");
    printf("Turnaround time of thread 2: %.2f\n", tt2);
    printf("Execution time of thread 2: %.2f\n", et2);
    printf("Waiting time of thread 2: %.2f\n", wt2);
    printf("\n");
    printf("Turnaround time of thread 3: %.2f\n", tt3);
    printf("Execution time of thread 3: %.2f\n", et3);
    printf("Waiting time of thread 3: %.2f\n", wt3);
}