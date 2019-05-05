#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    
    pid_t pid = 0;
    pid = fork();

    printf("PID: %d, PPID: %d\n", getpid(), getppid());

	return 0;
}

