#ifndef MICRO_H_
#define MICRO_H_

#define CTRL_KEY(k) ((k)&0x1f)

struct editorConfig
{
    struct termios original_termios;
    int screenRows;
    int screenCols;
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

#endif // MICRO_H_