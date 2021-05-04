#ifndef MICRO_H_
#define MICRO_H_

#include <termios.h>
#include <time.h>

#define CTRL_KEY(k) ((k)&0x1f)
#define APPEND_BUFFER_INIT \
    {                      \
        NULL, 0            \
    }
#define MICRO_VERSION "0.0.1"
#define AUTHOR "Swapneeth Gorantla (@dodeca12)"
#define REPO "https://github.com/dodeca12/Micro"
#define MICRO_TAB_STOP 8
#define MICRO_FORCE_QUIT 1

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

typedef struct microRow
{
    char *chars;
    int size;
    int rsize;
    char *render;
    unsigned char *highlight;
    int idx;
    int highlightOpenComment;
} microRow;

struct editorConfig
{
    struct termios original_termios;
    int screenRows, screenCols;
    int cursorPosX, cursorPosY;
    int renderPosX;
    microRow *row;
    char *fileName;
    int numRows;
    int rowOffset, colOffset;
    char statusMessage[80];
    time_t statusMessage_time;
    int dirtyBuffer;
    struct microSyntax *syntax;
} microConfig;

struct appendBuffer
{
    char *b;
    int len;
};

enum microKeyMap
{
    BACKSPACE = 127,
    LEFT_ARROW = 121212,
    RIGHT_ARROW,
    UP_ARROW,
    DOWN_ARROW,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DELETE_KEY
};

enum microHighlight
{
    HL_DEFAULT = 0,
    HL_COMMENT,
    HL_ML_COMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH

};

struct microSyntax
{
    char *fileType;
    char **fileMatch;
    char **keywords;
    char *singleLineCommentStart;
    char *multiLineCommentStart;
    char *multiLineCommentEnd;
    int flags;
};

#endif // MICRO_H_