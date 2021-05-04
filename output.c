#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "output.h"

void microScroll()
{
    microConfig.renderPosX = microConfig.cursorPosX;

    if (microConfig.cursorPosY < microConfig.numRows)
        microConfig.renderPosX = microRowCursorPosXToRenderPosX(&microConfig.row[microConfig.cursorPosY], microConfig.cursorPosX);

    if (microConfig.cursorPosY < microConfig.rowOffset)
    {
        microConfig.rowOffset = microConfig.cursorPosY;
    }
    if (microConfig.cursorPosY >= microConfig.rowOffset + microConfig.screenRows)
    {
        microConfig.rowOffset = microConfig.cursorPosY - microConfig.screenRows + 1;
    }
    if (microConfig.renderPosX < microConfig.colOffset)
    {
        microConfig.colOffset = microConfig.renderPosX;
    }
    if (microConfig.renderPosX >= microConfig.colOffset + microConfig.screenCols)
    {
        microConfig.colOffset = microConfig.renderPosX - microConfig.screenCols + 1;
    }
}

void microDrawMessageBar(struct appendBuffer *ab)
{
    appendBufferAppend(ab, "\x1b[K", 3);
    int messageLen = strlen(microConfig.statusMessage);
    if (messageLen > microConfig.screenCols)
        messageLen = microConfig.screenCols;
    if (messageLen && time(NULL) - microConfig.statusMessage_time < 5)
        appendBufferAppend(ab, microConfig.statusMessage, messageLen);
}

void microDrawRows(struct appendBuffer *ab)
{

    for (int r = 0; r < microConfig.screenRows; r++)
    {
        int fileRow = r + microConfig.rowOffset;
        if (fileRow >= microConfig.numRows)
        {
            if (microConfig.numRows == 0 && r == (microConfig.screenRows / 3) - 3)
            {
                char welcome[80];
                int welcomeLen = snprintf(welcome, sizeof(welcome),
                                          "Micro Editor -- Version %s", MICRO_VERSION);

                if (welcomeLen > microConfig.screenCols)
                    welcomeLen = microConfig.screenCols;
                int padding = (microConfig.screenCols - welcomeLen) / 2;
                if (padding)
                {
                    appendBufferAppend(ab, "~", 1);
                    padding++;
                }
                while (padding--)
                    appendBufferAppend(ab, " ", 1);

                appendBufferAppend(ab, welcome, welcomeLen);
            }
            else
            {
                appendBufferAppend(ab, "~", 1);
            }
            if (microConfig.numRows == 0 && r == (microConfig.screenRows / 3) + 1)
            {
                char author[80];
                int authorLen = snprintf(author, sizeof(author), "by %s", AUTHOR);

                if (authorLen > microConfig.screenCols)
                    authorLen = microConfig.screenCols;
                int padding = (microConfig.screenCols - authorLen) / 2;
                if (padding)
                {
                    padding++;
                }
                while (padding--)
                    appendBufferAppend(ab, " ", 1);
                appendBufferAppend(ab, author, authorLen);
            }
            if (microConfig.numRows == 0 && r == (microConfig.screenRows / 3) + 2)
            {
                char repo[80];
                int repoLen = snprintf(repo, sizeof(repo), "Source: %s", REPO);

                if (repoLen > microConfig.screenCols)
                    repoLen = microConfig.screenCols;
                int padding = (microConfig.screenCols - repoLen) / 2;
                if (padding)
                {
                    padding++;
                }
                while (padding--)
                    appendBufferAppend(ab, " ", 1);
                appendBufferAppend(ab, repo, repoLen);
            }
        }

        else
        {
            int len = microConfig.row[fileRow].rsize - microConfig.colOffset;
            if (len < 0)
                len = 0;
            if (len > microConfig.screenCols)
                len = microConfig.screenCols;

            char *c = &microConfig.row[fileRow].render[microConfig.colOffset];
            unsigned char *highlight = &microConfig.row[fileRow].highlight[microConfig.colOffset];
            int currentColour = -1;
            for (int i = 0; i < len; i++)
            {
                if (iscntrl(c[i]))
                {
                    char sym = (c[i] <= 26) ? '@' + c[i] : '?';
                    appendBufferAppend(ab, "\x1b[7m", 4);
                    appendBufferAppend(ab, &sym, 1);
                    appendBufferAppend(ab, "\x1b[m,", 3);
                    if (currentColour != -1)
                    {
                        char buffer[16];
                        int colourLen = snprintf(buffer, sizeof(buffer), "\x1b[%dm", currentColour);
                        appendBufferAppend(ab, buffer, colourLen);
                    }
                }
                else if (highlight[i] == HL_DEFAULT)
                {
                    if (currentColour != -1)
                    {
                        appendBufferAppend(ab, "\x1b[39m", 5);
                        currentColour = -1;
                    }

                    appendBufferAppend(ab, &c[i], 1);
                }
                else
                {
                    int colour = microSyntaxToColour(highlight[i]);
                    if (colour != currentColour)
                    {
                        currentColour = colour;
                        char buffer[16];
                        int colourLen = snprintf(buffer, sizeof(buffer), "\x1b[%dm", colour);
                        appendBufferAppend(ab, buffer, colourLen);
                    }

                    appendBufferAppend(ab, &c[i], 1);
                }
            }
            appendBufferAppend(ab, "\x1b[39m", 5);
        }

        appendBufferAppend(ab, "\x1b[K", 3);

        appendBufferAppend(ab, "\r\n", 2);
    }
}

