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
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

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
    // raw.c_lflag &= ~(XOFF);

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
    static int forceQuitTimes = MICRO_FORCE_QUIT;

    int c = microReadKey();

    switch (c)
    {

    case '\r':
        microInsertNewLine();
        break;

    case CTRL_KEY('q'):

        if (microConfig.dirtyBuffer && forceQuitTimes > 0)
        {
            microSetStatusMessage("ALERT! Current file has unsaved changes. ", "Press Ctrl-q again to force quit", forceQuitTimes);
            --forceQuitTimes;
            return;
        }

        // Clears screen on exit
        write(STDIN_FILENO, "\x1b[2J", 4);
        write(STDIN_FILENO, "\x1b[H", 3);

        exit(0);
        break;

    case CTRL_KEY('s'):
        microSave();
        break;

    case CTRL_KEY('f'):
        microSearch();
        break;

    case LEFT_ARROW:
    case RIGHT_ARROW:
    case UP_ARROW:
    case DOWN_ARROW:
        microMoveCursor(c);
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DELETE_KEY:
        if (c == DELETE_KEY)
            microMoveCursor(RIGHT_ARROW);
        microDeleteCharacter();
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

    case CTRL_KEY('l'):
    case '\x1b':
        break;

    default:
        microInsertCharacter(c);
        break;
    }

    forceQuitTimes = MICRO_FORCE_QUIT;
}

void microFreeRow(microRow *row)
{
    free(row->render);
    free(row->chars);
    free(row->highlight);
}

void microDeleteRow(int at)
{
    if (at < 0 || at > microConfig.numRows)
        return;

    microFreeRow(&microConfig.row[at]);
    memmove(&microConfig.row[at], &microConfig.row[at + 1], sizeof(microRow) * (microConfig.numRows - at - 1));
    --(microConfig.numRows);
    ++(microConfig.dirtyBuffer);
}

void microRowAppendString(microRow *row, char *s, size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    microUpdateRow(row);
    ++(microConfig.dirtyBuffer);
}

