#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <ctype.h> 
//#include <stdbool.h>  // Added for bool type
//#include "uthash.h"// didnt work at all sad


#define MAX_LINE_LENGTH 1024
#define MAX_VARS_PER 3  // look i am not sure here but it isnt i wish we used c++
#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 1024
#define MEMORY_SIZE 60 // Because we need 60 words bruh 
#define MAX_PROCESSES 3 // only three i think for now
#define NUM_PCB 5 //NUmber of vaariables in the PCB guys we might need to increase it UwU - increased to 8 for arrival and completion time
#define MAX_FILE_BUFFER 10000

// Added global clock
int system_clock = 0;

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
/*
 For Round Robin scheduling for hotdog and frizze ========================================
*/
    int arrival_time;    // When the process was created
    int completion_time; // When the process finished execution
    int remaining_time;  // For RR scheduling
    int quantum_used;    // For RR scheduling
    int code_size;       // Number of code lines
} PCB;

/*
 For Round Robin scheduling for hotdog and frizze ========================================
*/
int current_process_index = -1;

MemoryWord memory[MEMORY_SIZE];
int memory_allocated = 0;

PCB processes[MAX_PROCESSES];
int process_count = 0;

//sem 

void init_mutex(){
    
}

int find_mutex(const char *name) {
    return 0;
}
char *process_read_file(const char *path);

//variable

int find_variable(char *name, int process_id) {
  
    for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
        if (memory[i].type == VAR && strcmp(memory[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

char* get_variable_value(char *name, int process_id) {
    int index = find_variable(name, process_id);
    if (index != -1) {
        return memory[index].value;
    }
    return NULL;
}

void set_variable(char *name, char *value, int process_id) {
    int index = find_variable(name, process_id);
    if (index != -1) {
        strcpy(memory[index].value, value);
    } else {
        for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
            if (memory[i].type == VAR && memory[i].name[0] == '\0') {
                strcpy(memory[i].name, name);
                strcpy(memory[i].value, value);
                return;
            }
        }
        printf("Error: No variable space available for process %d\n", process_id);
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


void process_print(char * args, int process_id){
    system_clock++;
    
    trim(args);
    char *value = get_variable_value(args, process_id);
    if (value != NULL) {
        printf("[Clock: %d] Process %d: %s = %s\n", system_clock, process_id, args, value);
    } else {
        printf("[Clock: %d] Process %d: value not found %s\n", system_clock, process_id, args);
    }
}
//chatgpt 
void process_assign(char *args, int process_id) {
    system_clock++;
    
    // First, check if args is NULL
    if (args == NULL) {
        printf("[Clock: %d] Process %d: Error: Invalid assign command (NULL args)\n", system_clock, process_id);
        return;
    }
    
    // Make a copy of args to prevent modifying the original
    char args_copy[MAX_LINE_LENGTH];
    strncpy(args_copy, args, MAX_LINE_LENGTH - 1);
    args_copy[MAX_LINE_LENGTH - 1] = '\0';  // Ensure null termination
    
    char *token_context = NULL;
    char *name = strtok_r(args_copy, " ", &token_context);
    
    if (name == NULL) {
        printf("[Clock: %d] Process %d: Error: Missing variable name in assign command\n", system_clock, process_id);
        return;
    }
    
    trim(name);
    
    // Get the rest of the command after the variable name
    char *rest = strtok_r(NULL, "", &token_context);
    if (rest == NULL) {
        printf("[Clock: %d] Process %d: Error: Missing value in assign command\n", system_clock, process_id);
        return;
    }
    
    // Check for input command
    if (strstr(rest, "input") != NULL) {
        char user_input[100];
        printf("[Clock: %d] Process %d: Please enter a value for %s: ", system_clock, process_id, name);
        fflush(stdout); // Make sure the prompt is displayed
        // Using scanf instead of fgets
        scanf("%s", user_input);
        // Clear the input buffer
        trim(user_input);
        // Set the variable with the input value
        set_variable(name, user_input, process_id);
    }
    // Check for readFile command
    else if (strstr(rest, "readFile") != NULL) {
        // Extract the filename
        char *filepath = strstr(rest, "readFile") + 8;  // Skip "readFile"
        trim(filepath);
        char *path = get_variable_value(filepath, process_id);
        trim(path);
        
        char *file_content = process_read_file(path);
        if (file_content != NULL) {
            set_variable(name, file_content, process_id);
        } else {
            printf("[Clock: %d] Process %d: Error: Failed to read file %s\n", system_clock, process_id, filepath);
        }
    }
    // Direct assignment
    else {
        trim(rest);
        
        // Check if value is another variable
        char *var_value = get_variable_value(rest, process_id);
        if (var_value != NULL) {
            set_variable(name, var_value, process_id);
        } else {
            // Direct value assignment
            set_variable(name, rest, process_id);
        }
    }
}

//chatGPT
char *process_read_file(const char *path) {
    system_clock++;
    
    static char buf[MAX_FILE_BUFFER];
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    buf[0] = '\0';
    while (fgets(buf + strlen(buf), MAX_FILE_BUFFER - strlen(buf), f)) {
        ;
    }
    fclose(f);
    return buf;
}

void process_write_file(char* args, int process_id){
    system_clock++;
    
    char *space = strchr(args,' ');
    char name[100];
    char data[100];

    if(space == NULL)
        printf("[Clock: %d] Process %d: Error: Invalid printFromTo format\n", system_clock, process_id);
    
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
        printf("[Clock: %d] Process %d: File written successfully\n", system_clock, process_id);
    }else
        printf("[Clock: %d] Process %d: An error occurred writing to the file.\n", system_clock, process_id);

    fclose(fptr);
}

void process_print_from_to(char* args, int process_id){
    system_clock++;
    
    int a, b;

    char *num_1 = strtok(args, " ");
    char *num_2 = strtok(NULL, " ");

    trim(num_1);
    trim(num_2);
    a = atoi(get_variable_value(num_1, process_id));
    b = atoi(get_variable_value(num_2, process_id));

    printf("[Clock: %d] Process %d: num_1 = %s, num_2 = %s, a = %d b = %d \n", system_clock, process_id, num_1, num_2, a, b);
    for (int i = a; i <= b; i++){
        printf("%i\n", i);
    }
    printf("\n");
}

void sem_wait_resource(char *name) {
    system_clock++;
    
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "[Clock: %d] sem_wait_resource: unknown mutex \"%s\"\n", system_clock, name);
        return;
    }
    printf("[Clock: %d] sem_wait_resource end %s \n", system_clock, name);
}

void sem_signal_resource(char *name) {
    system_clock++;
    
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "[Clock: %d] sem_signal_resource: unknown mutex \"%s\"\n", system_clock, name);
        return;
    }
    printf("[Clock: %d] sem_signal_resource end %s \n", system_clock, name);
}

