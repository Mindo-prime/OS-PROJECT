#include <stdio.h>



void File_Execution(char* program) {
    FILE *fptr = fopen(program,"r");
    char content[1000];

    if(fptr != NULL) {
        while (fgets(content, 1000, fptr)) {
            printf("%s",content);
        }
    }
    else {
        printf("Error in opening file\n");
    }

    printf("\n");
    printf("-------------------------\n");
    fclose(fptr);
}



int main(void) {

    File_Execution("Program_1.txt");
    File_Execution("Program_2.txt");
    File_Execution("Program_3.txt");

    return 0;
}