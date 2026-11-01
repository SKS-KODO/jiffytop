#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "memory_monitor.h"

static void calculate_memory_usage(SystemMemory *memory);

static int parse_memory_value(const char *line,
                              const char *field,
                              uint64_t *value)
{
    size_t field_length = strlen(field);

    if (line == NULL || field == NULL || value == NULL)
        return -1;

    if (strncmp(line, field, field_length) != 0)
        return 0;

    const char *ptr = line + field_length;

    while (*ptr == ' ' || *ptr == '\t')
        ptr++;

    if (*ptr < '0' || *ptr > '9')
        return -1;

    errno = 0;

    char *endptr;
    unsigned long long parsed = strtoull(ptr, &endptr, 10);

    if (errno != 0 || endptr == ptr)
        return -1;

    while (*endptr == ' ' || *endptr == '\t')
        endptr++;

    if (strncmp(endptr, "kB", 2) != 0)
        return -1;

    endptr += 2;

    while (*endptr == ' ' || *endptr == '\t' ||
           *endptr == '\n' || *endptr == '\r')
        endptr++;

    if (*endptr != '\0')
        return -1;

    *value = (uint64_t)parsed;

    return 1;
}

int read_system_memory(SystemMemory *memory)
{
    if (memory == NULL)
        return -1;

    FILE *fp = fopen("/proc/meminfo", "r");

    if (fp == NULL)
        return -1;

    memset(memory, 0, sizeof(*memory));

    char line[256];

    unsigned int found = 0;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (parse_memory_value(line, "MemTotal:", &memory->total) == 1)
            found |= (1U << 0);

        else if (parse_memory_value(line, "MemFree:", &memory->free) == 1)
            found |= (1U << 1);

        else if (parse_memory_value(line, "MemAvailable:", &memory->available) == 1)
            found |= (1U << 2);

        else if (parse_memory_value(line, "Buffers:", &memory->buffers) == 1)
            found |= (1U << 3);

        else if (parse_memory_value(line, "Cached:", &memory->cached) == 1)
            found |= (1U << 4);

        else if (parse_memory_value(line, "SwapTotal:", &memory->swap_total) == 1)
            found |= (1U << 5);

        else if (parse_memory_value(line, "SwapFree:", &memory->swap_free) == 1)
            found |= (1U << 6);
    }

    fclose(fp);

    if (found != 0x7F)
    	return -1;

    calculate_memory_usage(memory);
    
    return 0;
}

static void calculate_memory_usage(SystemMemory *memory)
{
    if (memory == NULL)
        return;

    if (memory->total > 0 &&
        memory->available <= memory->total)
    {
        memory->usage_percent =
            ((double)(memory->total - memory->available)
             / memory->total) * 100.0;
    }
    else
    {
        memory->usage_percent = -1.0;
    }

    if (memory->swap_total > 0 &&
        memory->swap_free <= memory->swap_total)
    {
        memory->swap_usage_percent =
            ((double)(memory->swap_total - memory->swap_free)
             / memory->swap_total) * 100.0;
    }
    else if (memory->swap_total == 0)
    {
        memory->swap_usage_percent = 0.0;
    }
    else
    {
        memory->swap_usage_percent = -1.0;
    }
}


