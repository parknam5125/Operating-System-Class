#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 10

typedef enum { NEW, READY, RUNNING, FINISHED } State;

typedef struct Process {
    int pid, priority, arrival_time, burst_time;
    int remaining_time, start_time, finish_time;
    int response_time, waiting_time, turnaround_time;
    int started;
    State state;
    struct Process *next;
} Process;

Process* ready_queue = NULL;

void enqueue(Process* p) {
    p->next = NULL;
    if (!ready_queue) {
        ready_queue = p;
    } else {
        Process* temp = ready_queue;
        while (temp->next) temp = temp->next;
        temp->next = p;
    }
}

Process* dequeue() {
    if (!ready_queue) return NULL;
    Process* p = ready_queue;
    ready_queue = ready_queue->next;
    p->next = NULL;
    return p;
}

Process* select_highest_priority(float alpha, int current_time) {
    Process* prev = NULL, *curr = ready_queue, *max_prev = NULL, *max = NULL;
    int max_effective_priority = -1;

    while (curr) {
        int waiting_time = current_time - curr->arrival_time;
        int effective_priority = curr->priority + (int)(alpha * waiting_time);

        if (!max || effective_priority > max_effective_priority) {
            max = curr;
            max_prev = prev;
            max_effective_priority = effective_priority;
        }
        prev = curr;
        curr = curr->next;
    }

    if (max) {
        if (max_prev) max_prev->next = max->next;
        else ready_queue = max->next;
        max->next = NULL;
    }
    return max;
}

void load_processes_from_file(const char* filename, Process jobQueue[]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("파일 열기 실패");
        exit(1);
    }
    int i = 0;
    while (fscanf(file, "%d %d %d %d",
        &jobQueue[i].pid,
        &jobQueue[i].priority,
        &jobQueue[i].arrival_time,
        &jobQueue[i].burst_time) == 4) {
        jobQueue[i].remaining_time = jobQueue[i].burst_time;
        jobQueue[i].start_time = -1;
        jobQueue[i].finish_time = -1;
        jobQueue[i].state = NEW;
        jobQueue[i].started = 0;
        jobQueue[i].next = NULL;
        i++;
    }
    fclose(file);
}

void calculate_and_print_stats(Process jobQueue[], FILE* out, int total_time) {
    float total_wait = 0, total_turn = 0, total_resp = 0;
    int used_time = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        total_wait += jobQueue[i].waiting_time;
        total_turn += jobQueue[i].turnaround_time;
        total_resp += jobQueue[i].response_time;
        used_time += jobQueue[i].burst_time;
    }
    fprintf(out, "Average CPU usage : %.2f %%\n", 100.0 * used_time / total_time);
    fprintf(out, "Average waiting time : %.1f\n", total_wait / MAX_PROCESSES);
    fprintf(out, "Average response time : %.1f\n", total_resp / MAX_PROCESSES);
    fprintf(out, "Average turnaround time : %.1f\n", total_turn / MAX_PROCESSES);
}

void run_fcfs(Process jobQueue[], const char* outfile) {
    FILE* out = fopen(outfile, "w");
    fprintf(out, "Scheduling : FCFS\n==============================\n");

    int time = 0, done = 0;
    ready_queue = NULL;
    Process* running = NULL;

    while (done < MAX_PROCESSES) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (jobQueue[i].arrival_time == time && jobQueue[i].state == NEW) {
                jobQueue[i].state = READY;
                enqueue(&jobQueue[i]);
                fprintf(out, "<time %d> [new arrival] process %d\n", time, jobQueue[i].pid);
            }
        }

        if (!running && ready_queue) {
            running = dequeue();
            running->state = RUNNING;
            if (!running->started) {
                running->start_time = time;
                running->response_time = time - running->arrival_time;
                running->started = 1;
            }
        }

        if (running) {
            fprintf(out, "<time %d> process %d is running\n", time, running->pid);
            running->remaining_time--;
            if (running->remaining_time == 0) {
                running->state = FINISHED;
                running->finish_time = time + 1;
                running->turnaround_time = running->finish_time - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->burst_time;
                running = NULL;
                done++;
            }
        } else {
            fprintf(out, "<time %d> ---- system is idle ----\n", time);
        }
        time++;
    }

    fprintf(out, "<time %d> all processes finish\n", time);
    fprintf(out, "==============================\n");
    calculate_and_print_stats(jobQueue, out, time);
    fclose(out);
}

