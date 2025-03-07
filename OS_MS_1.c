#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


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
  
    pthread_t thread1,thread2,thread3;
    
    pthread_create(&thread1, NULL, displayLetters, NULL);
    pthread_create(&thread2, NULL, minThreePrintStatements, NULL);
    pthread_create(&thread3, NULL, methFunction, NULL);
    printf("1\n");
    pthread_join(thread1, NULL);
    printf("2\n");
    pthread_join(thread2, NULL);
    printf("3\n");
    pthread_join(thread3, NULL);
    printf("4\n");

    
    
    printf("Main: Thread has finished executing\n");
    return 0;
}
