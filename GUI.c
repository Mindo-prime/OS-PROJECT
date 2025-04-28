#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pango.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <ctype.h> 

#include <fontconfig/fontconfig.h>
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
#define ROUND_ROBIN 1
#define FIFO 2
#define MLFQ 3

// Added global clock
int scheduling = ROUND_ROBIN;
int system_clock = 1;

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

typedef struct node {
    int value;
    struct node* next;
} node;

typedef struct {
    node* tail;
    int size;
} queue;
//ui
extern void init();
extern int create_process(const char *program);
extern void run_clock_cycle();
extern void print_memory();
extern void print_process_stats();
extern MemoryWord memory[MEMORY_SIZE];
extern PCB processes[MAX_PROCESSES];
extern int process_count;
extern PCB current_process;
extern queue ready_queue[4];
extern int system_clock;
extern int memory_allocated;
extern int round_robin_quantum;
extern int scheduling;

// GUI components
GtkWidget *window;
GtkWidget *main_grid;
GtkWidget *header_bar;
GtkWidget *clock_label;
GtkWidget *algorithm_combo;
GtkWidget *quantum_spin;
GtkWidget *start_button;
GtkWidget *step_button;
GtkWidget *stop_button;
GtkWidget *reset_button;
GtkWidget *process_list;
GtkWidget *memory_view;
GtkWidget *console_view;
GtkWidget *ready_queue_list;
GtkWidget *blocked_queue_list;
GtkWidget *running_process_view;
GtkWidget *resource_panel;
GtkWidget *load_button;
GtkWidget *status_bar;

GtkWidget *input_label = NULL;
GtkWidget *input_dialog = NULL;
GtkWidget *input_entry = NULL;
char *current_input_var = NULL;

GtkCssProvider *provider;
GtkTextBuffer *console_buffer;
GtkTextIter console_iter;
GtkListStore *process_store;
GtkListStore *memory_store;
GtkListStore *ready_queue_store;
GtkListStore *blocked_queue_store;

gboolean is_running = FALSE;
guint timeout_id = 0;

void log_to_console(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(console_buffer, &end);
    gtk_text_buffer_insert(console_buffer, &end, buffer, -1);
    gtk_text_buffer_insert(console_buffer, &end, "\n", 1);
    
    // Scroll to bottom
    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
        GTK_SCROLLED_WINDOW(gtk_widget_get_parent(console_view)));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
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

// Functions to convert enums to strings
const char* process_state_to_string(ProcessState state) {
    switch (state) {
        case NEW: return "NEW";
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case BLOCKED: return "BLOCKED";
        case TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

const char* memory_type_to_string(MemType type) {
    switch (type) {
        case VAR: return "VAR";
        case CODE: return "CODE";
        case T_PCB: return "PCB";
        case GAY: return "GAY";
        case MINDO: return "MINDO";
        default: return "UNKNOWN";
    }
}
// Update the GUI with current system state
void update_gui() {
    // Update clock label
    char clock_text[32];
    sprintf(clock_text, "System Clock: %d", system_clock);
    gtk_label_set_text(GTK_LABEL(clock_label), clock_text);
    
    // Update process list
    gtk_list_store_clear(process_store);
    GtkTreeIter iter;
    
    for (int i = 0; i < process_count; i++) {
        PCB proc = processes[i];
        if (proc.process_id > 0) {
            gtk_list_store_append(process_store, &iter);
            gtk_list_store_set(process_store, &iter,
                0, proc.process_id,
                1, process_state_to_string(proc.state),
                2, proc.priority,
                3, proc.program_counter,
                4, proc.arrival_time,
                5, proc.completion_time == -1 ? "N/A" : g_strdup_printf("%d", proc.completion_time),
                -1);
        }
    }
    
    // Update memory view
    gtk_list_store_clear(memory_store);
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].process_id != -1) {
            gtk_list_store_append(memory_store, &iter);
            gtk_list_store_set(memory_store, &iter,
                0, i,
                1, memory[i].process_id,
                2, memory_type_to_string(memory[i].type),
                3, memory[i].name,
                4, memory[i].value,
                -1);
        }
    }
    
    // Update ready queue
    gtk_list_store_clear(ready_queue_store);
    for (int i = 0; i < 3; i++) {
        node* current = ready_queue[i].tail;
        if (ready_queue[i].size > 0) {
            current = current->next; // Start with head
            int count = 0;
            do {
                int pcb_addr = current->value;
                PCB proc = load_PCB(pcb_addr);
                
                gtk_list_store_append(ready_queue_store, &iter);
                gtk_list_store_set(ready_queue_store, &iter,
                    0, proc.process_id,
                    1, i, // Queue level
                    2, proc.priority,
                    3, proc.program_counter,
                    -1);
                
                current = current->next;
                count++;
                if (count >= ready_queue[i].size) break;
            } while (current != ready_queue[i].tail->next);
        }
    }
    
    // Update running process
    if (current_process.process_id != -1) {
        char running_text[256];
        sprintf(running_text, "PID: %d\nState: %s\nPriority: %d\nPC: %d/%d\nMemory: %d-%d",
            current_process.process_id,
            process_state_to_string(current_process.state),
            current_process.priority,
            current_process.program_counter,  // Show next line to execute
            current_process.code_size,
            current_process.lower_bound,
            current_process.upper_bound);
        
        if (current_process.program_counter > 0 && 
            current_process.program_counter <= current_process.code_size) {
            
            int offset = NUM_PCB + current_process.lower_bound + current_process.program_counter - 1;
            char instr_text[1400];
            sprintf(instr_text, "%s\n\nLast Executed Instruction:\n%s", 
                   running_text, memory[offset].value);
            gtk_label_set_text(GTK_LABEL(running_process_view), instr_text);
        } else {
            gtk_label_set_text(GTK_LABEL(running_process_view), running_text);
        }
    }
    
    // Update status bar
    char status_text[100];
    sprintf(status_text, "Memory used: %d/%d words | Processes: %d | Algorithm: %s | Quantum: %d",
        memory_allocated, MEMORY_SIZE, process_count,
        scheduling == ROUND_ROBIN ? "Round Robin" : 
        scheduling == FIFO ? "FIFO" : "MLFQ",
        round_robin_quantum);
    gtk_statusbar_push(GTK_STATUSBAR(status_bar), 0, status_text);
}



