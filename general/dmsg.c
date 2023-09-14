#include "general.h"
#include <stdarg.h>
//inline void dmsg(const char *fmt, __gnuc_va_list __arg)
inline void dmsg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
//    vprintf(fmt,ap);
    va_end(ap);
}
