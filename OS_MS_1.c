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

// Define a Structure to hold the Start and End times along with the Memory Usage of a Thread
typedef struct {
    double start_time;
    double end_time;
    size_t memory_usage;
} thread_Specs_s;

// Function to get the current time in milliseconds
double get_time_ms(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

double timespec_to_ms(struct timespec *ts) {
    return (double)ts->tv_sec * 1000.0 + (double)ts->tv_nsec / 1000000.0;
}

void dummy_loop(){
    int sum = 0;
    for(int i = 0; i<1e10; i++){
        sum += 1;
    }
}

void* displayLetters(void* args){
    struct timespec start_exe, end_exe;
    thread_Specs_s *Specs = (thread_Specs_s*) args;
    Specs -> start_time = get_time_ms();
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_exe) == -1) {
        perror("clock_gettime start");
        return NULL;
    }

    char char1,char2;
    printf("Enter two alphabetic characters\n");
    scanf(" %c %c" ,&char1,&char2);
    char minchar = char1 < char2? char1 : char2;
    char maxchar = char1 > char2? char1 : char2;
    for (char c = minchar; c <=maxchar;c++){
        printf("%c\n", c);
    }

    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_exe) == -1) {
        perror("clock_gettime end");
        return NULL;
    }
    
    dummy_loop();

    Specs -> end_time = get_time_ms();
    Specs -> memory_usage = malloc_usable_size(Specs);
    
    
    // Calculate elapsed CPU time
    double *cpu_time = malloc(sizeof(double));
    if (cpu_time == NULL) {
        perror("malloc");
        return NULL;
    }
    
    *cpu_time = timespec_to_ms(&end_exe) - timespec_to_ms(&start_exe);
    printf("$d /n",&cpu_time);
    pthread_exit(NULL);
    
}

void* minThreePrintStatements( void* args){
    
    thread_Specs_s *Specs = (thread_Specs_s*) args;
    Specs -> start_time = get_time_ms();

    printf("minThreeprintStatements() is executing, this is the first print\n");
    printf("minThreeprintStatements() is executing, this is the second print\n");
    printf("minThreeprintStatements() is executing, this is the last print with thread ID: %lu\n", (unsigned long) pthread_self());
    
    Specs -> end_time = get_time_ms();
    Specs -> memory_usage = sizeof(thread_Specs_s);
    pthread_exit(NULL);
}

void* methFunction( void* args){
    
    thread_Specs_s *Specs = (thread_Specs_s*) args;
    Specs -> start_time = get_time_ms();

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
    
    Specs -> end_time = get_time_ms();
    Specs -> memory_usage = malloc_usable_size(Specs);
    pthread_exit(NULL);
}


int main() {

    // Bound the CPU to 1 Core
    cpu_set_t cpuset; // Define CPU affinity set
    CPU_ZERO(&cpuset); // Initialize the CPU set
    CPU_SET(0, &cpuset); // Assign execution to CPU 0

    double elapsed_time; // used for multi-threading time calculation
    double start, end, exec_time1, exec_time2, exec_time3; // start is the Release Time
    double memory_usage1, memory_usage2, memory_usage3;
    double start_exec, end_exec, total_exec_time;
    double waiting_time, response_time, turnaround_time;
    double cpu_utilization, total_memory_consumption;
    thread_Specs_s Specs1, Specs2, Specs3;

    struct sched_param param;

    pthread_t thread1,thread2,thread3;

    pthread_attr_t attr; // Thread attributes
    pthread_attr_init(&attr); // Initialize thread attributes
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset); // Set thread CPU affinity

    pthread_attr_setschedpolicy(&attr, POLICY);
    param.sched_priority = sched_get_priority_max(POLICY);
    pthread_attr_setschedparam(&attr,&param);

    start = get_time_ms(); // Start timing parallel execution 

    pthread_create(&thread1, &attr, displayLetters, &Specs1);
    pthread_create(&thread2, &attr, minThreePrintStatements, &Specs2);
    pthread_create(&thread3, &attr, methFunction, &Specs3);

    printf("before thread join 1\n");
    pthread_join(thread1, NULL);
    printf("before thread join 2\n");
    pthread_join(thread2, NULL);
    printf("before thread join 3\n");
    pthread_join(thread3, NULL);
    printf("after thread join 3\n");

    end = get_time_ms(); // End timing parallel execution 

    exec_time1 = Specs1.end_time - Specs1.start_time;
    exec_time2 = Specs2.end_time - Specs2.start_time;
    exec_time3 = Specs3.end_time - Specs3.start_time;
    total_exec_time = exec_time1 + exec_time2 + exec_time3; // Sum of waiting time

    memory_usage1 = Specs1.memory_usage;
    memory_usage2 = Specs2.memory_usage;
    memory_usage3 = Specs3.memory_usage;
    total_memory_consumption = memory_usage1 + memory_usage2 + memory_usage3;
    
    start_exec = Specs1.start_time + Specs2.start_time + Specs3.start_time;
    end_exec = Specs1.start_time + Specs2.start_time + Specs3.start_time;

    response_time = start_exec - start;
    turnaround_time = end_exec - start;
    

    printf("Execution Time: %.2f ms\n\n", total_exec_time); // 1
    printf("Release Time: %.2f ms\n\n", start); // 2

    printf("Start Time: %.2f ms\n\n", start_exec); // 3
    printf("End Time: %.2f ms\n\n", end_exec); // 4

    // For the Wait Time Function (hehe W.T.F) <- Mindo
    printf("Waiting Time: %.2f ms\n\n", waiting_time); // 5

    // For the Response Time
    printf("Response Time: %.2f ms\n\n", response_time); // 6

    // For the Turnaround Time
    printf("Turnaround Time: %.2f ms\n\n", turnaround_time); // 7

    // 8

    // For the CPU utilization
    printf("CPU Utilization: %.2f ms\n\n", cpu_utilization); // 9

    // For the Memory Consumption
    printf("Memory Consumption 1: %.2f ms\n\n", memory_usage1); // 10
    printf("Memory Consumption 2: %.2f ms\n\n", memory_usage2); // 11
    printf("Memory Consumption 3: %.2f ms\n\n", memory_usage3); // 12
    printf("Memory Consumption: %.2f ms\n\n", total_memory_consumption); // 13

    printf("Total Elapsed Time: %.2f ms\n\n", elapsed_time);

    printf("Main: Thread has finished executing\n");
    pthread_attr_destroy(&attr);
    pthread_exit(NULL);
    return 0;
}