void microDrawStatusBar(struct appendBuffer *ab)
{
    appendBufferAppend(ab, "\x1b[7m", 4);
    char fileNameStatus[80], rstatus[80];

    int len = snprintf(fileNameStatus, sizeof(fileNameStatus), "%.24s - %d lines %s",
                       microConfig.fileName ? microConfig.fileName : "[No filename]", microConfig.numRows, microConfig.dirtyBuffer ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d ", microConfig.syntax ? microConfig.syntax->fileType : "No file type", microConfig.cursorPosY + 1, microConfig.numRows);

    if (len > microConfig.screenCols)
        len = microConfig.screenCols;

    appendBufferAppend(ab, fileNameStatus, len);

    while (len < microConfig.screenCols)
    {

        if (microConfig.screenCols - len == rlen)
        {
            appendBufferAppend(ab, rstatus, rlen);
            break;
        }
        else
        {
            appendBufferAppend(ab, " ", 1);
            len++;
        }
    }
    appendBufferAppend(ab, "\x1b[m", 3);
    appendBufferAppend(ab, "\r\n", 2);
}

void microRefreshScreen()
{

    microScroll();

    // 4 = writing 4 bytes
    // first byte = \x1b = escape character (27 ASCII decimal)
    // writing an escape sequence to the terminal; escape sequences
    // start with the escape character (27), along with '['
    // J stands for the "J command" aka the Erase In Display
    // found in the VT100 (https://vt100.net/docs/vt100-ug/chapter3.html#ED)
    struct appendBuffer ab = APPEND_BUFFER_INIT;

    appendBufferAppend(&ab, "\x1b[?25l", 6);
    // appendBufferAppend(&ab, "\x1b[2J", 4);
    appendBufferAppend(&ab, "\x1b[H", 3);

    microDrawRows(&ab);
    microDrawStatusBar(&ab);
    microDrawMessageBar(&ab);

    char buf[16];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (microConfig.cursorPosY - microConfig.rowOffset) + 1, (microConfig.renderPosX - microConfig.colOffset) + 1);
    appendBufferAppend(&ab, buf, strlen(buf));

    appendBufferAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);

    appendBufferFree(&ab);
}

void microSetStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(microConfig.statusMessage, sizeof(microConfig.statusMessage), fmt, ap);
    va_end(ap);
    microConfig.statusMessage_time = time(NULL);
}
