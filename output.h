#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "micro.h"
#include "bufferOperations.h"
#include "rowOperations.h"
#include "syntaxHighlighting.h"

void microScroll();
void microDrawMessageBar(struct appendBuffer *ab);
void microDrawRows();
void microDrawStatusBar(struct appendBuffer *ab);
void microRefreshScreen();
void microSetStatusMessage(const char *fmt, ...);

#endif // OUTPUT_H_