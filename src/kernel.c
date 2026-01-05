#include "types.h"
#include "serial.h"
#include "string.h"
#include "memory.h"
#include "process.h"   
#include "scheduler.h" 

#define MAX_INPUT 128

// Global variables
int pid_sender = -1;
int pid_receiver = -1;

void print_int(int n) {
    if (n == 0) { serial_putc('0'); return; }
    char buffer[12];
    int i = 0;
    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) serial_putc(buffer[i]);
}

/* --- Tasks --- */

void task_sender() {
    serial_puts("[Sender] Started. Waiting for Receiver...\n");
    while (pid_receiver == -1) yield();

    const char *msg = "Greetings from Task A!";
    serial_puts("[Sender] Sending message: ");
    serial_puts(msg);
    serial_puts("\n");
    send_message(pid_receiver, msg);

    while (1) {
        for (volatile int i = 0; i < 8000000; i++);
        yield();
    }
}

void task_receiver() {
    char buffer[64];
    serial_puts("[Receiver] Started. Waiting for message...\n");
    receive_message(buffer);

    serial_puts("[Receiver] Woke up! Received: ");
    serial_puts(buffer);
    serial_puts("\n");

    while (1) {
        for (volatile int i = 0; i < 8000000; i++);
        yield();
    }
}

/* --- Shell --- */

void shell_process() {
    char input[MAX_INPUT];
    int pos = 0;
    
    serial_puts("\n[Shell] Started. Type 'help', 'status', or 'exit'.\n");

    while (1) {
        serial_puts("kacchiOS> ");
        pos = 0;
        
        // 1. Read Input Loop
        while (1) {
            char c = serial_getc();
            if (c == '\r' || c == '\n') {
                input[pos] = '\0';
                serial_puts("\n");
                break;
            }
            else if ((c == '\b' || c == 0x7F) && pos > 0) {
                pos--;
                serial_puts("\b \b");
            }
            else if (c >= 32 && c < 127 && pos < MAX_INPUT - 1) {
                input[pos++] = c;
                serial_putc(c);
            }
            yield(); 
        }
         
        // 2. Process Command
        if (pos > 0) {
            if (strcmp(input, "help") == 0) {
                serial_puts("Commands: help, status, exit\n");
            } 
            else if (strcmp(input, "status") == 0) {
                serial_puts("\n--- Task Manager ---\n");
                serial_puts("PID  | State | Name\n");
                serial_puts("-----|-------|-----\n");
                for(int i=0; i<MAX_PROCESSES; i++) {
                    if (processes[i].state != STATE_UNUSED) {
                        print_int(processes[i].pid);
                        serial_puts("    | ");
                        print_int(processes[i].state);
                        serial_puts("     | ");
                        serial_puts(processes[i].name);
                        serial_puts("\n");
                    }
                }
                serial_puts("--------------------\n");
            }
            // --- NEW: Exit Command ---
            else if (strcmp(input, "exit") == 0) {
                serial_puts("Shutting down kacchiOS...\n");
                serial_puts("System Halted.\n");
                
                __asm__ volatile ("cli"); 
                while(1) {
                    __asm__ volatile ("hlt");
                }
            }
            else {
                serial_puts("Unknown command: ");
                serial_puts(input);
                serial_puts("\n");
            }
        }
    }
}


/* --- Main Entry --- */

void kmain(void) {
    serial_init();
    
    serial_puts("\n=== kacchiOS Booting ===\n");

    // --- NEW: Memory Manager Demo ---
    serial_puts("[Kernel] Initializing Memory... ");
    // Perform a test allocation
    void* test_ptr = kmalloc(128);
    if (test_ptr != 0) {
        serial_puts("OK (Test Alloc Success)\n");
    } else {
        serial_puts("FAILED\n");
    }

    init_processes(); 
    serial_puts("[Kernel] Process Manager Initialized.\n");

    // Create Processes
    pid_receiver = create_process(&task_receiver, "Receiver");
    pid_sender = create_process(&task_sender, "Sender");
    create_process(&shell_process, "Shell");

    serial_puts("[Kernel] Starting Scheduler.\n\n");
    schedule();
    
    for (;;) { __asm__ volatile ("hlt"); }
}
