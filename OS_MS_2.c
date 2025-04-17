#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <ctype.h> 
#include <semaphore.h>
#include <pthread.h>
//#include "uthash.h"// didnt work at all sad


#define MAX_LINE_LENGTH 1024
#define MAX_VARS 100  // look i am not sure here but it isnt i wish we used c++
#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 1024

pthread_mutex_t mutexRW = PTHREAD_MUTEX_INITIALIZER;//Accessing a file, to read or to write.
pthread_mutex_t mutexInput = PTHREAD_MUTEX_INITIALIZER;//Taking user input
pthread_mutex_t mutexOutput = PTHREAD_MUTEX_INITIALIZER;//Outputting on the screen.

typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} Variable;

Variable variables[MAX_VARS];
int var_count = 0;

typedef struct {
    const char *name;
    pthread_mutex_t *mtx;
} NamedMutex;

static NamedMutex named_mutexes[] = {
    { "file",     &mutexRW     },
    { "userInput",  &mutexInput  },
    { "userOutput", &mutexOutput }
};
#define NUM_MUTEXES (sizeof(named_mutexes) / sizeof(named_mutexes[0]))


void trim(char *str) {
    if (str == NULL || *str == '\0') return; // Null or empty string
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (*start == '\0') {
        *str = '\0';
        return;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}


void process_print(char * args){
    trim(args);
    char *value = get_variable_value(args);
    if (value != NULL) {
        printf("%s = %s\n",args, value);
    } else {
        printf("value not found %s\n", args);
    }
}
void process_assign(char * args){
    char *name = strtok(args, " ");
    char *op = strtok(NULL, " ");
    char *value = strtok(NULL, ""); 
    trim(name);
    trim(op);
    trim(value);

    if (strncmp(op, "input", 5) == 0) {
        char user_input[MAX_VALUE_LENGTH];
        printf("Please enter a value \n");
        if (fgets(user_input, MAX_VALUE_LENGTH, stdin)) {
            size_t len = strlen(user_input);
            if (len > 0 && user_input[len-1] == '\n') {
                user_input[len-1] = '\0';
            }
            set_variable(name, user_input);
        }
    }else if (strncmp(op, "readFile", 8) == 0) {        
        process_read_file(value);
        set_variable(name,value);
    }else{
        set_variable(name,op);
    }
}
char* process_read_file(char* args){
    FILE *fptr = fopen(args,"r");

    char content[100];
    char res[10000000];
    if (fptr != NULL){
        while(fgets(content,100,fptr)){
            strcat(res,content);
        }
    }else
        printf("an error occured reading the file.");
    fclose(fptr);
    return res;
}

void process_write_file(char* args){

    char *space = strchr(args,' ');
    char name[100];
    char data[100];

    if(space == NULL)
        printf("Error: Invalid printFromTo format\n");
    
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
        printf("an error occured writing to the file.\n");

    fclose(fptr);
}


void process_print_from_to (char* args){

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
void sem_wait_resource(const char *name) {
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "sem_wait_resource: unknown mutex \"%s\"\n", name);
        return;
    }
    pthread_mutex_lock(named_mutexes[idx].mtx);  
}
void sem_signal_resource(const char *name) {
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "sem_signal_resource: unknown mutex \"%s\"\n", name);
        return;
    }
    pthread_mutex_unlock(named_mutexes[idx].mtx);
}

//variable

char* get_variable_value(char *name) {
    int index = find_variable(name);
    if (index != -1) {
        return variables[index].value;
    }
    return NULL;
}

void set_variable(char *name, char *value) {
    int index = find_variable(name);
    if (index != -1) {//existing variable
        strcpy(variables[index].value, value);
    } else if (var_count < MAX_VARS) {//new variable
        strcpy(variables[var_count].name, name);
        strcpy(variables[var_count].value, value);
        var_count++;
    } else {
        fprintf(stderr, "Error: Maximum number of variables reached\n");
    }
}

int find_variable(char *name){
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}



void File_Execution(char* program) {
    FILE *fptr = fopen(program,"r");
    char content[100];

    if(fptr != NULL) {
        while (fgets(content, 1000, fptr)) {
            printf("line %s",content);
            execute_line(content);
        }
    }
    else {
        printf("Error in opening file\n");
    }
    
    fclose(fptr);
}
void execute_line(char *line) {
    //skip empty lines and comments to end the exection might not be needed
    if (line[0] == '\0') {
        return;
    }
    
    //extract command and arguments dosent need max length
    char *command = strtok(line, " ");
    char *args    = strtok(NULL, "");

    static char empty_string[] = ""; 
    if (!args) args = empty_string;
    
    if (strcmp(command, "print") == 0) {
        process_print(args);
    } else if (strcmp(command, "assign") == 0) {
        process_assign(args);
    } else if (strcmp(command, "writeFile") == 0) {
        process_write_file(args);
    } else if (strcmp(command, "readFile") == 0) {
        process_read_file(args);
    } else if (strcmp(command, "printFromTo") == 0) {
        process_print_from_to(args);
    } else if (strcmp(command, "semWait") == 0) {
        sem_wait_resource(args);
    } else if (strcmp(command, "semSignal") == 0) {
        sem_signal_resource(args);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);//guys this outputs a formated error message
    }
}


//sem 

void initmutex(){
    if (pthread_mutex_init(&mutexRW,NULL) != 0 ||
     pthread_mutex_init(&mutexInput,NULL) != 0 ||
     pthread_mutex_init(&mutexOutput,NULL) != 0) {
        printf("error");
    }
}

int find_mutex(const char *name) {
    for (int i = 0; i < NUM_MUTEXES; i++) {
        if (strcmp(named_mutexes[i].name, name) == 0)
            return i;
    }
    return -1;
}
void* exec_1(void* args){
    File_Execution("Program_1.txt");
    pthread_exit(NULL);
}

void* exec_2(void* args){
    pthread_exit(NULL);
}

void* exec_2(void* args){
    File_Execution("Program_2.txt");
    pthread_exit(NULL);
}

void* exec_3(void* args){
    pthread_exit(NULL);
}

void* exec_3(void* args){
    File_Execution("Program_3.txt");
    pthread_exit(NULL);
}
int main(void) {
    initmutex();
    pthread_t thread_1, thread_2, thread_3;
    
    pthread_create(&thread_1,NULL,exec_1,NULL);
    pthread_create(&thread_2,NULL,exec_2,NULL);
    pthread_create(&thread_3,NULL,exec_3,NULL);

    return 0;
}

