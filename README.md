# jiffytop

A small `top`-style process monitor for Linux, written in C. It reads everything from `/proc` by itself. No ncurses, no libraries, and it never calls `ps` or `top`. Once a second it clears the terminal and redraws system CPU, RAM, swap and the 20 biggest processes.

I wrote it to find out where tools like `top` actually get their numbers from. It turns out most of the work is parsing text files and doing subtraction carefully.

The binary is still called `proc_master`, and the banner on screen says ProcMaster. That was the project's first name.

## What it shows

At the top: system CPU %, number of cores, number of processes, how many processes it couldn't read, RAM usage / available / total, and swap usage.

Below that, one row per process: PID, PPID, state, name, CPU %, and RSS in KB.

## Build and run

You need Linux, `gcc` and `make`. On Fedora:

```
sudo dnf install gcc make
```

Then:

```
make
./proc_master
```

Ctrl+C quits.

## How the numbers are calculated

**System CPU.** The first line of `/proc/stat` has counters in jiffies (user, nice, system, idle, iowait, irq, softirq, steal). One sample on its own tells you nothing, so I take two samples a second apart and compare. Everything except idle and iowait counts as busy:

```
usage = (delta_total - delta_idle) / delta_total * 100
```

**Per-process CPU.** Fields 14 and 15 of `/proc/[pid]/stat` are the process's user and kernel time. Same idea: the change in `utime + stime`, divided by the change in total system jiffies, times the number of cores.

```
usage = delta_process / delta_system_total * cpu_count * 100
```

So 100% means one core fully used, the same as `top` by default. A multi-threaded process can go past 100%. A process that appeared since the last sample shows 0.00%, and `N/A` means the counters made no sense (went backwards).

**Memory.** RAM usage is `(MemTotal - MemAvailable) / MemTotal`. I used `MemAvailable` rather than `MemFree` because `MemFree` counts reclaimable cache as used, which makes a healthy machine look full.

Two parsing details that bit me:

- The process name in `/proc/[pid]/stat` sits inside parentheses and can itself contain spaces and parentheses. The parser looks for the first `(` and the last `)` and reads the fields after that.
- PIDs get reused. A process is matched to its previous sample by PID and start time together, so a new process that inherited an old PID doesn't get the old one's CPU history.

## Project layout

```
jiffytop/
├── include/
│   ├── proc_read.h
│   ├── cpu_monitor.h
│   └── memory_monitor.h
├── src/
│   ├── main.c              refresh loop and display
│   ├── proc_read.c         /proc/[pid]/stat and status
│   ├── cpu_monitor.c       /proc/stat and the CPU maths
│   └── memory_monitor.c    /proc/meminfo
├── tests/
├── docs/
├── Makefile
├── LICENSE
└── README.md
```

## Tests

The programs in `tests/` are small standalone checks, not a test framework. The Makefile doesn't build them. This compiles all of them into the project folder:

```
for t in tests/test_*.c; do
    gcc -Wall -Wextra -Iinclude "$t" src/proc_read.c src/cpu_monitor.c src/memory_monitor.c -o "$(basename "$t" .c)"
done
```

What each one does:

- `test_cpu` prints the raw counters from `/proc/stat`
- `test_cpu_usage` takes two samples and prints system CPU %
- `test_process_cpu` runs the per-process formula on fixed numbers; the answer should be `120.00%`
- `test_process_info <pid>` prints what the parser reads for one process, for example `./test_process_info $$`
- `test_live_cpu` is the earlier live per-process CPU display
- `test_memory` prints RAM and swap figures

## Known issues

- The table is sorted by RSS, not by CPU %. There is a `compare_cpu_usage` function in `main.c` that isn't used, which is why `gcc` prints an unused-function warning. Swapping it into the `qsort` call makes it sort by CPU.
- Kernel threads have no resident memory, so they show 0 KB and end up at the bottom.
- Refresh rate (1 second), row count (20) and process limit (4096) are fixed in the source.
- No keyboard controls. You can't sort, filter or kill anything from inside it.
- Linux only.

## License

MIT. See [LICENSE](LICENSE).

## Output

<img width="732" height="807" alt="image" src="https://github.com/user-attachments/assets/6762402a-200c-43fb-ae08-2bc8002fa7e8" />
