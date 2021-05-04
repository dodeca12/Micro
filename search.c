#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "search.h"
#include "input.h"
#include "rowOperations.h"

void microSearch()
{
    int savedCursorPosX, savedCursorPosY, savedColOffset, savedRowOffset;
    savedCursorPosX = microConfig.cursorPosX;
    savedCursorPosY = microConfig.cursorPosY;
    savedColOffset = microConfig.colOffset;
    savedRowOffset = microConfig.rowOffset;

    char *query = microPrompt("Search: %s (ESC to cancel/Arrow keys to move/Enter to  finish search)", microSearchCallback);
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