#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROCESS_COUNT 5

typedef enum { CREATED, READY_STATE, EXECUTING, COMPLETED } Status;

typedef struct Task {
    int id, base_priority, arrival, duration;
    int time_left, time_started, time_completed;
    int response_delay, wait_duration, total_duration;
    int has_started;
    Status stat;
    struct Task* link;
} Task;

Task* queue_head = NULL;

void push_to_queue(Task* t) {
    t->link = NULL;
    if (!queue_head) queue_head = t;
    else {
        Task* scan = queue_head;
        while (scan->link) scan = scan->link;
        scan->link = t;
    }
}

Task* pop_from_queue() {
    if (!queue_head) return NULL;
    Task* t = queue_head;
    queue_head = queue_head->link;
    t->link = NULL;
    return t;
}

Task* pop_highest_priority(float alpha, int current_time) {
    Task *prev = NULL, *curr = queue_head;
    Task *best = NULL, *best_prev = NULL;
    int best_score = -1;

    while (curr) {
        int age = current_time - curr->arrival;
        int score = curr->base_priority + (int)(alpha * age);
        if (!best || score > best_score) {
            best = curr;
            best_prev = prev;
            best_score = score;
        }
        prev = curr;
        curr = curr->link;
    }

    if (best) {
        if (best_prev) best_prev->link = best->link;
        else queue_head = best->link;
        best->link = NULL;
    }
    return best;
}

void read_tasks(const char* fname, Task task_list[]) {
    FILE* fp = fopen(fname, "r");
    if (!fp) { perror("File open error"); exit(1); }
    for (int i = 0; i < PROCESS_COUNT; i++) {
        fscanf(fp, "%d %d %d %d",
               &task_list[i].id,
               &task_list[i].base_priority,
               &task_list[i].arrival,
               &task_list[i].duration);
        task_list[i].time_left = task_list[i].duration;
        task_list[i].time_started = -1;
        task_list[i].time_completed = -1;
        task_list[i].stat = CREATED;
        task_list[i].has_started = 0;
        task_list[i].link = NULL;
    }
    fclose(fp);
}

void output_stats(Task task_list[], FILE* out, int runtime) {
    float sum_wait = 0, sum_turn = 0, sum_resp = 0;
    int used_cpu = 0;
    for (int i = 0; i < PROCESS_COUNT; i++) {
        sum_wait += task_list[i].wait_duration;
        sum_turn += task_list[i].total_duration;
        sum_resp += task_list[i].response_delay;
        used_cpu += task_list[i].duration;
    }
    fprintf(out, "CPU Utilization: %.2f%%\n", 100.0 * used_cpu / runtime);
    fprintf(out, "Avg Waiting: %.1f\nAvg Response: %.1f\nAvg Turnaround: %.1f\n",
            sum_wait / PROCESS_COUNT,
            sum_resp / PROCESS_COUNT,
            sum_turn / PROCESS_COUNT);
}

void schedule_fcfs(Task task_list[], const char* fname) {
    FILE* out = fopen(fname, "w");
    fprintf(out, "--- FCFS Scheduling ---\n");

    int clock = 0, finished = 0;
    Task* executing = NULL;
    queue_head = NULL;

    while (finished < PROCESS_COUNT) {
        for (int i = 0; i < PROCESS_COUNT; i++) {
            if (task_list[i].arrival == clock && task_list[i].stat == CREATED) {
                task_list[i].stat = READY_STATE;
                push_to_queue(&task_list[i]);
                fprintf(out, "[t=%d] Arrived Task %d\n", clock, task_list[i].id);
            }
        }

        if (!executing && queue_head) {
            executing = pop_from_queue();
            executing->stat = EXECUTING;
            if (!executing->has_started) {
                executing->time_started = clock;
                executing->response_delay = clock - executing->arrival;
                executing->has_started = 1;
            }
        }

        if (executing) {
            fprintf(out, "[t=%d] Running Task %d\n", clock, executing->id);
            executing->time_left--;
            if (executing->time_left == 0) {
                executing->stat = COMPLETED;
                executing->time_completed = clock + 1;
                executing->total_duration = executing->time_completed - executing->arrival;
                executing->wait_duration = executing->total_duration - executing->duration;
                executing = NULL;
                finished++;
            }
        } else {
            fprintf(out, "[t=%d] IDLE\n", clock);
        }
        clock++;
    }

    fprintf(out, "[t=%d] All done\n", clock);
    output_stats(task_list, out, clock);
    fclose(out);
}

