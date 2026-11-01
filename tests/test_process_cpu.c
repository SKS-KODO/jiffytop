#include <stdio.h>
#include "cpu_monitor.h"

int main(void)
{
    double usage = calculate_process_cpu_usage(
        100, 50,       /* Previous process ticks */
        150, 60,       /* Current process ticks */
        1000,          /* Previous system total */
        1200,          /* Current system total */
        4              /* Number of CPUs */
    );

    printf("Process CPU Usage: %.2f%%\n", usage);

    return 0;
}
