#ifndef INPUT_H_
#define INPUT_H_

#include "micro.h"

char *microPrompt(char *prompt, void (*callback)(char *, int));
void microMoveCursor(int key);
void microProcessKeypress();

#endif // INPUT_H_