void execute_line(char *line, int process_id) {
    if (line[0] == '\0') {
        return;
    }
    
    // Extract command and arguments doesn't need max length
    char *command = strtok(line, " ");
    char *args = strtok(NULL, "");

    static char empty_string[] = ""; 
    if (!args) args = empty_string;
    
    if (strcmp(command, "print") == 0) {
        process_print(args, process_id);
    } else if (strcmp(command, "assign") == 0) {
        process_assign(args, process_id);
    } else if (strcmp(command, "writeFile") == 0) {
        process_write_file(args, process_id);
    } else if (strcmp(command, "readFile") == 0) {
        process_read_file(args);
    } else if (strcmp(command, "printFromTo") == 0) {
        process_print_from_to(args, process_id);
    } else if (strcmp(command, "semWait") == 0) {
        sem_wait_resource(args);
    } else if (strcmp(command, "semSignal") == 0) {
        sem_signal_resource(args);
    } else {
        fprintf(stderr, "[Clock: %d] Process %d: Error: Unknown command '%s'\n", system_clock, process_id, command);
    }
    
    // Update program counter in the PCB
    for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "PC") == 0) {
            int pc = atoi(memory[i].value);
            processes[process_id-1].program_counter++;
            pc++;
            sprintf(memory[i].value, "%d", pc);
            break;
        }
    }
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

int create_process(const char *program, int priority){
    // Process creation consumes a clock cycle
    system_clock++;
    
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
    printf("[Clock: %d] line_count = %d, NUM_PCB = %d, MAX_VARS_PER = %d, total = %d\n", 
           system_clock, line_count, NUM_PCB, MAX_VARS_PER, line_count + NUM_PCB + MAX_VARS_PER);
    
    int pid = process_count + 1;
    int start_index = memory_allocated;
    
    processes[process_count].process_id = pid;
    processes[process_count].state = READY;
    processes[process_count].priority = priority;
    processes[process_count].program_counter = 0;
    processes[process_count].lower_bound = start_index;
    processes[process_count].upper_bound = start_index + line_count + NUM_PCB + MAX_VARS_PER - 1;
    processes[process_count].arrival_time = system_clock;  // Record arrival time
    processes[process_count].completion_time = -1;        // Not completed yet
    processes[process_count].quantum_used = 0;           // Initialize quantum
    processes[process_count].code_size = line_count;     // Number of code lines

    int mem_index = start_index;

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
    sprintf(memory[mem_index].value, "%d", priority);
    memory[mem_index].process_id = pid;
    memory[mem_index].type = T_PCB;
    mem_index++;
    
    strcpy(memory[mem_index].name, "PC");
    strcpy(memory[mem_index].value, "0");
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
    
    printf("[Clock: %d] Process %d created at time %d\n", 
           system_clock, pid, processes[process_count-1].arrival_time);
           
    return pid;
}

