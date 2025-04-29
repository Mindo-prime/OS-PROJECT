//gcc gui2.c queue.c PriorityQueue.c -o gui2 `pkg-config --cflags --libs gtk+-3.0` -lfontconfig
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>   
#include <math.h>
#include <ctype.h> 
#include "queue.h"
#include "PriorityQueue.h"
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
//ui
// GUI components
GtkWidget *window;
GtkWidget *main_grid;
GtkWidget *sidebar;
GtkWidget *console_text_view;
GtkTextBuffer *console_buffer;
GtkWidget *memory_list;
GtkListStore *memory_store;
GtkWidget *process_list;
GtkListStore *process_store;
GtkWidget *ready_queue_list;
GtkListStore *ready_queue_store;
GtkWidget *blocked_queue_list;
GtkListStore *blocked_queue_store;
GtkWidget *resource_list;
GtkListStore *resource_store;
GtkWidget *clock_label;
GtkWidget *process_count_label;
GtkWidget *scheduler_combo;
GtkWidget *quantum_spinner;
GtkWidget *step_button;
GtkWidget *auto_button;
GtkWidget *reset_button;
GtkWidget *add_process_button;
GtkWidget *add_process_dialog;
GtkWidget *arrival_time_entry;
GtkWidget *program_name_entry;
GtkWidget *add_process_entry;
//dark mode
gboolean dark_mode = FALSE;
GtkWidget *theme_button;
//tabs
GtkWidget *memory_list_tab;
GtkWidget *console_text_view_tab;
GtkWidget *ready_queue_list_tab;
GtkWidget *blocked_queue_list_tab;
GtkWidget *resource_list_tab;
GtkWidget *process_frame;
GtkWidget *queue_frame;
GtkWidget *resource_frame;
GtkWidget *console_frame;
GtkWidget *main_notebook;
GtkWidget *default_view;
//clock
GtkWidget *real_time_label;
guint clock_timer_id = 0;

int no_rest = 0;
// Define the process_icons structure at the top of the file
typedef struct {
    GtkWidget *drawing_area;
    GtkWidget *label;
    gboolean active;
    int pcb_address;
} ProcessIcon;

// Global array of process icons
ProcessIcon process_icons[MAX_PROCESSES];


// Added global clock
int scheduling = FIFO;
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
//backend
int create_process(const char *program);
PCB load_PCB(int address);
char *process_read_file(const char *path);
void trim(char *str);
void print_memory();
//UI
void update_gui(void);
void console_printf(const char *format, ...);

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
        console_printf("[Clock: %d] Process %d unblocked\n", system_clock, unblocked_process.process_id);
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
        console_printf("[Clock: %d] Process %d blocked, resource %s unavailable\n", system_clock, current_process.process_id, name);
        block_process(idx);
        
    } else {
        mutexes[idx].value=-1;
        console_printf("[Clock: %d] Process %d: resource %s acquired\n", system_clock, current_process.process_id, name);
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
   
    console_printf("[Clock: %d] Process %d: resource %s released\n", system_clock, current_process.process_id, name);
    if (mutexes[idx].value <= 0) {
        if (blocked_queue[idx].size ==0) {
            mutexes[idx].value=1;
        }else{
            unblock_process(idx);
        }
        
        //printf("mutex[%d] = %d\n",idx,mutexes[idx].value);
    }
    
   
}



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
        console_printf("Error: No variable space available for process %d\n", current_process.process_id);
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
        console_printf("[Clock: %d] Process %d: %s = %s\n", system_clock, current_process.process_id, args, value);
        printf("[Clock: %d] Process %d: %s = %s\n", system_clock, current_process.process_id, args, value);
    } else {
        console_printf("[Clock: %d] Process %d: value not found %s\n", system_clock, current_process.process_id, args);
        console_printf("[Clock: %d] Process %d: value not found %s\n", system_clock, current_process.process_id, args);
    }
}
//chatgpt 
void process_assign(char *args) {    
    // First, check if args is NULL
    if (args == NULL) {
        console_printf("[Clock: %d] Process %d: Error: Invalid assign command (NULL args)\n", system_clock, current_process.process_id);
        return;
    }
    
    // Make a copy of args to prevent modifying the original
    char args_copy[MAX_LINE_LENGTH];
    strncpy(args_copy, args, MAX_LINE_LENGTH - 1);
    args_copy[MAX_LINE_LENGTH - 1] = '\0';  // Ensure null termination
    
    char *token_context = NULL;
    char *name = strtok_r(args_copy, " ", &token_context);
    
    if (name == NULL) {
        console_printf("[Clock: %d] Process %d: Error: Missing variable name in assign command\n", system_clock, current_process.process_id);
        return;
    }
    
    trim(name);
    
    // Get the rest of the command after the variable name
    char *rest = strtok_r(NULL, "", &token_context);
    if (rest == NULL) {
        console_printf("[Clock: %d] Process %d: Error: Missing value in assign command\n", system_clock, current_process.process_id);
        return;
    }
    
    // Check for input command
    if (strstr(rest, "input") != NULL) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_OK_CANCEL,
            "Process %d: Enter value for %s:",
            current_process.process_id, name);
        
        console_printf("[Clock: %d] Process %d: Value of %s taken\n", system_clock, current_process.process_id, name);
        GtkWidget *entry = gtk_entry_new();
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
        gtk_widget_show_all(dialog);
        
        gint result = gtk_dialog_run(GTK_DIALOG(dialog));
        if (result == GTK_RESPONSE_OK) {
            const gchar *user_input = gtk_entry_get_text(GTK_ENTRY(entry));
            set_variable(name, (char *)user_input);
        }
        gtk_widget_destroy(dialog);
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
            console_printf("[Clock: %d] Process %d: Error: Failed to read file %s\n", system_clock, current_process.process_id, filepath);
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
        console_printf("[Clock: %d] Process %d: Error: Invalid writeFile format\n", system_clock, current_process.process_id);
        return;
    }
    trim(name);
    trim(data);
    FILE *fptr = fopen(name,"w");

    if (fptr != NULL){
        fputs(data,fptr);
        console_printf("[Clock: %d] Process %d: File written successfully\n", system_clock, current_process.process_id);
    }else
    console_printf("[Clock: %d] Process %d: An error occurred writing to the file.\n", system_clock, current_process.process_id);

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
    update_gui();
    console_printf("[Clock: %d] Process %d: num_1 = %s, num_2 = %s, a = %d b = %d \n", system_clock, current_process.process_id, num_1, num_2, a, b);
    for (int i = a; i <= b; i++){
        console_printf("%i\n", i);
        console_printf("%i, ",i);
    }
    console_printf("\n");
    console_printf("\n");
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
        console_printf("Error: invalid PCB address.\n");
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
        case MLFQ:{
            int mlfq_quantum = 1 << (current_process.priority);
            if (quantum_tracking == mlfq_quantum && current_process.priority < 3) {
                current_process.priority++;
                int offset = current_process.lower_bound;
                sprintf(memory[offset + 2].value, "%d", current_process.priority);
            }
            int src_priority = program_done ? 3 : current_process.priority;
            queue* src = find_src(src_priority);
            queue* dest = &ready_queue[current_process.priority];
            if (src->size > 0 && quantum_tracking == mlfq_quantum || program_done || is_current_process_blocked) {
                switch_context(src, dest);
            }
            if (quantum_tracking == mlfq_quantum || program_done || is_current_process_blocked) {
                quantum_tracking = 0;
            }
            break;
        }
    }
    is_current_process_blocked = 0;
}


