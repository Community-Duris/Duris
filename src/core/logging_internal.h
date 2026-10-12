#ifndef CORE_LOGGING_INTERNAL_H
#define CORE_LOGGING_INTERNAL_H

#include <stdarg.h>

// One original counter shared by ordinary and bounded logging.
extern unsigned int debugcount;

namespace core_logging_detail
{
char *format_variadic_message(const char *prefix, const char *suffix, const char *format,
			      va_list args);
bool create_log_parent_directories(const char *filename);
} // namespace core_logging_detail

#endif
