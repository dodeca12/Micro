#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bufferOperations.h"

void appendBufferAppend(struct appendBuffer *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL)
        return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void appendBufferFree(struct appendBuffer *ab)
{
    free(ab->b);
}
