#include "color_codes.h"
#include "columns.h"
#include "config.h"
#include "files.h"
#include "terminal.h"
#include "utils.h"

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CUR_DIR_SIZE (PATH_MAX << 2)
#define COPY_BUFFER_SIZE 4096
#define HEADER_HEIGHT 2
#define FOOTER_HEIGHT 2

#define EXTENSIONS_DIRECTORY "extensions"
#define EXTENSIONS_CONFIG "config.txt"
#define EXTENSIONS_CNT 100
#define EXT_SO_NAME_LEN 15
#define EXTENSION_FOO_NAME "openFile"

static struct State {
    char curDir[CUR_DIR_SIZE];
    Files files;
    int fileCursor;
    int fileIno;
    int fileCursorScreenInd;
    int colCursor;
    bool sortDesc;
    bool showHidden;
    FILE *toCopy;
    char copiedName[SizeOfField(DirEnt, d_name) + 20];
    char cutPath[CUR_DIR_SIZE];

    struct {
        char ext[EXT_SO_NAME_LEN];
        char so[EXT_SO_NAME_LEN];
    } extensions[EXTENSIONS_CNT];
    char extensionsePath[CUR_DIR_SIZE];
} State;

static void LoadExtensions() {
    char *end = State.extensionsePath + strlen(State.extensionsePath);
    strncpy(end, "/"EXTENSIONS_CONFIG, CUR_DIR_SIZE - (end - State.extensionsePath) - 1);
    FILE *extensionsConfig = fopen(State.extensionsePath, "r");
    *end = '\0';
    if (extensionsConfig != NULL) {
        size_t size = 0;
        char *buf = NULL;
        size_t extInd = 0;
        while (extInd < EXTENSIONS_CNT && getline(&buf, &size, extensionsConfig) >= 0) {
            if (*buf == '#' || *buf == '\0' || *buf == '\n') {
                continue;
            }
            char *ptr1 = buf;
            while (ptr1 < buf + size && !isspace(*ptr1)) {
                ++ptr1;
            }
            strncpy(State.extensions[extInd].ext, buf, Min(ptr1 - buf, EXT_SO_NAME_LEN));
            State.extensions[extInd].ext[EXT_SO_NAME_LEN - 1] = '\0';

            while (ptr1 < buf + size && isspace(*ptr1)) {
                ++ptr1;
            }
            char *ptr2 = ptr1;

            while (ptr2 < buf + size && !isspace(*ptr2)) {
                ++ptr2;
            }
            strncpy(State.extensions[extInd].so, ptr1, Min(ptr2 - ptr1, EXT_SO_NAME_LEN));
            State.extensions[extInd].so[EXT_SO_NAME_LEN - 1] = '\0';
            ++extInd;
        }
        if (buf != NULL) {
            free(buf);
        }
    }
}

static void InitState() {
    State.fileCursor = State.fileIno = State.fileCursorScreenInd = State.colCursor = 0;
    State.sortDesc = State.showHidden = false;
    getcwd(State.curDir, CUR_DIR_SIZE - 1);
    State.curDir[CUR_DIR_SIZE - 1] = '\0';
    size_t len;
    #ifdef __APPLE__
    size_t bufsize = CUR_DIR_SIZE - 1;
    int res = _NSGetExecutablePath(State.extensionsePath, &bufsize);
    len = (res == 0 ? strlen(State.extensionsePath) : -1);
    char *ptr = State.extensionsePath + len;
    do {
        *(ptr--) = '\0';
    } while (*ptr != '/');
    #else
    len = readlink("/proc/self/exe", State.extensionsePath, CUR_DIR_SIZE - 1);
    #endif
    if (len < 0) {
        strncpy(State.extensionsePath, State.curDir, CUR_DIR_SIZE - 1);
        len = strnlen(State.extensionsePath, CUR_DIR_SIZE - 1);
    }
    len = strlen(State.extensionsePath);
    strncpy(State.extensionsePath + len, EXTENSIONS_DIRECTORY, CUR_DIR_SIZE - len - 1);
    State.extensionsePath[CUR_DIR_SIZE - 1] = '\0';
    FilesInit(&State.files);
    State.toCopy = NULL;
}

static int GetCh() {
    return getc(stdin);
}

static void Debug(const char *str) {
    Action(SET_CURSOR, H, 0);
    Print(NULL, NULL, str);
}

