#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <ctype.h> 
#include <semaphore.h>
#include <pthread.h>
//#include "uthash.h"// didnt work at all sad


#define MAX_LINE_LENGTH 1024
#define MAX_VARS_PER 100  // look i am not sure here but it isnt i wish we used c++
#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 1024
#define MEMORY_SIZE 60 // Because we need 60 words bruh 
#define MAX_PROCESSES 3 // only three i think for now
#define NUM_PCB 6 //NUmber of vaariables in the PCB guys we might need to increase it UwU 

pthread_mutex_t mutexRW = PTHREAD_MUTEX_INITIALIZER;//Accessing a file, to read or to write.
pthread_mutex_t mutexInput = PTHREAD_MUTEX_INITIALIZER;//Taking user input
pthread_mutex_t mutexOutput = PTHREAD_MUTEX_INITIALIZER;//Outputting on the screen.

pthread_mutex_t mutexMemory = PTHREAD_MUTEX_INITIALIZER;// Surprise bros we got another

// Process states
typedef enum {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

// Memory Type
typedef enum {
    VAR,
    CODE,
    T_PCB,
    GAY,
    MINDO
} type;

// Memory word
typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int process_id;   // To track which process owns this memory word
    type type;       // 0 = var, 1 = pcb, 2 = code
} MemoryWord;

// PCB
typedef struct {
    int process_id;
    ProcessState state;
    int priority;
    int program_counter;
    int lower_bound; 
    int upper_bound; 
} PCB;

MemoryWord memory[MEMORY_SIZE];
int memory_allocated = 0;

PCB processes[MAX_PROCESSES];
int process_count = 0;

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


void process_print(char * args,int process_id){
    trim(args);
    char *value = get_variable_value(args,process_id);
    if (value != NULL) {
        printf("%s = %s\n",args, value);
    } else {
        printf("value not found %s\n", args);
    }
}
void process_assign(char * args,int process_id){
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
            set_variable(name, user_input,process_id);
        }
    }else if (strncmp(op, "readFile", 8) == 0) {        
        process_read_file(value);
        set_variable(name,value,process_id);
    }else{
        set_variable(name,op,process_id);
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

char* get_variable_value(char *name, int process_id) {
    int index = find_variable(name, process_id);
    if (index != -1) {
        return memory[index].value;
    }
    return NULL;
}

void set_variable(char *name, char *value, int process_id) {
    int index = find_variable(name, process_id);
    pthread_mutex_lock(&mutexMemory);
    if (index != -1) {
        strcpy(memory[index].value, value);
    } else {
        for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
            if (memory[i].type == VAR && memory[i].name[0] == '\0') {
                strcpy(memory[i].name, name);
                strcpy(memory[i].value, value);
                pthread_mutex_unlock(&mutexMemory);
                return;
            }
        }
        printf("Error: No variable space available for process %d\n", process_id);
    }
    pthread_mutex_unlock(&mutexMemory);
}

int find_variable(char *name, int process_id) {
    pthread_mutex_lock(&mutexMemory);
    for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
        if (memory[i].type == VAR && strcmp(memory[i].name, name) == 0) {
            pthread_mutex_unlock(&mutexMemory);
            return i;
        }
    }
    pthread_mutex_unlock(&mutexMemory);
    return -1;
}

void execute_line(char *line,int process_id) {
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
        process_print(args,process_id);
    } else if (strcmp(command, "assign") == 0) {
        process_assign(args,process_id);
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

    pthread_mutex_lock(&mutexMemory);
    for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "PC") == 0) {
            int pc = atoi(memory[i].value);//from ASCII to int 
            processes[process_id-1].program_counter++;
            pc++;
            sprintf(memory[i].value, "%d", pc);//covert to string
            break;
        }
    }
    pthread_mutex_unlock(&mutexMemory);
}

//memory

// Initialize memory
void init_memory() {
    pthread_mutex_lock(&mutexMemory);
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i].name[0] = '\0';
        memory[i].value[0] = '\0';
        memory[i].process_id = -1;  // -1 indicates free memory no with any process
        memory[i].type = MINDO;
    }
    memory_allocated = 0;
    pthread_mutex_unlock(&mutexMemory);
}

//sem 