void microUpdateSyntax(microRow *row)
{
    row->highlight = realloc(row->highlight, row->rsize);
    memset(row->highlight, HL_DEFAULT, row->rsize);

    if (microConfig.syntax == NULL)
        return;

    char **keywords = microConfig.syntax->keywords;

    char *slcs = microConfig.syntax->singleLineCommentStart;
    int slcsLen = slcs ? strlen(slcs) : 0;

    int previousSeperator = 1;
    int inString = 0;
    int i = 0;
    while (i < row->rsize)
    {
        char c = row->render[i];
        unsigned char previousHighlight = (i > 0) ? row->highlight[i - 1] : HL_DEFAULT;

        if(slcsLen && !inString)
        {
            if(!strncmp(&row->render[i], slcs, slcsLen))
            {
                memset(&row->highlight[i], HL_COMMENT, row->size - i);
                break;
            }
        }

        if (microConfig.syntax->flags & HL_HIGHLIGHT_STRINGS)
        {
            if (c == '\\' && i + 1 < row->rsize)
            {
                row->highlight[i + 1] = HL_STRING;
                i += 2;
                continue;
            }

            if (inString)
            {
                row->highlight[i] = HL_STRING;
                if (c == inString)
                    inString = 0;
                i++;
                previousSeperator = 1;
                continue;
            }
            else
            {
                if (c == '"' || c == '\'')
                {
                    inString = c;
                    row->highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (microConfig.syntax->flags & HL_HIGLIGHT_NUMBERS)
        {
            if ((isdigit(c) && (previousSeperator || previousHighlight == HL_NUMBER)) ||
                (c == '.' && previousHighlight == HL_NUMBER))
            {
                row->highlight[i] = HL_NUMBER;
                i++;
                previousSeperator = 0;
                continue;
            }

        }

        if(previousSeperator)
        {
            for (int j = 0; keywords[j]; j++)
            {
                int keywordLen = strlen(keywords[j]);
                int keyword2 = keywords[j][keywordLen - 1] == '|';
                if(keyword2)
                    keywordLen--;
                
                if(!strncmp(&row->render[i], keywords[j], keywordLen) && hasSeperator(row->render[i+keywordLen]))
                {
                    memset(&row->highlight[i], keyword2 ? HL_KEYWORD2 : HL_KEYWORD1, keywordLen);
                    i += keywordLen;
                    break;
                }
                if(keywords[j] != NULL)
                {
                    previousSeperator = 0;
                    continue;
                }
            }
        }

        previousSeperator = hasSeperator(c);
        i++;
        
    }
}

int hasSeperator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

char *microRowsToString(int *bufferLen)
{
    int totalLen = 0;
    for (int i = 0; i < microConfig.numRows; i++)
    {
        totalLen += microConfig.row[i].size + 1;
    }
    *bufferLen = totalLen;

    char *buffer = malloc(totalLen);
    char *paragraph = buffer;
    for (int i = 0; i < microConfig.numRows; i++)
    {
        memcpy(paragraph, microConfig.row[i].chars, microConfig.row[i].size);
        paragraph += microConfig.row[i].size;
        *paragraph = '\n';
        paragraph++;
    }
    return buffer;
}

void microRefreshScreen()
{

    microScroll();

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
    microDrawStatusBar(&ab);
    microDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (microConfig.cursorPosY - microConfig.rowOffset) + 1, (microConfig.renderPosX - microConfig.colOffset) + 1);
    appendBufferAppend(&ab, buf, strlen(buf));

    appendBufferAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);

    appendBufferFree(&ab);
}

void microDrawRows(struct appendBuffer *ab)
{

    for (int r = 0; r < microConfig.screenRows; ++r)
    {
        int fileRow = r + microConfig.rowOffset;
        if (fileRow >= microConfig.numRows)
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
                while (padding--)
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
            int len = microConfig.row[fileRow].rsize - microConfig.colOffset;
            if (len < 0)
                len = 0;
            if (len > microConfig.screenCols)
                len = microConfig.screenCols;

            char *c = &microConfig.row[fileRow].render[microConfig.colOffset];
            unsigned char *highlight = &microConfig.row[fileRow].highlight[microConfig.colOffset];
            int currentColour = -1;
            for (int i = 0; i < len; i++)
            {
                if (iscntrl(c[i]))
                {
                    char sym = (c[i] <= 26) ? '@' + c[i] : '?';
                    appendBufferAppend(ab, "\x1b[7m", 4);
                    appendBufferAppend(ab, &sym, 1);
                    appendBufferAppend(ab, "\x1b[m,", 3);
                    if (currentColour != -1)
                    {
                        char buffer[16];
                        int colourLen = snprintf(buffer, sizeof(buffer), "\x1b[%dm", currentColour);
                        appendBufferAppend(ab, buffer, colourLen);
                    }
                }
                else if (highlight[i] == HL_DEFAULT)
                {
                    if (currentColour != -1)
                    {
                        appendBufferAppend(ab, "\x1b[39m", 5);
                        currentColour = -1;
                    }
                    // appendBufferAppend(ab, "\x1b[39m", 5);

                    // appendBufferAppend(ab, "\x1b[31m", 5);
                    appendBufferAppend(ab, &c[i], 1);
                }
                else
                {
                    int colour = microSyntaxToColour(highlight[i]);
                    if (colour != currentColour)
                    {
                        currentColour = colour;
                        char buffer[16];
                        int colourLen = snprintf(buffer, sizeof(buffer), "\x1b[%dm", colour);
                        appendBufferAppend(ab, buffer, colourLen);
                    }

                    appendBufferAppend(ab, &c[i], 1);
                }
            }
            appendBufferAppend(ab, "\x1b[39m", 5);

            // appendBufferAppend(ab, &microConfig.row[fileRow].render[microConfig.colOffset], len);
        }

        appendBufferAppend(ab, "\x1b[K", 3);

        // if (r < microConfig.screenRows - 1)
        appendBufferAppend(ab, "\r\n", 2);
    }
}

