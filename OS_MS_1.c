#define _GNU_SOURCE
#define POLICY SCHED_FIFO
// #define POLICY SCHED_RR

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h> 

// Define a Structure to hof the Start and End times along with the Memory Usage of a Thread
typedef struct {
    double release_time;   
    double start_time;      // When thread starts executiing
    double end_time;        // When thread finishes executing
    double cpu_start;       // When thread starts executing (CPU time)
    double cpu_end;         // When thread finishes executing (CPU time, waiting time is not counted)
    double waiting_time;    // Time spent waiting to be scheduled
    double response_time;   // Time from release to first execution
    double turnaround_time; // Time from release to completion
    double cpu_time;        // Actual CPU time used (from CLOCK_THREAD_CPUTIME_ID)
    double cpu_utilization; // CPU time / turnaround time
    size_t memory_usage; // Memory consumption 
    size_t stack_size;
} thread_metrics_t;

double release_time;

// Function to get the current time in milliseconds
double get_time_ms(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

double get_cpu_time_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == -1) {
        perror("get_cpu_time_ms");
        return -1;
    }
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

void dummy_loop(){
    int sum = 0;
    for(int i = 0; i<1e8; i++){
        sum += 1;
    }
}

void initialize_metrics(thread_metrics_t* metrics) {
    metrics->start_time = get_time_ms();
    metrics->cpu_start = get_cpu_time_ms();
}

void finalize_memory_usage(thread_metrics_t* metrics) {
    pthread_attr_t attr;
    void *stack_addr;
    size_t stack_size;
    // Get this thread's attributes
    if (pthread_getattr_np(pthread_self(), &attr) != 0) {
        perror("pthread_getattr_np");
        return;
    }

    // Retrieve stack address and size
    if (pthread_attr_getstack(&attr, &stack_addr, &stack_size) != 0) {
        perror("pthread_attr_getstack");
        pthread_attr_destroy(&attr);
        return;
    }

    // Create a new variable on the current thread to approximate the position of the thread's stack pointer
    int thread_var; 
    uintptr_t stack_ptr = (uintptr_t)&thread_var;
    metrics->memory_usage = (uintptr_t)stack_addr + stack_size - stack_ptr;
    metrics->stack_size = stack_size;

    pthread_attr_destroy(&attr);
}

void finalize_metrics(thread_metrics_t* metrics) {
    metrics->end_time = get_time_ms();
    metrics->cpu_end = get_cpu_time_ms();
    metrics->turnaround_time = metrics->end_time - metrics->release_time;
    metrics->cpu_time = metrics->cpu_end - metrics->cpu_start;
    metrics->waiting_time = metrics->turnaround_time - metrics->cpu_time;
    metrics->response_time = metrics->start_time - metrics-> release_time;
    metrics->cpu_utilization = (metrics->cpu_time / metrics->turnaround_time) * 100.0;
    finalize_memory_usage(metrics);
}

void* displayLetters(void* args){
    thread_metrics_t *metrics = (thread_metrics_t *)args;
    initialize_metrics(metrics);
    char char1,char2;
    printf("Enter two alphabetic characters\n");
    scanf(" %c %c" ,&char1,&char2);
    char minchar = char1 < char2? char1 : char2;
    char maxchar = char1 > char2? char1 : char2;
    for (char c = minchar; c <=maxchar;c++){
        printf("%c\n", c);
    }
    
    //dummy_loop();
    finalize_metrics(metrics);
    pthread_exit(NULL);
}

void* minThreePrintStatements( void* args){ 
    thread_metrics_t *metrics = (thread_metrics_t *)args;
    initialize_metrics(metrics);
    //dummy_loop();
    printf("minThreeprintStatements() is executing, this is the first print\n");
    printf("minThreeprintStatements() is executing, this is the second print\n");
    printf("minThreeprintStatements() is executing, this is the last print with thread ID: %lu\n", (unsigned long) pthread_self());
    finalize_metrics(metrics);
    pthread_exit(NULL);
}

void* mathFunction( void* args){
    thread_metrics_t *metrics = (thread_metrics_t *)args;
    initialize_metrics(metrics);
    //dummy_loop();
    int n1,n2;
    printf("enter two numbers please\n");
    scanf("%d %d", &n1, &n2);
    int lb = n1<n2? n1 : n2,
    ub = n1>n2? n1 : n2;
    int sum = (((ub+1)* ub )/2) - (( lb * (lb-1))/2);
    double avg = sum / (ub-lb+1);
    long product=1;
    for (int i=lb;i<=ub;i++){
        product*=i;
    }
    printf("The sum of all numbers between %d %d is %d\nwhile the average is %lf while the product is %lu\n",lb,ub,sum,avg,product);
    finalize_metrics(metrics);
    pthread_exit(NULL);
}


