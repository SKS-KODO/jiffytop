#ifndef PROC_READ_H
#define PROC_READ_H

#include<sys/types.h>
#include<stdint.h>

#define MAX_PROCESSES 4096

typedef struct{
	pid_t pid;
	pid_t ppid;

	char state;
	char name[256];

	unsigned long vmsize_kb;
	unsigned long vmrss_kb;

	uint64_t utime;
	uint64_t stime;

	double cpu_usage;

	uint64_t starttime;

}ProcessInfo;

int list_processes(void);
int read_process_stat(pid_t pid, ProcessInfo *info);
int read_process_status(pid_t pid, ProcessInfo *info);

int collect_processes(ProcessInfo processes[], int capacity, int *failed);
#endif
