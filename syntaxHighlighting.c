#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "syntaxHighlighting.h"

char *C_HL_extensions[] = {".c", ".h", ".cpp", ".cc", NULL};

char *C_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "", "enum", "class", "case",
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL};

struct microSyntax HLDB[] = {
    {"c",
     C_HL_extensions,
     C_HL_keywords,
     "//", "/*", "***",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

int hasSeperator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void microUpdateSyntax(microRow *row)
{
    row->highlight = realloc(row->highlight, row->rsize);
    memset(row->highlight, HL_DEFAULT, row->rsize);

    if (microConfig.syntax == NULL)
        return;

    char **keywords = microConfig.syntax->keywords;

    char *slcs = microConfig.syntax->singleLineCommentStart;
    char *mlcs = microConfig.syntax->multiLineCommentStart;
    char *mlce = microConfig.syntax->multiLineCommentEnd;

    int slcsLen = slcs ? strlen(slcs) : 0;
    int mlcsLen = mlcs ? strlen(mlcs) : 0;
    int mlceLen = mlce ? strlen(mlce) : 0;

    int previousSeperator = 1;
    int inString = 0;
    // int inComment = 0;
    int inComment = (row->idx > 0 && microConfig.row[row->idx - 1].highlightOpenComment);

    int i = 0;
    while (i < row->rsize)
    {
        char c = row->render[i];
        unsigned char previousHighlight = (i > 0) ? row->highlight[i - 1] : HL_DEFAULT;
        if (slcsLen && !inString && !inComment)
        {
            if (!strncmp(&row->render[i], slcs, slcsLen))
            {
                memset(&row->highlight[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }

        if (mlcsLen && mlceLen && !inString)
        {
            if (inComment)
            {
                row->highlight[i] = HL_ML_COMMENT;
                if (!strncmp(&row->render[i], mlce, mlceLen))
                {
                    memset(&row->highlight[i], HL_ML_COMMENT, mlceLen);
                    i += mlceLen;
                    inComment = 0;
                    previousSeperator = 1;
                    continue;
                }
                else
                {
                    i++;
                    continue;
                }
            }
            else if (!strncmp(&row->render[i], mlcs, mlcsLen))
            {
                memset(&row->highlight[i], HL_ML_COMMENT, mlcsLen);
                i += mlcsLen;
                inComment = 1;
                continue;
            }
        }

        if (microConfig.syntax->flags & HL_HIGHLIGHT_STRINGS)
        {
            if (inString)
            {
                row->highlight[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize)
                {
                    row->highlight[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == inString)
                    inString = 0;
                i++;
                previousSeperator = 1;
                continue;
            }
            else
            {
                if (c == '"' || c == '\'')
                {
                    inString = c;
                    row->highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (microConfig.syntax->flags & HL_HIGHLIGHT_NUMBERS)
        {

            if ((isdigit(c) && (previousSeperator || previousHighlight == HL_NUMBER)) || (c == '.' && previousHighlight == HL_NUMBER))
            {
                row->highlight[i] = HL_NUMBER;
                i++;
                previousSeperator = 0;
                continue;
            }
        }

        if (previousSeperator)
        {
            int j;
            for (j = 0; keywords[j]; j++)
            {
                int keywordLen = strlen(keywords[j]);
                int keywords2 = keywords[j][keywordLen - 1] == '|';
                if (keywords2)
                    keywordLen--;
                if (!strncmp(&row->render[i], keywords[j], keywordLen) && hasSeperator(row->render[i + keywordLen]))
                {
                    memset(&row->highlight[i], keywords2 ? HL_KEYWORD2 : HL_KEYWORD1, keywordLen);
                    i += keywordLen;
                    break;
                }
            }
            if (keywords[j] != NULL)
            {
                previousSeperator = 0;
                continue;
            }
        }

        previousSeperator = hasSeperator(c);
        i++;
    }
    int changed = (row->highlightOpenComment != inComment);
    row->highlightOpenComment = inComment;
    if (changed && row->idx + 1 < microConfig.numRows)
    {
        microUpdateSyntax(&microConfig.row[row->idx + 1]);
    }
}

int microSyntaxToColour(int highlight)
{
    switch (highlight)
    {
    case HL_NUMBER:
        return 31;
    case HL_STRING:
        return 35;
    case HL_MATCH:
        return 34;
    case HL_COMMENT:
    case HL_ML_COMMENT:
        return 36;
    case HL_KEYWORD1:
        return 33;
    case HL_KEYWORD2:
        return 32;

    default:
        return 37;
    }
}

void microSelectSyntaxHighlight()
{
    microConfig.syntax = NULL;
    if (microConfig.fileName == NULL)
        return;
    char *ext = strrchr(microConfig.fileName, '.');
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++)
    {
        struct microSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->fileMatch[i])
        {
            int is_ext = (s->fileMatch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->fileMatch[i])) ||
                (!is_ext && strstr(microConfig.fileName, s->fileMatch[i])))
            {
                microConfig.syntax = s;
                int filerow;
                for (filerow = 0; filerow < microConfig.numRows; filerow++)
                {
                    microUpdateSyntax(&microConfig.row[filerow]);
                }
                return;
            }
            i++;
        }
    }
}