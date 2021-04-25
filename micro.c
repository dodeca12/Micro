#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>

#include "micro.h"

void die(const char *s)
{
    // Clears screen on exit
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawInputMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &microConfig.original_termios) == -1)
        die("tcsetattr");
}

void enableRawInputMode()
{
    if (tcgetattr(STDIN_FILENO, &microConfig.original_termios) == -1)
        die("tcsetattr");

    atexit(disableRawInputMode);

    // c_lflag = field for "local flags"
    // c_oflag = field for "output flags"'
    // c_iflag = field for "input flags"
    // c_cflag = field for "control flags"

    // BRKINT = Signal interrupt on break
    // ICRNL = Map CR to NL on input
    // INPCK = Enable input parity check
    // ISTIRP = Strip 8th bit off characters
    // IXON = Enable start/stop output control
    // OPOST = Post-process output
    // CS8 = Sets the character size (CS) to 8 bits per byte
    // ECHO = Enable echo
    // ICANON = Canonical input (erase and kill processing)
    // ISIG = Enable signals
    // IEXTEN = Enable non-POSIX special characters

    struct termios raw = microConfig.original_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Turning the above flags off are not necessary for modern usage, but it is done to keep with the tradition when enabling "raw mode"
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    // c_cc; where cc stands for "control characters"
    // VMIN, VTIME indices into c_cc field
    raw.c_cc[VMIN] = 0;  // VMIN value sets the min number of bytes of input before read() returns
    raw.c_cc[VTIME] = 1; // VTIME value sets max value of time (in 1/10 sec) before read() returns

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

int getTerminalWindowSize(int *rows, int *cols)
{
    struct winsize wsize;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize) == -1 || wsize.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        // microReadKey();
        return getCursorPosition(rows, cols);
    }
    else
    {
        *rows = wsize.ws_row;
        *cols = wsize.ws_col;
        return 0;
    }
}

int microReadKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    if (c == '\x1b')
    {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[')
        {
            if (seq[1] >= '0' && seq[1] <= '9')
            {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~')
                {
                    switch (seq[1])
                    {
                    case '1':
                    case '7':
                        return HOME_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    case '4':
                    case '8':
                        return END_KEY;
                    case '3':
                        return DELETE_KEY;
                    }
                }
            }
            else
            {
                switch (seq[1])
                {
                case 'A':
                    return UP_ARROW;
                case 'B':
                    return DOWN_ARROW;
                case 'C':
                    return RIGHT_ARROW;
                case 'D':
                    return LEFT_ARROW;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        }
        else if (seq[0] == 'O')
        {
            switch (seq[1])
            {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }

        return '\x1b';
    }
    else
    {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols)
{

    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    // Commented code below not required
    // printf("\r\n");
    // char c;
    // while(read(STDIN_FILENO, &c, 1) == 1)
    // {
    //     if(iscntrl(c))
    //     {
    //         printf("%d\r\n", c);
    //     }
    //     else
    //     {
    //         printf("%d ('%c')\r\n", c, c);
    //     }
    // }

    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        ++i;
    }
    buf[i] = '\0';

    // printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);
    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;

    // microReadKey();

    return 0;
}

void microProcessKeypress()
{
    int c = microReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):

        // Clears screen on exit
        write(STDIN_FILENO, "\x1b[2J", 4);
        write(STDIN_FILENO, "\x1b[H", 3);

        exit(0);
        break;

    case LEFT_ARROW:
    case RIGHT_ARROW:
    case UP_ARROW:
    case DOWN_ARROW:
        microMoveCursor(c);
        break;

    case PAGE_UP:
    case PAGE_DOWN:
    {
        int times = microConfig.screenRows;
        while (--times)
        {
            microMoveCursor(c == PAGE_UP ? UP_ARROW : DOWN_ARROW);
        }
        break;
    }

    case HOME_KEY:
        microConfig.cursorPosX = 0;
        break;
    case END_KEY:
        microConfig.cursorPosX = microConfig.screenCols - 1;
        break;
    }
}

void microRefreshScreen()
{
    // 4 = writing 4 bytes
    // first byte = \x1b = escape character (27 ASCII decimal)
    // writing an escape sequence to the terminal; escape sequences
    // start with the escape character (27), along with '['
    // J stands for the "J command" aka the Erase In Display
    // found in the VT100 (https://vt100.net/docs/vt100-ug/chapter3.html#ED)
    struct appendBuffer ab = APPEND_BUFFER_INIT;

    appendBufferAppend(&ab, "\x1b[?25l", 6);
    // appendBufferAppend(&ab, "\x1b[2J", 4);
    appendBufferAppend(&ab, "\x1b[H", 3);

    microDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", microConfig.cursorPosY + 1, microConfig.cursorPosX + 1);
    appendBufferAppend(&ab, buf, strlen(buf));

    appendBufferAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);

    appendBufferFree(&ab);
}