void init_mutex(){
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

int create_process(char *program, int priority){//hot dog and frezzer you need this part miss you guys can i help with this tooo pleeeeasssee
    pthread_mutex_lock(&mutexMemory);
    if (process_count >= MAX_PROCESSES) {
        printf("Error: Out of process (Ask MINDO)\n");
        pthread_mutex_unlock(&mutexMemory);
        return -1;
    }//We are thinking of everything :>
    if (memory_allocated >= MEMORY_SIZE){
        printf("Error: Out of memory (Ask Ahmed hassen)\n");
        pthread_mutex_unlock(&mutexMemory);
        return -1;
    }
    FILE *fptr = fopen(program,"r");
    if (fptr == NULL) {
        printf("Error: Dumb ass wrong file name %s\n", program);
        pthread_mutex_unlock(&mutexMemory);
        return -1;
    }
    int line_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, fptr)) {
        if (strlen(line) > 1) {  //Skip empty lines my friend told me to do this (Claude)
            line_count++;
        }
    }
    if (memory_allocated + line_count + NUM_PCB > MEMORY_SIZE) {//Just the needed memory
        printf("Error: Not enough memory for process\n");
        fclose(fptr);
        pthread_mutex_unlock(&mutexMemory);
        return -1;
    }
    if (line_count + NUM_PCB + MAX_VARS_PER > 60) {//Just the needed memory
        printf("Error: Not enough memory for process must be less than 60 words\n");
        fclose(fptr);
        pthread_mutex_unlock(&mutexMemory);
        return -1;
    }
    int pid = process_count + 1;
    int start_index = memory_allocated;
    
    processes[process_count].process_id = pid;
    processes[process_count].state = NEW;//check the lecture
    processes[process_count].priority = priority;//your call bros?? scheculing team Hotdog and Frezz
    processes[process_count].program_counter = 0;//is it 0 or 1
    processes[process_count].lower_bound = start_index;
    processes[process_count].upper_bound = start_index + line_count + NUM_PCB + MAX_VARS_PER - 1;//just the memeory we need
    
    int mem_index = start_index;

    //PCB

    strcpy(memory[mem_index].name, "Process_ID");//saves name of the thingy we saved   
    sprintf(memory[mem_index].value, "%d", pid);//saves pid
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;//pcb
    mem_index++;

    strcpy(memory[mem_index].name, "Process_ID");
    sprintf(memory[mem_index].value, "%d", pid);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "State");
    strcpy(memory[mem_index].value, "NEW");
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "Priority");
    sprintf(memory[mem_index].value, "%d", 0);// Hotdog and frezzez it is your master once again asking you can he help you guys
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "PC");
    strcpy(memory[mem_index].value, "0");
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "Memory_Bounds");
    sprintf(memory[mem_index].value, "%d-%d", start_index, start_index + line_count + NUM_PCB + MAX_VARS_PER- 1);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;

    //CODE

    rewind(fptr);
    int line_index = 0;
    while (fgets(line, MAX_LINE_LENGTH, fptr) && mem_index < MEMORY_SIZE) {
        if (strlen(line) > 1) {  // Skip empty lines my friend
            line[strcspn(line, "\n")] = 0;
            
            sprintf(memory[mem_index].name, "Line_%d", line_index);
            strcpy(memory[mem_index].value, line);
            memory[mem_index].process_id = pid;
            memory[mem_index].type = CODE;//code
            mem_index++;
            line_index++;
        }
    }
    
    fclose(fptr);

    //VAR

    for (int i = 0; i < MAX_VARS_PER; i++) {
        memory[mem_index].name[0] = '\0'; 
        memory[mem_index].value[0] = '\0';
        memory[mem_index].process_id = pid;
        memory[mem_index].type = VAR;//var
        mem_index++;
    }

    memory_allocated = mem_index;
    process_count++;
    
    pthread_mutex_unlock(&mutexMemory);
    return pid;
}
void* exec_process(void* args) {
    int process_id = *((int*)args);
    int p_index = process_id - 1;
    
    pthread_mutex_lock(&mutexMemory);
    processes[p_index].state = RUNNING;
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, "RUNNING");
            break;
        }
    }
    pthread_mutex_unlock(&mutexMemory);
    
    for (int i = processes[p_index].lower_bound + NUM_PCB; i <= processes[p_index].upper_bound; i++) {
        if (memory[i].type == CODE) {
            execute_line(memory[i].value, process_id);
        }
    }
    
    pthread_mutex_lock(&mutexMemory);
    processes[p_index].state = TERMINATED;
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, "TERMINATED");
            break;
        }
    }
    pthread_mutex_unlock(&mutexMemory);
    
    pthread_exit(NULL);
}
int main(void) {
    init_mutex();
    init_memory();
    pthread_t thread_1, thread_2, thread_3;

    int pid1 = create_process("Program_1.txt", 0);
    int pid2 = create_process("Program_2.txt", 0);
    int pid3 = create_process("Program_3.txt", 0);

    pthread_t thread_1, thread_2, thread_3;
    
    pthread_create(&thread_1, NULL, exec_process, &pid1);
    pthread_create(&thread_2, NULL, exec_process, &pid2);
    pthread_create(&thread_3, NULL, exec_process, &pid3);

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);
    pthread_join(thread_3, NULL);

    return 0;
}

