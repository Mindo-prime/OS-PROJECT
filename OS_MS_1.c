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

// Mutex to prevent issues from occurring with shared resources between threads
pthread_mutex_t input_lock;

// Struct for measuring the performance metrics of a thread
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

// Function to get the current time in milliseconds
double get_time_ms(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

// Function to get the current cpu time in milliseconds - this excludes time that is not spend executing instructions
double get_cpu_time_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == -1) {
        perror("get_cpu_time_ms");
        return -1;
    }
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

void dummy_loop() {
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

void printMetrics(int threadNo, thread_metrics_t* metrics) {
    printf("---------------THREAD %d---------------\n", threadNo);
    printf("Release time of thread %d: %.5f ms\n", threadNo, metrics->release_time);
    printf("Start time of thread %d: %.5f ms\n", threadNo, metrics->start_time);
    printf("End time of thread %d: %.5f ms\n", threadNo, metrics->end_time);
    printf("Turnaround time of thread %d: %.5f ms\n", threadNo, metrics->turnaround_time);
    printf("Execution time of thread %d: %.5f ms\n", threadNo, metrics->cpu_time);
    printf("Waiting time of thread %d: %.5f ms\n", threadNo, metrics->waiting_time);
    printf("CPU utilization of thread %d: %.5f%%\n", threadNo, metrics->cpu_utilization);
    printf("Memory consumption of thread %d: %zu bytes\n", threadNo, metrics->memory_usage);
}

void* displayLetters(void* args){
    thread_metrics_t *metrics = (thread_metrics_t *)args;
    initialize_metrics(metrics);
    char char1,char2;
    printf("Enter two alphabetic characters separated by a space: \n");

    pthread_mutex_lock(&input_lock);
    scanf("%c %c" ,&char1,&char2);
    pthread_mutex_unlock(&input_lock);

    char minchar = char1 < char2? char1 : char2;
    char maxchar = char1 > char2? char1 : char2;
    for (char c = minchar; c <=maxchar;c++){
        printf("%c ", c);
    }
    printf("\n");
    
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

void* mathFunction(void* args){
    thread_metrics_t *metrics = (thread_metrics_t *) args;
    initialize_metrics(metrics);
    //dummy_loop();
    int n1, n2;
    printf("Enter two numbers separated by a space:\n");

    pthread_mutex_lock(&input_lock);
    scanf("%d %d", &n1, &n2);
    pthread_mutex_unlock(&input_lock);

    int lower_bound = n1 < n2 ? n1 : n2;
    int upper_bound = n1 > n2 ? n1 : n2;

    int sum = (((upper_bound + 1) * upper_bound) / 2) - ((lower_bound * (lower_bound - 1)) / 2);
    double avg = sum / (upper_bound - lower_bound +1);
    long long product=1;
    for (int i = lower_bound; i <= upper_bound; i++) {
        product *= i;
    }

    printf("The sum of all numbers between %d and %d is %d\n", n1, n2, sum);
    printf("The product of all numbers between %d and %d is %lld\n", n1, n2, product);
    printf("The average of the two numbers %d and %d is %f\n", n1, n2, avg);
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
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    param.sched_priority = sched_get_priority_max(POLICY);
    pthread_attr_setschedparam(&attr,&param);

    thread1_metrics.release_time = get_time_ms(); 
    pthread_create(&thread1, &attr, displayLetters, &thread1_metrics);
    printf("Thread 1 created\n");
    
    thread2_metrics.release_time = get_time_ms();
    pthread_create(&thread2, &attr, minThreePrintStatements, &thread2_metrics);
    printf("Thread 2 created\n");

    thread3_metrics.release_time = get_time_ms();
    pthread_create(&thread3, &attr, mathFunction, &thread3_metrics);
    printf("Thread 3 created\n");

    printf("Before thread join 1\n");
    pthread_join(thread1, NULL);
    printf("Before thread join 2\n");
    pthread_join(thread2, NULL);
    printf("Before thread join 3\n");
    pthread_join(thread3, NULL);
    printf("After all thread joins\n");

    printf("\n");
    printMetrics(1, &thread1_metrics);
    printf("\n");
    printMetrics(2, &thread2_metrics);
    printf("\n");
    printMetrics(3, &thread3_metrics);

    printf("\n---------------TOTAL---------------\n");
    double total_turnaround_time = thread1_metrics.turnaround_time + thread2_metrics.turnaround_time + thread3_metrics.turnaround_time;
    double total_execution_time = thread1_metrics.cpu_time + thread2_metrics.cpu_time + thread3_metrics.cpu_time;
    double total_waiting_time = thread1_metrics.waiting_time + thread2_metrics.waiting_time + thread3_metrics.waiting_time;
    size_t total_memory_usage = thread1_metrics.memory_usage + thread2_metrics.memory_usage + thread3_metrics.memory_usage;
    printf("Total turnaround time: %.5f ms\n", total_turnaround_time);
    printf("Total execution time: %.5f ms\n", total_execution_time); 
    printf("Total waiting time: %.5f ms\n", total_waiting_time); 
    printf("Total memory consumption: %zu bytes\n", total_memory_usage); 

    printf("\n---------------AVERAGE---------------\n");
    double average_turnaround_time = total_turnaround_time / 3;
    double average_execution_time = total_execution_time / 3;
    double average_waiting_time = total_waiting_time / 3;
    size_t average_memory_usage = total_memory_usage / 3;
    double average_cpu_utilization = (thread1_metrics.cpu_utilization + thread2_metrics.cpu_utilization + thread3_metrics.cpu_utilization) / 3;
    printf("Average turnaround time: %.5f ms\n", average_turnaround_time / 3);
    printf("Average execution time: %.5f ms\n", average_execution_time); 
    printf("Average waiting time: %.5f ms\n", average_waiting_time); 
    printf("Average CPU utilization: %.5f%%\n", average_cpu_utilization);
    printf("Average memory consumption: %zu bytes\n", average_memory_usage); 

    printf("\n");
    printf("Main thread has finished executing\n");
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&input_lock);
    pthread_exit(NULL);
    return 0;
}
