#ifndef FILE_IO_H_
#define FILE_IO_H_

#include "micro.h"

char *microRowsToString(int *bufferLen);
void microOpen(char *filename);
void microSave();

#endif // FILE_IO_H_