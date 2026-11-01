
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "proc_read.h"


/*
 * Function: is_numeric_pid
 * Purpose : Validate whether a directory name represents
 *           a valid positive Linux PID.
 */

static int is_numeric_pid(const char *name, int *pid)
{
    char *endptr;
    long value;

    if (name == NULL || pid == NULL || *name == '\0')
        return 0;

    for (const unsigned char *p = (const unsigned char *)name;
         *p != '\0'; p++)
    {
        if (!isdigit(*p))
            return 0;
    }

    errno = 0;
    value = strtol(name, &endptr, 10);

    if (errno == ERANGE || *endptr != '\0')
        return 0;

    if (value <= 0 || value > INT_MAX)
        return 0;

    *pid = (int)value;

    return 1;
}


/*
 * Function: list_processes
 * Purpose : Enumerate all visible process directories
 *           inside /proc.
 */

int list_processes(void)
{
    DIR *proc_dir;
    struct dirent *entry;
    struct stat st;

    int pid;
    int count = 0;

    proc_dir = opendir("/proc");

    if (proc_dir == NULL)
    {
        perror("opendir /proc");
        return -1;
    }

    printf("\n========== PROCESSES ==========\n");
    printf("%-10s %-20s\n", "PID", "DIRECTORY");

    while (1)
    {
        errno = 0;

        entry = readdir(proc_dir);

        if (entry == NULL)
        {
            if (errno != 0)
                perror("readdir");

            break;
        }

        if (!is_numeric_pid(entry->d_name, &pid))
            continue;

        if (fstatat(dirfd(proc_dir), entry->d_name,
                    &st, 0) == -1)
        {
            continue;
        }

        if (!S_ISDIR(st.st_mode))
            continue;

        printf("%-10d %-20s\n", pid, entry->d_name);

        count++;
    }

    closedir(proc_dir);

    printf("\nTotal processes found: %d\n", count);

    return count;
}

int read_process_stat(pid_t pid, ProcessInfo *info)
{
    char path[64];
    char buffer[4096];

    FILE *fp;

    if (info == NULL || pid <= 0)
        return -1;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    fp = fopen(path, "r");

    if (fp == NULL)
        return -1;

    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    char *left = strchr(buffer, '(');
    char *right = strrchr(buffer, ')');

    if (left == NULL || right == NULL || right <= left)
        return -1;

    *right = '\0';

    /* Extract process name */

    char *name_start = left + 1;

    size_t name_length = strlen(name_start);

    if (name_length >= sizeof(info->name))
        name_length = sizeof(info->name) - 1;

    memcpy(info->name, name_start, name_length);
    info->name[name_length] = '\0';

    /* Extract PID */

    /* Field 1: PID */
    	
    	char *endptr;
	*left = '\0';

	errno = 0;

	long parsed_pid = strtol(buffer, &endptr, 10);

	while (isspace((unsigned char)*endptr))
    		endptr++;

	if (errno != 0 || endptr == buffer || *endptr != '\0' || parsed_pid <= 0 || parsed_pid > INT_MAX){
    		errno = EINVAL;
    		return -1;
	}

    /* Tokenize fields following comm */

    char *saveptr;
    char *token;

    token = strtok_r(right + 1, " \n", &saveptr);

    /* Field 3: state */

    if (token == NULL || strlen(token) != 1)
        return -1;

    info->state = token[0];

    /* Field 4: PPID */

    token = strtok_r(NULL, " \n", &saveptr);

    if (token == NULL)
        return -1;

    errno = 0;

    long parsed_ppid = strtol(token, &endptr, 10);

    if (errno != 0 || *endptr != '\0' ||
        parsed_ppid < 0 || parsed_ppid > INT_MAX)
        return -1;

    /* Skip fields 5 through 13 */

    for (int i = 0; i < 9; i++)
    {
        token = strtok_r(NULL, " \n", &saveptr);

        if (token == NULL)
            return -1;
    }

    /* Field 14: utime */

    token = strtok_r(NULL, " \n", &saveptr);

    if (token == NULL)
        return -1;

    errno = 0;

    unsigned long long utime = strtoull(token, &endptr, 10);

    if (errno != 0 || *endptr != '\0')
        return -1;

    /* Field 15: stime */

    token = strtok_r(NULL, " \n", &saveptr);

    if (token == NULL)
        return -1;

    errno = 0;

    unsigned long long stime = strtoull(token, &endptr, 10);

    if (errno != 0 || *endptr != '\0')
        return -1;

    info->pid = (pid_t)parsed_pid;
    info->ppid = (pid_t)parsed_ppid;

    info->utime = (uint64_t)utime;
    info->stime = (uint64_t)stime;

    /* starttime*/

	for (int i = 0; i < 6; i++){
    		token = strtok_r(NULL, " \n", &saveptr);

    		if (token == NULL)
        		return -1;
	}

	/* Field 22: starttime */

	token = strtok_r(NULL, " \n", &saveptr);

	if (token == NULL)
    		return -1;

	errno = 0;

	unsigned long long starttime = strtoull(token, &endptr, 10);

	if (errno != 0 || *endptr != '\0')
    		return -1;

	info->starttime = (uint64_t)starttime;

    	return 0;
	}
