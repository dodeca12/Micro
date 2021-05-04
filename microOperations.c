#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "microOperations.h"
#include "rowOperations.h"

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

void microDeleteCharacter()
{
    if (microConfig.cursorPosY == microConfig.numRows)
        return;
    if (microConfig.cursorPosX == 0 && microConfig.cursorPosY == 0)
        return;

    microRow *row = &microConfig.row[microConfig.cursorPosY];

    if (microConfig.cursorPosX > 0)
    {
        microRowDeleteCharacter(row, microConfig.cursorPosX - 1);
        (microConfig.cursorPosX)--;
    }
    else
    {
        microConfig.cursorPosX = microConfig.row[microConfig.cursorPosY - 1].size;
        microRowAppendString(&microConfig.row[microConfig.cursorPosY - 1], row->chars, row->size);
        microDeleteRow(microConfig.cursorPosY);
        (microConfig.cursorPosY)--;
    }
}

void microInsertCharacter(int c)
{
    if (microConfig.cursorPosY == microConfig.numRows)
        microInsertRow(microConfig.numRows, "", 0);

    microRowInsertCharacter(&microConfig.row[microConfig.cursorPosY], microConfig.cursorPosX, c);
    (microConfig.cursorPosX)++;
}