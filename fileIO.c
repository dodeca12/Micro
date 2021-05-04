#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "fileIO.h"
#include "terminalManipulation.h"
#include "syntaxHighlighting.h"
#include "rowOperations.h"
#include "output.h"
#include "input.h"

char *microRowsToString(int *bufferLen)
{
    int totalLen = 0;
    for (int i = 0; i < microConfig.numRows; i++)
    {
        totalLen += microConfig.row[i].size + 1;
    }
    *bufferLen = totalLen;

    char *buffer = malloc(totalLen);
    char *paragraph = buffer;
    for (int i = 0; i < microConfig.numRows; i++)
    {
        memcpy(paragraph, microConfig.row[i].chars, microConfig.row[i].size);
        paragraph += microConfig.row[i].size;
        *paragraph = '\n';
        paragraph++;
    }
    return buffer;
}

void microOpen(char *filename)
{
    free(microConfig.fileName);
    microConfig.fileName = strdup(filename);
    microSelectSyntaxHighlight();

    FILE *fp = fopen(filename, "r");
    if (!fp)
        die("fopen");

    char *line = NULL;
    size_t lineCapacity = 0;
    ssize_t lineLen;
    while ((lineLen = getline(&line, &lineCapacity, fp)) != -1)
    {

        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r'))
            lineLen--;

        microInsertRow(microConfig.numRows, line, lineLen);
    }
    free(line);
    fclose(fp);
    microConfig.dirtyBuffer = 0;
}

void microSave()
{
    if (microConfig.fileName == NULL)
    {
        microConfig.fileName = microPrompt("Save file as: %s (ESC to cancel | Enter to save)", NULL);
        if (microConfig.fileName == NULL)
        {
            microSetStatusMessage("Save cancelled");
            return;
        }
        microSelectSyntaxHighlight();
    }

    int len;
    char *buffer = microRowsToString(&len);

    int fd = open(microConfig.fileName, O_RDWR | O_CREAT, 0644);

    if (fd != -1)
    {
        if (ftruncate(fd, len) != -1)
        {
            if (write(fd, buffer, len) == len)
            {
                close(fd);
                free(buffer);
                microConfig.dirtyBuffer = 0;
                microSetStatusMessage("%d bytes written", len);
                return;
            }
        }
        close(fd);
    }

    free(buffer);
    microSetStatusMessage("Micro cannot save file! I/O Error: %s", strerror(errno));
}