int main() {

    // Bound the CPU to 1 Core
    cpu_set_t cpuset; // Define CPU affinity set
    CPU_ZERO(&cpuset); // Initialize the CPU set
    CPU_SET(0, &cpuset); // Assign execution to CPU 0

    double total_exec_time;
    double cpu_utilization ,start_time;
    thread_metrics_t thread1_metrics, thread2_metrics, thread3_metrics;

    struct sched_param param;

    pthread_t thread1,thread2,thread3;

    pthread_attr_t attr; // Thread attributes
    pthread_attr_init(&attr); // Initialize thread attributes
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset); // Set thread CPU affinity

    // Explicitly defining the scheduling policy. This requires administrator (sudo) privileges.
    pthread_attr_setschedpolicy(&attr, POLICY);
    pthread_attr_getinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    param.sched_priority = sched_get_priority_max(POLICY);
    pthread_attr_setschedparam(&attr,&param);

    double release_time = get_time_ms();// Start timing parallel execution 

    thread1_metrics.release_time = release_time; 
    thread2_metrics.release_time = release_time;
    thread3_metrics.release_time = release_time;

    pthread_create(&thread1, &attr, displayLetters, &thread1_metrics);
    pthread_create(&thread2, &attr, minThreePrintStatements, &thread2_metrics);
    pthread_create(&thread3, &attr, mathFunction, &thread3_metrics);

    printf("before thread join 1\n");
    pthread_join(thread1, NULL);
    printf("before thread join 2\n");
    pthread_join(thread2, NULL);
    printf("before thread join 3\n");
    pthread_join(thread3, NULL);
    printf("after thread join 3\n");

    total_exec_time = thread1_metrics.cpu_time + thread2_metrics.cpu_time + thread3_metrics.cpu_time; // Sum of waiting time

    size_t total_memory_consumption = thread1_metrics.memory_usage + thread2_metrics.memory_usage + thread3_metrics.memory_usage;
    

    printf("Execution Time: %f ms\n\n", total_exec_time); // 1

    printf("Release Time for all: %f ms\n\n", release_time); // 2

    // Start excution time :> ali
    printf("Start Time 1: %f ms\n\n", thread1_metrics.start_time); // 3
    printf("Start Time 2: %f ms\n\n", thread2_metrics.start_time); // 3
    printf("Start Time 3: %f ms\n\n", thread3_metrics.start_time); // 3

    // End excution time :> ali
    printf("End Time 1: %f ms\n\n", thread1_metrics.end_time); // 4
    printf("End Time 2: %f ms\n\n", thread2_metrics.end_time); // 4
    printf("End Time 3: %f ms\n\n", thread3_metrics.end_time); // 4

    // For the Wait Time Function (hehe W.T.F) <- Mindo
    printf("Waiting Time 1: %f ms\n\n", thread1_metrics.waiting_time); // 5
    printf("Waiting Time 2: %f ms\n\n", thread2_metrics.waiting_time); // 5
    printf("Waiting Time 3: %f ms\n\n", thread3_metrics.waiting_time); // 5
    printf("Waiting Time sum: %f ms\n\n", thread1_metrics.waiting_time + thread2_metrics.waiting_time + thread3_metrics.waiting_time); // 5
    printf("Waiting Time avg: %f ms\n\n", (thread1_metrics.waiting_time + thread2_metrics.waiting_time + thread3_metrics.waiting_time)/3); // 5

    // For the Response Time
    printf("Response Time 1: %f ms\n\n", thread1_metrics.response_time); // 6
    printf("Response Time 2: %f ms\n\n", thread2_metrics.response_time); // 6
    printf("Response Time 3: %f ms\n\n", thread3_metrics.response_time); // 6
    printf("Response Time sum: %f ms\n\n", thread1_metrics.response_time + thread2_metrics.response_time + thread3_metrics.response_time); // 6
    printf("Response Time avg: %f ms\n\n", (thread1_metrics.response_time + thread2_metrics.response_time + thread3_metrics.response_time)/3); // 6

    // For the Turnaround Time
    printf("Turnaround Time 1: %f ms\n\n", thread1_metrics.turnaround_time); // 7
    printf("Turnaround Time 2: %f ms\n\n", thread2_metrics.turnaround_time); // 7
    printf("Turnaround Time 3: %f ms\n\n", thread3_metrics.turnaround_time); // 7
    printf("Turnaround Time sum: %f ms\n\n", thread1_metrics.turnaround_time + thread2_metrics.turnaround_time + thread3_metrics.turnaround_time); // 7
    printf("Turnaround Time avg: %f ms\n\n", (thread1_metrics.turnaround_time + thread2_metrics.turnaround_time + thread3_metrics.turnaround_time)/3); // 7

    // 8

    // For the CPU utilization
    printf("CPU Utilization 1: %f %%\n\n", thread1_metrics.cpu_utilization); // 9
    printf("CPU Utilization 2: %f %%\n\n", thread2_metrics.cpu_utilization); // 9
    printf("CPU Utilization 3: %f %%\n\n", thread3_metrics.cpu_utilization); // 9
    printf("CPU Utilization sum: %f %%\n\n", thread1_metrics.cpu_utilization + thread2_metrics.cpu_utilization + thread3_metrics.cpu_utilization); // 9
    printf("CPU Utilization avg: %f %%\n\n", (thread1_metrics.cpu_utilization + thread2_metrics.cpu_utilization + thread3_metrics.cpu_utilization)/3); // 9

    // For the Memory Consumption
    printf("Memory Consumption 1: %zu bytes\n\n", thread1_metrics.memory_usage); // 10
    printf("Memory Consumption 2: %zu bytes\n\n", thread2_metrics.memory_usage); // 11
    printf("Memory Consumption 3: %zu bytes\n\n", thread3_metrics.memory_usage); // 12
    printf("Memory Consumption sum: %zu bytes\n\n", total_memory_consumption); // 13

    printf("Main: Thread has finished executing\n");
    pthread_attr_destroy(&attr);
    pthread_exit(NULL);
    return 0;
}
