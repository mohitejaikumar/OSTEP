#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int x = 100;
    printf("before fork: x = %d (pid %d)\n", x, (int) getpid());

    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        return 1;
    } else if (rc == 0) {
        // child
        x = x - 20;
        printf("child (pid %d): x = %d\n", (int) getpid(), x);
    } else {
        // parent
        x = x + 20;
        printf("parent (pid %d) of child %d: x = %d\n", (int) getpid(), rc, x);
    }

    return 0;
}
