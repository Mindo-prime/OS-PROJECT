#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>




void trim(char* str) {
    
    char *start = str;
    while (isspace((unsigned char)*start)) start++;

    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0'; 

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void print(char* args){
    
    printf("%s\n", args);

}

void assign(char* args){

    printf("in assign\n");

}

char* readFile(char* args){

    FILE *fptr = fopen(args,"r");

    char content[100];
    char result[10000];

    if (fptr != NULL){
        printf("Reading from file '%s':\n", args);
        printf("-------------------------\n");
        while(fgets(content,100,fptr)){
            strcat(result,content);
        }
    }else
        printf("an error occured reading the file.");

    printf("-------------------------\n");
    fclose(fptr);

    return result;
}

void writeFile(char* args){

    char *space = strchr(args,' ');
    char name[100];
    char data[100];

    if(space == NULL)
        printf("Error: Invalid writeFile format\n");
    
    else{
        int name_ln = space - args;
        if (name_ln >= 100)
        name_ln = 99;
        strncpy(name,args, name_ln);
        name[name_ln] = '\0';
        strncpy(data,space+1,99);
        data[99] = '\0';
    }

    trim(name);
    trim(data);

    FILE *fptr = fopen(name,"w");

    if (fptr != NULL){
        fputs(data,fptr);
        printf("\n");
    }else
        printf("an error occured writing to the file.");

    fclose(fptr);
}


void printFromTo (char* args){

    int a, b;

    char *space = strchr(args,' ');
    char num_1[10];
    char num_2[10];

    if(space == NULL)
        printf("Error: Invalid printFromTo format\n");

    int num_1_ln = space - args;
    if (num_1_ln >= 10)
        num_1_ln = 9;
    
    strncpy(num_1,args,num_1_ln);
    num_1[9] = '\0';
    strncpy(num_2,space+1,9);
    num_2[9] = '\0';

    trim(num_1);
    trim(num_2);

    a = atoi(num_1);
    b = atoi(num_2);

    for (int i = a; i<=b; i++){
        printf("%i\n",i);
    }
    printf("\n");
}

void semWait(char* args){

    printf("in semWait\n");

}

void semSignal(char* args){

    printf("in semSignal\n");

}


void Execute_Line(char* line){

    trim(line);

    char *space = strchr(line,' ');
    char command[100];
    char args[100];

    if(space == NULL){
        strncpy(command,line,99);
        command[99] = '\0';
        args[0] = '\0';
    }
    else{
        int cmd_ln = space - line;
        if (cmd_ln >= 100)
            cmd_ln = 99;
        strncpy(command,line, cmd_ln);
        command[cmd_ln] = '\0';
        strncpy(args,space+1,99);
        args[99] = '\0';
    }

    trim(command);
    trim(args);

    if(strcmp(command,"print")==0){
        print(args);
    }else if(strcmp(command,"assign")==0){
        assign(args);
    }else if(strcmp(command,"readFile")==0){
        readFile(args);
    }else if(strcmp(command,"writeFile")==0){
        writeFile(args);
    }else if(strcmp(command,"printFromTo")==0){
        printFromTo(args);
    }else if(strcmp(command,"semWait")==0){
        semWait(args);
    }else if(strcmp(command,"semSignal")==0){
        semSignal(args);
    }else{
        printf("Error: Unknown command '%s'\n", command);
    }

    //usleep(1000);
}


void File_Execution(char* program) {
    FILE *fptr = fopen(program,"r");
    char content[100];

    if(fptr != NULL) {
        while (fgets(content, 100, fptr)) {
            printf("\n%s",content);
            printf("-------------------------\n");
            Execute_Line(content);
        }
    }
    else {
        printf("Error in opening file\n");
    }
    
    fclose(fptr);
}



int main(void) {
    
    File_Execution("Program_1.txt");
    File_Execution("Program_2.txt");
    File_Execution("Program_3.txt");

    return 0;
}