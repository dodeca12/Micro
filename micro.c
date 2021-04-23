#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>
#include <ctype.h>

struct termios original_termios;

void disableRawInputMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enableRawInputMode()
{
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disableRawInputMode);

    /*
    c_lflag = field for "local flags"
    c_oflag = field for "output flags"'
    c_iflag = field for "input flags"
    c_cflag = field for "control flags"
    */
    
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
    raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Turning the above flags off are not necessary for modern usage, but it is done to keep with the tradition when enabling "raw mode"
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
    enableRawInputMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    {
        if (iscntrl(c))
        {
            printf("%d\r\n", c);
        }
        else
        {
            printf("%d ('%c')\r\n", c, c);
        }
    }
    return 0;
}