static void PrintHeader() {
    Action(SET_CURSOR, 1, 1);
    Action(ROW_BG_COLOR, BLUB);
    Print(BWHT, BLUB, ">>%-*s", W - 2, State.curDir);

    Action(SET_CURSOR, 2, 1);
    Action(ROW_BG_COLOR, BLUB);

    Action(TEXT_BG_COLOR, BLUB);
    int left_offset = 3;
    int right_offset = Max(LEFT_SIZE, W - RIGHT_SIZE) + 1;
    for (int col = 0; col < CNT_COLUMNS; ++col) {
        if (COLUMNS[col].width <= 0) {
            continue;
        }
        const char *color = col == State.colCursor ? BRED : BWHT;

        if (COLUMNS[col].align == ALIGN_LEFT) {
            Action(SET_CURSOR, 2, left_offset);
            Print(color, BLUB, "%-*s", COLUMNS[col].width, COLUMNS[col].name);
            left_offset += COLUMNS[col].width;
        } else {
            Action(SET_CURSOR, 2, right_offset);
            Print(color, BLUB, "%*s", COLUMNS[col].width, COLUMNS[col].name);
            right_offset += COLUMNS[col].width;
        }
    }
    Action(RESET_TEXT_COLOR);
}

static void PrintFooter() {
    Action(SET_CURSOR, H - 1, 1);
    Action(ROW_BG_COLOR, BLUB);
    Action(SET_CURSOR, H - 0, 1);
    Action(ROW_BG_COLOR, BLUB);

    Action(SET_CURSOR, H - 1, 1);
    Print(NULL, BLUB, "%.*s", W, "Q - quit     Arrow Up/Down - move cursor  S - sort order   D - delete  X - cut");
    Action(SET_CURSOR, H - 0, 1);
    Print(NULL, BLUB, "%.*s", W, "R - refresh  Arrow Left/Right - sort by   H - show hidden  C - copy    P - paste");
}

static bool SkipFile(File *file) {
    return (!State.showHidden && file->Name[0] == '.' && strcmp(file->Name, "..") != 0 || strcmp(file->Name, ".") == 0);
}

static void PrintFile(File *file, int line, char *buf, bool highlight) {
    bool needFree = false;
    if (buf == NULL) {
        buf = malloc(W);
        buf[W - 1] = '\0';
        needFree = true;
    }
    int left_offset = 3;
    int right_offset = Max(LEFT_SIZE, W - RIGHT_SIZE) + 1;
    for (int col = 0; col < CNT_COLUMNS; ++col) {
        COLUMNS[col].toString(file, buf, W);
        buf[COLUMNS[col].width] = '\0';
        char *textColor = highlight ? WHT : CYN;
        if (strcmp(COLUMNS[col].name, "Name") == 0) {
            textColor = ColorOfFile(file);
        }
        char *bgColor = highlight ? BLKB : NULL;
        if (COLUMNS[col].align == ALIGN_LEFT) {
            Action(SET_CURSOR, 3 + line, left_offset);
            Print(textColor, bgColor, "%-*s", COLUMNS[col].width, buf);
            left_offset += COLUMNS[col].width;
        } else {
            Action(SET_CURSOR, 3 + line, right_offset);
            Print(textColor, bgColor, "%*s", COLUMNS[col].width, buf);
            right_offset += COLUMNS[col].width;
        }
    }
    if (needFree) {
        free(buf);
    }
}

static void EraseCursor() {
    PrintFile(State.files.files + State.fileCursor, State.fileCursorScreenInd, NULL, false);
    Action(SET_CURSOR, 3 + State.fileCursorScreenInd, 1);
    Print(NULL, NULL, "  ");
}

static void PrintCursor() {
    PrintFile(State.files.files + State.fileCursor, State.fileCursorScreenInd, NULL, true);
    Action(SET_CURSOR, 3 + State.fileCursorScreenInd, 1);
    Print(NULL, BLKB, "->");
}

static void PrintFiles() {
    char *buf = malloc(W);
    buf[W - 1] = '\0';

    int fileEnd = State.fileCursor;
    int movedUp = 0;
    while (fileEnd < State.files.size - 1 && State.fileCursorScreenInd + movedUp < H - 5) {
        ++fileEnd;
        if (!SkipFile(State.files.files + fileEnd)) {
            ++movedUp;
        }
    }
    State.fileCursorScreenInd += (H - 5) - (State.fileCursorScreenInd + movedUp);

    int fileSt = State.fileCursor;
    int actualFileStart = fileSt;
    int movedDown = 0;
    while (fileSt > 0 && movedDown < State.fileCursorScreenInd) {
        --fileSt;
        if (!SkipFile(State.files.files + fileSt)) {
            ++movedDown;
            actualFileStart = fileSt;
        }
    }
    State.fileCursorScreenInd = movedDown;

    for (int fileInd = actualFileStart, drawed = 0; fileInd < State.files.size && drawed < H - 4; ++fileInd) {
        if (SkipFile(State.files.files + fileInd)) {
            continue;
        }

        PrintFile(State.files.files + fileInd, drawed, buf, fileInd == State.fileCursor);
        ++drawed;
    }
    free(buf);
}

static void Redraw() {
    Action(CLEAR_SCREEN);
    PrintHeader();
    PrintFiles();
    PrintFooter();
    PrintCursor();
}

