#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int rc = fork();

    if (rc == 0) {
        // child: takes a couple seconds to finish
        sleep(2);
        printf("child (pid %d): done\n", (int) getpid());
        exit(7);
    }

    // parent: poll non-blockingly with WNOHANG while doing other "work"
    int status;
    int wc;
    int tick = 0;
    while ((wc = waitpid(rc, &status, WNOHANG)) == 0) {
        printf("parent: child not done yet, doing other work (tick %d)\n", tick++);
        usleep(300 * 1000); // pretend to do 300ms of other work
    }

    if (wc == -1) {
        perror("waitpid");
        exit(1);
    }

    printf("parent: waitpid returned %d, child exit status = %d\n",
           wc, WEXITSTATUS(status));

    return 0;
}
