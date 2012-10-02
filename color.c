#include "color.h"

#include <stdarg.h>
#include <unistd.h>

static const char *ANSI_SEQ[_C_MAX] = {
        [NONE] = "\033[0m",
        [RED] = "\033[31m",
        [GREEN] = "\033[32m",
        [YELLOW] = "\033[33m",
};

int c_fprintf(enum color c, FILE *stream, const char *fmt, ...)
{
        va_list ap;
        int ret;
        int tty;

        tty = isatty(fileno(stream));

        if (tty) {
                fprintf(stream, "%s", ANSI_SEQ[c]);
        }

        va_start(ap, fmt);
        ret = vfprintf(stream, fmt, ap);
        va_end(ap);

        if (tty) {
                fprintf(stream, "%s", ANSI_SEQ[NONE]);
        }

        return ret;
}
