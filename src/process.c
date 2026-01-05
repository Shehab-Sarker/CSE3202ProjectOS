#include "process.h"
#include "memory.h"
#include "string.h"   
#include "scheduler.h" 

// Process Table
process_t processes[MAX_PROCESSES];
int current_pid = -1;
static int next_pid = 1;

// Context Storage
uint32_t kernel_esp; 

// --- Helper: Context Switch Assembly ---
void __attribute__((naked)) context_switch(uint32_t *new_esp, uint32_t **old_esp_ptr) {
    (void)new_esp; (void)old_esp_ptr;

    __asm__ volatile (
        "push %ebp \n"
        "push %ebx \n"
        "push %esi \n"
        "push %edi \n"
        "mov 24(%esp), %eax \n"   
        "mov %esp, (%eax) \n"     
        "mov 20(%esp), %esp \n"   
        "pop %edi \n"
        "pop %esi \n"
        "pop %ebx \n"
        "pop %ebp \n"
        "ret \n"
    );
}

// --- Initialization ---
void init_processes() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = -1;
        processes[i].state = STATE_UNUSED;
        processes[i].has_message = 0;
    }
}

// --- Process Creation ---
int create_process(void (*entry)(), const char *name) {
    int i;
    // Find free slot
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == STATE_UNUSED || processes[i].state == STATE_TERMINATED) {
            break;
        }
    }
    if (i == MAX_PROCESSES) return -1;

    process_t *p = &processes[i];
    p->pid = next_pid++;
    p->state = STATE_READY;
    p->has_message = 0;
    
    // Copy name safely
    int n = 0;
    while(name[n] && n < 31) { p->name[n] = name[n]; n++; }
    p->name[n] = '\0';

    p->stack_base = kmalloc(STACK_SIZE); 
    if (!p->stack_base) return -1;

    // Setup Context
    uint32_t *sp = (uint32_t*)((char*)p->stack_base + STACK_SIZE);
    sp--; *sp = (uint32_t)entry; // EIP
    for(int j=0; j<4; j++) { sp--; *sp = 0; } // Registers
    
    p->esp = (uint32_t)sp;
    return p->pid;
}

// --- State Transition & Termination ---
void terminate_process(int pid) {
    process_t *p = get_process_by_pid(pid);
    if (p) {
        p->state = STATE_TERMINATED;
    }
}

void switch_to_process(int pid) {
    int old_pid = current_pid;
    current_pid = pid;
    processes[pid].state = STATE_CURRENT;

    uint32_t **save_ptr;
    if (old_pid != -1) save_ptr = (uint32_t**)&processes[old_pid].esp;
    else save_ptr = (uint32_t**)&kernel_esp;

    context_switch((uint32_t*)processes[pid].esp, save_ptr);
}

// --- Utility Functions ---
process_t* get_process_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state != STATE_UNUSED) {
            return &processes[i];
        }
    }
    return 0;
}

process_t* get_current_process() {
    if (current_pid == -1) return 0;
    return &processes[current_pid];
}

// --- IPC Functions ---
int send_message(int dest_pid, const char *msg) {
    process_t *dest = get_process_by_pid(dest_pid);
    if (!dest) return -1; 

    int i = 0;
    while (msg[i] && i < IPC_BUFFER_SIZE - 1) {
        dest->msg_buffer[i] = msg[i];
        i++;
    }
    dest->msg_buffer[i] = '\0';
    dest->has_message = 1;

    if (dest->state == STATE_WAITING) {
        dest->state = STATE_READY;
    }
    return 0;
}

int receive_message(char *buffer) {
    process_t *current = &processes[current_pid]; 

    while (current->has_message == 0) {
        current->state = STATE_WAITING;
        yield(); 
    }

    int i = 0;
    while (current->msg_buffer[i] && i < IPC_BUFFER_SIZE - 1) {
        buffer[i] = current->msg_buffer[i];
        i++;
    }
    buffer[i] = '\0';
    
    current->has_message = 0; 
    return 0;
}
