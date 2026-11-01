#include <stdio.h>
#include "proc_read.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>

#include "memory_monitor.h"
#include "cpu_monitor.h"
static uint64_t get_total_cpu_ticks(const SystemCPU *cpu)
{
    return cpu->user +
           cpu->nice +
           cpu->system +
           cpu->idle +
           cpu->iowait +
           cpu->irq +
           cpu->softirq +
           cpu->steal;
}
static ProcessInfo *find_previous_process(
    ProcessInfo processes[],
    int count,
    pid_t pid,
    uint64_t starttime)
{
    for (int i = 0; i < count; i++)
    {
        if (processes[i].pid == pid &&
            processes[i].starttime == starttime)
        {
            return &processes[i];
        }
    }

    return NULL;
}
static int compare_cpu_usage(const void *a, const void *b)
{
    const ProcessInfo *p1 = a;
    const ProcessInfo *p2 = b;

    if (p1->cpu_usage < p2->cpu_usage)
        return 1;

    if (p1->cpu_usage > p2->cpu_usage)
        return -1;

    return 0;
}
static volatile sig_atomic_t keep_running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}
static int compare_memory_usage(const void *a, const void *b)
{
    const ProcessInfo *p1 = a;
    const ProcessInfo *p2 = b;

    if (p1->vmrss_kb < p2->vmrss_kb)
        return 1;

    if (p1->vmrss_kb > p2->vmrss_kb)
        return -1;

    return 0;
}

int main(void)
{
    ProcessInfo *previous;
    ProcessInfo *current;

    int previous_count = 0;
    int current_count = 0;

    int failed = 0;
    int cpu_count;

    SystemCPU previous_cpu;
    SystemCPU current_cpu;
    SystemMemory memory;

    signal(SIGINT, handle_sigint);

    previous = calloc(MAX_PROCESSES, sizeof(ProcessInfo));
    current = calloc(MAX_PROCESSES, sizeof(ProcessInfo));

    if (previous == NULL || current == NULL)
    {
        perror("calloc");
        free(previous);
        free(current);
        return 1;
    }

    cpu_count = (int)sysconf(_SC_NPROCESSORS_ONLN);

    if (cpu_count <= 0)
    {
        fprintf(stderr, "Unable to determine CPU count\n");
        free(previous);
        free(current);
        return 1;
    }

    /* Initial snapshot */

    if (read_system_cpu(&previous_cpu) < 0)
    {
        fprintf(stderr, "Unable to read system CPU information\n");
        free(previous);
        free(current);
        return 1;
    }

    previous_count = collect_processes(
        previous, MAX_PROCESSES, &failed);

    if (previous_count < 0)
    {
        fprintf(stderr, "Initial process collection failed\n");
        free(previous);
        free(current);
        return 1;
    }

    while (keep_running)
    {
        sleep(1);

        if (!keep_running)
            break;

        if (read_system_cpu(&current_cpu) < 0)
        {
            fprintf(stderr, "Unable to read system CPU information\n");
            break;
        }

        failed = 0;

        current_count = collect_processes(
            current, MAX_PROCESSES, &failed);

        if (current_count < 0)
        {
            fprintf(stderr, "Process collection failed\n");
            break;
        }

	if (read_system_memory(&memory) < 0){
    		fprintf(stderr, "Unable to read system memory\n");
    		break;
	}

        uint64_t previous_total =
            get_total_cpu_ticks(&previous_cpu);

        uint64_t current_total =
            get_total_cpu_ticks(&current_cpu);

        double system_usage =
            calculate_cpu_usage(&previous_cpu, &current_cpu);

        /* Calculate individual process CPU usage */

        for (int i = 0; i < current_count; i++)
        {
            ProcessInfo *old = find_previous_process(
                previous,
                previous_count,
                current[i].pid,
                current[i].starttime);

            if (old != NULL)
            {
                current[i].cpu_usage =
                    calculate_process_cpu_usage(
                        old->utime,
                        old->stime,
                        current[i].utime,
                        current[i].stime,
                        previous_total,
                        current_total,
                        cpu_count);
            }
            else
            {
                current[i].cpu_usage = 0.0;
            }
        }

        /* Sort by CPU usage */

        qsort(
            current,
            current_count,
            sizeof(ProcessInfo),
            compare_memory_usage);

        /* Display */

        printf("\033[H\033[J");

        printf("============================================\n");
        printf("          PROCMASTER LIVE MONITOR\n");
        printf("============================================\n");

        printf("System CPU Usage : %.2f%%\n", system_usage);
        printf("CPU Cores        : %d\n", cpu_count);
        printf("Total Processes  : %d\n", current_count);
        printf("Failed Reads     : %d\n", failed);

	printf("\n");
	printf("RAM Usage        : %.2f%%\n", memory.usage_percent);

	printf("RAM Available    : %.2f GB\n",(double)memory.available / (1024.0 * 1024.0));

	printf("RAM Total        : %.2f GB\n",(double)memory.total / (1024.0 * 1024.0));

	printf("Swap Usage       : %.2f%%\n",memory.swap_usage_percent);

        printf("--------------------------------------------\n");

        printf("%-8s %-8s %-7s %-22s %9s %9s\n",
               "PID", "PPID", "STATE", "NAME", "CPU%", "RSS");

        printf("---------------------------------------------------------------\n");

        int display_count = current_count < 20 ? current_count : 20;

        for (int i = 0; i < display_count; i++)
        {
            printf("%-8d %-8d %-7c %-22.22s ",
                   (int)current[i].pid,
                   (int)current[i].ppid,
                   current[i].state,
                   current[i].name);

            if (current[i].cpu_usage < 0.0)
                printf("%8s ", "N/A");
            else
                printf("%7.2f%% ", current[i].cpu_usage);

            printf("%8lu KB\n", current[i].vmrss_kb);
        }

        printf("\nPress Ctrl+C to exit.\n");

        /* Swap snapshots */

        ProcessInfo *temp = previous;
        previous = current;
        current = temp;

        previous_count = current_count;
        previous_cpu = current_cpu;
    }

    printf("\nProcMaster shutting down...\n");

    free(previous);
    free(current);

    return 0;
}
