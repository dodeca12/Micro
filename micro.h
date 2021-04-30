#ifndef MICRO_H_
#define MICRO_H_

#define CTRL_KEY(k) ((k)&0x1f)
#define APPEND_BUFFER_INIT \
    {                      \
        NULL, 0            \
    }
#define MICRO_VERSION "0.0.1"
#define MICRO_TAB_STOP 8
#define MICRO_FORCE_QUIT 1

typedef struct microRow
{
    char *chars;
    int size;
    int rsize;
    char *render;
} microRow;

struct editorConfig
{
    struct termios original_termios;
    int screenRows, screenCols;
    int cursorPosX, cursorPosY;
    int renderPosX;
    microRow *row;
    char *fileName;
    int numRows;
    int rowOffset, colOffset;
    char statusMessage[80];
    time_t statusMessage_time;
    int dirtyBuffer;
} microConfig;

struct appendBuffer
{
    char *b;
    int len;
};

enum microKeyMap
{
    BACKSPACE = 127,
    LEFT_ARROW = 121212,
    RIGHT_ARROW,
    UP_ARROW,
    DOWN_ARROW,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DELETE_KEY
};

void die(const char *s);
void disableRawInputMode();
void enableRawInputMode();
int microReadKey();
void microProcessKeypress();
void microRefreshScreen();
void microDrawRows();
int getTerminalWindowSize(int *rows, int *cols);
void initializeMicro();
int getCursorPosition(int *rows, int *cols);
void appendBufferAppend(struct appendBuffer *ab, const char *s, int len);
void appendBufferFree(struct appendBuffer *ab);
void microMoveCursor(int key);
void microOpen(char *filename);
// void microAppendRow(char *s, size_t len);
void microInsertRow(int at, char *s, size_t len);
void microScroll();
void microUpdateRow(microRow *row);
int microRowCursorPosXToRenderPosX(microRow *row, int cursorPosX);
void microDrawStatusBar(struct appendBuffer *ab);
void microSetStatusMessage(const char *fmt, ...);
void microDrawMessageBar(struct appendBuffer *ab);
void microRowInsertCharacter(microRow *row, int at, int c);
void microInsertCharacter(int c);
char *microRowsToString(int *bufferLen);
void microSave();
void microRowDeleteCharacter(microRow *row, int at);
void microDeleteCharacter();
void microFreeRow(microRow *row);
void microDeleteRow(int at);
void microRowAppendString(microRow *row, char *s, size_t len);
void microInsertNewLine();
char *microPrompt(char *prompt, void (*callback)(char *, int));
void microSearch();
int microRowRenderPosXtoCursorPosX(microRow *row, int renderPosX);
void microSearchCallback(char *query, int key);

#endif // MICRO_H_
