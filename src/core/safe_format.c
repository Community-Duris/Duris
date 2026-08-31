#include "core/safe_format.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checked_vsnprintf(char *destination, size_t destination_size, const char *format,
			     va_list args)
{
	char *rendered;
	int required;
	va_list measure_args;

	if ((!destination && destination_size) || !format)
		return -1;

	va_copy(measure_args, args);
	required = vsnprintf(NULL, 0, format, measure_args);
	va_end(measure_args);
	if (required < 0)
	{
		if (destination_size)
			destination[0] = '\0';
		return required;
	}

	rendered = (char *)malloc((size_t)required + 1);
	if (!rendered)
	{
		if (destination_size)
			destination[0] = '\0';
		return -1;
	}
	vsnprintf(rendered, (size_t)required + 1, format, args);

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

int checked_snprintf(char *destination, size_t destination_size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int required = checked_vsnprintf(destination, destination_size, format, args);
	va_end(args);
	return required;
}

int checked_snprintf_runtime(char *destination, size_t destination_size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int required = checked_vsnprintf(destination, destination_size, format, args);
	va_end(args);
	return required;
}

static void substitute_copy(char *destination, size_t destination_size, size_t *required,
			    const char *source, size_t source_length)
{
	if (destination_size && *required < destination_size - 1)
	{
		size_t available = destination_size - 1 - *required;
		size_t copy_size = source_length < available ? source_length : available;
		memcpy(destination + *required, source, copy_size);
	}
	*required += source_length;
}

int checked_substitute_strings(char *destination, size_t destination_size, const char *format,
			       const char *const *substitutions, size_t substitution_count)
{
	size_t required = 0;
	size_t substitution_index = 0;

	if ((!destination && destination_size) || !format || (!substitutions && substitution_count))
		return -1;

	for (size_t index = 0; format[index]; ++index)
	{
		if (format[index] == '%' && format[index + 1] == '%')
		{
			substitute_copy(destination, destination_size, &required, "%", 1);
			++index;
		}
		else if (format[index] == '%' && format[index + 1] == 's' &&
			 substitution_index < substitution_count)
		{
			const char *value = substitutions[substitution_index++];
			if (!value)
				value = "(null)";
			substitute_copy(destination, destination_size, &required, value,
					strlen(value));
			++index;
		}
		else
		{
			substitute_copy(destination, destination_size, &required, format + index,
					1);
		}
	}

	if (destination_size)
		destination[required < destination_size ? required : destination_size - 1] = '\0';
	if (required >= destination_size)
		fprintf(stderr,
			"checked_substitute_strings: output requires %zu bytes but destination holds %zu; truncated.\n",
			required, destination_size ? destination_size - 1 : 0);

	return required > (size_t)INT_MAX ? INT_MAX : (int)required;
}

int checked_appendf(char *buffer, size_t capacity, const char *format, ...)
{
	size_t used;
	va_list args;
	int required;

	if (!buffer || !capacity || !format)
		return -1;

	used = strnlen(buffer, capacity);
	if (used + 1 >= capacity)
		return 0; /* no room left; leave the buffer as it is */

	va_start(args, format);
	required = vsnprintf(buffer + used, capacity - used, format, args);
	va_end(args);
	return required;
}
