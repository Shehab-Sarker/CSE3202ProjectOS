#ifndef PROCESS_H
#define PROCESS_H

#include "types.h" 

// Config
#define MAX_PROCESSES 16

#ifndef STACK_SIZE
#define STACK_SIZE 1024
#endif

#define IPC_BUFFER_SIZE 64

// 1. Process States
typedef enum {
    STATE_UNUSED = 0,
    STATE_READY,      
    STATE_CURRENT,    
    STATE_TERMINATED, 
    STATE_WAITING     
} ProcessState;

// 2. Process Control Block (PCB)
typedef struct {
    int pid;                
    ProcessState state;     
    uint32_t esp;           
    void *stack_base;       
    char name[32];          
    
    // --- IPC Fields ---
    char msg_buffer[IPC_BUFFER_SIZE]; 
    int has_message;                  
} process_t;

// Global Data
extern process_t processes[MAX_PROCESSES];
extern int current_pid;

// Core Functions
void init_processes();
int create_process(void (*entry)(), const char *name);
void terminate_process(int pid);
void switch_to_process(int pid);

// Utility Functions
process_t* get_process_by_pid(int pid);
process_t* get_current_process();

// IPC Functions
int send_message(int dest_pid, const char *msg);
int receive_message(char *buffer);

#endif 
