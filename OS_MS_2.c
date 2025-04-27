#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <math.h>
#include <ctype.h> 
//#include <stdbool.h>  // Added for bool type
//#include "uthash.h"// didnt work at all sad

//#define TEST_MODE 0
#define MAX_LINE_LENGTH 1024
#define MAX_VARS_PER 3  // look i am not sure here but it isnt i wish we used c++
#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 1024
#define MEMORY_SIZE 60 // Because we need 60 words bruh 
#define MAX_PROCESSES 6 // only three i think for now
#define NUM_PCB 5 //NUmber of vaariables in the PCB guys we might need to increase it UwU - increased to 8 for arrival and completion time
#define READY_QUEUE_SIZE 4 // max amount of ready queues
#define MAX_MUTEXES 3 //max amount of mutexes
#define MAX_FILE_BUFFER 10000
#define ROUND_ROBIN 1
#define FIFO 2
#define MLFQ 3

// Added global clock
int scheduling = ROUND_ROBIN;
int system_clock = 1;


typedef struct {
    char name[MAX_NAME_LENGTH];
    int value;  
}mutex;

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
} MemType;

// Memory word
typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int process_id;   // To track which process owns this memory word
    MemType type;       // 0 = var, 1 = pcb, 2 = code
} MemoryWord;

// PCB - Extended to include arrival time and completion time
typedef struct {
    int process_id;
    ProcessState state;
    int priority;
    int program_counter;
    int lower_bound; 
    int upper_bound;
    int arrival_time;    // When the process was created
    int completion_time; // When the process finished execution
    int code_size;       // Number of code lines
} PCB;

typedef struct  {
    char* name;
    int arrival_time;
} program;

typedef struct node {
    int value;
    struct node* next;
} node;
struct PqNode {
    int data;
    int priority;
    struct PqNode* next;
};
typedef struct {
    struct PqNode* head;
    int size;
}  PriorityQueue;

typedef struct {
    node* tail;
    int size;
} queue;

int create_process(const char *program);
PCB load_PCB(int address);
char *process_read_file(const char *path);
void trim(char *str);
void print_memory();




