#ifndef SYNTAX_HIGHLIGHTING_H_
#define SYNTAX_HIGHLIGHTING_H_

#include "micro.h"

void microUpdateSyntax(microRow *row);
int microSyntaxToColour(int highlight);
int hasSeperator(int c);
void microSelectSyntaxHighlight();


#endif // SYNTAX_HIGHLIGHTING_H_