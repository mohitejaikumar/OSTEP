#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    

    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // child: write a bunch of lines through the shared fd
        int fd = open("p2.out", O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        for (int i = 0; i < 1000; i++) {
            dprintf(fd, "child  line %d\n", i);
        }
        close(fd);
    } else {
        // parent: write a bunch of lines through the same fd
        int fd = open("p2.out", O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        for (int i = 0; i < 1000; i++) {
            dprintf(fd, "parent line %d\n", i);
        }
        close(fd);
    }

    
    return 0;
}