struct PqNode* createPqNode(int data, int priority) {
    struct PqNode* newPqNode = (struct PqNode*)malloc(sizeof(struct PqNode));
    if (newPqNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newPqNode->data = data;
    newPqNode->priority = priority;
    newPqNode->next = NULL;
    return newPqNode;
}

int isEmpty(struct PqNode* head) {
    return head == NULL;
}


void enqueue(PriorityQueue* pq, int data, int priority) {
    struct PqNode** head = &pq->head;
    struct PqNode* newPqNode = createPqNode(data, priority);
    
    if (*head == NULL) {
        *head = newPqNode;
    } else if (newPqNode->priority < (*head)->priority) {
        newPqNode->next = *head;
        *head = newPqNode;
    } else {
        struct PqNode* current = *head;
        while (current->next != NULL && current->next->priority <= newPqNode->priority) {
            current = current->next;
        }
        newPqNode->next = current->next;
        current->next = newPqNode;
    }
    pq->size++;
}


int dequeue(PriorityQueue* pq) {
    struct PqNode** head = &pq->head;
    if (isEmpty(*head)) {
        printf("Queue is empty\n");
        return -1;
    }
    
    struct PqNode* temp = *head;
    int data = temp->data;
    *head = (*head)->next;
    free(temp);
    pq->size--;
    return data;
}

void display(struct PqNode* head) {
    struct PqNode* current = head;
    if (current == NULL) {
        printf("Queue is empty\n");
        return;
    }
    
    printf("Priority Queue elements: ");
    while (current != NULL) {
        printf("%d (p%d) ", current->data, current->priority);
        current = current->next;
    }
    printf("\n");
}


void freeQueue(struct PqNode** head) {
    struct PqNode* current = *head;
    while (current != NULL) {
        struct PqNode* temp = current;
        current = current->next;
        free(temp);
    }
    *head = NULL;
}



void init_queue(queue* q) {
    q->tail = NULL;
    q->size = 0;
}

void push(queue* q, int value) {
    node* new_node = (node*) malloc(sizeof(node));
if (new_node == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
}
new_node->value = value;

    if (q->tail == NULL) {
        q->tail = new_node;
        q->tail->next = new_node;
    } else {
        new_node->next = q->tail->next;
        q->tail->next = new_node;
        q->tail = new_node;
    }
    q->size++;
}

int pop(queue* q) {
    if (q->size == 0) {
        return -1;
    }
    int value = q->tail->next->value;
    node* temp = q->tail->next;
    if (q->size == 1) {
        q->tail = NULL;
    } else {
        q->tail->next = q->tail->next->next;
    }
    q->size--;
    free(temp);
    return value;
}

int peek(queue* q) {
    if (q->size == 0) {
        return -1;
    }
    return q->tail->next->value;
}

int round_robin_quantum = 2;
int quantum_tracking = 0;
int MY_clock = 0;
int is_current_process_blocked = 0;
int programs_size = 4;
int process_count = 0;
int total_programs = 0;
int program_index = 0;
int memory_allocated = 0;
queue ready_queue[READY_QUEUE_SIZE];
PriorityQueue blocked_queue[MAX_MUTEXES];
program programs[MAX_PROCESSES];
MemoryWord memory[MEMORY_SIZE];
mutex mutexes[MAX_MUTEXES];
PCB current_process = {-1};


//sem 

void init_mutex() {
    strcpy(mutexes[0].name,"userInput");
    strcpy(mutexes[1].name,"file");
    strcpy(mutexes[2].name,"userOutput");
    for(int i = 0; i < MAX_MUTEXES; i++) {
        mutexes[i].value = 1;
    }
}

int find_mutex(const char *name) {
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (strcmp(mutexes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
int get_enqueue_priority(int priority) {
    if (scheduling == ROUND_ROBIN || scheduling == FIFO) {
        return 0;
    }else if (scheduling == MLFQ) {
        return priority;
    }
    return priority;
}
void block_process(int mutex_index){
    if (mutex_index < 0 || mutex_index >= MAX_MUTEXES) {
        fprintf(stderr, "[Clock: %d] block_process: invalid mutex index %d\n", system_clock, mutex_index);
        return;
    }
    if (current_process.process_id != -1) {
        int offset = current_process.lower_bound;
        strcpy(memory[offset + 1].value, "BLOCKED");
        current_process.state = BLOCKED;
        int enqueue_priority = get_enqueue_priority(current_process.priority);
        enqueue(&blocked_queue[mutex_index], current_process.lower_bound, enqueue_priority);
        is_current_process_blocked = 1;

   }
}
void unblock_process(int mutex_index) {
    if (mutex_index < 0 || mutex_index >= MAX_MUTEXES) {
        fprintf(stderr, "[Clock: %d] unblock_process: invalid mutex index %d\n", system_clock, mutex_index);
        return;
    }
    if (blocked_queue[mutex_index].size > 0) {
        PCB unblocked_process = load_PCB(dequeue(&blocked_queue[mutex_index]));
        printf("[Clock: %d] Process %d: sem_signal_resource unblocked %s \n", system_clock, unblocked_process.process_id, mutexes[mutex_index].name);
        int offset = unblocked_process.lower_bound;
        strcpy(memory[offset + 1].value, "READY");
        //printf("changed address %d to READY\n", offset + 1);
        push(&ready_queue[unblocked_process.priority], unblocked_process.lower_bound);
    } 
}

void sem_wait_resource(char *name) {
    
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "[Clock: %d] sem_wait_resource: unknown mutex \"%s\"\n", system_clock, name);
        return;
    }
    
     if (mutexes[idx].value < 0) {
        printf("[Clock: %d] Process %d: sem_wait_resource blocked %s \n", system_clock, current_process.process_id, name);
        block_process(idx);
        current_process.program_counter--;
        sprintf(memory[current_process.lower_bound + 3].value, "%d", current_process.program_counter);
    } else {
        mutexes[idx].value=-1;
        printf("[Clock: %d] Process %d: sem_wait_resource acquired %s \n", system_clock, current_process.process_id, name);
        //printf("mutex[%d] = %d\n",idx,mutexes[idx].value);
    }
    
}

void sem_signal_resource(char *name) {    
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "[Clock: %d] sem_signal_resource: unknown mutex \"%s\"\n", system_clock, name);
        return;
    }
   
