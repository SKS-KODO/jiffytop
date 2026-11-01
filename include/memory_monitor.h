#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <stdint.h>

typedef struct
{
    uint64_t total;
    uint64_t free;
    uint64_t available;
    uint64_t buffers;
    uint64_t cached;

    uint64_t swap_total;
    uint64_t swap_free;

    double usage_percent;
    double swap_usage_percent;

} SystemMemory;

int read_system_memory(SystemMemory *memory);

#endif