void check_for_processes() {
    while (program_index < total_programs && programs[program_index].arrival_time == system_clock) {
        console_printf("[Clock: %d] %s created.\n", system_clock, programs[program_index].name);
        create_process(programs[program_index++].name);
    }
}

void run_clock_cycle() {
    // check if new process is to be created
    check_for_processes();
    
    schedule();
   
    if (current_process.process_id == -1) {
        if (program_index == total_programs) {
            return;
        }
        console_printf("[Clock: %d] Idle\n", system_clock);
        system_clock++;
        return;
    }
   
    execute_line(memory[current_process.program_counter].value); 
    current_process.program_counter++;
    char program_done = current_process.program_counter == current_process.lower_bound + current_process.code_size + NUM_PCB;
    if (program_done) {
        console_printf("[Clock: %d] Process %d terminated\n", system_clock, current_process.process_id);
    }
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
        console_printf("[Clock: %d] Error: Out of process slots (Ask MINDO)\n", system_clock);
        return -1;
    }
    if (memory_allocated >= MEMORY_SIZE){
        console_printf("[Clock: %d] Error: Out of memory (Ask Ahmed Hassan)\n", system_clock);
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
        console_printf("[Clock: %d] Error: Not enough memory for process\n", system_clock);
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

void init_queues() {
    for (int i = 0; i < READY_QUEUE_SIZE; i++) {
        init_queue(&ready_queue[i]);
    }
    

    for (int i = 0; i < MAX_MUTEXES; i++) {
        createPriorityQueue(MAX_PROCESSES); 
    }
}

void init() {
    init_mutex();
    init_memory();
    init_queues();
}


int compare(const void *a, const void *b) {
    return ((program *)a)->arrival_time - ((program *)b)->arrival_time;
}

// int main(void) {
//     system_clock = 1;  // Initialize system clock
//     init();
    
//     printf("[Clock: %d] System initialized\n", system_clock);
//     print_memory();
    
//     char buffer[1024];
//     while (total_programs < MAX_PROCESSES) {
//         printf("Enter process name (or \"done\") - maximum of %d processes: ", MAX_PROCESSES);
//         if (!fgets(buffer, MAX_NAME_LENGTH, stdin)) break;
//         buffer[strcspn(buffer, "\n")] = '\0';
//         if (strcmp(buffer, "done") == 0) break;
//         programs[total_programs].name =  (char*) malloc(strlen(buffer));
//         strcpy(programs[total_programs].name, buffer);

//         printf("Enter arrival time: ");
//         if (scanf("%d", &programs[total_programs].arrival_time) != 1) break;
//         getchar();

//         total_programs++;
//     }

    

//     qsort(programs, total_programs, sizeof(program), compare);
//     print_memory();
//     while (1) {
//         run_clock_cycle();
//         if (current_process.process_id == -1 && program_index == total_programs) {
//             break;
//         }
//     }
    
//     printf("[Clock: %d] All processes completed\n", system_clock);
//     return 0;
// }

// Declare extern functions and variables from main.c
extern void run_clock_cycle();
extern void init();
extern void print_memory();
extern void init_queues();
extern void init_mutex();
extern void init_memory();
extern int create_process(const char *program);
extern int system_clock;
extern PCB current_process;
extern int scheduling;
extern int round_robin_quantum;
extern MemoryWord memory[MEMORY_SIZE];
extern queue ready_queue[READY_QUEUE_SIZE];
extern PriorityQueue blocked_queue[MAX_MUTEXES];
extern mutex mutexes[MAX_MUTEXES];
extern int process_count;
extern int memory_allocated;
extern int program_index;
extern int total_programs;
extern program programs[MAX_PROCESSES];

const char *light_theme_css = "window { background-color: #ffffff; }"
    "label { color: #000000; }"
    "button { background-color: #e0e0e0; color: #000000; }"
    "button:hover { background-color: #d0d0d0; }"
    "entry { background-color: #ffffff; color: #000000; }"
    "combobox { color: #000000; }"
    "treeview { background-color: #ffffff; color: #000000; }"
    "treeview:selected { background-color: #0077cc; color: #ffffff; }"
    "textview { background-color: #ffffff; color: #000000; }";

const char *dark_theme_css = "window { background-color: #1e1e1e; }"
    "label { color: #c0c0c0; }"
    "button { background-color: #2a2a2a; color: #c0c0c0; }"
    "button:hover { background-color: #3a3a3a; }"
    "entry { background-color: #2a2a2a; color: #c0c0c0; }"
    "combobox { color: #c0c0c0; }"
    "treeview { background-color: #1a1a1a; color: #c0c0c0; }"
    "treeview:selected { background-color: #3a3a3a; }"
    "textview { background-color: #000000; color: #00ff00; }";


// Auto execution
gboolean auto_execution = FALSE;
guint auto_timeout_id = 0;

// CSS Provider
GtkCssProvider *provider;

// Forward declarations
void update_gui();
void reset_simulation();
void show_add_process_dialog();
void add_process_to_simulation();
void update_memory_view();
void update_process_list();
void update_queue_lists();
void update_resource_list();
void append_to_console(const char *text);
void redirect_stdout();
static gboolean read_stdout(GIOChannel *channel, GIOCondition condition, gpointer data);
//dark mode
void toggle_theme(GtkButton *button, gpointer user_data);

// Pipe for stdout redirection
int pipefd[2];
GIOChannel *channel;

// Function to get current time as string (needs to be defined before use)
char* get_current_time_string() {
    static char time_str[20];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", t);
    return time_str;
}
//dynamic clock
static gboolean update_clock_display(gpointer data) {
    GtkWidget *label = GTK_WIDGET(data);
    char buffer[128];
    sprintf(buffer, "Real Time: %s", get_current_time_string());
    gtk_label_set_text(GTK_LABEL(label), buffer);
    return G_SOURCE_CONTINUE;
}

// Custom print to console function
void console_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(console_buffer, &iter);
    gtk_text_buffer_insert(console_buffer, &iter, buffer, -1);
    
    // Auto scroll to the end
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(console_text_view),
                               gtk_text_buffer_get_insert(console_buffer),
                               0.0, TRUE, 0.0, 1.0);
    
    va_end(args);
}