    if (mutexes[idx].value <= 0) {
        unblock_process(idx);
        mutexes[idx].value=1;
        //printf("[Clock: %d] Process %d: sem_signal_resource released %s \n", system_clock, current_process.process_id, name);
        //printf("mutex[%d] = %d\n",idx,mutexes[idx].value);
    }
    
   
}


//variable

int find_variable(char *name) {
    int offset = NUM_PCB + current_process.code_size;
    for (int i = current_process.lower_bound + offset; i <= current_process.upper_bound; i++) {
        if (memory[i].type == VAR && strcmp(memory[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

char* get_variable_value(char *name) {
    int index = find_variable(name);
    if (index != -1) {
        return memory[index].value;
    }
    return NULL;
}

void set_variable(char *name, char *value) {
    int index = find_variable(name);
    if (index != -1) {
        strcpy(memory[index].value, value);
    } else {
        int offset = NUM_PCB + current_process.code_size;
        for (int i = current_process.lower_bound + offset; i <= current_process.upper_bound; i++) {
            if (memory[i].type == VAR && memory[i].name[0] == '\0') {
                strcpy(memory[i].name, name);
                strcpy(memory[i].value, value);
                return;
            }
        }
        printf("Error: No variable space available for process %d\n", current_process.process_id);
    }
}

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


void process_print(char * args) {    
    trim(args);
    char *value = get_variable_value(args);
    if (value != NULL) {
        printf("[Clock: %d] Process %d: %s = %s\n", system_clock, current_process.process_id, args, value);
    } else {
        printf("[Clock: %d] Process %d: value not found %s\n", system_clock, current_process.process_id, args);
    }
}
//chatgpt 
void process_assign(char *args) {    
    // First, check if args is NULL
    if (args == NULL) {
        printf("[Clock: %d] Process %d: Error: Invalid assign command (NULL args)\n", system_clock, current_process.process_id);
        return;
    }
    
    // Make a copy of args to prevent modifying the original
    char args_copy[MAX_LINE_LENGTH];
    strncpy(args_copy, args, MAX_LINE_LENGTH - 1);
    args_copy[MAX_LINE_LENGTH - 1] = '\0';  // Ensure null termination
    
    char *token_context = NULL;
    char *name = strtok_r(args_copy, " ", &token_context);
    
    if (name == NULL) {
        printf("[Clock: %d] Process %d: Error: Missing variable name in assign command\n", system_clock, current_process.process_id);
        return;
    }
    
    trim(name);
    
    // Get the rest of the command after the variable name
    char *rest = strtok_r(NULL, "", &token_context);
    if (rest == NULL) {
        printf("[Clock: %d] Process %d: Error: Missing value in assign command\n", system_clock, current_process.process_id);
        return;
    }
    
    // Check for input command
    if (strstr(rest, "input") != NULL) {
        char user_input[100];
        printf("[Clock: %d] Process %d: Please enter a value for %s: ", system_clock, current_process.process_id, name);
        fflush(stdout); // Make sure the prompt is displayed
        // Using scanf instead of fgets
        scanf("%s", user_input);
        // Clear the input buffer
        trim(user_input);
        // Set the variable with the input value
        set_variable(name, user_input);
    }
    // Check for readFile command
    else if (strstr(rest, "readFile") != NULL) {
        // Extract the filename
        char *filepath = strstr(rest, "readFile") + 8;  // Skip "readFile"
        trim(filepath);
        char *path = get_variable_value(filepath);
        trim(path);
        
        char *file_content = process_read_file(path);
        if (file_content != NULL) {
            set_variable(name, file_content);
        } else {
            printf("[Clock: %d] Process %d: Error: Failed to read file %s\n", system_clock, current_process.process_id, filepath);
        }
    }
    // Direct assignment
    else {
        trim(rest);
        
        // Check if value is another variable
        char *var_value = get_variable_value(rest);
        if (var_value != NULL) {
            set_variable(name, var_value);
        } else {
            // Direct value assignment
            set_variable(name, rest);
        }
    }
}

//chatGPT
char *process_read_file(const char *path) {    
    static char buf[MAX_FILE_BUFFER];
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    buf[0] = '\0';
    while (fgets(buf + strlen(buf), MAX_FILE_BUFFER - strlen(buf), f)) {}
    fclose(f);
    return buf;
}

void process_write_file(char* args){    
    char *space = strchr(args,' ');

    char* num_1 = strtok(args, " ");
    char* num_2 = strtok(NULL, " ");
    trim(num_1);
    trim(num_2);
    char* name = get_variable_value(num_1);
    char* data = get_variable_value(num_2);

    if(space == NULL) {
        printf("[Clock: %d] Process %d: Error: Invalid writeFile format\n", system_clock, current_process.process_id);
        return;
    }
    trim(name);
    trim(data);
    FILE *fptr = fopen(name,"w");

    if (fptr != NULL){
        fputs(data,fptr);
        printf("[Clock: %d] Process %d: File written successfully\n", system_clock, current_process.process_id);
    }else
        printf("[Clock: %d] Process %d: An error occurred writing to the file.\n", system_clock, current_process.process_id);

    fclose(fptr);
}

void process_print_from_to(char* args){    
    int a, b;

    char *num_1 = strtok(args, " ");
    char *num_2 = strtok(NULL, " ");

    trim(num_1);
    trim(num_2);
    a = atoi(get_variable_value(num_1));
    b = atoi(get_variable_value(num_2));

    printf("[Clock: %d] Process %d: num_1 = %s, num_2 = %s, a = %d b = %d \n", system_clock, current_process.process_id, num_1, num_2, a, b);
    for (int i = a; i <= b; i++){
        printf("%i\n", i);
    }
    printf("\n");
}

void execute_line(char *line) {
    char* goddamn_copy_because_someone_chatgpt_his_way =  malloc(strlen(line) + 1);
    strcpy(goddamn_copy_because_someone_chatgpt_his_way, line);
    if (goddamn_copy_because_someone_chatgpt_his_way==NULL || goddamn_copy_because_someone_chatgpt_his_way[0] == '\0') {
        return;
    }
    #ifdef TEST_MODE
        printf("[Clock: %d] Process %d: Executing line at address %d: %s\n", system_clock, current_process.process_id,current_process.program_counter ,line);
    #endif
   
    // Extract command and arguments doesn't need max length
    char *command = strtok(goddamn_copy_because_someone_chatgpt_his_way, " ");
    char *args = strtok(NULL, "");
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
        fprintf(stderr, "[Clock: %d] Process %d: Error: Unknown command '%s'\n", system_clock, current_process.process_id, command);
    }
   free(goddamn_copy_because_someone_chatgpt_his_way);
}

// Initialize memory
void init_memory() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i].name[0] = '\0';
        memory[i].value[0] = '\0';
        memory[i].process_id = -1;  // -1 indicates free memory not with any process
        memory[i].type = MINDO;
    }
    memory_allocated = 0;
}

int state_to_int(char* state) {
    if (strcmp(state, "NEW") == 0) {
        return 0;
    } else if (strcmp(state, "READY") == 0) {
        return 1;
    } else if (strcmp(state, "RUNNING") == 0) {
        return 2;
    } else if (strcmp(state, "BLOCKED") == 0) {
        return 3;
    } else if (strcmp(state, "TERMINATED") == 0) {
        return 4;
    }
    return -1;
}

PCB load_PCB(int address) {
    if (address < 0 || address >= MEMORY_SIZE || memory[address].type != T_PCB || strcmp(memory[address].name, "Process_ID") != 0) {
        printf("Error: invalid PCB address.\n");
        return (PCB) {-1};
    }
    PCB process;
    process.process_id = atoi(memory[address].value);
    process.state = (ProcessState) state_to_int(memory[address + 1].value);
    process.priority = atoi(memory[address + 2].value);
    process.program_counter = atoi(memory[address + 3].value);
    sscanf(memory[address + 4].value, "%d-%d", &process.lower_bound, &process.upper_bound);
    process.code_size = process.upper_bound - process.lower_bound - NUM_PCB - 2;
    return process;
}

void init_process(int address) {
    push(&ready_queue[0], address);
}

void switch_context(queue* src, queue* dest) {
   #ifdef TEST_MODE
     printf("--------switching context--------\n");
    #endif
     int offset = current_process.lower_bound;
    
    if (current_process.process_id != -1) {
        if (current_process.program_counter == current_process.lower_bound + current_process.code_size + NUM_PCB) {
            strcpy(memory[offset + 1].value, "TERMINATED");
            current_process.process_id = -1;
        } else if (!is_current_process_blocked) {
            strcpy(memory[offset + 1].value, "READY");
            push(dest, current_process.lower_bound);
        }
    }
    if (src->size > 0) {
        current_process = load_PCB(pop(src));
        offset = current_process.lower_bound;
        strcpy(memory[offset + 1].value, "RUNNING");
    }
    
    
    
}

queue* find_src(int priority) {
    for (int i = 0; i <= priority; i++) {
        if (ready_queue[i].size > 0) {
            return &ready_queue[i];
        }
    }
    return &ready_queue[0];
}

void print_memory() {
    printf("\n------ MEMORY CONTENTS [Clock: %d] ------\n", system_clock);
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].process_id != -1) {
            printf("[%2d] Process %d | ", i, memory[i].process_id);
            
            if (memory[i].type == T_PCB)
                printf("PCB | ");
            else if (memory[i].type == CODE)
                printf("CODE | ");
            else if (memory[i].type == VAR)
                printf("VAR | ");
            
            printf("%s: %s\n", memory[i].name, memory[i].value);
        } else {
            printf("[%2d] Empty\n", i);
        }
    }
    printf("----------------------------\n\n");
}

