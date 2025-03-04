#include <stdio.h>
#include <stdlib.h>
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

int main() {
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_join(t1, NULL);
    
}