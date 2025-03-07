#include <stdio.h>

int main(){

    FILE *fptr = fopen("text.txt","r");

    char content[1000];

    if (fptr != NULL){
        while(fgets(content,1000,fptr)){
            printf("%s", content);
        }
    }else
        printf("an error occured reading the file.");

    fclose(fptr);
    return 0;
}
