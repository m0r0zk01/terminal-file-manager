#include <sys/wait.h>
#include <unistd.h>

void openFile(const char *path) {
    int pid = fork();
    if (pid < 0) {
        return;
    } else if (pid == 0) {
        char *const argv[] = {"vim", (char *)path, NULL};
        execvp("vim", argv);
    } else {
        waitpid(pid, 0, 0);
    }
}