void schedule() {
    char program_done = current_process.process_id == -1 || current_process.program_counter == current_process.lower_bound + current_process.code_size + NUM_PCB;
    switch(scheduling) {
        case ROUND_ROBIN:
            if (ready_queue[0].size > 0 && quantum_tracking == round_robin_quantum || program_done || is_current_process_blocked) {
                switch_context(&ready_queue[0], &ready_queue[0]);
            }
            if (quantum_tracking == round_robin_quantum || program_done || is_current_process_blocked) {
                quantum_tracking = 0;
            }
            break;
        case FIFO:
            if (program_done) {
                switch_context(&ready_queue[0], &ready_queue[0]);
            }
            break;
        case MLFQ:{//mutex won't work till now with mlfq cuz it reinserts the process in readyqueue[0]
            int mlfq_quantum = 1 << (current_process.priority);
            if (quantum_tracking == mlfq_quantum && current_process.priority < 3) {
                current_process.priority++;
                int offset = current_process.lower_bound;
                sprintf(memory[offset + 2].value, "%d", current_process.priority);
            }
            int src_priority = program_done ? 3 : current_process.priority;
            queue* src = find_src(src_priority);
            queue* dest = &ready_queue[current_process.priority];
            if (src->size > 0 && quantum_tracking == mlfq_quantum || program_done) {
                switch_context(src, dest);
            }
            if (quantum_tracking == mlfq_quantum || program_done) {
                quantum_tracking = 0;
            }
            break;}
    }
    is_current_process_blocked = 0;
}

