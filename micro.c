#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

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

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize) == -1 || wsize.ws_col == 0)
    {
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
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

char microReadKey()
{
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}

int getCursorPosition(int *rows, int *cols)
{

    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
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

    while(i < sizeof(buf) - 1)
    {
        if(read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if(buf[i] == 'R')
            break;
        ++i;
    }
    buf[i] = '\0';

    // printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);
    if(buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;

    // microReadKey();

    return 0;
}

void microProcessKeypress()
{
    char c = microReadKey();
    
    switch(c)
    {
        case CTRL_KEY('q'):

            // Clears screen on exit
            write(STDIN_FILENO, "\x1b[2J", 4);
            write(STDIN_FILENO, "\x1b[H", 3);
            
            exit(0);
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
    appendBufferAppend(&ab, "\x1b[2J", 4);
    appendBufferAppend(&ab, "\x1b[H", 3);

    microDrawRows(&ab);

    appendBufferAppend(&ab, "\x1b[H", 3);
    appendBufferAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);

    appendBufferFree(&ab);
}

void microDrawRows(struct appendBuffer *ab)
{
    int r;
    for (r = 0; r < microConfig.screenRows; ++r)
    {
        appendBufferAppend(ab, "~", 1);

        if(r < microConfig.screenRows -1)
            appendBufferAppend(ab, "\r\n", 2);
    }
}

void initializeMicro()
{
    if(getTerminalWindowSize(&microConfig.screenRows, &microConfig.screenCols) == -1)
        die("getTerminalWindowSize");
}

void appendBufferAppend(struct appendBuffer *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len);

    if(new == NULL)
        return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void appendBufferFree(struct appendBuffer *ab)
{
    free(ab->b);
}

int main()
{
    enableRawInputMode();
    initializeMicro();

    while (1)
    {
        microRefreshScreen();
        microProcessKeypress();
    }
    return 0;
}