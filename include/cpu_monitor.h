#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include<stdint.h>

typedef struct{
	uint64_t user;
	uint64_t nice;
	uint64_t system;
	uint64_t idle;
	uint64_t iowait;
	uint64_t irq;
	uint64_t softirq;
	uint64_t steal;
}SystemCPU;

int read_system_cpu(SystemCPU *cpu);

double calculate_cpu_usage(SystemCPU *previous, SystemCPU *current);

double calculate_process_cpu_usage(
    uint64_t previous_utime,
    uint64_t previous_stime,
    uint64_t current_utime,
    uint64_t current_stime,
    uint64_t previous_system_total,
    uint64_t current_system_total,
    int cpu_count
);
#endif
