#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char *argv[]) {
    // regular variables are NOT shared across fork() (each process gets its
    // own copy), so we need actual shared memory to signal between them
    // without using wait().
    int *done = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (done == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    *done = 0;

    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // child
        printf("hello\n");
        *done = 1;
    } else {
        // parent: spin-wait (no wait()) until the child signals it's done
        while (*done == 0)
            ;
        printf("goodbye\n");
    }

    return 0;
}
