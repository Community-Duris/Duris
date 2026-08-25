#include "safe_format.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checked_snprintf(char *destination, size_t destination_size, const char *format, ...)
{
	char *rendered;
	int required;
	va_list args, measure_args;

	if ((!destination && destination_size) || !format)
		return -1;

	va_start(args, format);
	va_copy(measure_args, args);
	required = vsnprintf(NULL, 0, format, measure_args);
	va_end(measure_args);
	if (required < 0)
	{
		va_end(args);
		if (destination_size)
			destination[0] = '\0';
		return required;
	}

	rendered = (char *)malloc((size_t)required + 1);
	if (!rendered)
	{
		va_end(args);
		if (destination_size)
			destination[0] = '\0';
		return -1;
	}
	vsnprintf(rendered, (size_t)required + 1, format, args);
	va_end(args);

	if (destination_size)
	{
		size_t available = destination_size - 1;
		size_t copy_size = (size_t)required < available ? (size_t)required : available;

		memcpy(destination, rendered, copy_size);
		destination[copy_size] = '\0';
	}
	if ((size_t)required >= destination_size)
		fprintf(stderr,
			"checked_snprintf: output requires %d bytes but destination holds %zu; truncated.\n",
			required, destination_size ? destination_size - 1 : 0);

	free(rendered);
	return required;
}
