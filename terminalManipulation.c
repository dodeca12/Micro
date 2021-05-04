#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "terminalManipulation.h"

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

int getCursorPosition(int *rows, int *cols)
{

    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;

    return 0;
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

int getTerminalWindowSize(int *rows, int *cols)
{
    struct winsize wsize;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize) == -1 || wsize.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    }
    else
    {
        *rows = wsize.ws_row;
        *cols = wsize.ws_col;
        return 0;
    }
}