static void AdjustFileCursor() {
    State.fileCursorScreenInd = Max(0, Min(State.fileCursorScreenInd, H - 5));
    State.fileCursor = Max(0, Min(State.fileCursor, State.files.size));
}

static void UpdateCursor()  {
    State.fileCursor = 0;
    for (size_t i = 0; i < State.files.size; ++i) {
        if (State.files.files[i].Meta.ino == State.fileIno) {
            State.fileCursor = i;
            break;
        }
    }
    while (SkipFile(State.files.files + State.fileCursor)) {
        ++State.fileCursor;
    }
    State.fileIno = State.files.files[State.fileCursor].Meta.ino;
}

static void UpdateSort() {
    SortFiles(&State.files, State.colCursor, State.sortDesc);
}

static void Refresh(bool resetCursor) {
    UpdateWinSize();
    AdjustFileCursor();
    UpdateColumnWidths(W);
    FetchFiles(&State.files, State.curDir, CUR_DIR_SIZE);
    UpdateSort();
    if (resetCursor) {
        UpdateCursor();
    } else {

    }
    Redraw();
}

static void MoveUp() {
    EraseCursor();
    int old = State.fileCursor;
    do {
        State.fileCursor = (State.fileCursor + State.files.size - 1) % State.files.size;
    } while (SkipFile(State.files.files + State.fileCursor));
    State.fileIno = State.files.files[State.fileCursor].Meta.ino;
    if (--State.fileCursorScreenInd <= 0) {
        State.fileCursorScreenInd = (H - 4) / 2;
        Redraw();
        return;
    }
    if (old <= State.fileCursor) {
        Redraw();
        return;
    }
    PrintCursor();
}

static void MoveDown() {
    EraseCursor();
    int old = State.fileCursor;
    do {
        State.fileCursor = (State.fileCursor + 1) % State.files.size;
    } while (SkipFile(State.files.files + State.fileCursor));
    State.fileIno = State.files.files[State.fileCursor].Meta.ino;
    if (++State.fileCursorScreenInd >= H - 4) {
        State.fileCursorScreenInd = (H - 4) / 2;
        Redraw();
        return;
    }
    if (old >= State.fileCursor) {
        Redraw();
        return;
    }
    
    PrintCursor();
}

static char *ConcatName(const char *name, bool endSlash) {
    File *to = State.files.files + State.fileCursor;
    char *end = State.curDir + strlen(State.curDir);
    char *old = end = EnsureEndSlash(State.curDir, end);
    if (name == NULL) {
        name = to->Name;
    }
    if (strcmp(name, "..") == 0) {
        --end;
        do {
            *(end--) = '\0';
        } while (*end != '/' && end > State.curDir);
    } else {
        end = stpncpy(end, name, CUR_DIR_SIZE - (end - State.curDir) - 1);
    }
    if (endSlash) {
        end = EnsureEndSlash(State.curDir, end);
    } else {
        end = EnsureNoEndSlash(State.curDir, end);
    }
    *end = '\0';
    return old;
}

static void EnterDir() {
    char *old = ConcatName(NULL, true);
    if (access(State.curDir, R_OK) != 0) {
        *old = '\0';
    }
}

static void DeleteFile() {
    char *old = ConcatName(NULL, false);
    if (remove(State.curDir) == 0) {
        if (State.fileCursor == State.files.size - 1) {
            State.fileIno = State.files.files[State.fileCursor - 1].Meta.ino;
        } else {
            State.fileIno = State.files.files[State.fileCursor + 1].Meta.ino;
        }
    }
    *old = '\0';
}

static void ProcessFile() {
    LoadExtensions();
    char *extension = GetExtension(State.curDir, State.curDir + strnlen(State.curDir, CUR_DIR_SIZE - 1));
    for (size_t i = 0; i < EXTENSIONS_CNT; ++i) {
        if (strcmp(State.extensions[i].ext, extension) == 0) {
            char *end = State.extensionsePath + strlen(State.extensionsePath);
            snprintf(end, CUR_DIR_SIZE - (end - State.extensionsePath) - 1, "/%s.so", State.extensions[i].so);

            void *handle = dlopen(State.extensionsePath, RTLD_LAZY);
            *end = '\0';
            if (handle == NULL) {
                continue;
            }

            void *foo = dlsym(handle, EXTENSION_FOO_NAME);
            if (foo == NULL) {
                continue;
            }

            ((void(*)(const char *))foo)(State.curDir);

            dlclose(handle);
        }
    }
}

static void ProcessEnter() {
    File *curFile = State.files.files + State.fileCursor;
    if (curFile->Meta.type == DT_DIR) {
        EnterDir();
        Refresh(true);
    } else {
        Action(CLEAR_SCREEN);
        ExitAltMode();
        EnableCursor();
        char *old = ConcatName(NULL, false);
        ProcessFile();
        *old = '\0';
        DisableCursor();
        EnterAltMode();
        Refresh(false);
    }
}