//chatgpt first come fist server 
void exec_process(int args) {
    int process_id = args;
    int p_index = process_id - 1;
    
    // Set process to running
    processes[p_index].state = RUNNING;
    printf("[Clock: %d] Process %d begins execution\n", system_clock, process_id);
    
    // Update state in memory
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, "RUNNING");
            break;
        }
    }
    
    // Execute each instruction in the process
    for (int i = processes[p_index].lower_bound + NUM_PCB; i <= processes[p_index].upper_bound; i++) {
        if (memory[i].type == CODE) {
            execute_line(memory[i].value, process_id);
        }
    }
    
    // Record completion time
    processes[p_index].completion_time = system_clock;
    processes[p_index].state = TERMINATED;
    
    // Update memory with termination info
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, "TERMINATED");
        }
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "Completion_Time") == 0) {
            sprintf(memory[i].value, "%d", system_clock);
        }
    }
    
    printf("[Clock: %d] Process %d terminated. Execution time: %d cycles\n", 
           system_clock, process_id, 
           processes[p_index].completion_time - processes[p_index].arrival_time);
}
/*
 For Round Robin scheduling for hotdog and frizze ========================================
*/
int processes_remaining() {//for hotdog and frizz
    for (int i = 0; i < process_count; i++) {
        if (processes[i].state != TERMINATED) {
            return 1;
        }
    }
    return 0;
}
/*
 For Round Robin scheduling for hotdog and frizze ========================================
*/
// Update process state in memory
void update_process_state(int p_index, ProcessState new_state) {
    const char* state_str;
    
    switch (new_state) {
        case NEW:
            state_str = "NEW";
            break;
        case READY:
            state_str = "READY";
            break;
        case RUNNING:
            state_str = "RUNNING";
            break;
        case BLOCKED:
            state_str = "BLOCKED";
            break;
        case TERMINATED:
            state_str = "TERMINATED";
            break;
        default:
            state_str = "UNKNOWN";
    }
    
    processes[p_index].state = new_state;
    
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, state_str);
            break;
        }
    }
}
/*
 For Round Robin scheduling for hotdog and frizze ========================================
*/
int get_next_process() { //for hotdog and frizzer 
    if (process_count == 0) {
        return -1;  // No processes
    }

    int start = (current_process_index + 1) % process_count;
    int index = start;
    
    do {
        if (processes[index].state == READY) {
            return index;
        }
        index = (index + 1) % process_count;
    } while (index != start);
    
    return -1;
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

// New function to print process timing statistics
void print_process_stats() {
    printf("\n------ PROCESS STATISTICS [Clock: %d] ------\n", system_clock);
    printf("PID | Priority | Arrival | Completion | Turnaround | Status\n");
    printf("--------------------------------------------------------\n");
    
    for (int i = 0; i < process_count; i++) {
        int turnaround = -1;
        char status[20] = "UNKNOWN";
        
        if (processes[i].completion_time != -1) {
            turnaround = processes[i].completion_time - processes[i].arrival_time;
            strcpy(status, "TERMINATED");
        } else {
            switch(processes[i].state) {
                case NEW: strcpy(status, "NEW"); break;
                case READY: strcpy(status, "READY"); break;
                case RUNNING: strcpy(status, "RUNNING"); break;
                case BLOCKED: strcpy(status, "BLOCKED"); break;
                default: strcpy(status, "UNKNOWN"); break;
            }
        }
        
        printf("%3d | %8d | %7d | %10d | %10d | %s\n", 
               processes[i].process_id, 
               processes[i].priority, 
               processes[i].arrival_time, 
               processes[i].completion_time, 
               turnaround, 
               status);
    }
    printf("-------------------------------------------\n\n");
}

int main(void) {
    system_clock = 0;  // Initialize system clock
    init_mutex();
    init_memory();
    
    printf("[Clock: %d] System initialized\n", system_clock);
    print_memory();
    
    int pid1 = create_process("Program_1.txt", 0);
    if (pid1 > 0) {
        exec_process(pid1);
        print_process_stats();
    }
    
    int pid2 = create_process("Program_2.txt", 0);
    if (pid2 > 0) {
        exec_process(pid2);
        print_process_stats();
    }
    
    int pid3 = create_process("Program_3.txt", 0);
    if (pid3 > 0) {
        exec_process(pid3);
        print_process_stats();
    }
    
    printf("[Clock: %d] All processes completed\n", system_clock);
    print_process_stats();
    
    return 0;
}