#include <stdio.h>
#include <stdlib.h>

#include "proc_read.h"

int main(int argc, char *argv[])
{
    ProcessInfo info;

    if (argc != 2)
    {
        printf("Usage: %s <PID>\n", argv[0]);
        return 1;
    }

    pid_t pid = (pid_t)atoi(argv[1]);

    if (read_process_stat(pid, &info) == -1)
    {
        perror("read_process_stat");
        return 1;
    }

    if (read_process_status(pid, &info) == -1)
    {
        perror("read_process_status");
        return 1;
    }

    printf("\n========== PROCESS INFORMATION ==========\n");

    printf("PID       : %d\n", info.pid);
    printf("Name      : %s\n", info.name);
    printf("PPID      : %d\n", info.ppid);
    printf("State     : %c\n", info.state);
    printf("VmSize    : %lu kB\n", info.vmsize_kb);
    printf("VmRSS     : %lu kB\n", info.vmrss_kb);

    printf("User CPU ticks : %lu\n", (unsigned long)info.utime);
    printf("System CPU ticks: %lu\n", (unsigned long)info.stime);
    return 0;
}