void initializeMicro()
{
    microConfig.cursorPosX = microConfig.cursorPosY = microConfig.numRows = microConfig.rowOffset = microConfig.colOffset = microConfig.renderPosX = microConfig.dirtyBuffer = 0;
    microConfig.row = NULL;
    microConfig.fileName = NULL;

    microConfig.statusMessage[0] = '\0';
    microConfig.statusMessage_time = 0;

    if (getTerminalWindowSize(&microConfig.screenRows, &microConfig.screenCols) == -1)
        die("getTerminalWindowSize");

    microConfig.screenRows -= 2;
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
    microRow *row = (microConfig.cursorPosY >= microConfig.numRows) ? NULL : &microConfig.row[microConfig.cursorPosY];
    switch (key)
    {
    case LEFT_ARROW:
        if (microConfig.cursorPosX != 0)
        {
            (microConfig.cursorPosX)--;
        }
        else if (microConfig.cursorPosY > 0)
        {
            (microConfig.cursorPosY)--;
            microConfig.cursorPosX = microConfig.row[microConfig.cursorPosY].size;
        }
        break;
    case RIGHT_ARROW:
        if (row && microConfig.cursorPosX < row->size)
        {
            (microConfig.cursorPosX)++;
        }
        else if (row && microConfig.cursorPosX == row->size)
        {
            (microConfig.cursorPosY)++;
            microConfig.cursorPosX = 0;
        }
        break;
    case UP_ARROW:
        if (microConfig.cursorPosY != 0)
        {
            (microConfig.cursorPosY)--;
        }
        break;
    case DOWN_ARROW:
        if (microConfig.cursorPosY < microConfig.numRows)
        {
            (microConfig.cursorPosY)++;
        }
        break;
    }

    row = (microConfig.cursorPosY >= microConfig.numRows) ? NULL : &microConfig.row[microConfig.cursorPosY];
    int rowLen = row ? row->size : 0;
    if (microConfig.cursorPosX > rowLen)
    {
        microConfig.cursorPosX = rowLen;
    }
}

void microSave()
{
    if (microConfig.fileName == NULL)
    {
        microConfig.fileName = microPrompt("Save file as: %s (ESC to cancel save)", NULL);
        if (microConfig.fileName == NULL)
        {
            microSetStatusMessage("Save cancelled");
            return;
        }
        microSelectSyntaxHighlight();
    }

    int len;
    char *buffer = microRowsToString(&len);

    int fd = open(microConfig.fileName, O_RDWR | O_CREAT, 0644);

    if (fd != -1)
    {
        if (ftruncate(fd, len) != -1)
        {
            if (write(fd, buffer, len) == len)
            {
                close(fd);
                free(buffer);
                microConfig.dirtyBuffer = 0;
                microSetStatusMessage("%d bytes written", len);
                return;
            }
        }
        close(fd);
    }

    free(buffer);
    microSetStatusMessage("Micro cannot save file! I/O Error: %s", strerror(errno));
}

void microScroll()
{
    microConfig.renderPosX = microConfig.cursorPosX;

    if (microConfig.cursorPosY < microConfig.numRows)
        microConfig.renderPosX = microRowCursorPosXToRenderPosX(&microConfig.row[microConfig.cursorPosY], microConfig.cursorPosX);

    if (microConfig.cursorPosY < microConfig.rowOffset)
    {
        microConfig.rowOffset = microConfig.cursorPosY;
    }
    if (microConfig.cursorPosY >= microConfig.rowOffset + microConfig.screenRows)
    {
        microConfig.rowOffset = microConfig.cursorPosY - microConfig.screenRows + 1;
    }
    if (microConfig.renderPosX < microConfig.colOffset)
    {
        microConfig.colOffset = microConfig.renderPosX;
    }
    if (microConfig.renderPosX >= microConfig.colOffset + microConfig.screenCols)
    {
        microConfig.colOffset = microConfig.renderPosX - microConfig.screenCols + 1;
    }
}

void microRowDeleteCharacter(microRow *row, int at)
{
    if (at >= row->size || at < 0)
        return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    --(row->size);
    microUpdateRow(row);
    ++(microConfig.dirtyBuffer);
}

