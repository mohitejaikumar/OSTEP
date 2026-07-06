#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Cost of a system call: repeatedly issue a 0-byte read() on /dev/zero
// (a valid fd, so the syscall does real work resolving the fd and
// returning, but no actual data movement) and time the whole loop.

int main(int argc, char *argv[]) {
    long iters = (argc > 1) ? atol(argv[1]) : 1000000L;

    int fd = open("/dev/zero", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/zero");
        exit(1);
    }

    char buf[1];
    struct timeval start, end;

    gettimeofday(&start, NULL);
    for (long i = 0; i < iters; i++) {
        read(fd, buf, 0);
    }
    gettimeofday(&end, NULL);

    close(fd);

    double elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
    printf("%ld calls to read(fd, buf, 0) took %.2f us total\n", iters, elapsed_us);
    printf("average system-call cost: %.4f us\n", elapsed_us / iters);

    return 0;
}
