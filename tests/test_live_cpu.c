#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "proc_read.h"
#include "cpu_monitor.h"

#define DISPLAY_LIMIT 20

static uint64_t system_total(const SystemCPU *cpu)
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

static ProcessInfo *find_previous(
    ProcessInfo *previous,
    int count,
    const ProcessInfo *current)
{
    for (int i = 0; i < count; i++)
    {
        if (previous[i].pid == current->pid &&
            previous[i].starttime == current->starttime)
        {
            return &previous[i];
        }
    }

    return NULL;
}

static int compare_cpu(const void *a, const void *b)
{
    const ProcessInfo *p1 = a;
    const ProcessInfo *p2 = b;

    if (p1->cpu_usage < p2->cpu_usage)
        return 1;

    if (p1->cpu_usage > p2->cpu_usage)
        return -1;

    return 0;
}

int main(void)
{
    ProcessInfo *previous;
    ProcessInfo *current;

    SystemCPU sys_previous = {0};
    SystemCPU sys_current = {0};

    int failed_previous = 0;
    int failed_current = 0;

    int cpu_count = (int)sysconf(_SC_NPROCESSORS_ONLN);

    if (cpu_count <= 0)
        cpu_count = 1;

    previous = calloc(MAX_PROCESSES, sizeof(ProcessInfo));
    current = calloc(MAX_PROCESSES, sizeof(ProcessInfo));

    if (previous == NULL || current == NULL)
    {
        perror("calloc");
        free(previous);
        free(current);
        return 1;
    }

    /* First snapshot */

    if (read_system_cpu(&sys_previous) != 0)
    {
        printf("First system CPU sample failed\n");
        goto cleanup;
    }

    int previous_count = collect_processes(
        previous, MAX_PROCESSES, &failed_previous);

    if (previous_count < 0)
        goto cleanup;

    sleep(1);

    /* Second snapshot */

    if (read_system_cpu(&sys_current) != 0)
    {
        printf("Second system CPU sample failed\n");
        goto cleanup;
    }

    int current_count = collect_processes(
        current, MAX_PROCESSES, &failed_current);

    if (current_count < 0)
        goto cleanup;

    uint64_t previous_total = system_total(&sys_previous);
    uint64_t current_total = system_total(&sys_current);

    /* Match process instances and calculate CPU usage */

    for (int i = 0; i < current_count; i++)
    {
        ProcessInfo *old = find_previous(
            previous, previous_count, &current[i]);

        if (old == NULL)
        {
            current[i].cpu_usage = 0.0;
            continue;
        }

        double usage = calculate_process_cpu_usage(
            old->utime,
            old->stime,
            current[i].utime,
            current[i].stime,
            previous_total,
            current_total,
            cpu_count
        );

        current[i].cpu_usage = usage < 0.0 ? 0.0 : usage;
    }

    qsort(current, current_count,
          sizeof(ProcessInfo), compare_cpu);

    printf("\n========== PROCMASTER LIVE CPU ==========\n");

    printf("%-8s %-24s %10s\n",
           "PID", "PROCESS", "CPU %");

    printf("------------------------------------------\n");

    int limit = current_count < DISPLAY_LIMIT
              ? current_count : DISPLAY_LIMIT;

    for (int i = 0; i < limit; i++)
    {
        printf("%-8d %-24.24s %9.2f%%\n",
               current[i].pid,
               current[i].name,
               current[i].cpu_usage);
    }

    printf("\nProcesses: %d\n", current_count);
    printf("CPU count: %d\n", cpu_count);

    printf("First snapshot failures: %d\n", failed_previous);
    printf("Second snapshot failures: %d\n", failed_current);

cleanup:

    free(previous);
    free(current);

    return 0;
}
