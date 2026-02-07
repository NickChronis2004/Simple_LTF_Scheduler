#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <time.h>

#define __NR_set_proc_info 341
#define __NR_get_proc_info 342

static long set_proc_info(int deadline, int est_runtime) {
    return syscall(__NR_set_proc_info, deadline, est_runtime);
}

static long get_proc_info(int *deadline, int *est_runtime) {
    return syscall(__NR_get_proc_info, deadline, est_runtime);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <deadline> <est_runtime> [burn_seconds]\n",
            argv[0]);
        return 1;
    }

    int deadline = atoi(argv[1]);
    int runtime  = atoi(argv[2]);
    int burn     = (argc >= 4) ? atoi(argv[3]) : 0;

    printf("[PID %d] set_proc_info(%d, %d)\n",
           getpid(), deadline, runtime);
    fflush(stdout);

    if (set_proc_info(deadline, runtime) < 0) {
        perror("set_proc_info");
        return 1;
    }

    /* If no burn_seconds given, just test the syscall */
    if (burn <= 0) {
        int dl, rt;
        if (get_proc_info(&dl, &rt) == 0) {
            printf("[PID %d] get_proc_info -> deadline=%d runtime=%d\n",
                   getpid(), dl, rt);
        }
        return 0;
    }

    /* CPU burn with heartbeat */
    time_t start = time(NULL);
    time_t last  = start;
    volatile unsigned long long it = 0;

    while (time(NULL) - start < burn) {
        it++;

        time_t now = time(NULL);
        if (now != last) {
            last = now;
            printf("[PID %d] t=%lds it=%llu\n",
                   getpid(),
                   (long)(now - start),
                   it);
            fflush(stdout);
        }
    }

    printf("[PID %d] DONE it=%llu\n", getpid(), it);
    fflush(stdout);

    return 0;
}
