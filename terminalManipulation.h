#ifndef TERMINAL_MANIPULATION_
#define TERMINAL_MANIPULATION_

#include "micro.h"

void die(const char *s);
void disableRawInputMode();
void enableRawInputMode();
int getCursorPosition(int *rows, int *cols);
int microReadKey();
int getTerminalWindowSize(int *rows, int *cols);


#endif // TERMINAL_MANIPULATION_