void init_queue(queue* q) {
    q->tail = NULL;
    q->size = 0;
}

void push(queue* q, int value) {
    node* new_node = (node*) malloc(sizeof(node));
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
MemoryWord memory[MEMORY_SIZE];
int memory_allocated = 0;
queue ready_queue[4];
PCB current_process = {-1};
PCB processes[MAX_PROCESSES];
int process_count = 0;

//sem 

void init_mutex() {
    
}

int find_mutex(const char *name) {
    return 0;
}
char *process_read_file(const char *path);

//variable
//[PCB,CODE,VAR]
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
        log_to_console("Process %d: %s = %s", current_process.process_id, args, value);
    }else {
        log_to_console("Process %d: value not found %s", current_process.process_id, args);
        printf("[Clock: %d] Process %d: value not found %s\n", system_clock, current_process.process_id, args);
    }
    update_gui();
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
        // Set up dialog text
        char prompt[256];
        snprintf(prompt, sizeof(prompt), "Please enter value for '%s':", name);
        gtk_label_set_text(GTK_LABEL(input_label), prompt);
        
        // Clear previous input and show dialog
        gtk_entry_set_text(GTK_ENTRY(input_entry), "");
        gtk_window_set_transient_for(GTK_WINDOW(input_dialog), GTK_WINDOW(window));
        gtk_window_set_modal(GTK_WINDOW(input_dialog), TRUE);
        gtk_widget_show_all(input_dialog);
        
        // Pause simulation
        is_running = FALSE;
        if (timeout_id != 0) {
            g_source_remove(timeout_id);
            timeout_id = 0;
        }
        return;
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
    log_to_console("Process %d: num_1 = %s, num_2 = %s, a = %d b = %d", 
        current_process.process_id, num_1, num_2, a, b);

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(console_buffer, &end);
    for (int i = a; i <= b; i++) {
    char line[32];
    snprintf(line, sizeof(line), "%d\n", i);
    gtk_text_buffer_insert(console_buffer, &end, line, -1);
    }

    // Scroll to the end
    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
    GTK_SCROLLED_WINDOW(gtk_widget_get_parent(console_view)));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
    update_gui();
}

void sem_wait_resource(char *name) {
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "[Clock: %d] sem_wait_resource: unknown mutex \"%s\"\n", system_clock, name);
        return;
    }
    printf("[Clock: %d] Process %d: sem_wait_resource end %s \n", system_clock, current_process.process_id, name);
}

