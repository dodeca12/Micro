#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include "micro.h"

struct termios original_termios;

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
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1)
        die("tcsetattr");
}

void enableRawInputMode()
{
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1)
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

    struct termios raw = original_termios;
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

    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b[H", 3);

    microDrawRows();

    write(STDIN_FILENO, "\x1b[H", 3);
    
}

void microDrawRows()
{
    int r;
    for (r = 0; r < 36; ++r)
    {
        write(STDIN_FILENO, "~\r\n", 3);
    }
}

int main()
{
    enableRawInputMode();
    while (1)
    {
        microRefreshScreen();
        microProcessKeypress();
    }
    return 0;
}