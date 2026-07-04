#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    // spawn two children; the second one exits quickly, the first one
    // sleeps a bit longer, so they don't finish in fork order.
    int rc1 = fork();
    if (rc1 == 0) {
        printf("child A (pid %d): sleeping...\n", (int) getpid());
        sleep(2);
        printf("child A (pid %d): done\n", (int) getpid());
        exit(11);
    }

    int rc2 = fork();
    if (rc2 == 0) {
        printf("child B (pid %d): done immediately\n", (int) getpid());
        exit(22);
    }

    // parent: use waitpid() to wait for child B specifically, even
    // though child A will actually finish second.
    int status;
    int wc = waitpid(rc2, &status, 0);
    printf("parent: waitpid(%d, ...) returned %d, child B exit status = %d\n",
           rc2, wc, WEXITSTATUS(status));

    // now wait for child A (whichever pid is left)
    wc = waitpid(rc1, &status, 0);
    printf("parent: waitpid(%d, ...) returned %d, child A exit status = %d\n",
           rc1, wc, WEXITSTATUS(status));

    return 0;
}
