#ifndef ROW_OPERATIONS_H_
#define ROW_OPERATIONS_H_

#include "micro.h"
#include "syntaxHighlighting.h"


int microRowCursorPosXToRenderPosX(microRow *row, int cursorPosX);
int microRowRenderPosXtoCursorPosX(microRow *row, int renderPosX);
void microUpdateRow(microRow *row);
void microInsertRow(int at, char *s, size_t len);
void microFreeRow(microRow *row);
void microDeleteRow(int at);
void microRowAppendString(microRow *row, char *s, size_t len);
void microRowDeleteCharacter(microRow *row, int at);
void microRowInsertCharacter(microRow *row, int at, int c);

#endif // ROW_OPERATIONS_H_