// Function to run a single clock cycle and update GUI
void step_simulation() {
    run_clock_cycle();
    update_gui();
    
    // Check if simulation is complete
    if (current_process.process_id == -1 && program_index == total_programs) {
        console_printf("[Clock: %d] All processes completed\n", system_clock);
        if (auto_execution) {
            g_source_remove(auto_timeout_id);
            auto_execution = FALSE;
            gtk_button_set_label(GTK_BUTTON(auto_button), "Start Auto");
        }
    }
}

// Auto execution timeout function
static gboolean auto_execution_callback(gpointer data) {
    if (!auto_execution) return FALSE;
    
    if (current_process.process_id == -1 && program_index == total_programs) {
        auto_execution = FALSE;
        gtk_button_set_label(GTK_BUTTON(auto_button), "Start Auto");
        return FALSE;
    }
    
    step_simulation();
    return TRUE;
}

// Signal handlers
void on_step_button_clicked(GtkWidget *widget, gpointer data) {
    step_simulation();
}

void on_auto_button_clicked(GtkWidget *widget, gpointer data) {
    auto_execution = !auto_execution;
    
    if (auto_execution) {
        gtk_button_set_label(GTK_BUTTON(auto_button), "Stop Auto");
        auto_timeout_id = g_timeout_add(500, auto_execution_callback, NULL);
    } else {
        gtk_button_set_label(GTK_BUTTON(auto_button), "Start Auto");
        if (auto_timeout_id > 0) {
            g_source_remove(auto_timeout_id);
            auto_timeout_id = 0;
        }
    }
}

void on_reset_button_clicked(GtkWidget *widget, gpointer data) {
    if (auto_execution) {
        auto_execution = FALSE;
        gtk_button_set_label(GTK_BUTTON(auto_button), "Start Auto");
        if (auto_timeout_id > 0) {
            g_source_remove(auto_timeout_id);
            auto_timeout_id = 0;
        }
    }
    
    reset_simulation();
    console_printf("╔════════════════════════════════════════════════════╗\n");
    console_printf("║            CACOOS OS Simulator Started             ║\n");
    console_printf("║                                                    ║\n");
    console_printf("║  Welcome to the CACOOS OS Simulator!               ║\n");
    console_printf("║  I think I know you ?                              ║\n");
    console_printf("║  Current Time: %s                            ║\n", get_current_time_string());
    console_printf("║                                                    ║\n");
    console_printf("║  Click 'Add Process' to load programs              ║\n");
    console_printf("║  Click 'Step' to execute one clock cycle           ║\n");
    console_printf("║  Click 'Start Auto' for continuous execution       ║\n");
    console_printf("╚════════════════════════════════════════════════════╝\n\n");

    update_gui();
}

void on_add_process_button_clicked(GtkWidget *widget, gpointer data) {
    show_add_process_dialog();
}

void on_scheduler_changed(GtkComboBox *widget, gpointer data) {
    int active = gtk_combo_box_get_active(widget);
    
    switch (active) {
        case 0:
            scheduling = FIFO;
            gtk_widget_set_sensitive(quantum_spinner, FALSE);
            break;
        case 1:
            scheduling = ROUND_ROBIN;
            gtk_widget_set_sensitive(quantum_spinner, TRUE);
            break;
        case 2:
            scheduling = MLFQ;
            gtk_widget_set_sensitive(quantum_spinner, FALSE);
            break;
    }
    
    console_printf("[Clock: %d] Scheduler algorithm changed to %s\n", 
                  system_clock, 
                  active == 0 ? "FIFO" : 
                  active == 1 ? "Round Robin" : "MLFQ");
}

void on_quantum_changed(GtkSpinButton *spin_button, gpointer data) {
    round_robin_quantum = gtk_spin_button_get_value_as_int(spin_button);
    console_printf("[Clock: %d] Round Robin quantum set to %d\n", system_clock, round_robin_quantum);
}


// Update all GUI components
// void update_gui() {
//     char buffer[128];
    
//     // Update clock and process count
//     sprintf(buffer, "System Clock: %d", system_clock);
//     gtk_label_set_text(GTK_LABEL(clock_label), buffer);
    
//     sprintf(buffer, "Process Count: %d/%d", process_count, MAX_PROCESSES);
//     gtk_label_set_text(GTK_LABEL(process_count_label), buffer);
    
//     // Update views
//     update_memory_view();
//     update_process_list();
//     update_queue_lists();
//     update_resource_list();
    
//     // Process events to update UI
//     while (gtk_events_pending())
//         gtk_main_iteration();
// }

// Function to draw process icons (forward declaration before create_gui)
gboolean draw_process_icon(GtkWidget *widget, cairo_t *cr, gpointer data);

// Reset simulation
void reset_simulation() {
    // Clear console
    gtk_text_buffer_set_text(console_buffer, "", -1);
    
    // Reset simulation variables
    system_clock = 1;
    total_programs = 0;
    program_index = 0;
    process_count = 0;
    memory_allocated = 0;
    current_process.process_id = -1;
    
    // Re-initialize subsystems
    init_memory();
    init_queues();
    init_mutex();
    
    console_printf("[Clock: %d] System reset and initialized\n", system_clock);
}

