#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rowOperations.h"

int microRowCursorPosXToRenderPosX(microRow *row, int cursorPosX)
{
    int renderPosX = 0;
    for (int i = 0; i < cursorPosX; ++i)
    {
        if (row->chars[i] == '\t')
        {
            renderPosX += (MICRO_TAB_STOP - 1) - (renderPosX % MICRO_TAB_STOP);
        }
        renderPosX++;
    }

    return renderPosX;
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
            tabs++;
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (MICRO_TAB_STOP - 1) + 1);

    int idx = 0;
    for (int i = 0; i < row->size; i++)
    {
        if (row->chars[i] == '\t')
        {
            row->render[idx++] = ' ';

            while (idx % MICRO_TAB_STOP != 0)
                row->render[idx++] = ' ';
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

void microInsertRow(int at, char *s, size_t len)
{

    if (at < 0 || at > microConfig.numRows)
        return;

    microConfig.row = realloc(microConfig.row, sizeof(microRow) * (microConfig.numRows + 1));

    memmove(&microConfig.row[at + 1], &microConfig.row[at], sizeof(microRow) * (microConfig.numRows - at));

    for (int i = at + 1; i <= microConfig.numRows; i++)
        microConfig.row[i].idx++;

    microConfig.row[at].idx = at;

    microConfig.row[at].size = len;
    microConfig.row[at].chars = malloc(len + 1);
    memcpy(microConfig.row[at].chars, s, len);
    microConfig.row[at].chars[len] = '\0';

    microConfig.row[at].rsize = 0;
    microConfig.row[at].render = NULL;
    microConfig.row[at].highlight = NULL;
    microConfig.row[at].highlightOpenComment = 0;

    microUpdateRow(&microConfig.row[at]);

    (microConfig.numRows)++;
    ++(microConfig.dirtyBuffer);
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

    for (int i = at; i < microConfig.numRows - 1; i++)
        microConfig.row[i].idx--;

    (microConfig.numRows)--;
    (microConfig.dirtyBuffer)++;
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

void microRowDeleteCharacter(microRow *row, int at)
{
    if (at >= row->size || at < 0)
        return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    (row->size)--;
    microUpdateRow(row);
    ++(microConfig.dirtyBuffer);
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
