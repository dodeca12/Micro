#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "input.h"
#include "output.h"
#include "search.h"
#include "fileIO.h"
#include "microOperations.h"
#include "terminalManipulation.h"

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