//test
GtkWidget *load_button;
char *on_load_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *dialog;
    GtkFileFilter *filter;
    
    dialog = gtk_file_chooser_dialog_new("Open Program File",
                                        GTK_WINDOW(window),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Open", GTK_RESPONSE_ACCEPT,
                                        NULL);
    
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text Files");
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Create process with the selected file
        int pid = create_process(filename);
        if (pid > 0) {
            console_printf("[Clock: %d] Added program \"%s\"\n", 
                system_clock, filename);
        } else {
            console_printf("[Clock: %d] Failed to add program \"%s\"\n", 
                system_clock, filename);
        }
        
        //g_free(filename);

        update_gui();
        
    }
    
    gtk_widget_destroy(dialog);
}

// Add process dialog
void show_add_process_dialog() {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *grid;
    
    dialog = gtk_dialog_new_with_buttons("Add Process",
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       "Cancel",
                                       GTK_RESPONSE_CANCEL,
                                       "Add",
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);
    
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);

    // Create button box for load button
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    load_button = gtk_button_new_with_label("Load Program");
    g_signal_connect(load_button, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), load_button, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid), button_box, 0, 0, 3, 1);

    // Program name
    GtkWidget *name_label = gtk_label_new("Program name:");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 1, 1, 1);
    
    program_name_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), program_name_entry, 1, 1, 2, 1);
    
    // Arrival time
    GtkWidget *time_label = gtk_label_new("Arrival time:");
    gtk_widget_set_halign(time_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), time_label, 0, 2, 1, 1);
    
    arrival_time_entry = gtk_spin_button_new_with_range(system_clock, 999, 1);
    gtk_grid_attach(GTK_GRID(grid), arrival_time_entry, 1, 2, 2, 1);
    
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        const char *program_name = gtk_entry_get_text(GTK_ENTRY(program_name_entry));
        int arrival_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(arrival_time_entry));
        
        if (program_name && strlen(program_name) > 0) {
            if (total_programs < MAX_PROCESSES) {
                programs[total_programs].name = strdup(program_name);
                programs[total_programs].arrival_time = arrival_time;
                total_programs++;
                
                console_printf("[Clock: %d] Added program \"%s\" with arrival time %d\n", 
                              system_clock, program_name, arrival_time);
                update_gui();
            } else {
                console_printf("[Clock: %d] Error: Maximum process limit reached\n", system_clock);
            }
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Add the process to the simulation
void add_process_to_simulation() {
    const char *program_name = gtk_entry_get_text(GTK_ENTRY(program_name_entry));
    int arrival_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(arrival_time_entry));
    
    if (program_name && strlen(program_name) > 0) {
        if (total_programs < MAX_PROCESSES) {
            programs[total_programs].name = strdup(program_name);
            programs[total_programs].arrival_time = arrival_time;
            total_programs++;
            
            console_printf("[Clock: %d] Added program \"%s\" with arrival time %d\n", 
                          system_clock, program_name, arrival_time);
            update_gui();
        } else {
            console_printf("[Clock: %d] Error: Maximum process limit reached\n", system_clock);
        }
    }
}

// Update memory view
void update_memory_view() {
    gtk_list_store_clear(memory_store);
    
    GtkTreeIter iter;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        gtk_list_store_append(memory_store, &iter);
        
        const char *type_str;
        switch (memory[i].type) {
            case VAR: type_str = "Variable"; break;
            case CODE: type_str = "Code"; break;
            case T_PCB: type_str = "PCB"; break;
            default: type_str = "Empty"; break;
        }
        
        gtk_list_store_set(memory_store, &iter,
                         0, i,  // Address
                         1, memory[i].process_id,  // Process ID
                         2, type_str,  // Type
                         3, memory[i].name,  // Name
                         4, memory[i].value,  // Value
                         -1);
    }
}

// Update process list
void update_process_list() {
    gtk_list_store_clear(process_store);
    
    GtkTreeIter iter;
    for (int i = 0; i < memory_allocated; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "Process_ID") == 0) {
            PCB process = load_PCB(i);
            
            const char *state_str;
            switch (process.state) {
                case NEW: state_str = "NEW"; break;
                case READY: state_str = "READY"; break;
                case RUNNING: state_str = "RUNNING"; break;
                case BLOCKED: state_str = "BLOCKED"; break;
                case TERMINATED: state_str = "TERMINATED"; break;
                default: state_str = "UNKNOWN"; break;
            }
            
            char bounds[32];
            sprintf(bounds, "%d-%d", process.lower_bound, process.upper_bound);
            
            gtk_list_store_append(process_store, &iter);
            gtk_list_store_set(process_store, &iter,
                             0, process.process_id,  // Process ID
                             1, state_str,  // State
                             2, process.priority,  // Priority
                             3, process.program_counter,  // PC
                             4, bounds,  // Memory bounds
                             -1);
        }
    }
}

// Update queue lists
void update_queue_lists() {
    // Clear ready queue list
    gtk_list_store_clear(ready_queue_store);
    
    // Add items from all ready queues
    GtkTreeIter iter;
    for (int q = 0; q < READY_QUEUE_SIZE; q++) {
        if (ready_queue[q].size > 0) {
            node* current = ready_queue[q].tail ? ready_queue[q].tail->next : NULL;
            for (int i = 0; i < ready_queue[q].size; i++) {
                PCB process = load_PCB(current->value);
                
                gtk_list_store_append(ready_queue_store, &iter);
                gtk_list_store_set(ready_queue_store, &iter,
                                 0, process.process_id,  // Process ID
                                 1, q,  // Queue level
                                 2, process.program_counter,  // PC
                                 -1);
                
                current = current->next;
                if (current == ready_queue[q].tail->next) break;
            }
        }
    }
    
    // Clear blocked queue list
    gtk_list_store_clear(blocked_queue_store);
    
    // Add items from all blocked queues
    for (int m = 0; m < MAX_MUTEXES; m++) {
        for (int i = 1; i <= blocked_queue[m].size; i++) {
            PCB process = load_PCB(blocked_queue[m].array[i].data);
            
            gtk_list_store_append(blocked_queue_store, &iter);
            gtk_list_store_set(blocked_queue_store, &iter,
                             0, process.process_id,    // Process ID
                             1, mutexes[m].name,       // Resource name
                             2, blocked_queue[m].array[i].priority,  // Priority
                             -1);
        }
    }
}

