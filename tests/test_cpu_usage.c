#include <stdio.h>
#include <unistd.h>

#include "cpu_monitor.h"

int main(void)
{
    SystemCPU previous = {0};
    SystemCPU current = {0};

    if (read_system_cpu(&previous) != 0)
    {
        printf("First CPU sample failed\n");
        return 1;
    }

    sleep(1);

    if (read_system_cpu(&current) != 0)
    {
        printf("Second CPU sample failed\n");
        return 1;
    }

    double usage = calculate_cpu_usage(&previous, &current);

    if (usage < 0.0)
    {
        printf("CPU calculation failed\n");
        return 1;
    }

    printf("\nSystem CPU Usage: %.2f%%\n", usage);

    return 0;
}