void microDrawRows(struct appendBuffer *ab)
{

    for (int r = 0; r < microConfig.screenRows; ++r)
    {
        if (r >= microConfig.numRows)
        {
            if (microConfig.numRows == 0 && r == microConfig.screenRows / 3)
            {
                char welcome[80];
                int welcomeLen = snprintf(welcome, sizeof(welcome),
                                          "Micro Editor -- Version %s", MICRO_VERSION);
                if (welcomeLen > microConfig.screenCols)
                    welcomeLen = microConfig.screenCols;
                int padding = (microConfig.screenCols - welcomeLen) / 2;
                if (padding)
                {
                    appendBufferAppend(ab, "~", 1);
                    --padding;
                }
                while (--padding)
                    appendBufferAppend(ab, " ", 1);

                appendBufferAppend(ab, welcome, welcomeLen);
            }
            else
            {
                appendBufferAppend(ab, "~", 1);
            }
        }
        else
        {
            int len = microConfig.row[r].size;
            if (len > microConfig.screenCols)
                len = microConfig.screenCols;
            appendBufferAppend(ab, microConfig.row[r].chars, len);
        }

        appendBufferAppend(ab, "\x1b[K", 3);

        if (r < microConfig.screenRows - 1)
            appendBufferAppend(ab, "\r\n", 2);
    }
}

void initializeMicro()
{
    microConfig.cursorPosX = microConfig.cursorPosY = microConfig.numRows = 0;
    microConfig.row = NULL;

    if (getTerminalWindowSize(&microConfig.screenRows, &microConfig.screenCols) == -1)
        die("getTerminalWindowSize");
}

void appendBufferAppend(struct appendBuffer *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL)
        return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void appendBufferFree(struct appendBuffer *ab)
{
    free(ab->b);
}

void microMoveCursor(int key)
{
    switch (key)
    {
    case LEFT_ARROW:
        if (microConfig.cursorPosX != 0)
        {
            --(microConfig.cursorPosX);
        }
        break;
    case RIGHT_ARROW:
        if (microConfig.cursorPosX != microConfig.screenCols - 1)
        {
            ++(microConfig.cursorPosX);
        }
        break;
    case UP_ARROW:
        if (microConfig.cursorPosY != 0)
        {
            --(microConfig.cursorPosY);
        }
        break;
    case DOWN_ARROW:
        if (microConfig.cursorPosY != microConfig.screenRows - 1)
        {
            ++(microConfig.cursorPosY);
        }
        break;
    }
}

void microAppendRow(char *s, size_t len)
{
    // microConfig.row.size = len;
    // microConfig.row.chars = malloc(len + 1);
    // memcpy(microConfig.row.chars, s, len);
    // microConfig.row.chars[len] = '\0';
    // microConfig.numRows = 1;
    microConfig.row = realloc(microConfig.row, sizeof(microRow) * (microConfig.numRows + 1));
    int at = microConfig.numRows;
    microConfig.row[at].size = len;
    microConfig.row[at].chars = malloc(len + 1);
    memcpy(microConfig.row[at].chars, s, len);
    microConfig.row[at].chars[len] = '\0';
    ++(microConfig.numRows);
}

void microOpen(char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        die("fopen");

    char *line = NULL;
    size_t lineCapacity = 0;
    ssize_t lineLen;
    while ((lineLen = getline(&line, &lineCapacity, fp)) != -1)
    {

        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r'))
            --lineLen;

        // microConfig.row.size = lineLen;
        // microConfig.row.chars = malloc(lineLen + 1);
        // memcpy(microConfig.row.chars, line, lineLen);
        // microConfig.row.chars[lineLen] = '\0';
        // microConfig.numRows = 1;
        microAppendRow(line, lineLen);
    }
    free(line);
    fclose(fp);
}

int main(int argc, char *argv[])
{
    enableRawInputMode();
    initializeMicro();

    if (argc >= 2)
    {
        microOpen(argv[1]);
    }
    while (1)
    {
        microRefreshScreen();
        microProcessKeypress();
    }
    return 0;
}