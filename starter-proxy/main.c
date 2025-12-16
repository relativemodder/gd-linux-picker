#define _GNU_SOURCE
#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <signal.h>

extern char **environ;

int main(int argc, char *argv[]) {
    HANDLE watchdogMutex = OpenMutexA(SYNCHRONIZE, FALSE, "PickerWatchdogMutex");
    if (watchdogMutex == NULL) {
        printf("Failed to open watchdog mutex. Exiting.\n");
        return 1;
    }

    char* execx = "/tmp/picker-linux-side";

    if (chmod(execx, S_IRUSR | S_IWUSR | S_IXUSR | 
                        S_IRGRP | S_IXGRP |          
                        S_IROTH | S_IXOTH) < 0) {    
        perror("Warning: chmod to add executable flag failed");
    }

    printf("Proxy started\n");
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        CloseHandle(watchdogMutex);
        return 765;
    }

    if (pid == 0) {
        char *argsss[] = { execx, NULL };
        execve(execx, argsss, environ);
        perror("execve failed");
        CloseHandle(watchdogMutex);
        _exit(errno);
    }

    WaitForSingleObject(watchdogMutex, INFINITE);
    kill(pid, SIGTERM);
    CloseHandle(watchdogMutex);

    return 1;
}