/*
int read_process_stat(pid_t pid, ProcessInfo *info)
{
    char path[64];
    char buffer[4096];

    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    fp = fopen(path, "r");

    if (fp == NULL)
        return -1;

    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    char *left = strchr(buffer, '(');
    char *right = strrchr(buffer, ')');

    if (left == NULL || right == NULL || right <= left)
        return -1;

    *right = '\0';

    char *name_start = left + 1;

    size_t name_length = strlen(name_start);

    if (name_length >= sizeof(info->name))
        name_length = sizeof(info->name) - 1;

    memcpy(info->name, name_start, name_length);
    info->name[name_length] = '\0';

    long parsed_pid;
    char state;

    long parsed_ppid;

    if (sscanf(buffer, "%ld", &parsed_pid) != 1)
        return -1;

    if (sscanf(right + 1, " %c %ld", &state, &parsed_ppid) != 2)
        return -1;

    if (parsed_pid <= 0 || parsed_pid > INT_MAX ||
        parsed_ppid < 0 || parsed_ppid > INT_MAX)
        return -1;

    info->pid = (pid_t)parsed_pid;
    info->ppid = (pid_t)parsed_ppid;
    info->state = state;

    return 0;
}
*/
int read_process_status(pid_t pid, ProcessInfo *info)
{
    char path[64];
    char line[256];

    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    fp = fopen(path, "r");

    if (fp == NULL)
        return -1;

    info->vmsize_kb = 0;
    info->vmrss_kb = 0;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (sscanf(line, "VmSize: %lu kB",
                   &info->vmsize_kb) == 1)
            continue;

        if (sscanf(line, "VmRSS: %lu kB",
                   &info->vmrss_kb) == 1)
            continue;
    }

    fclose(fp);

    return 0;
}

int collect_processes(ProcessInfo processes[],
                      int capacity,
                      int *failed)
{
    DIR *proc_dir;
    struct dirent *entry;
    struct stat st;

    int pid;
    int count = 0;

    if (processes == NULL || failed == NULL || capacity <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    *failed = 0;

    proc_dir = opendir("/proc");

    if (proc_dir == NULL)
    {
        perror("opendir");
        return -1;
    }

    while (1)
    {
        errno = 0;

        entry = readdir(proc_dir);

        if (entry == NULL)
        {
            if (errno != 0)
                perror("readdir");

            break;
        }

        if (!is_numeric_pid(entry->d_name, &pid))
            continue;

        if (fstatat(dirfd(proc_dir), entry->d_name,
                    &st, 0) == -1)
        {
            (*failed)++;
            continue;
        }

        if (!S_ISDIR(st.st_mode))
            continue;

        if (count >= capacity)
        {
            (*failed)++;
            continue;
        }

        ProcessInfo info = {0};

        if (read_process_stat(pid, &info) == -1)
        {
            (*failed)++;
            continue;
        }

        if (read_process_status(pid, &info) == -1)
        {
            (*failed)++;
            continue;
        }

        processes[count] = info;
        count++;
    }

    closedir(proc_dir);

    return count;
}
