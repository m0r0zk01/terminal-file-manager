#include <sys/wait.h>
#include <unistd.h>

void openFile(const char *path) {
    int pid = fork();
    int status = 0;
    if (pid < 0) {
        return;
    } else if (pid == 0) {
        char *const argv[] = {"gzip", "-d", (char *)path, NULL};
        execvp("gzip", argv);
    } else {
        waitpid(pid, 0, 0);
    }
}