static void StartCopy(bool cut) {
    if (cut && access(State.curDir, W_OK | X_OK) != 0) {
        return;
    }
    char *old = ConcatName(NULL, false);
    if (State.toCopy != NULL) {
        fclose(State.toCopy);
    }
    State.toCopy = fopen(State.curDir, "r");
    strncpy(State.copiedName, old, SizeOfField(struct State, copiedName));
    State.cutPath[0] = '\0';
    if (cut) {
        strncpy(State.cutPath, State.curDir, CUR_DIR_SIZE);
    }
    *old = '\0';
}

static void Paste(bool cut) {
    if (State.toCopy == NULL) {
        return;
    }

    char *old = ConcatName(State.copiedName, false);
    if (access(State.curDir, R_OK) == 0) {
        char *end = State.curDir + strnlen(State.curDir, CUR_DIR_SIZE - 1);
        int version = 0;
        do {
            ++version;
            *end = '\0';
            char numBuf[18];
            snprintf(numBuf, sizeof(numBuf), "%d", version);
            strncpy(end, numBuf, sizeof(numBuf));
        } while (access(State.curDir, R_OK) == 0);
    }

    FILE *toPaste = fopen(State.curDir, "w");
    if (toPaste == NULL) {
        *old = '\0';
        return;
    }
    char copyBuffer[COPY_BUFFER_SIZE];
    int cntRead = 0;
    while ((cntRead = fread(copyBuffer, 1, COPY_BUFFER_SIZE, State.toCopy)) > 0) {
        fwrite(copyBuffer, 1, cntRead, toPaste);
    }
    fclose(toPaste);
    if (cut) {
        remove(State.cutPath);
        State.cutPath[0] = '\0';
    }
    *old ='\0';
}

enum Keys {
    ARROW_UP    = 31337 + 0,
    ARROW_DOWN  = 31337 + 1,
    ARROW_LEFT  = 31337 + 2,
    ARROW_RIGHT = 31337 + 3,
    ENTER1 = 0x0a,
    ENTER2 = 0x0d,
};

static int ProcessKey() {
    int key = GetCh();

    if (key == '\e') {
        int a = GetCh();
        if (a == '[') {
            a = GetCh();
            switch (a) {
                case 'A': key = ARROW_UP; break;
                case 'B': key = ARROW_DOWN; break;
                case 'C': key = ARROW_RIGHT; break;
                case 'D': key = ARROW_LEFT; break;
            }
        }
    }

    switch (key) {

    case 'r':
    case 'R': {
        Refresh(true);
        break;
    }

    case 'q':
    case 'Q': {
        return 1;
    }

    case 's':
    case 'S': {
        State.sortDesc ^= true;
        UpdateSort();
        UpdateCursor();
        Redraw();
        break;
    }

    case 'h':
    case 'H': {
        State.showHidden ^= true;
        Redraw();
        break;
    }

    case 'd':
    case 'D': {
        DeleteFile();
        Refresh(false);
        break;
    }

    case 'c':
    case 'C': {
        StartCopy(false);
        break;
    }

    case 'p':
    case 'P': {
        Paste(*State.cutPath != '\0');
        Refresh(false);
        break;
    }

    case 'x':
    case 'X': {
        StartCopy(true);
        break;
    }

    case ARROW_UP: {
        MoveUp();
        break;
    }

    case ARROW_DOWN: {
        MoveDown();
        break;
    }

    case ARROW_LEFT: {
        do {
            State.colCursor = (State.colCursor + CNT_COLUMNS - 1) % CNT_COLUMNS;
        } while (COLUMNS[State.colCursor].width <= 0);
        State.sortDesc = false;
        UpdateSort();
        UpdateCursor();
        Redraw();
        break;
    }

    case ARROW_RIGHT: {
        do {
            State.colCursor = (State.colCursor + 1) % CNT_COLUMNS;
        } while (COLUMNS[State.colCursor].width <= 0);
        State.sortDesc = false;
        UpdateSort();
        UpdateCursor();
        Redraw();
        break;
    }

    case ENTER1:
    case ENTER2: {
        ProcessEnter();
        break;
    }

    default: {
        break;
    }

    }

    return 0;
}

static int InfinitePolling() {
    Refresh(true);
    while (!ProcessKey()) {
        // spin...
    }
    return 0;
}

int FMStart() {
    SetTerminal(XTERM);
    EnterRawMode();

    atexit(ExitAltMode);
    EnterAltMode();

    atexit(EnableCursor);
    DisableCursor();

    InitState();

    int res = InfinitePolling();

    FilesDestroy(&State.files);

    Action(EXIT_ALT_MODE);

    return res;
}
