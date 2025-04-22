#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <ctype.h> 
//#include "uthash.h"// didnt work at all sad


#define MAX_LINE_LENGTH 1024
#define MAX_VARS_PER 3  // look i am not sure here but it isnt i wish we used c++
#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 1024
#define MEMORY_SIZE 60 // Because we need 60 words bruh 
#define MAX_PROCESSES 3 // only three i think for now
#define NUM_PCB 6 //NUmber of vaariables in the PCB guys we might need to increase it UwU 
#define MAX_FILE_BUFFER 10000

// Surprise bros we got another

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


void process_print(char * args,int process_id){
    trim(args);
    char *value = get_variable_value(args,process_id);
    if (value != NULL) {
        printf("%s = %s\n",args, value);
    } else {
        printf("value not found %s\n", args);
    }
}
void process_assign(char *args, int process_id) {
    // First, check if args is NULL
    if (args == NULL) {
        printf("Error: Invalid assign command (NULL args)\n");
        return;
    }
    
    // Make a copy of args to prevent modifying the original
    char args_copy[MAX_LINE_LENGTH];
    strncpy(args_copy, args, MAX_LINE_LENGTH - 1);
    args_copy[MAX_LINE_LENGTH - 1] = '\0';  // Ensure null termination
    
    char *token_context = NULL;
    char *name = strtok_r(args_copy, " ", &token_context);
    
    if (name == NULL) {
        printf("Error: Missing variable name in assign command\n");
        return;
    }
    
    trim(name);
    
    // Get the rest of the command after the variable name
    char *rest = strtok_r(NULL, "", &token_context);
    if (rest == NULL) {
        printf("Error: Missing value in assign command\n");
        return;
    }
    
    // Check for input command
    if (strstr(rest, "input") != NULL) {
        char user_input[100];
        printf("Please enter a value for %s: ", name);
        fflush(stdout); // Make sure the prompt is displayed
        // Using scanf instead of fgets
        scanf("%s", user_input);
        // Clear the input buffer
        trim(user_input);
        // Set the variable with the input value
        set_variable(name, user_input, process_id);
        // } else {
        //     printf("Error: Failed to read input\n");
        //     // Clear the input buffer on error too
        //     int c;
        //     while ((c = getchar()) != '\n' && c != EOF);
        // }
    }
    // Check for readFile command
    else if (strstr(rest, "readFile") != NULL) {
        // Extract the filename
        char *filepath = strstr(rest, "readFile") + 8;  // Skip "readFile"
        trim(filepath);
        char *path = get_variable_value(filepath,process_id);
        trim(path);
        
        char *file_content = process_read_file(path);
        if (file_content != NULL) {
            set_variable(name, file_content, process_id);
        } else {
            printf("Error: Failed to read file %s\n", filepath);
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


void process_print_from_to (char* args,int process_id){
    int a, b;

    char *num_1 = strtok(args, " ");
    char *num_2 = strtok(NULL, " ");

    trim(num_1);
    trim(num_2);
    a = atoi(get_variable_value(num_1,process_id));
    b = atoi(get_variable_value(num_2,process_id));

    printf("num_1 = %s,num_2 = %s,a = %d b = %d \n",num_1,num_2,a,b);
    for (int i = a; i<=b; i++){
        printf("%i\n",i);
    }
    printf("\n");
}
void sem_wait_resource(char *name) {
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "sem_wait_resource: unknown mutex \"%s\"\n", name);
        return;
    }
    printf("sem_wait_resource end %s \n",name);
}
void sem_signal_resource(char *name) {
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "sem_signal_resource: unknown mutex \"%s\"\n", name);
        return;
    }
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
        process_print_from_to(args,process_id);
    } else if (strcmp(command, "semWait") == 0) {
        sem_wait_resource(args);
    } else if (strcmp(command, "semSignal") == 0) {
        sem_signal_resource(args);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);//guys this outputs a formated error message
    }
    for (int i = processes[process_id-1].lower_bound; i <= processes[process_id-1].upper_bound; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "PC") == 0) {
            int pc = atoi(memory[i].value);//from ASCII to int 
            processes[process_id-1].program_counter++;
            pc++;
            sprintf(memory[i].value, "%d", pc);//covert to string
            break;
        }
    }
}

//memory

// Initialize memory
void init_memory() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i].name[0] = '\0';
        memory[i].value[0] = '\0';
        memory[i].process_id = -1;  // -1 indicates free memory no with any process
        memory[i].type = MINDO;
    }
    memory_allocated = 0;
}

int create_process(const char *program, int priority){//hot dog and frezzer you need this part miss you guys can i help with this tooo pleeeeasssee
    if (process_count >= MAX_PROCESSES) {
        printf("Error: Out of process (Ask MINDO)\n");
        return -1;
    }//We are thinking of everything :>
    if (memory_allocated >= MEMORY_SIZE){
        printf("Error: Out of memory (Ask Ahmed hassen)\n");
        return -1;
    }
    FILE *fptr = fopen(program,"r");
    if (fptr == NULL) {
        printf("Error: Dumb ass wrong file name %s\n", program);
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
        return -1;
    }
    printf("line_count = %d, NUM_PCB = %d, MAX_VARS_PER = %d, total = %d\n",line_count,NUM_PCB,MAX_VARS_PER,line_count + NUM_PCB + MAX_VARS_PER);
    if (line_count + NUM_PCB + MAX_VARS_PER > 60) {//Just the needed memory
        printf("Error: Not enough memory for process must be less than 60 words\n");
        fclose(fptr);
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
    
    return pid;
}
void exec_process(int args) {
    int process_id = args;
    int p_index = process_id - 1;
    
    processes[p_index].state = RUNNING;
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, "RUNNING");
            break;
        }
    }
    
    for (int i = processes[p_index].lower_bound + NUM_PCB; i <= processes[p_index].upper_bound; i++) {
        if (memory[i].type == CODE) {
            execute_line(memory[i].value, process_id);
        }
    }
    
    processes[p_index].state = TERMINATED;
    for (int i = processes[p_index].lower_bound; i <= processes[p_index].lower_bound + NUM_PCB; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "State") == 0) {
            strcpy(memory[i].value, "TERMINATED");
            break;
        }
    }
}

void print_memory() {
    printf("\n------ MEMORY CONTENTS ------\n");
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

int main(void) {
    init_mutex();
    init_memory();
    print_memory();
    int pid1 = create_process("Program_1.txt", 0);
    int pid2 = create_process("Program_2.txt", 0);
    int pid3 = create_process("Program_3.txt", 0);
    print_memory();
    printf("p1 \n");
    exec_process(pid1);
    exec_process(pid2);
    exec_process(pid3);
    return 0;
}

