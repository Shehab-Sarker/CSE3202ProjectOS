#include "scheduler.h"
#include "process.h"

void schedule() {
    int next_idx = current_pid;
    int found = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        next_idx = (next_idx + 1) % MAX_PROCESSES;
        
        if (processes[next_idx].state == STATE_READY) {
            found = 1;
            break;
        }
    }

    if (!found) return;

    if (processes[current_pid].state == STATE_CURRENT) {
        processes[current_pid].state = STATE_READY;
    }

    switch_to_process(next_idx);
}

void yield() {
    schedule();
}
