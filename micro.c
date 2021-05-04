#include "micro.h"
#include "terminalManipulation.h"
#include "fileIO.h"
#include "output.h"
#include "input.h"

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
// End of file single-line comment

/* End of file multi-line comment */