void check_for_processes() {
    while (program_index < total_programs && programs[program_index].arrival_time == system_clock) {
        printf("[Clock: %d] %s created.\n", system_clock, programs[program_index].name);
        create_process(programs[program_index++].name);
    }
}

void run_clock_cycle() {
    // check if new process is to be created
    check_for_processes();
    
    schedule();
   
    if (current_process.process_id == -1) {
        printf("[Clock: %d] Idle\n", system_clock);
        system_clock++;
        return;
    }
   
    execute_line(memory[current_process.program_counter].value); 
   
    current_process.program_counter++;
    int offset = current_process.lower_bound;
    sprintf(memory[offset + 3].value, "%d", current_process.program_counter);
    #ifdef TEST_MODE
        printf("------------TestMode Statistics----------------\n");
        print_memory();
/*
        printf("\n------ BLOCKED QUEUES [Clock: %d] ------\n", system_clock);
        for (int i = 0; i < MAX_MUTEXES; i++) {
            printf("Mutex: %s | Blocked Processes: ", mutexes[i].name);
            if (blocked_queue[i].size == 0) {
                 printf("None\n");
            }else {
            node* current = blocked_queue[i].tail->next;
            for (int j = 0; j < blocked_queue[i].size; j++) {
                PCB process = load_PCB(current->value);
                printf("%d ", process.process_id);
                current = current->next;
            }
            printf("\n");
            }
        }
        printf("---------------------------------------\n");
        
        printf("\nquantum tracking: %d",quantum_tracking);*/
        printf("\n-------------END TESTMode print ----------------\n");
    #endif
    //these need to be at the end of this method
    quantum_tracking++;
    system_clock++;
}

