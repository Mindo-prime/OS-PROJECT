#define _GNU_SOURCE
#define POLICY SCHED_FIFO

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>


double get_time_ms(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

void* displayLetters(void* nichts){
   char char1,char2; 
   printf("Enter two alphabetic characters\n");
   scanf(" %c %c" ,&char1,&char2);
   char minchar = char1 < char2? char1 : char2;
   char maxchar = char1 > char2? char1 : char2;
   for (char c = minchar; c <=maxchar;c++){
        printf("%c\n", c);
   }

}

void* minThreePrintStatements( void* auchnichts){
    printf("minThreeprintStatements() is executing, this is the first print\n");
    printf("minThreeprintStatements() is executing, this is the second print\n");
    printf("minThreeprintStatements() is executing, this is the last print with thread ID: %lu\n", (unsigned long) pthread_self());
}

void* methFunction( void* ich_bin_echt_gelangweilt){
    int n1,n2;
    printf("enter two numbers schatze\n");
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


}


int main() {

    // Define CPU affinity set
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset); // Initialize the CPU set
    CPU_SET(0, &cpuset); // Assign execution to CPU 0

    double parallel_time;
    double start, end;
    struct sched_param param;
    pthread_t thread1,thread2,thread3;
    pthread_attr_t attr; // Thread attributes
    pthread_attr_init(&attr); // Initialize thread attributes
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset); // Set thread CPU affinity
    pthread_attr_setschedpolicy(&attr, POLICY);
    param.sched_priority = sched_get_priority_max(POLICY);
    pthread_attr_setschedparam(&attr,&param);

    start = get_time_ms(); // Start timing parallel execution

    pthread_create(&thread1, &attr, displayLetters, NULL);
    pthread_create(&thread2, &attr, minThreePrintStatements, NULL);
    pthread_create(&thread3, &attr, methFunction, NULL);
    printf("before thread join 1\n");
    pthread_join(thread1, NULL);
    printf("before thread join 2\n");
    pthread_join(thread2, NULL);
    printf("before thread join 3\n");
    pthread_join(thread3, NULL);
    printf("after thread join 3\n");

    end = get_time_ms(); // End timing parallel execution
    parallel_time = end - start;
    printf("Total parallel time: %.2f ms\n\n", parallel_time);
    printf("Main: Thread has finished executing\n");
    pthread_attr_destroy(&attr);
    return 0;
}