// Update resource list
void update_resource_list() {
    gtk_list_store_clear(resource_store);
    
    GtkTreeIter iter;
    for (int i = 0; i < MAX_MUTEXES; i++) {
        gtk_list_store_append(resource_store, &iter);
        
        const char *status = (mutexes[i].value > 0) ? "Available" : "In Use";
        
        int owner_pid = -1;
        if (mutexes[i].value <= 0) {
            // Find which process is currently using this resource
            for (int j = 0; j < memory_allocated; j++) {
                if (memory[j].type == T_PCB && strcmp(memory[j].name, "Process_ID") == 0) {
                    PCB process = load_PCB(j);
                    if (process.state == RUNNING) {
                        owner_pid = process.process_id;
                        break;
                    }
                }
            }
        }
        
        gtk_list_store_set(resource_store, &iter,
                         0, mutexes[i].name,  // Resource name
                         1, status,  // Status
                         2, owner_pid,  // Owner (if any)
                         3, blocked_queue[i].size,  // Waiting count
                         -1);
    }
}

// Set up stdout redirection to console
void redirect_stdout() {
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Redirect stdout to the pipe
    dup2(pipefd[1], STDOUT_FILENO);
    
    // Create GIOChannel for the pipe
    channel = g_io_channel_unix_new(pipefd[0]);
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP, read_stdout, NULL);
}

// Read from stdout and append to console
static gboolean read_stdout(GIOChannel *channel, GIOCondition condition, gpointer data) {
    gchar buf[1024];
    gsize bytes_read;
    GIOStatus status;
    
    if (condition & G_IO_HUP) {
        g_io_channel_unref(channel);
        return FALSE;
    }
    
    while ((status = g_io_channel_read_chars(channel, buf, sizeof(buf) - 1, &bytes_read, NULL)) == G_IO_STATUS_NORMAL) {
        if (bytes_read > 0) {
            buf[bytes_read] = '\0';
            GtkTextIter iter;
            gtk_text_buffer_get_end_iter(console_buffer, &iter);
            gtk_text_buffer_insert(console_buffer, &iter, buf, -1);
            
            // Auto scroll to the end
            gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(console_text_view),
                                       gtk_text_buffer_get_insert(console_buffer),
                                       0.0, TRUE, 0.0, 1.0);
        }
    }
    
    return TRUE;
}