void microDeleteCharacter()
{
    if (microConfig.cursorPosY == microConfig.numRows)
        return;

    microRow *row = &microConfig.row[microConfig.cursorPosY];

    if (microConfig.cursorPosX > 0)
    {
        microRowDeleteCharacter(row, microConfig.cursorPosX - 1);
        --(microConfig.cursorPosX);
    }
    else
    {
        microConfig.cursorPosX = microConfig.row[microConfig.cursorPosY - 1].size;
        microRowAppendString(&microConfig.row[microConfig.cursorPosY - 1], row->chars, row->size);
        microDeleteRow(microConfig.cursorPosY);
        --(microConfig.cursorPosY);
    }
}

void microInsertRow(int at, char *s, size_t len)
{

    if (at < 0 || at > microConfig.numRows)
        return;

    microConfig.row = realloc(microConfig.row, sizeof(microRow) * (microConfig.numRows + 1));

    memmove(&microConfig.row[at + 1], &microConfig.row[at], sizeof(microRow) * (microConfig.numRows - at));

    microConfig.row = realloc(microConfig.row, sizeof(microRow) * (microConfig.numRows + 1));

    // int at = microConfig.numRows;
    microConfig.row[at].size = len;
    microConfig.row[at].chars = malloc(len + 1);
    memcpy(microConfig.row[at].chars, s, len);
    microConfig.row[at].chars[len] = '\0';

    microConfig.row[at].rsize = 0;
    microConfig.row[at].render = NULL;
    microConfig.row[at].highlight = NULL;

    microUpdateRow(&microConfig.row[at]);

    (microConfig.numRows)++;
    ++(microConfig.dirtyBuffer);
}

char *microPrompt(char *prompt, void (*callback)(char *, int))
{
    size_t bufferSize = 128;
    char *buffer = malloc(bufferSize);

    size_t bufferLen = 0;
    buffer[0] = '\0';

    while (1)
    {
        microSetStatusMessage(prompt, buffer);
        microRefreshScreen();

        int c = microReadKey();

        if (c == BACKSPACE || c == DELETE_KEY || c == CTRL_KEY('h'))
        {
            if (bufferLen != 0)
                buffer[--bufferLen] = '\0';
        }
        else if (c == '\x1b')
        {
            microSetStatusMessage("");
            if (callback)
                callback(buffer, c);
            free(buffer);
            return NULL;
        }
        else if (c == '\r')
        {
            if (bufferLen != 0)
            {
                microSetStatusMessage("");
                if (callback)
                    callback(buffer, c);
                return buffer;
            }
        }
        else if (!iscntrl(c) && c < 128)
        {
            if (bufferLen == bufferSize - 1)
            {
                bufferSize *= 2;
                buffer = realloc(buffer, bufferSize);
            }
            buffer[bufferLen++] = c;
            buffer[bufferLen] = '\0';
        }
        if (callback)
            callback(buffer, c);
    }
}

void microSearch()
{
    int savedCursorPosX, savedCursorPosY, savedColOffset, savedRowOffset;
    savedCursorPosX = microConfig.cursorPosX;
    savedCursorPosY = microConfig.cursorPosY;
    savedColOffset = microConfig.colOffset;
    savedRowOffset = microConfig.rowOffset;

    char *query = microPrompt("Search: %s (ESC to cancel/Arrow keys to move/Enter to search)", microSearchCallback);
    if (query)
    {
        free(query);
        microConfig.cursorPosX = savedCursorPosX;
        microConfig.cursorPosY = savedCursorPosY;
        microConfig.colOffset = savedColOffset;
        microConfig.rowOffset = savedRowOffset;
    }
}

