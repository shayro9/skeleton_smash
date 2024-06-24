#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main() {
    int i = 0;
    printf("%d\n", getpid());

FORK:
    pid_t p = fork();
    i++;
    if (i < 3) {
        goto FORK;
    }
    while (waitpid(p, NULL, 0) != -1);

    printf("(PID: %d, fork: %d)\n", getpid(), i);
    return 0;
}

 alias L='sleep '
 L 10 0&
 jobs
 L 100&
