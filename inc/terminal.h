#pragma once

#include "color_codes.h"

extern int CUR_TERMINAL;
extern int H, W;

enum TerminalType {
    XTERM = 0,
    // don't forget to change below value when adding new entry!
    TERMINAL_COUNT = 1
};

enum ControlSequence {
    ENTER_ALT_MODE = 0,
    EXIT_ALT_MODE = 1,
    CLEAR_SCREEN = 2,
    SET_CURSOR = 3,
    ROW_BG_COLOR = 4,
    TEXT_COLOR = 5,
    TEXT_BG_COLOR = 6,
    RESET_TEXT_COLOR = 7,
    ENABLE_CURSOR = 8,
    DISABLE_CURSOR = 9,
    // don't forget to change below value when adding new entry!
    CONTROL_SEQUENCE_COUNT = 10,
};

typedef struct Pos {
    int x;
    int y;
} Pos;

void SetTerminal(enum TerminalType type);

void EnterAltMode();
void ExitAltMode();

void ExitRawMode();
void EnterRawMode();

void EnableCursor();
void DisableCursor();

void Action(enum ControlSequence action, ...);

int UpdateWinSize();

void Print(const char* textColor, const char *bgColor, const char *format, ...);

static const char *CONTROL[TERMINAL_COUNT][CONTROL_SEQUENCE_COUNT] = {
    [XTERM] = {
        [ENTER_ALT_MODE]        = "\e[?1049h",
        [EXIT_ALT_MODE]         = "\e[?1049l",
        [CLEAR_SCREEN]          = "\e[H\e[J",
        [SET_CURSOR]            = "\e[%d;%dH",    // row;col coords
        [ROW_BG_COLOR]          = "%s\e[K\e[0m",
        [TEXT_COLOR]            = "%s",
        [TEXT_BG_COLOR]         = "%s",
        [RESET_TEXT_COLOR]      = COLOR_RESET,
        [ENABLE_CURSOR]         = "\e[?25h",
        [DISABLE_CURSOR]        = "\e[?25l",
    },
};
