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
        // child: has no children of its own, so wait() should fail
        printf("child (pid %d): calling wait()...\n", (int) getpid());
        int wc = wait(NULL);
        printf("child (pid %d): wait() returned %d, errno=%d (%s)\n",
               (int) getpid(), wc, errno, strerror(errno));
    } else {
        // parent: waits for the real child to finish
        int status;
        int wc = wait(&status);
        printf("parent (pid %d): wait() returned %d (child's pid), "
               "my own pid is %d\n", (int) getpid(), wc, (int) getpid());
        if (WIFEXITED(status)) {
            printf("parent: child exited normally with status %d\n",
                   WEXITSTATUS(status));
        }
    }

    return 0;
}