void microSearchCallback(char *query, int key)
{

    static int previousMatch = -1;
    static int direction = 1;

    static int savedHighlightLine;
    static char *savedHighlight = NULL;

    if (savedHighlight)
    {
        memcpy(microConfig.row[savedHighlightLine].highlight, savedHighlight, microConfig.row[savedHighlightLine].rsize);
        free(savedHighlight);
        savedHighlight = NULL;
    }

    if (key == '\r' || key == '\x1b')
    {
        previousMatch = -1;
        direction = 1;
        return;
    }
    else if (key == RIGHT_ARROW || key == DOWN_ARROW)
    {
        direction = 1;
    }
    else if (key == LEFT_ARROW || key == UP_ARROW)
    {
        direction = -1;
    }
    else
    {
        previousMatch = -1;
        direction = 1;
    }

    if (previousMatch == -1)
        direction = 1;

    int current = previousMatch;

    for (int i = 0; i < microConfig.numRows; i++)
    {
        current += direction;
        if (current == -1)
            current = microConfig.numRows - 1;
        else if (current == microConfig.numRows)
            current = 0;

        microRow *row = &microConfig.row[current];
        // row = &microConfig.row[i];
        char *match = strstr(row->render, query);
        if (match)
        {
            previousMatch = current;
            microConfig.cursorPosY = current;
            // microConfig.cursorPosY = i;
            microConfig.cursorPosX = microRowRenderPosXtoCursorPosX(row, match - row->render);
            microConfig.rowOffset = microConfig.numRows;

            savedHighlightLine = current;
            savedHighlight = malloc(row->size);
            memcpy(savedHighlight, row->highlight, row->size);
            memset(&row->highlight[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

int microRowRenderPosXtoCursorPosX(microRow *row, int renderPosX)
{
    int currentRenderPosX = 0;
    int cursorPosX;
    for (cursorPosX = 0; cursorPosX < row->size; ++cursorPosX)
    {
        if (row->chars[cursorPosX] == '\t')
            currentRenderPosX += (MICRO_TAB_STOP - 1) - (currentRenderPosX % MICRO_TAB_STOP);

        currentRenderPosX++;

        if (currentRenderPosX > renderPosX)
            return cursorPosX;
    }
    return cursorPosX;
}

void microUpdateRow(microRow *row)
{
    int tabs = 0;
    for (int i = 0; i < row->size; i++)
    {
        if (row->chars[i] == '\t')
            ++tabs;
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (MICRO_TAB_STOP - 1) + 1);

    int idx = 0;
    for (int i = 0; i < row->size; ++i)
    {
        if (row->chars[i] == '\t')
        {
            row->render[idx++] = ' ';

            while (idx % MICRO_TAB_STOP != 0)
                row->render[idx++] = ' ';

            // row->render[idx++] = row->chars[i];
        }
        else
        {
            row->render[idx++] = row->chars[i];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;

    microUpdateSyntax(row);
}

int microSyntaxToColour(int highlight)
{
    switch (highlight)
    {
    case HL_NUMBER:
        return 31;
    case HL_STRING:
        return 35;
    case HL_MATCH:
        return 34;
    case HL_COMMENT:
        return 36;
    case HL_KEYWORD1:
        return 33;
    case HL_KEYWORD2:
        return 32;

    default:
        return 37;
    }
}

void microOpen(char *filename)
{
    free(microConfig.fileName);
    microConfig.fileName = strdup(filename);

    microSelectSyntaxHighlight();

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

        microInsertRow(microConfig.numRows, line, lineLen);
    }
    free(line);
    fclose(fp);
    microConfig.dirtyBuffer = 0;
}

int microRowCursorPosXToRenderPosX(microRow *row, int cursorPosX)
{
    int renderPosX = 0;
    for (int i = 0; i < cursorPosX; ++i)
    {
        if (row->chars[i] == '\t')
        {
            renderPosX += (MICRO_TAB_STOP - 1) - (renderPosX % MICRO_TAB_STOP);
        }
        ++renderPosX;
    }

    return renderPosX;
}

void microDrawStatusBar(struct appendBuffer *ab)
{
    appendBufferAppend(ab, "\x1b[7m", 4);
    char fileNameStatus[80], rstatus[80];

    int len = snprintf(fileNameStatus, sizeof(fileNameStatus), "%.24s - %d lines %s",
                       microConfig.fileName ? microConfig.fileName : "[No filename]", microConfig.numRows, microConfig.dirtyBuffer ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", microConfig.syntax ? microConfig.syntax->fileType : "No file type", microConfig.cursorPosY + 1, microConfig.numRows);

    if (len > microConfig.screenCols)
        len = microConfig.screenCols;

    appendBufferAppend(ab, fileNameStatus, len);

    while (len < microConfig.screenCols)
    {

        if (microConfig.screenCols - len == rlen)
        {
            appendBufferAppend(ab, rstatus, rlen);
            break;
        }
        else
        {
            appendBufferAppend(ab, " ", 1);
            ++len;
        }
    }
    appendBufferAppend(ab, "\x1b[m", 3);
    appendBufferAppend(ab, "\r\n", 2);
}

void microSetStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(microConfig.statusMessage, sizeof(microConfig.statusMessage), fmt, ap);
    va_end(ap);
    microConfig.statusMessage_time = time(NULL);
}

void microDrawMessageBar(struct appendBuffer *ab)
{
    appendBufferAppend(ab, "\x1b[K", 3);
    int messageLen = strlen(microConfig.statusMessage);
    if (messageLen > microConfig.screenCols)
        messageLen = microConfig.screenCols;
    if (messageLen && time(NULL) - microConfig.statusMessage_time < 5)
        appendBufferAppend(ab, microConfig.statusMessage, messageLen);
}

void microRowInsertCharacter(microRow *row, int at, int c)
{
    if (at > row->size || at < 0)
        at = row->size;

    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    (row->size)++;
    row->chars[at] = c;
    microUpdateRow(row);
    ++(microConfig.dirtyBuffer);
}

void microInsertCharacter(int c)
{
    if (microConfig.cursorPosY == microConfig.numRows)
        microInsertRow(microConfig.numRows, "", 0);

    microRowInsertCharacter(&microConfig.row[microConfig.cursorPosY], microConfig.cursorPosX, c);
    ++(microConfig.cursorPosX);
    // ++(microConfig.dirtyBuffer);
}

void microSelectSyntaxHighlight()
{
    microConfig.syntax = NULL;
    if (microConfig.fileName == NULL)
        return;

    // char *ext = strrchr(microConfig.fileName, '.');

    for (unsigned int i = 0; i < HLDB_ENTRIES; i++)
    {
        struct microSyntax *s = &HLDB[i];
        unsigned int j = 0;
        while (s->fileMatch[j])
        {
            // int isExt = (s->fileMatch[j][0] == '.');
            char *p = strstr(microConfig.fileName, s->fileMatch[j]);
            // if((isExt && ext && !strcmp(ext, s->fileMatch[j])) || (!isExt && strstr(microConfig.fileName, s->fileMatch[j])))
            if (p != NULL)
            {
                int patLen = strlen(s->fileMatch[j]);
                if (s->fileMatch[j][0] != '.' || p[patLen] == '\0')
                {
                    microConfig.syntax = s;
                }

                for (int fileRow = 0; fileRow < microConfig.numRows; fileRow++)
                {
                    microUpdateSyntax(&microConfig.row[fileRow]);
                }

                return;
            }
            i++;
        }
    }
}

void microInsertNewLine()
{
    if (microConfig.cursorPosX == 0)
    {
        microInsertRow(microConfig.cursorPosY, "", 0);
    }
    else
    {
        microRow *row = &microConfig.row[microConfig.cursorPosY];
        microInsertRow(microConfig.cursorPosY + 1, &row->chars[microConfig.cursorPosX], row->size - microConfig.cursorPosX);
        row = &microConfig.row[microConfig.cursorPosY];
        row->size = microConfig.cursorPosX;
        row->chars[row->size] = '\0';
        microUpdateRow(row);
    }
    ++(microConfig.cursorPosY);
    microConfig.cursorPosX = 0;
}

int main(int argc, char *argv[])
{
    enableRawInputMode();
    initializeMicro();

    if (argc >= 2)
    {
        microOpen(argv[1]);
    }

    microSetStatusMessage("To Exit: Ctrl-q | To Save: Ctrl-s | To Find: Ctrl-f");

    while (1)
    {
        microRefreshScreen();
        microProcessKeypress();
    }
    return 0;
}