// Create GUI
void create_gui() {
    // Create window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "CACOOS OS Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Create CSS provider
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #ffffff; }"
        "label { color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "button { background-color: #e0e0e0; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "button:hover { background-color: #d0d0d0; }"
        "entry { background-color: #ffffff; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "combobox { color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "combobox * { font-size: 14px !important; }"
        "treeview { background-color: #ffffff; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "treeview:selected { background-color: #0077cc; color: #ffffff; }"
        "textview { background-color: #ffffff; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "textview text { font-size: 14px !important; }"
        , -1, NULL);
    
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    // Create main grid
    main_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(main_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_grid), 15);
    gtk_container_add(GTK_CONTAINER(window), main_grid);
    
    // Create sidebar (control panel)
    sidebar = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(sidebar), 10);
    gtk_container_set_border_width(GTK_CONTAINER(sidebar), 10);
    gtk_grid_attach(GTK_GRID(main_grid), sidebar, 0, 0, 1, 3);
    
    // Create system info labels
    GtkWidget *info_frame = gtk_frame_new("System Info");
    GtkWidget *info_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(info_grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(info_grid), 10);
    gtk_container_add(GTK_CONTAINER(info_frame), info_grid);
    
    clock_label = gtk_label_new("System Clock: 1");
    gtk_widget_set_halign(clock_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(info_grid), clock_label, 0, 0, 1, 1);
    
    process_count_label = gtk_label_new("Process Count: 0/6");
    gtk_widget_set_halign(process_count_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(info_grid), process_count_label, 0, 1, 1, 1);
    
    //clock
    GtkWidget *real_time_label = gtk_label_new("");
    gtk_widget_set_halign(real_time_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(info_grid), real_time_label, 0, 2, 1, 1);

    // Start the clock timer
    clock_timer_id = g_timeout_add(1000, update_clock_display, real_time_label);

    gtk_grid_attach(GTK_GRID(sidebar), info_frame, 0, 0, 1, 1);
    
    // Create scheduler controls
    GtkWidget *scheduler_frame = gtk_frame_new("Scheduler");
    GtkWidget *scheduler_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(scheduler_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(scheduler_grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(scheduler_grid), 10);
    gtk_container_add(GTK_CONTAINER(scheduler_frame), scheduler_grid);
    
    GtkWidget *scheduler_label = gtk_label_new("Algorithm:");
    gtk_widget_set_halign(scheduler_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(scheduler_grid), scheduler_label, 0, 0, 1, 1);
    
    scheduler_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scheduler_combo), "FIFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scheduler_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scheduler_combo), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(scheduler_combo), 0);
    g_signal_connect(scheduler_combo, "changed", G_CALLBACK(on_scheduler_changed), NULL);
    gtk_grid_attach(GTK_GRID(scheduler_grid), scheduler_combo, 1, 0, 1, 1);
    
    GtkWidget *quantum_label = gtk_label_new("Quantum:");
    gtk_widget_set_halign(quantum_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(scheduler_grid), quantum_label, 0, 1, 1, 1);
    
    quantum_spinner = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(quantum_spinner), round_robin_quantum);
    g_signal_connect(quantum_spinner, "value-changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_grid_attach(GTK_GRID(scheduler_grid), quantum_spinner, 1, 1, 1, 1);
    gtk_widget_set_sensitive(quantum_spinner, FALSE);
    
    gtk_grid_attach(GTK_GRID(sidebar), scheduler_frame, 0, 1, 1, 1);

    // Create simulation controls
    GtkWidget *control_frame = gtk_frame_new("Controls");
    GtkWidget *control_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(control_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(control_grid), 10);
    gtk_container_add(GTK_CONTAINER(control_frame), control_grid);
    
    step_button = gtk_button_new_with_label("Step");
    g_signal_connect(step_button, "clicked", G_CALLBACK(on_step_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(control_grid), step_button, 0, 0, 1, 1);
    
    auto_button = gtk_button_new_with_label("Start Auto");
    g_signal_connect(auto_button, "clicked", G_CALLBACK(on_auto_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(control_grid), auto_button, 0, 1, 1, 1);
    
    reset_button = gtk_button_new_with_label("Reset");
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(control_grid), reset_button, 0, 2, 1, 1);
    
    add_process_button = gtk_button_new_with_label("Add Process");
    g_signal_connect(add_process_button, "clicked", G_CALLBACK(on_add_process_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(control_grid), add_process_button, 0, 3, 1, 1);
    
    // Add theme button
    theme_button = gtk_button_new_with_label("Dark Mode");
    g_signal_connect(theme_button, "clicked", G_CALLBACK(toggle_theme), NULL);
    gtk_grid_attach(GTK_GRID(control_grid), theme_button, 0, 4, 1, 1);
    
    gtk_grid_attach(GTK_GRID(sidebar), control_frame, 0, 2, 1, 1);
    
    // Create memory view
    GtkWidget *memory_frame = gtk_frame_new("Memory");
    GtkWidget *memory_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(memory_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    memory_store = gtk_list_store_new(5, 
                                    G_TYPE_INT,     // Address
                                    G_TYPE_INT,     // Process ID
                                    G_TYPE_STRING,  // Type
                                    G_TYPE_STRING,  // Name
                                    G_TYPE_STRING); // Value
    
    memory_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(memory_store));
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Addr", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Value", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_list), column);
    
    gtk_container_add(GTK_CONTAINER(memory_scroll), memory_list);
    gtk_container_add(GTK_CONTAINER(memory_frame), memory_scroll);
    gtk_widget_set_size_request(memory_scroll, 500, 250);
    
    // Create process list
    GtkWidget *process_frame = gtk_frame_new("Processes");
    GtkWidget *process_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(process_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    process_store = gtk_list_store_new(5, 
                                     G_TYPE_INT,     // Process ID
                                     G_TYPE_STRING,  // State
                                     G_TYPE_INT,     // Priority
                                     G_TYPE_INT,     // Program Counter
                                     G_TYPE_STRING); // Memory bounds
    
    process_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(process_store));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("State", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PC", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Memory", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list), column);
    
    gtk_container_add(GTK_CONTAINER(process_scroll), process_list);
    gtk_container_add(GTK_CONTAINER(process_frame), process_scroll);
    gtk_widget_set_size_request(process_scroll, 500, 150);
    
    // Create queue lists
    GtkWidget *queue_frame = gtk_frame_new("Queues");
    GtkWidget *queue_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(queue_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(queue_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(queue_grid), 10);
    gtk_container_add(GTK_CONTAINER(queue_frame), queue_grid);
    
    // Ready queue
    GtkWidget *ready_label = gtk_label_new("Ready Queue");
    gtk_grid_attach(GTK_GRID(queue_grid), ready_label, 0, 0, 1, 1);
    
    GtkWidget *ready_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ready_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    ready_queue_store = gtk_list_store_new(3,
                                         G_TYPE_INT,    // Process ID
                                         G_TYPE_INT,    // Queue level
                                         G_TYPE_INT);   // Program Counter
    
    ready_queue_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ready_queue_store));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Queue", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PC", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_list), column);
    
    gtk_container_add(GTK_CONTAINER(ready_scroll), ready_queue_list);
    gtk_widget_set_size_request(ready_scroll, 230, 150);
    gtk_grid_attach(GTK_GRID(queue_grid), ready_scroll, 0, 1, 1, 1);
    
    // Blocked queue
    GtkWidget *blocked_label = gtk_label_new("Blocked Queue");
    gtk_grid_attach(GTK_GRID(queue_grid), blocked_label, 1, 0, 1, 1);
    
    GtkWidget *blocked_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(blocked_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    blocked_queue_store = gtk_list_store_new(3,
                                           G_TYPE_INT,     // Process ID
                                           G_TYPE_STRING,  // Resource name
                                           G_TYPE_INT);    // Priority
    
    blocked_queue_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(blocked_queue_store));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Resource", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_list), column);
    
    gtk_container_add(GTK_CONTAINER(blocked_scroll), blocked_queue_list);
    gtk_widget_set_size_request(blocked_scroll, 230, 150);
    gtk_grid_attach(GTK_GRID(queue_grid), blocked_scroll, 1, 1, 1, 1);
    
    // Create resource list
    GtkWidget *resource_frame = gtk_frame_new("Resources");
    GtkWidget *resource_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(resource_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    resource_store = gtk_list_store_new(4,
                                      G_TYPE_STRING,  // Resource name
                                      G_TYPE_STRING,  // Status
                                      G_TYPE_INT,     // Owner process ID
                                      G_TYPE_INT);    // Waiting count
    
    resource_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(resource_store));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Resource", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(resource_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Status", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(resource_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Owner", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(resource_list), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Waiting", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(resource_list), column);
    
    gtk_container_add(GTK_CONTAINER(resource_scroll), resource_list);
    gtk_container_add(GTK_CONTAINER(resource_frame), resource_scroll);
    gtk_widget_set_size_request(resource_scroll, 500, 150);
    
    // Create console output
    GtkWidget *console_frame = gtk_frame_new("Console Output");
    GtkWidget *console_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(console_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    console_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(console_text_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(console_text_view), TRUE);
    
    // Set console background to black - using proper GTK3 style context
    GtkStyleContext *context = gtk_widget_get_style_context(console_text_view);
    GdkRGBA console_bg_color;
    gdk_rgba_parse(&console_bg_color, "#000000");
    gtk_style_context_add_class(context, "console-view");
    
    console_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_text_view));
    
    // Add custom font - monospace with larger size for retro feel
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 12");
    GtkCssProvider *font_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(font_provider,
        "textview { font-family: 'Monospace'; font-size: 12px; }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(console_text_view),
        GTK_STYLE_PROVIDER(font_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(font_provider);
    pango_font_description_free(font_desc);
    
    gtk_container_add(GTK_CONTAINER(console_scroll), console_text_view);
    gtk_container_add(GTK_CONTAINER(console_frame), console_scroll);
    gtk_widget_set_size_request(console_scroll, 500, 300);
    
    // Add process icons section (Xerox Star-like)
    GtkWidget *icons_frame = gtk_frame_new("Process Icons");
    GtkWidget *icons_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(icons_box), 10);
    gtk_container_add(GTK_CONTAINER(icons_frame), icons_box);
    
    // We'll dynamically add process icons during runtime
    // Just add placeholder for now
    for (int i = 0; i < MAX_PROCESSES; i++) {
        GtkWidget *icon_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        
        // Create a drawing area for the icon
        GtkWidget *icon = gtk_drawing_area_new();
        gtk_widget_set_size_request(icon, 48, 48);
        
        // Connect draw signal
        g_signal_connect(icon, "draw", G_CALLBACK(draw_process_icon), GINT_TO_POINTER(i));
        
        GtkWidget *icon_label = gtk_label_new("---");
        
        gtk_box_pack_start(GTK_BOX(icon_box), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(icon_box), icon_label, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(icons_box), icon_box, FALSE, FALSE, 0);
        
        // Store references to update later
        process_icons[i].drawing_area = icon;
        process_icons[i].label = icon_label;
        process_icons[i].active = FALSE;
    }
    
    gtk_grid_attach(GTK_GRID(sidebar), icons_frame, 0, 3, 1, 1);
    
    // Create main notebook for switching between views
    GtkWidget *main_notebook = gtk_notebook_new();
    
    // Create "Default View" page
    GtkWidget *default_view = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(default_view), 10);
    gtk_grid_set_column_spacing(GTK_GRID(default_view), 10);
    gtk_container_set_border_width(GTK_CONTAINER(default_view), 10);

    // Add existing views to default view
    gtk_grid_attach(GTK_GRID(default_view), memory_frame, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(default_view), process_frame, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(default_view), queue_frame, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(default_view), resource_frame, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(default_view), console_frame, 1, 1, 1, 2);

    // Add default view to notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), default_view, 
                            gtk_label_new("Default View"));

    // Memory View Tab
    GtkWidget *memory_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *memory_scroll_tab = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *memory_list_tab = gtk_tree_view_new_with_model(GTK_TREE_MODEL(memory_store));

    // Copy columns from memory_list
    for (int i = 0; i < gtk_tree_view_get_n_columns(GTK_TREE_VIEW(memory_list)); i++) {
        GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(memory_list), i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(memory_list_tab), 
                                gtk_tree_view_column_new_with_attributes(
                                    gtk_tree_view_column_get_title(col),
                                    gtk_cell_renderer_text_new(),
                                    "text", i,
                                    NULL));
    }

    gtk_container_add(GTK_CONTAINER(memory_scroll_tab), memory_list_tab);
    gtk_box_pack_start(GTK_BOX(memory_page), memory_scroll_tab, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), memory_page,
                            gtk_label_new("Memory"));

    // Console View Tab
    GtkWidget *console_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *console_scroll_tab = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *console_text_view_tab = gtk_text_view_new_with_buffer(console_buffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text_view_tab), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(console_text_view_tab), FALSE);
    gtk_container_add(GTK_CONTAINER(console_scroll_tab), console_text_view_tab);
    gtk_box_pack_start(GTK_BOX(console_page), console_scroll_tab, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), console_page,
                            gtk_label_new("Console"));

    // Queue View Tab
    GtkWidget *queue_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *queue_scroll_tab = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *queue_grid_tab = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(queue_grid_tab), 10);
    gtk_grid_set_column_spacing(GTK_GRID(queue_grid_tab), 10);
    gtk_container_set_border_width(GTK_CONTAINER(queue_grid_tab), 10);
    
    // Ready queue tab
    GtkWidget *ready_queue_list_tab = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ready_queue_store));
    // Blocked queue tab
    GtkWidget *blocked_queue_list_tab = gtk_tree_view_new_with_model(GTK_TREE_MODEL(blocked_queue_store));

    // Copy columns for ready queue
    for (int i = 0; i < gtk_tree_view_get_n_columns(GTK_TREE_VIEW(ready_queue_list)); i++) {
        GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(ready_queue_list), i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_list_tab), 
                                gtk_tree_view_column_new_with_attributes(
                                    gtk_tree_view_column_get_title(col),
                                    gtk_cell_renderer_text_new(),
                                    "text", i,
                                    NULL));
    }

    // Copy columns for blocked queue
    for (int i = 0; i < gtk_tree_view_get_n_columns(GTK_TREE_VIEW(blocked_queue_list)); i++) {
        GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(blocked_queue_list), i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_list_tab), 
                                gtk_tree_view_column_new_with_attributes(
                                    gtk_tree_view_column_get_title(col),
                                    gtk_cell_renderer_text_new(),
                                    "text", i,
                                    NULL));
    }

    // Add queues to the grid
    gtk_grid_attach(GTK_GRID(queue_grid_tab), ready_queue_list_tab, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(queue_grid_tab), blocked_queue_list_tab, 1, 0, 1, 1);
    
    // Add grid to scroll window
    gtk_container_add(GTK_CONTAINER(queue_scroll_tab), queue_grid_tab);
    gtk_box_pack_start(GTK_BOX(queue_page), queue_scroll_tab, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), queue_page,
                            gtk_label_new("Queues"));

    // Resource View Tab
    GtkWidget *resource_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *resource_scroll_tab = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *resource_list_tab = gtk_tree_view_new_with_model(GTK_TREE_MODEL(resource_store));

    // Copy columns from resource_list
    for (int i = 0; i < gtk_tree_view_get_n_columns(GTK_TREE_VIEW(resource_list)); i++) {
        GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(resource_list), i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(resource_list_tab), 
                                gtk_tree_view_column_new_with_attributes(
                                    gtk_tree_view_column_get_title(col),
                                    gtk_cell_renderer_text_new(),
                                    "text", i,
                                    NULL));
    }

    gtk_container_add(GTK_CONTAINER(resource_scroll_tab), resource_list_tab);
    gtk_box_pack_start(GTK_BOX(resource_page), resource_scroll_tab, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), resource_page,
                            gtk_label_new("Resources"));

    // Add notebook to main grid
    gtk_grid_attach(GTK_GRID(main_grid), main_notebook, 1, 0, 2, 3);

    // Display all widgets
    gtk_widget_show_all(window);
    
    // Initially update GUI
    update_gui();
    
    // Welcome message
    console_printf("╔════════════════════════════════════════════════════╗\n");
    console_printf("║            CACOOS OS Simulator Started             ║\n");
    console_printf("║                                                    ║\n");
    console_printf("║  Welcome to the CACOOS OS Simulator!               ║\n");
    console_printf("║  Current Time: %s                            ║\n", get_current_time_string());
    console_printf("║                                                    ║\n");
    console_printf("║  Click 'Add Process' to load programs              ║\n");
    console_printf("║  Click 'Step' to execute one clock cycle           ║\n");
    console_printf("║  Click 'Start Auto' for continuous execution       ║\n");
    console_printf("╚════════════════════════════════════════════════════╝\n\n");
}
//dark mode
void toggle_theme(GtkButton *button, gpointer user_data) {
    dark_mode = !dark_mode;
    
    // Update button label
    gtk_button_set_label(GTK_BUTTON(theme_button), 
                        dark_mode ? "Light Mode" : "Dark Mode");
    
    // Create CSS with consistent font sizes
    const char *theme_css = dark_mode ? 
        "window { background-color: #1e1e1e; }"
        "label { color: #c0c0c0; font-family: 'Monospace'; font-size: 14px !important; }"
        "button { background-color: #2a2a2a; color: #c0c0c0; font-family: 'Monospace'; font-size: 14px !important; }"
        "button:hover { background-color: #3a3a3a; }"
        "entry { background-color: #2a2a2a; color: #c0c0c0; font-family: 'Monospace'; font-size: 14px !important; }"
        "combobox { color: #c0c0c0; font-family: 'Monospace'; font-size: 14px !important; }"
        "combobox * { font-size: 14px !important; }"
        "treeview { background-color: #1a1a1a; color: #c0c0c0; font-family: 'Monospace'; font-size: 14px !important; }"
        "treeview:selected { background-color: #3a3a3a; }"
        "textview { background-color: #000000; color: #00ff00; font-family: 'Monospace'; font-size: 14px !important; }"
        "textview text { font-size: 14px !important; }" :
        
        "window { background-color: #ffffff; }"
        "label { color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "button { background-color: #e0e0e0; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "button:hover { background-color: #d0d0d0; }"
        "entry { background-color: #ffffff; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "combobox { color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "combobox * { font-size: 14px !important; }"
        "treeview { background-color: #ffffff; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "treeview:selected { background-color: #0077cc; color: #ffffff; }"
        "textview { background-color: #ffffff; color: #000000; font-family: 'Monospace'; font-size: 14px !important; }"
        "textview text { font-size: 14px !important; }";
    
    // Apply theme
    gtk_css_provider_load_from_data(provider, theme_css, -1, NULL);
    
    // Force consistent console font
    GtkCssProvider *console_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(console_provider,
        "textview, textview text { font-family: 'Monospace'; font-size: 14px !important; }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(console_text_view),
        GTK_STYLE_PROVIDER(console_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(console_provider);
    
    console_printf("[Clock: %d] Switched to %s theme\n", 
                  system_clock, 
                  dark_mode ? "dark" : "light");
}
// Function to draw process icons
gboolean draw_process_icon(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int idx = GPOINTER_TO_INT(data);
    
    // Get process info if active
    if (process_icons[idx].active) {
        PCB process = load_PCB(process_icons[idx].pcb_address);
        
        // Choose color based on process state
        if (process.state == RUNNING) {
            // Green for running
            cairo_set_source_rgb(cr, 0.2, 0.8, 0.2);
        } else if (process.state == READY) {
            // Blue for ready
            cairo_set_source_rgb(cr, 0.2, 0.6, 0.8);
        } else if (process.state == BLOCKED) {
            // Red for blocked
            cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);
        } else if (process.state == TERMINATED) {
            // Gray for terminated
            cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        } else {
            // Yellow for new
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.2);
        }
    } else {
        // Empty slot - dark gray
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    }
    
    // Draw document icon (Xerox Star style)
    double width = gtk_widget_get_allocated_width(widget);
    double height = gtk_widget_get_allocated_height(widget);
    
    // Document body
    cairo_rectangle(cr, width * 0.2, height * 0.1, width * 0.6, height * 0.8);
    cairo_fill(cr);
    
    // Document dog-ear
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_move_to(cr, width * 0.6, height * 0.1);
    cairo_line_to(cr, width * 0.8, height * 0.1);
    cairo_line_to(cr, width * 0.8, height * 0.3);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Document lines
    if (process_icons[idx].active) {
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        for (int i = 0; i < 3; i++) {
            cairo_move_to(cr, width * 0.3, height * (0.3 + i * 0.15));
            cairo_line_to(cr, width * 0.7, height * (0.3 + i * 0.15));
            cairo_stroke(cr);
        }
    }
    
    return FALSE;
}

// Update process icons
void update_process_icons() {
    // Reset all icons
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_icons[i].active = FALSE;
        gtk_label_set_text(GTK_LABEL(process_icons[i].label), "---");
    }
    
    // Find active processes and update icons
    int icon_idx = 0;
    for (int i = 0; i < memory_allocated; i++) {
        if (memory[i].type == T_PCB && strcmp(memory[i].name, "Process_ID") == 0 && icon_idx < MAX_PROCESSES) {
            PCB process = load_PCB(i);
            
            process_icons[icon_idx].active = TRUE;
            process_icons[icon_idx].pcb_address = i;
            
            char pid_str[10];
            sprintf(pid_str, "PID %d", process.process_id);
            gtk_label_set_text(GTK_LABEL(process_icons[icon_idx].label), pid_str);
            
            // Force redraw of icon
            gtk_widget_queue_draw(process_icons[icon_idx].drawing_area);
            
            icon_idx++;
        }
    }
}

// Enhanced update GUI function
void update_gui() {
    char buffer[128];
    
    // Update clock and process count
    sprintf(buffer, "System Clock: %d", system_clock);
    gtk_label_set_text(GTK_LABEL(clock_label), buffer);
    
    sprintf(buffer, "Process Count: %d/%d", process_count, MAX_PROCESSES);
    gtk_label_set_text(GTK_LABEL(process_count_label), buffer);
    
    // Update views
    update_memory_view();
    update_process_list();
    update_queue_lists();
    update_resource_list();
    update_process_icons();
    gtk_widget_queue_draw(memory_list_tab);
    gtk_widget_queue_draw(console_text_view_tab);
    gtk_widget_queue_draw(ready_queue_list_tab);
    gtk_widget_queue_draw(blocked_queue_list_tab);
    gtk_widget_queue_draw(resource_list_tab);
    
    // Process events to update UI
    while (gtk_events_pending())
        gtk_main_iteration();
}

// Main function for GUI application
int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Set up stdout redirection to console
    redirect_stdout();
    
    // Initialize simulator
    init();
    
    // Create GUI
    create_gui();
    
    // Start GTK main loop
    gtk_main();
    
    return 0;
}

