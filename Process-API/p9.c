#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int fds[2];
    if (pipe(fds) < 0) {
        fprintf(stderr, "pipe failed\n");
        exit(1);
    }
    int read_end = fds[0];
    int write_end = fds[1];

    // child A: writer -- its stdout becomes the pipe's write end,
    // then runs "ls -l" so its output flows into the pipe
    int rc1 = fork();
    if (rc1 < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc1 == 0) {
        close(read_end);              // child A never reads from the pipe
        dup2(write_end, STDOUT_FILENO); // redirect stdout -> pipe write end
        close(write_end);              // original fd no longer needed

        execlp("ls", "ls", "-l", NULL);
        perror("execlp ls");
        exit(1);
    }

    // child B: reader -- its stdin becomes the pipe's read end,
    // then runs "wc -l" which counts lines coming from the pipe
    int rc2 = fork();
    if (rc2 < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc2 == 0) {
        close(write_end);             // child B never writes to the pipe
        dup2(read_end, STDIN_FILENO); // redirect stdin -> pipe read end
        close(read_end);              // original fd no longer needed

        execlp("wc", "wc", "-l", NULL);
        perror("execlp wc");
        exit(1);
    }

    // parent: must close BOTH ends here, or child B will never see EOF
    // (the write end would still be open in the parent, so the pipe
    // would never be considered fully closed)
    close(read_end);
    close(write_end);

    waitpid(rc1, NULL, 0);
    waitpid(rc2, NULL, 0);

    return 0;
}
