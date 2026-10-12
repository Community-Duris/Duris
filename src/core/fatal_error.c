#include "core/defines.h"
#include "core/prototypes.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void fatal_message_v(const char *component, const char *fmt, va_list ap)
{
	char msg[MAX_STRING_LENGTH];
	const char *site = component ? component : "fatal";

	vsnprintf(msg, sizeof(msg), fmt ? fmt : "", ap);
#ifndef _DE_
	logit(LOG_EXIT, "%s: %s", site, msg);
#endif
	fprintf(stderr, "%s: %s\n", site, msg);
}

void fatal_boot_error(const char *component, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fatal_message_v(component, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void panic_corruption(const char *component, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fatal_message_v(component, fmt, ap);
	va_end(ap);
	abort();
}

[[noreturn]] int panic_corruption_int(const char *component, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fatal_message_v(component, fmt, ap);
	va_end(ap);
	abort();
}

bool require_char(P_char ch, const char *component, const char *fmt, ...)
{
	if (ch)
		return true;

	va_list ap;
	va_start(ap, fmt);
	fatal_message_v(component, fmt, ap);
	va_end(ap);
	return false;
}

bool require_data(const void *data, const char *component, const char *fmt, ...)
{
	if (data)
		return true;

	va_list ap;
	va_start(ap, fmt);
	fatal_message_v(component, fmt, ap);
	va_end(ap);
	return false;
}
