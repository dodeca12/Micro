#ifndef MICRO_H_
#define MICRO_H_

#define CTRL_KEY(k) ((k)&0x1f)
#define APPEND_BUFFER_INIT {NULL, 0}

struct editorConfig
{
    struct termios original_termios;
    int screenRows;
    int screenCols;
};

struct appendBuffer
{
    char *b;
    int len;
};

struct editorConfig microConfig;

void die(const char *s);
void disableRawInputMode();
void enableRawInputMode();
char microReadKey();
void microProcessKeypress();
void microRefreshScreen();
void microDrawRows();
int getTerminalWindowSize(int *rows, int *cols);
void initializeMicro();
int getCursorPosition(int *rows, int *cols);
void appendBufferAppend(struct appendBuffer *ab, const char *s, int len);
void appendBufferFree(struct appendBuffer *ab);

#endif // MICRO_H_