void sem_signal_resource(char *name) {    
    trim(name);
    int idx = find_mutex(name);
    if (idx < 0) {
        fprintf(stderr, "[Clock: %d] sem_signal_resource: unknown mutex \"%s\"\n", system_clock, name);
        return;
    }
    printf("[Clock: %d] Process %d: sem_signal_resource end %s \n", system_clock, current_process.process_id, name);
}

void execute_line(char *line) {
    if (line[0] == '\0') {
        return;
    }
    
    // Extract command and arguments doesn't need max length
    char *command = strtok(line, " ");
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
    
    // Update program counter in the PCB
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





void init_process(int address) {
    push(&ready_queue[0], address);
}

void switch_context(queue* src, queue* dest) {
    int offset = current_process.lower_bound;
    if (current_process.process_id != -1) {
        if (current_process.program_counter == current_process.code_size) {
            strcpy(memory[offset + 1].value, "TERMINATED");
            current_process.process_id = -1;
        } else {
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

void run_clock_cycle() {
    static int update_counter = 0;
    if (++update_counter % 5 == 0) {
        while (gtk_events_pending())
            gtk_main_iteration_do(FALSE);
        update_counter = 0;
    }
    switch(scheduling) {
        case ROUND_ROBIN:
            if (quantum_tracking >= round_robin_quantum || 
                current_process.program_counter == current_process.code_size) {
                switch_context(&ready_queue[0], &ready_queue[0]);
                quantum_tracking = 0;
            }
            break;
            if (ready_queue[0].size > 0 && quantum_tracking == round_robin_quantum || current_process.program_counter == current_process.code_size) {
                switch_context(&ready_queue[0], &ready_queue[0]);
            }
            if (quantum_tracking == round_robin_quantum) {
                quantum_tracking = 0;
            }
            break;
        case FIFO:
            if (current_process.program_counter == current_process.code_size) {
                switch_context(&ready_queue[0], &ready_queue[0]);
            }
            break;
        case MLFQ:
            if (quantum_tracking == (1 << (current_process.priority - 1)) || current_process.program_counter == current_process.code_size) {
            }
            break;
    }
    if (current_process.process_id == -1) {
        return;
    }
    int offset = NUM_PCB + current_process.lower_bound;
    if (current_process.program_counter < current_process.code_size) {
        execute_line(memory[current_process.program_counter + offset].value);
        current_process.program_counter++;
        quantum_tracking++;
    }
    
    system_clock++;
    
    // Update GUI
    if (++update_counter % 2 == 0) {
        while (gtk_events_pending())
            gtk_main_iteration_do(FALSE);
        update_counter = 0;
    }
    
    // Update PC in memory
    offset = current_process.lower_bound;
    sprintf(memory[offset + 3].value, "%d", current_process.program_counter);
    sprintf(memory[offset + 3].value, "%d", current_process.program_counter);
}

int create_process(const char *program) {
    // Process creation consumes a clock cycle    
    // if (process_count >= MAX_PROCESSES) {
    //     printf("[Clock: %d] Error: Out of process slots (Ask MINDO)\n", system_clock);
    //     return -1;
    // }
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
    sprintf(memory[mem_index].value, "%d", 1);
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

void init() {
    init_mutex();
    init_memory();
    for (int i = 0; i < 3; i++) {
        init_queue(&ready_queue[i]);
    }
}

// int main(void) {
//     system_clock = 1;  // Initialize system clock
//     init();
    
//     printf("[Clock: %d] System initialized\n", system_clock);
//     print_memory();
    
//     int pid1 = create_process("Program_1.txt");
//     print_memory();
//     if (pid1 > 0) {
//         print_process_stats();
//     }
    
//     int pid2 = create_process("Program_2.txt");
//     if (pid2 > 0) {
//         print_process_stats();
//     }
    
//     int pid3 = create_process("Program_3.txt");
//     if (pid3 > 0) {
//         print_process_stats();
//     }

//     int pid4 = create_process("Program_4.txt");
//     if (pid4 > 0) {
//         print_process_stats();
//     }
    
    
//     print_memory();
//     while (1) {
//         run_clock_cycle();
//         if (current_process.process_id == -1) {
//             break;
//         }
//     }
//     printf("[Clock: %d] All processes completed\n", system_clock);
//     print_process_stats();
    
//     return 0;
// }

// int main(void) {
//     system_clock = 1;  // Initialize system clock
//     init();
    
//     printf("[Clock: %d] System initialized\n", system_clock);
//     print_memory();
    
//     int pid1 = create_process("Program_1.txt");
//     print_memory();
//     if (pid1 > 0) {
//         print_process_stats();
//     }
    
//     int pid2 = create_process("Program_2.txt");
//     if (pid2 > 0) {
//         print_process_stats();
//     }
    
//     int pid3 = create_process("Program_3.txt");
//     if (pid3 > 0) {
//         print_process_stats();
//     }

//     int pid4 = create_process("Program_4.txt");
//     if (pid4 > 0) {
//         print_process_stats();
//     }
    
    
//     print_memory();
//     while (1) {
//         run_clock_cycle();
//         if (current_process.process_id == -1) {
//             break;
//         }
//     }
//     printf("[Clock: %d] All processes completed\n", system_clock);
//     print_process_stats();
    
//     return 0;
// }

// Forward declarations for the existing system functions


// CSS for styling
const char *css_data = "                                           \
@define-color bg_color #222222;                                    \
@define-color text_color #000000;                                  \
@define-color accent_color #4a90d9;                                \
@define-color border_color #444444;                                \
@define-color header_color #333333;                                \
@define-color button_hover #555555;                                \
@define-color console_bg #000000;                                  \
                                                                   \
.input-dialog {                                                    \
    background-color: white;                                       \
}                                                                  \
.input-dialog label,                                               \
.input-dialog entry {                                              \
    color: black;                                                  \
}                                                                  \
                                                                   \
* {                                                                \
    font-family: \"VT323\", \"Ubuntu Mono\", monospace;            \
    color: black;                                            \
    border-radius: 2px;                                            \
    transition: all 200ms ease;                                    \
}                                                                     \
                                                  \
entry {\
    background-color: white;\
    color: black;\
}                                                                   \
.console-view {\
    background-color: white;\
    color: black;\
    font-family: "VT323", "Ubuntu Mono", monospace;\
    padding: 8px;\
    border: 1px solid @border_color;\
}\
spinbutton {\
    background-color: white;\
    color: black;\
}\
notebook tab label {\
    color: black;\
}\
#input-dialog {\
    background-color: white;\
}\
#input-dialog * {\
    color: black;\
}                                                                 \
                                                                   \
window {                                                           \
    background-color: @bg_color;                                   \
}                                                                  \
                                                                   \
headerbar {                                                        \
    background-color: @header_color;                               \
    border-bottom: 1px solid @border_color;                        \
    padding: 8px;                                                  \
}                                                                  \
                                                                   \
button {                                                           \
    background-color: @header_color;                               \
    border: 1px solid @border_color;                               \
    padding: 6px 12px;                                             \
}                                                                  \
                                                                   \
button:hover {                                                     \
    background-color: @button_hover;                               \
    transform: translateY(-2px);                                   \
    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.3);                      \
}                                                                  \
                                                                   \
button:active {                                                    \
    background-color: white;                                       \
    color: black;                                                  \
}                                                                  \
                                                                   \
label {                                                            \
    padding: 4px;                                                  \
}                                                                  \
                                                                   \
combobox {                                                         \
    background-color: @header_color;                               \
    border: 1px solid @border_color;                               \
}                                                                  \                                                                \
                                                                   \
.title-label {                                                     \
    font-size: 16px;                                               \
    font-weight: bold;                                             \
}                                                                  \
                                                                   \
.section-header {                                                  \
    font-weight: bold;                                             \
    background-color: @header_color;                               \
    padding: 6px;                                                  \
    border-bottom: 1px solid @border_color;                        \
}                                                                  \
                                                                   \                                                                \
                                                                   \
.memory-cell {                                                     \
    border: 1px solid @border_color;                               \
    padding: 2px;                                                  \
    background-color: @header_color;                               \
}                                                                  \
                                                                   \
.memory-cell-pcb {                                                 \
    background-color: #2a5599;                                     \
}                                                                  \
                                                                   \
.memory-cell-code {                                                \
    background-color: #669933;                                     \
}                                                                  \
                                                                   \
.memory-cell-var {                                                 \
    background-color: #993366;                                     \
}                                                                  \
                                                                   \
treeview {                                                         \
    background-color: @header_color;                               \
    border: 1px solid @border_color;                               \
}                                                                  \
                                                                   \
treeview:selected {                                                \
    background-color: @accent_color;                               \
}                                                                  \
                                                                   \
scrolledwindow {                                                   \
    border: 1px solid @border_color;                               \
}                                                                  \
                                                                   \
.process-icon {                                                    \
    color: @accent_color;                                          \
    font-size: 16px;                                               \
}                                                                  \
                                                                   \
statusbar {                                                        \
    background-color: @header_color;                               \
    border-top: 1px solid @border_color;                           \
    padding: 4px;                                                  \
    font-size: 12px;                                               \
}                                                                  \
";

// Input dialog callback
void on_input_ok(GtkButton *button, gpointer user_data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(input_entry));
    if (current_input_var && text) {
        set_variable(current_input_var, (char*)text);
    }
    gtk_widget_hide(input_dialog);
    current_input_var = NULL;
}

// Create input dialog (call this during GUI initialization)


void create_input_dialog() {
    input_dialog = gtk_dialog_new_with_buttons("Process Input",
                                             GTK_WINDOW(window),
                                             GTK_DIALOG_MODAL,
                                             "OK",
                                             GTK_RESPONSE_ACCEPT,
                                             NULL);
    // Add CSS class
    gtk_widget_set_name(input_dialog, "input-dialog");
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(input_dialog));
    input_entry = gtk_entry_new();
    input_label = gtk_label_new("");
    
    // Style the entry widget
    gtk_widget_set_name(input_entry, "console-entry");
    
    gtk_box_pack_start(GTK_BOX(content), input_label, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(content), input_entry, TRUE, TRUE, 5);
    g_signal_connect(input_dialog, "response", G_CALLBACK(on_input_ok), NULL);
}


// Function to log to console
// void log_to_console(const char *format, ...) {
//     va_list args;
//     va_start(args, format);
    
//     char buffer[1024];
//     vsnprintf(buffer, sizeof(buffer), format, args);
    
//     va_end(args);
    
//     // Append timestamp
//     char timestamped_buffer[1100];
//     snprintf(timestamped_buffer, sizeof(timestamped_buffer), "[Clock: %d] %s\n", system_clock, buffer);
    
//     GtkTextIter end;
//     gtk_text_buffer_get_end_iter(console_buffer, &end);
//     gtk_text_buffer_insert(console_buffer, &end, timestamped_buffer, -1);
    
//     // Scroll to bottom
//     GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
//         GTK_SCROLLED_WINDOW(gtk_widget_get_parent(console_view)));
//     gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
// }


// Replace the load_custom_font function with this corrected version
void load_custom_font() {
    // This would load a retro font
    FcConfig *config = FcInitLoadConfigAndFonts();
    FcPattern *pat = FcNameParse((const FcChar8*)"VT323");
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    
    FcResult result;
    FcPattern *font = FcFontMatch(config, pat, &result);
    
    if (font) {
        FcChar8 *file = NULL;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            // Font found
            PangoFontDescription *font_desc = pango_font_description_from_string("VT323 12");
            // Apply to context...
        }
        FcPatternDestroy(font);
    }
    FcPatternDestroy(pat);
}

// For the run_one_cycle function, make sure the gtk_button_set_sensitive call is correct
// The GTK_BUTTON macro is likely already defined in gtk/gtk.h
void run_one_cycle() {
    if (current_process.process_id == -1 && ready_queue[0].size == 0 && 
        ready_queue[1].size == 0 && ready_queue[2].size == 0) {
        
        log_to_console("All processes completed");
        gtk_widget_set_sensitive(start_button, FALSE);
        gtk_widget_set_sensitive(step_button, FALSE);
        is_running = FALSE;
        
        if (timeout_id != 0) {
            g_source_remove(timeout_id);
            timeout_id = 0;
        }
        
        return;
    }
    
    run_clock_cycle();
    update_gui();
}

// Callback for auto-run timer
gboolean auto_run_callback(gpointer data) {
    if (!is_running) {
        timeout_id = 0;
        return G_SOURCE_REMOVE;
    }
    
    // Add small delay to allow GUI updates
    g_usleep(100000); // 100ms delay
    
    // Process pending GTK events
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);

    run_one_cycle();
    // If all processes finished, stop running
    if (current_process.process_id == -1 && ready_queue[0].size == 0 && 
        ready_queue[1].size == 0 && ready_queue[2].size == 0) {
        is_running = FALSE;
        timeout_id = 0;
        return G_SOURCE_REMOVE;
    }
    
    return G_SOURCE_CONTINUE;
}

// Button callbacks
void on_start_clicked(GtkButton *button, gpointer user_data) {
    if (!is_running) {
        is_running = TRUE;
        timeout_id = g_timeout_add(500, auto_run_callback, NULL); // Run every 500ms
        gtk_button_set_label(button, "Pause");
    } else {
        is_running = FALSE;
        if (timeout_id != 0) {
            g_source_remove(timeout_id);
            timeout_id = 0;
        }
        gtk_button_set_label(button, "Start");
    }
}

void on_step_clicked(GtkButton *button, gpointer user_data) {
    run_one_cycle();
}

void on_stop_clicked(GtkButton *button, gpointer user_data) {
    is_running = FALSE;
    if (timeout_id != 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
    gtk_button_set_label(GTK_BUTTON(start_button), "Start");
}

void on_reset_clicked(GtkButton *button, gpointer user_data) {
    // Stop any running simulation
    is_running = FALSE;
    if (timeout_id != 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
    
    // Reset system
    init();
    system_clock = 1;
    
    // Clear GUI components
    gtk_list_store_clear(process_store);
    gtk_list_store_clear(memory_store);
    gtk_list_store_clear(ready_queue_store);
    gtk_list_store_clear(blocked_queue_store);
    gtk_text_buffer_set_text(console_buffer, "", 0);
    
    // Enable buttons
    gtk_widget_set_sensitive(start_button, TRUE);
    gtk_widget_set_sensitive(step_button, TRUE);
    gtk_button_set_label(GTK_BUTTON(start_button), "Start");
    
    log_to_console("System reset");
    update_gui();
}

void on_algorithm_changed(GtkComboBox *widget, gpointer user_data) {
    int active = gtk_combo_box_get_active(widget);
    switch (active) {
        case 0:
            scheduling = FIFO;
            gtk_widget_set_sensitive(quantum_spin, FALSE);
            break;
        case 1:
            scheduling = ROUND_ROBIN;
            gtk_widget_set_sensitive(quantum_spin, TRUE);
            break;
        case 2:
            scheduling = MLFQ;
            gtk_widget_set_sensitive(quantum_spin, FALSE);
            break;
    }
    
    log_to_console("Scheduling algorithm changed to %s", 
        scheduling == ROUND_ROBIN ? "Round Robin" : 
        scheduling == FIFO ? "FIFO" : "MLFQ");
    
    update_gui();
}

void on_quantum_changed(GtkSpinButton *spin_button, gpointer user_data) {
    round_robin_quantum = gtk_spin_button_get_value_as_int(spin_button);
    log_to_console("Round Robin quantum set to %d", round_robin_quantum);
    update_gui();
}

void on_load_clicked(GtkButton *button, gpointer user_data) {
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
            log_to_console("Process %d created from file %s", pid, filename);
        } else {
            log_to_console("Failed to create process from file %s", filename);
        }
        
        g_free(filename);
        update_gui();
    }
    
    gtk_widget_destroy(dialog);
}

// Get icon name based on process state
const char* get_process_icon(ProcessState state) {
    switch (state) {
        case NEW: return "document-new-symbolic";
        case READY: return "emblem-default-symbolic";
        case RUNNING: return "media-playback-start-symbolic";
        case BLOCKED: return "media-playback-pause-symbolic";
        case TERMINATED: return "process-stop-symbolic";
        default: return "dialog-question-symbolic";
    }
}

// Create process list view
GtkWidget* create_process_list() {
    GtkWidget *scroll;
    GtkWidget *view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    // Create model
    process_store = gtk_list_store_new(6, 
        G_TYPE_INT,      // PID
        G_TYPE_STRING,   // State
        G_TYPE_INT,      // Priority
        G_TYPE_INT,      // PC
        G_TYPE_INT,      // Arrival Time
        G_TYPE_STRING    // Completion Time
    );
    
    // Create view
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(process_store));
    
    // Create columns
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("State", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PC", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Arrival", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Completion", renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    // Put view in scrolled window
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    
    return scroll;
}

// Create memory view
GtkWidget* create_memory_view() {
    GtkWidget *scroll;
    GtkWidget *view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    // Create model
    memory_store = gtk_list_store_new(5, 
        G_TYPE_INT,      // Address
        G_TYPE_INT,      // Process ID
        G_TYPE_STRING,   // Type
        G_TYPE_STRING,   // Name
        G_TYPE_STRING    // Value
    );
    
    // Create view
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(memory_store));
    
    // Create columns
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Addr", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Value", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    // Put view in scrolled window
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    
    return scroll;
}