int create_process(const char *program) {
    if (process_count >= MAX_PROCESSES) {
        printf("[Clock: %d] Error: Out of process slots (Ask MINDO)\n", system_clock);
        return -1;
    }
    if (memory_allocated >= MEMORY_SIZE){
        printf("[Clock: %d] Error: Out of memory (Ask Ahmed Hassan)\n", system_clock);
        return -1;
    }
    FILE *fptr = fopen(program,"r");
    if (fptr == NULL) {
        printf("[Clock: %d] Error: Dumb ass wrong file name %s\n", system_clock, program);
        return -1;
    }
    int line_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, fptr)) {
        if (strlen(line) > 1) {  //Skip empty lines
            line_count++;
        }
    }
    if (memory_allocated + line_count + NUM_PCB + MAX_VARS_PER  > MEMORY_SIZE) {
        printf("[Clock: %d] Error: Not enough memory for process\n", system_clock);
        fclose(fptr);
        return -1;
    }
    // printf("[Clock: %d] line_count = %d, NUM_PCB = %d, MAX_VARS_PER = %d, total = %d\n", 
    //        system_clock, line_count, NUM_PCB, MAX_VARS_PER, line_count + NUM_PCB + MAX_VARS_PER);
    
    int pid = process_count + 1;
    int start_index = memory_allocated;
    int mem_index = memory_allocated;

    init_process(start_index);

    //PCB
    strcpy(memory[mem_index].name, "Process_ID");
    sprintf(memory[mem_index].value, "%d", pid);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "State");
    strcpy(memory[mem_index].value, "READY");
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "Priority");
    sprintf(memory[mem_index].value, "%d", 0);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "PC");
    sprintf(memory[mem_index].value, "%d", start_index + NUM_PCB);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "Memory_Bounds");
    sprintf(memory[mem_index].value, "%d-%d", start_index, start_index + line_count + NUM_PCB + MAX_VARS_PER - 1);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;

    //CODE
    rewind(fptr);
    int line_index = 0;
    while (fgets(line, MAX_LINE_LENGTH, fptr) && mem_index < MEMORY_SIZE) {
        if (strlen(line) > 1) {  // Skip empty lines
            line[strcspn(line, "\n")] = 0;
            
            sprintf(memory[mem_index].name, "Line_%d", line_index);
            strcpy(memory[mem_index].value, line);
            memory[mem_index].process_id = pid;
            memory[mem_index].type = CODE;
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
        memory[mem_index].type = VAR;
        mem_index++;
    }

    memory_allocated = mem_index;
    process_count++;
           
    return pid;
}



void init() {
    init_mutex();
    init_memory();
    init_queues();
}



void init_queues() {
    for (int i = 0; i < READY_QUEUE_SIZE; i++) {
        init_queue(&ready_queue[i]);
    }
    

    for (int i = 0; i < MAX_MUTEXES; i++) {
        blocked_queue[i].head = NULL;
        blocked_queue[i].size = 0;
    }
}


int compare(const void *a, const void *b) {
    return ((program *)a)->arrival_time - ((program *)b)->arrival_time;
}

int main(void) {
    system_clock = 1;  // Initialize system clock
    init();
    
    printf("[Clock: %d] System initialized\n", system_clock);
    print_memory();
    
    char buffer[1024];
    while (total_programs < MAX_PROCESSES) {
        printf("Enter process name (or \"done\") - maximum of %d processes: ", MAX_PROCESSES);
        if (!fgets(buffer, MAX_NAME_LENGTH, stdin)) break;
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "done") == 0) break;
        programs[total_programs].name =  (char*) malloc(strlen(buffer));
        strcpy(programs[total_programs].name, buffer);

        printf("Enter arrival time: ");
        if (scanf("%d", &programs[total_programs].arrival_time) != 1) break;
        getchar();

        total_programs++;
    }

    

    qsort(programs, total_programs, sizeof(program), compare);
    print_memory();
    while (1) {
        run_clock_cycle();
        if (current_process.process_id == -1 && program_index == total_programs) {
            break;
        }
    }
    
    printf("[Clock: %d] All processes completed\n", system_clock);
    return 0;
}
