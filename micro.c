#include <stdlib.h>
#include <termio.h>
#include <unistd.h>

struct termios original_termios;

void disableRawInputMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enableRawInputMode()
{
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disableRawInputMode);

    // c_lflag = field for "local flags"
    // ECHO is a bitflag with value 00000000000000000000000000001000
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
    return 0;
}