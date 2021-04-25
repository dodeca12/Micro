#ifndef MICRO_H_
#define MICRO_H_



#define CTRL_KEY(k) ((k)&0x1f)
#define APPEND_BUFFER_INIT {NULL, 0}
#define MICRO_VERSION "0.0.1"

struct editorConfig microConfig;

typedef struct microRow
{
    char *chars;
    int size;
} microRow;

struct editorConfig
{
    struct termios original_termios;
    int screenRows, screenCols;
    int cursorPosX, cursorPosY;
    microRow *row;
    int numRows;
};

struct appendBuffer
{
    char *b;
    int len;
};

enum microKeyMap
{
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
void microAppendRow(char *s, size_t len);

#endif // MICRO_H_

