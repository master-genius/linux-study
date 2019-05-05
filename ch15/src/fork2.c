#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    
    pid_t pid = 0;

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid > 0) {
        printf("I am parent, child pid : %d\n", pid);
    } else {
        printf("My pid : %d\n", getpid());
    }

    return 0;
}
