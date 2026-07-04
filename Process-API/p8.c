#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // child: close stdout, then try to printf
        close(STDOUT_FILENO);

        int n = printf("hello from child\n");
        int flushed = fflush(stdout);

        // use stderr for diagnostics since stdout is gone
        fprintf(stderr, "child: printf() returned %d\n", n);
        fprintf(stderr, "child: fflush(stdout) returned %d, errno=%d (%s)\n",
                flushed, errno, strerror(errno));
    } else {
        // parent
        wait(NULL);
        printf("parent: child finished\n");
    }

    return 0;
}
