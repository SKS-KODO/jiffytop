#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>

#include "cpu_monitor.h"
#include <inttypes.h>

int read_system_cpu(SystemCPU *cpu)
{
    FILE *fp;

    if (cpu == NULL)
        return -1;

    fp = fopen("/proc/stat", "r");

    if (fp == NULL)
        return -1;
/*
    int result = fscanf(fp,
        "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
        &cpu->user,
        &cpu->nice,
        &cpu->system,
        &cpu->idle,
        &cpu->iowait,
        &cpu->irq,
        &cpu->softirq,
        &cpu->steal);*/
   
    int result = fscanf(fp,
    "cpu %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
    " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64,
    &cpu->user,
    &cpu->nice,
    &cpu->system,
    &cpu->idle,
    &cpu->iowait,
    &cpu->irq,
    &cpu->softirq,
    &cpu->steal);
    fclose(fp);

    if (result != 8)
        return -1;

    return 0;
}

double calculate_cpu_usage(SystemCPU *previous,
                           SystemCPU *current)
{
    if (previous == NULL || current == NULL)
        return -1.0;

    uint64_t previous_total =
        previous->user +
        previous->nice +
        previous->system +
        previous->idle +
        previous->iowait +
        previous->irq +
        previous->softirq +
        previous->steal;

    uint64_t current_total =
        current->user +
        current->nice +
        current->system +
        current->idle +
        current->iowait +
        current->irq +
        current->softirq +
        current->steal;

    if (current_total <= previous_total)
        return -1.0;

    uint64_t previous_idle =
        previous->idle + previous->iowait;

    uint64_t current_idle =
        current->idle + current->iowait;

    uint64_t total_delta = current_total - previous_total;

    uint64_t idle_delta = current_idle - previous_idle;

    if (idle_delta > total_delta)
        return -1.0;

    uint64_t busy_delta = total_delta - idle_delta;

    return ((double)busy_delta / total_delta) * 100.0;
}

double calculate_process_cpu_usage(
    uint64_t previous_utime,
    uint64_t previous_stime,
    uint64_t current_utime,
    uint64_t current_stime,
    uint64_t previous_system_total,
    uint64_t current_system_total,
    int cpu_count)
{
    if (cpu_count <= 0)
        return -1.0;

    if (current_utime < previous_utime ||
        current_stime < previous_stime ||
        current_system_total <= previous_system_total)
        return -1.0;

    uint64_t previous_process =
        previous_utime + previous_stime;

    uint64_t current_process =
        current_utime + current_stime;

    if (current_process < previous_process)
        return -1.0;

    uint64_t process_delta =
        current_process - previous_process;

    uint64_t system_delta =
        current_system_total - previous_system_total;

    return ((double)process_delta / system_delta)
           * cpu_count * 100.0;
}