void run_rr(Process jobQueue[], const char* outfile, int quantum) {
    FILE* out = fopen(outfile, "w");
    fprintf(out, "Scheduling : Round Robin (Time Quantum = %d)\n==============================\n", quantum);

    int time = 0, done = 0;
    ready_queue = NULL;
    Process* running = NULL;
    int time_slice = 0;

    while (done < MAX_PROCESSES) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (jobQueue[i].arrival_time == time && jobQueue[i].state == NEW) {
                jobQueue[i].state = READY;
                enqueue(&jobQueue[i]);
                fprintf(out, "<time %d> [new arrival] process %d\n", time, jobQueue[i].pid);
            }
        }

        if (running && (time_slice == quantum || running->remaining_time == 0)) {
            if (running->remaining_time > 0) {
                enqueue(running);
                running->state = READY;
            } else {
                running->state = FINISHED;
                running->finish_time = time;
                running->turnaround_time = time - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->burst_time;
                done++;
            }
            running = NULL;
            time_slice = 0;
        }

        if (!running && ready_queue) {
            running = dequeue();
            running->state = RUNNING;
            if (!running->started) {
                running->start_time = time;
                running->response_time = time - running->arrival_time;
                running->started = 1;
            }
        }

        if (running) {
            fprintf(out, "<time %d> process %d is running\n", time, running->pid);
            running->remaining_time--;
            time_slice++;
        } else {
            fprintf(out, "<time %d> ---- system is idle ----\n", time);
        }

        time++;
    }

    fprintf(out, "<time %d> all processes finish\n", time);
    fprintf(out, "==============================\n");
    calculate_and_print_stats(jobQueue, out, time);
    fclose(out);
}

void run_priority(Process jobQueue[], const char* outfile, float alpha) {
    FILE* out = fopen(outfile, "w");
    fprintf(out, "Scheduling : Preemptive Priority with Aging (alpha = %.2f)\n==============================\n", alpha);

    int time = 0, done = 0;
    ready_queue = NULL;
    Process* running = NULL;

    while (done < MAX_PROCESSES) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (jobQueue[i].arrival_time == time && jobQueue[i].state == NEW) {
                jobQueue[i].state = READY;
                enqueue(&jobQueue[i]);
                fprintf(out, "<time %d> [new arrival] process %d\n", time, jobQueue[i].pid);
            }
        }

        if (ready_queue) {
            Process* candidate = select_highest_priority(alpha, time);
            if (!running || candidate->priority + (int)(alpha * (time - candidate->arrival_time)) >
                            running->priority + (int)(alpha * (time - running->arrival_time))) {
                if (running) {
                    running->state = READY;
                    enqueue(running);
                }
                running = candidate;
                running->state = RUNNING;
                if (!running->started) {
                    running->start_time = time;
                    running->response_time = time - running->arrival_time;
                    running->started = 1;
                }
            } else {
                enqueue(candidate);
            }
        }

        if (running) {
            fprintf(out, "<time %d> process %d is running\n", time, running->pid);
            running->remaining_time--;
            if (running->remaining_time == 0) {
                running->state = FINISHED;
                running->finish_time = time + 1;
                running->turnaround_time = running->finish_time - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->burst_time;
                running = NULL;
                done++;
            }
        } else {
            fprintf(out, "<time %d> ---- system is idle ----\n", time);
        }

        time++;
    }

    fprintf(out, "<time %d> all processes finish\n", time);
    fprintf(out, "==============================\n");
    calculate_and_print_stats(jobQueue, out, time);
    fclose(out);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: %s [input_file] [output_file] [RR_quantum] [PRIO_alpha]\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];
    int rr_quantum = atoi(argv[3]);
    float prio_alpha = atof(argv[4]);

    Process jobQueue[MAX_PROCESSES];

    load_processes_from_file(input_file, jobQueue);
    run_fcfs(jobQueue, "fcfs_output.txt");

    load_processes_from_file(input_file, jobQueue);
    run_rr(jobQueue, "rr_output.txt", rr_quantum);

    load_processes_from_file(input_file, jobQueue);
    run_priority(jobQueue, "priority_output.txt", prio_alpha);

    return 0;
}