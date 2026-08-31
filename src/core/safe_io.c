#include "core/safe_io.h"

#include "core/prototypes.h"

#include <stdarg.h>

void required_fscanf_impl(FILE *stream, int expected_fields, const char *source_file,
			  int source_line, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int fields_read = vfscanf(stream, format, args);
	va_end(args);

	if (fields_read != expected_fields)
	{
		fatal_boot_error("file_io", "%s:%d: expected %d fields but read %d", source_file,
				 source_line, expected_fields, fields_read);
	}
}

void required_fgets_impl(char *buffer, int buffer_size, FILE *stream, const char *source_file,
			 int source_line)
{
	if (!fgets(buffer, buffer_size, stream))
	{
		fatal_boot_error("file_io", "%s:%d: required line could not be read", source_file,
				 source_line);
	}
}

void required_fread_impl(void *buffer, size_t element_size, size_t element_count, FILE *stream,
			 const char *source_file, int source_line)
{
	size_t elements_read = fread(buffer, element_size, element_count, stream);
	if (elements_read != element_count)
	{
		fatal_boot_error("file_io", "%s:%d: expected %zu elements but read %zu",
				 source_file, source_line, element_count, elements_read);
	}
}