// Create ready queue view
GtkWidget* create_ready_queue_view() {
    GtkWidget *scroll;
    GtkWidget *view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    // Create model
    ready_queue_store = gtk_list_store_new(4, 
        G_TYPE_INT,      // PID
        G_TYPE_INT,      // Queue Level
        G_TYPE_INT,      // Priority
        G_TYPE_INT       // PC
    );
    
    // Create view
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ready_queue_store));
    
    // Create columns
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Queue", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PC", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    // Put view in scrolled window
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    
    return scroll;
}

// Create blocked queue view
GtkWidget* create_blocked_queue_view() {
    GtkWidget *scroll;
    GtkWidget *view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    // Create model
    blocked_queue_store = gtk_list_store_new(3, 
        G_TYPE_INT,      // PID
        G_TYPE_STRING,   // Resource
        G_TYPE_INT       // Wait Time
    );
    
    // Create view
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(blocked_queue_store));
    
    // Create columns
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Resource", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Wait Time", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    // Put view in scrolled window
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    
    return scroll;
}

// Create console view
GtkWidget* create_console_view() {
    GtkWidget *scroll;
    
    // Create text view
    console_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(console_view), FALSE);
    gtk_widget_set_name(console_view, "console-view");
    
    // Set up text buffer
    console_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_view));
    
    // Put view in scrolled window
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), console_view);
    
    return scroll;
}

