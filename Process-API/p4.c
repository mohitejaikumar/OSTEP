#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

extern char **environ;

// run one exec variant in a child, wait for it, then return
void run(const char *label, void (*variant)(void)) {
    printf("--- %s ---\n", label);
    fflush(stdout);
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        variant();
        // only reached if exec failed
        perror("exec");
        exit(1);
    } else {
        waitpid(rc, NULL, 0);
    }
    printf("\n");
    fflush(stdout);
}

void v_execl(void)  { execl("/bin/ls", "ls", "-l", NULL); }
void v_execle(void) { char *envp[] = { "MY_VAR=hello", NULL };
                      execle("/bin/ls", "ls", "-l", NULL, envp); }
void v_execlp(void) { execlp("ls", "ls", "-l", NULL); }
void v_execv(void)  { char *argv[] = { "ls", "-l", NULL };
                      execv("/bin/ls", argv); }
void v_execvp(void) { char *argv[] = { "ls", "-l", NULL };
                      execvp("ls", argv); }
#ifdef __linux__
void v_execvpe(void) { char *argv[] = { "ls", "-l", NULL };
                       char *envp[] = { "MY_VAR=hello", NULL };
                       execvpe("ls", argv, envp); }
#endif

int main(int argc, char *argv[]) {
    run("execl",  v_execl);
    run("execle", v_execle);
    run("execlp", v_execlp);
    run("execv",  v_execv);
    run("execvp", v_execvp);
#ifdef __linux__
    run("execvpe", v_execvpe);
#else
    printf("--- execvpe ---\n(skipped: execvpe is a GNU/glibc extension, not available on macOS/BSD)\n\n");
    fflush(stdout);
#endif
    return 0;
}
