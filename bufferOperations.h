#ifndef BUFFER_OPERATIONS_H_
#define BUFFER_OPERATIONS_H_

#include "micro.h"

void appendBufferAppend(struct appendBuffer *ab, const char *s, int len);
void appendBufferFree(struct appendBuffer *ab);

#endif // BUFFER_OPERATIONS_H_