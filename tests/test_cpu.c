#include <stdio.h>

#include "cpu_monitor.h"

int main(void)
{
    SystemCPU cpu = {0};

    if (read_system_cpu(&cpu) != 0)
    {
        printf("Failed to read CPU statistics\n");
        return 1;
    }

    printf("\n========== SYSTEM CPU STATISTICS ==========\n");

    printf("User:    %lu\n", (unsigned long)cpu.user);
    printf("Nice:    %lu\n", (unsigned long)cpu.nice);
    printf("System:  %lu\n", (unsigned long)cpu.system);
    printf("Idle:    %lu\n", (unsigned long)cpu.idle);
    printf("Iowait:  %lu\n", (unsigned long)cpu.iowait);
    printf("IRQ:     %lu\n", (unsigned long)cpu.irq);
    printf("SoftIRQ: %lu\n", (unsigned long)cpu.softirq);
    printf("Steal:   %lu\n", (unsigned long)cpu.steal);

    return 0;
}
