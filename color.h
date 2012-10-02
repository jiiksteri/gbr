#ifndef COLOR_H__INCLUDED
#define COLOR_H__INCLUDED

enum color {
        NONE,
        RED,
        GREEN,
        YELLOW,
        _C_MAX,
};

#include <stdio.h>

int c_fprintf(enum color c, FILE *stream, const char *fmt, ...);

#endif
