#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
static void pin_to_cpu0(void) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);
}
#else
// macOS has no sched_setaffinity(); the closest thing (thread affinity
// tags via thread_policy_set) is only a scheduling hint, not a hard pin,
// so there's no reliable way to force both processes onto one core here.
static void pin_to_cpu0(void) {}
#endif

// Cost of a context switch, lmbench-style: two processes ping-pong a
// single byte over two pipes. Each round trip (parent write -> child
// read -> child write -> parent read) forces exactly two context
// switches (parent->child, child->parent), since each side blocks in
// read() waiting on the other.

int main(int argc, char *argv[]) {
    long iters = (argc > 1) ? atol(argv[1]) : 100000L;

    int parent_to_child[2];
    int child_to_parent[2];
    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    char token = 'x';

    if (pid == 0) {
        // child: read a byte, immediately bounce it back
        pin_to_cpu0();
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        for (long i = 0; i < iters; i++) {
            if (read(parent_to_child[0], &token, 1) != 1) break;
            if (write(child_to_parent[1], &token, 1) != 1) break;
        }

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        exit(0);
    }

    // parent: send a byte, wait for it to come back
    pin_to_cpu0();
    close(parent_to_child[0]);
    close(child_to_parent[1]);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (long i = 0; i < iters; i++) {
        if (write(parent_to_child[1], &token, 1) != 1) break;
        if (read(child_to_parent[0], &token, 1) != 1) break;
    }
    gettimeofday(&end, NULL);

    close(parent_to_child[1]);
    close(child_to_parent[0]);
    waitpid(pid, NULL, 0);

    double elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
    double switches = iters * 2.0; // two switches per round trip
    printf("%ld ping-pong round trips took %.2f us total\n", iters, elapsed_us);
    printf("average context-switch cost: %.4f us\n", elapsed_us / switches);

    return 0;
}