void schedule_rr(Task task_list[], const char* fname, int quantum) {
    FILE* out = fopen(fname, "w");
    fprintf(out, "--- Round Robin (q=%d) ---\n", quantum);

    int clock = 0, finished = 0;
    Task* executing = NULL;
    int timeslice = 0;
    queue_head = NULL;

    while (finished < PROCESS_COUNT) {
        for (int i = 0; i < PROCESS_COUNT; i++) {
            if (task_list[i].arrival == clock && task_list[i].stat == CREATED) {
                task_list[i].stat = READY_STATE;
                push_to_queue(&task_list[i]);
                fprintf(out, "[t=%d] Arrived Task %d\n", clock, task_list[i].id);
            }
        }

        if (executing && (timeslice == quantum || executing->time_left == 0)) {
            if (executing->time_left > 0) {
                executing->stat = READY_STATE;
                push_to_queue(executing);
            } else {
                executing->stat = COMPLETED;
                executing->time_completed = clock;
                executing->total_duration = clock - executing->arrival;
                executing->wait_duration = executing->total_duration - executing->duration;
                finished++;
            }
            executing = NULL;
            timeslice = 0;
        }

        if (!executing && queue_head) {
            executing = pop_from_queue();
            executing->stat = EXECUTING;
            if (!executing->has_started) {
                executing->time_started = clock;
                executing->response_delay = clock - executing->arrival;
                executing->has_started = 1;
            }
        }

        if (executing) {
            fprintf(out, "[t=%d] Running Task %d\n", clock, executing->id);
            executing->time_left--;
            timeslice++;
        } else {
            fprintf(out, "[t=%d] IDLE\n", clock);
        }
        clock++;
    }

    fprintf(out, "[t=%d] All done\n", clock);
    output_stats(task_list, out, clock);
    fclose(out);
}

void schedule_priority(Task task_list[], const char* fname, float alpha) {
    FILE* out = fopen(fname, "w");
    fprintf(out, "--- Priority Scheduling (aging=%.2f) ---\n", alpha);

    int clock = 0, finished = 0;
    Task* executing = NULL;
    queue_head = NULL;

    while (finished < PROCESS_COUNT) {
        for (int i = 0; i < PROCESS_COUNT; i++) {
            if (task_list[i].arrival == clock && task_list[i].stat == CREATED) {
                task_list[i].stat = READY_STATE;
                push_to_queue(&task_list[i]);
                fprintf(out, "[t=%d] Arrived Task %d\n", clock, task_list[i].id);
            }
        }

        if (queue_head) {
            Task* selected = pop_highest_priority(alpha, clock);
            if (!executing || (selected->base_priority + (int)(alpha * (clock - selected->arrival))) >
                               (executing->base_priority + (int)(alpha * (clock - executing->arrival)))) {
                if (executing) {
                    executing->stat = READY_STATE;
                    push_to_queue(executing);
                }
                executing = selected;
                executing->stat = EXECUTING;
                if (!executing->has_started) {
                    executing->time_started = clock;
                    executing->response_delay = clock - executing->arrival;
                    executing->has_started = 1;
                }
            } else {
                push_to_queue(selected);
            }
        }

        if (executing) {
            fprintf(out, "[t=%d] Running Task %d\n", clock, executing->id);
            executing->time_left--;
            if (executing->time_left == 0) {
                executing->stat = COMPLETED;
                executing->time_completed = clock + 1;
                executing->total_duration = executing->time_completed - executing->arrival;
                executing->wait_duration = executing->total_duration - executing->duration;
                executing = NULL;
                finished++;
            }
        } else {
            fprintf(out, "[t=%d] IDLE\n", clock);
        }
        clock++;
    }

    fprintf(out, "[t=%d] All done\n", clock);
    output_stats(task_list, out, clock);
    fclose(out);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: %s input.txt output.txt quantum alpha\n", argv[0]);
        return 1;
    }

    const char* input = argv[1];
    int rr_quantum = atoi(argv[3]);
    float prio_alpha = atof(argv[4]);

    Task tasks[PROCESS_COUNT];

    read_tasks(input, tasks);
    schedule_fcfs(tasks, "FCFS.txt");

    read_tasks(input, tasks);
    schedule_rr(tasks, "RR.txt", rr_quantum);

    read_tasks(input, tasks);
    schedule_priority(tasks, "Priority.txt", prio_alpha);

    return 0;
}
