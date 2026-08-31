#ifndef DURIS_SAFE_IO_H
#define DURIS_SAFE_IO_H

#include <stddef.h>
#include <stdio.h>

void required_fscanf_impl(FILE *stream, int expected_fields, const char *source_file,
			  int source_line, const char *format, ...)
	__attribute__((format(scanf, 5, 6)));
void required_fgets_impl(char *buffer, int buffer_size, FILE *stream, const char *source_file,
			 int source_line);
void required_fread_impl(void *buffer, size_t element_size, size_t element_count, FILE *stream,
			 const char *source_file, int source_line);

#define DURIS_SCAN_ARG_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, \
				  _15, _16, N, ...)                                            \
	N
#define DURIS_SCAN_ARG_COUNT(...)                                                                  \
	DURIS_SCAN_ARG_COUNT_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, \
				  1)

#define REQUIRED_FSCANF(stream, format, ...)                                                  \
	required_fscanf_impl((stream), DURIS_SCAN_ARG_COUNT(__VA_ARGS__), __FILE__, __LINE__, \
			     (format), __VA_ARGS__)
#define REQUIRED_FSCANF_NO_FIELDS(stream, format) \
	required_fscanf_impl((stream), 0, __FILE__, __LINE__, (format))
#define REQUIRED_FGETS(buffer, buffer_size, stream) \
	required_fgets_impl((buffer), (buffer_size), (stream), __FILE__, __LINE__)
#define REQUIRED_FREAD(buffer, element_size, element_count, stream) \
	required_fread_impl((buffer), (element_size), (element_count), (stream), __FILE__, __LINE__)

#endif
