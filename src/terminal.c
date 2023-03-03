#include "terminal.h"
#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

int CUR_TERMINAL = XTERM;
int H = 0, W = 0;
static struct termios term;

void EnterAltMode() {
    Action(ENTER_ALT_MODE);
}

void ExitAltMode() {
    Action(EXIT_ALT_MODE);
}

void ExitRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void EnterRawMode() {
    tcgetattr(STDIN_FILENO, &term);

    struct termios raw = term;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    setvbuf(stdout, NULL, _IONBF, 0);

    atexit(ExitRawMode);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void EnableCursor() {
    Action(ENABLE_CURSOR);
}

void DisableCursor() {
    Action(DISABLE_CURSOR);
}

void SetTerminal(enum TerminalType terminal) {
    CUR_TERMINAL = terminal;
}

void Action(enum ControlSequence action, ...) {
    va_list args;
    va_start (args, action);
    vprintf(CONTROL[CUR_TERMINAL][action], args);
    va_end(args);
}

int UpdateWinSize() {
    struct winsize res;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &res) == -1) {
        return 1;
    }
    H = res.ws_row;
    W = res.ws_col;
    return 0;
}

void Print(const char* textColor, const char *bgColor, const char *format, ...) {
    if (textColor != NULL) {
        Action(TEXT_COLOR, textColor);
    }
    if (bgColor != NULL) {
        Action(TEXT_BG_COLOR, bgColor);
    }

    va_list args;
    va_start (args, format);
    vprintf(format, args);
    va_end(args);

    Action(RESET_TEXT_COLOR);
}
