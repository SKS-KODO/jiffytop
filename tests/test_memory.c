#include <stdio.h>
#include "memory_monitor.h"

int main(void)
{
    SystemMemory memory;

    if (read_system_memory(&memory) < 0)
    {
        fprintf(stderr, "Failed to read system memory\n");
        return 1;
    }

    printf("========= SYSTEM MEMORY =========\n");

    printf("Total RAM       : %lu kB\n", (unsigned long)memory.total);
    printf("Free RAM        : %lu kB\n", (unsigned long)memory.free);
    printf("Available RAM   : %lu kB\n", (unsigned long)memory.available);
    printf("Buffers         : %lu kB\n", (unsigned long)memory.buffers);
    printf("Cached          : %lu kB\n", (unsigned long)memory.cached);

    printf("\nRAM Usage       : %.2f%%\n", memory.usage_percent);

    printf("\n========= SWAP MEMORY =========\n");

    printf("Total Swap      : %lu kB\n", (unsigned long)memory.swap_total);
    printf("Free Swap       : %lu kB\n", (unsigned long)memory.swap_free);

    printf("Swap Usage      : %.2f%%\n", memory.swap_usage_percent);

    return 0;
}