// Main function
int main(int argc, char *argv[]) {
    GtkWidget *main_vbox;
    GtkWidget *top_hbox;
    GtkWidget *control_panel;
    GtkWidget *notebook;
    GtkWidget *left_panel;
    GtkWidget *right_panel;
    GtkWidget *queue_panel;
    GtkWidget *paned;
    GtkWidget *section_label;
    
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Load CSS
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                           GTK_STYLE_PROVIDER(provider),
                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Xerox 8010 Star OS Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Setup main layout
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);
    
    // Header bar with controls
    header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "MINDO OS Scheduler Simulation");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header_bar), "MINDO-OS");
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    
    // Clock display
    clock_label = gtk_label_new("System Clock: 0");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), clock_label);
    
    // Load process button
    load_button = gtk_button_new_with_label("Load Program");
    g_signal_connect(load_button, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), load_button);
    
    // Algorithm selection
    GtkWidget *algo_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *algo_label = gtk_label_new("Algorithm:");
    gtk_box_pack_start(GTK_BOX(algo_box), algo_label, FALSE, FALSE, 0);
    
    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "FIFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 1); // Default to Round Robin
    g_signal_connect(algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), NULL);
    gtk_box_pack_start(GTK_BOX(algo_box), algorithm_combo, FALSE, FALSE, 0);
    
    // Quantum control
    GtkWidget *quantum_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *quantum_label = gtk_label_new("Quantum:");
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_label, FALSE, FALSE, 0);
    
    quantum_spin = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(quantum_spin), round_robin_quantum);
    g_signal_connect(quantum_spin, "value-changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_spin, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(algo_box), quantum_box, FALSE, FALSE, 10);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), algo_box);
    
    // Control buttons
    control_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), control_panel, FALSE, FALSE, 5);
    
    // start_button = gtk_button_new_with_label("Start");
    // g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    // gtk_box_pack_start(GTK_BOX(control_panel), start_button, TRUE, TRUE, 5);
    
    step_button = gtk_button_new_with_label("Step");
    g_signal_connect(step_button, "clicked", G_CALLBACK(on_step_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_panel), step_button, TRUE, TRUE, 5);
    
    // stop_button = gtk_button_new_with_label("Stop");
    // g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    // gtk_box_pack_start(GTK_BOX(control_panel), stop_button, TRUE, TRUE, 5);
    
    // reset_button = gtk_button_new_with_label("Reset");
    // g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    // gtk_box_pack_start(GTK_BOX(control_panel), reset_button, TRUE, TRUE, 5);
    
    // Main content area with paned view
    paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), paned, TRUE, TRUE, 0);
    
    // Left panel with processes and memory
    left_panel = gtk_notebook_new();
    gtk_paned_pack1(GTK_PANED(paned), left_panel, TRUE, TRUE);
    
    // Process list tab
    GtkWidget *process_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(process_box), 10);
    
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>Process List</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(process_box), section_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(process_box), create_process_list(), TRUE, TRUE, 5);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(left_panel), process_box, 
                           gtk_label_new("Processes"));
    
    // Memory view tab
    GtkWidget *memory_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(memory_box), 10);
    
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>Memory View</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(memory_box), section_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(memory_box), create_memory_view(), TRUE, TRUE, 5);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(left_panel), memory_box, 
                           gtk_label_new("Memory"));
    // Add the input dialog creation
    create_input_dialog();
    

    // Right panel with queues, current process, and console
    right_panel = gtk_notebook_new();
    gtk_paned_pack2(GTK_PANED(paned), right_panel, TRUE, TRUE);
    
    // Queues tab
    queue_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(queue_panel), 10);
    
    // Ready queue
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>Ready Queue</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(queue_panel), section_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(queue_panel), create_ready_queue_view(), TRUE, TRUE, 5);
    
    // Running process
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>Running Process</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(queue_panel), section_label, FALSE, FALSE, 0);
    
    GtkWidget *running_frame = gtk_frame_new(NULL);
    running_process_view = gtk_label_new("No running process");
    gtk_label_set_xalign(GTK_LABEL(running_process_view), 0.0);
    gtk_label_set_selectable(GTK_LABEL(running_process_view), TRUE);
    gtk_container_add(GTK_CONTAINER(running_frame), running_process_view);
    gtk_box_pack_start(GTK_BOX(queue_panel), running_frame, TRUE, TRUE, 5);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(right_panel), queue_panel, 
                           gtk_label_new("Queues"));
    
    // Resource panel
    resource_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(resource_panel), 10);
    
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>Resource Management</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(resource_panel), section_label, FALSE, FALSE, 0);
    
    // Mutex status (simple table for now)
    GtkWidget *mutex_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(mutex_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(mutex_grid), 5);
    
    // Headers
    GtkWidget *header = gtk_label_new("Resource");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), header, 0, 0, 1, 1);
    
    header = gtk_label_new("Status");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), header, 1, 0, 1, 1);
    
    header = gtk_label_new("Owner");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), header, 2, 0, 1, 1);
    
    // Resource rows
    GtkWidget *label = gtk_label_new("userInput");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 0, 1, 1, 1);
    
    label = gtk_label_new("Available");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 1, 1, 1, 1);
    
    label = gtk_label_new("None");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 2, 1, 1, 1);
    
    // userOutput
    label = gtk_label_new("userOutput");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 0, 2, 1, 1);
    
    label = gtk_label_new("Available");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 1, 2, 1, 1);
    
    label = gtk_label_new("None");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 2, 2, 1, 1);
    
    // file
    label = gtk_label_new("file");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 0, 3, 1, 1);
    
    label = gtk_label_new("Available");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 1, 3, 1, 1);
    
    label = gtk_label_new("None");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(mutex_grid), label, 2, 3, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(resource_panel), mutex_grid, FALSE, FALSE, 10);
    
    // Blocked processes
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>Blocked Processes</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(resource_panel), section_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(resource_panel), create_blocked_queue_view(), TRUE, TRUE, 5);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(right_panel), resource_panel, 
                           gtk_label_new("Resources"));
    
    // Console tab
    GtkWidget *console_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(console_box), 10);
    
    section_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(section_label), "<span font_weight='bold'>System Console</span>");
    gtk_widget_set_halign(section_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(console_box), section_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(console_box), create_console_view(), TRUE, TRUE, 5);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(right_panel), console_box, 
                           gtk_label_new("Console"));
    
    // Status bar
    status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), status_bar, FALSE, FALSE, 0);
    
    // Initialize system and display
    init();
    system_clock = 1;
    log_to_console("System initialized");
    update_gui();
    
    // Show all widgets
    gtk_widget_show_all(window);
    
    // Set initial pane position
    gtk_paned_set_position(GTK_PANED(paned), 600);
    
    // Add Xerox 8010 Star-like icons
    // You would need to create actual pixbufs for these icons
    
    // Main event loop
    gtk_main();
    
    return 0;
}
// Add these functions to customize window decorations for an even more retro look
void create_custom_title_bar(GtkWidget *window) {
    // Custom title bar would go here
    // In a real implementation, this would create a custom window frame
}

// Function to create Xerox 8010 Star-like icons
GtkWidget* create_star_icon(const char *icon_name) {
    // This would create bitmapped icons similar to the Xerox Star UI
    GtkWidget *image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    return image;
}

// Process icon dock - for a more authentic Xerox Star look
GtkWidget* create_process_icon_dock() {
    GtkWidget *dock = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    // Would add process icons here
    
    return dock;
}
