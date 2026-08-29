#ifndef DURIS_NO_MYSQL_COMPAT_H
#define DURIS_NO_MYSQL_COMPAT_H

#ifndef __NO_MYSQL__
#error "The client-free MySQL compatibility surface is for flat-file builds only"
#endif

#include <errno.h>
#include <stddef.h>
#include <string.h>

typedef unsigned long long my_ulonglong;
typedef bool my_bool;

struct MYSQL
{
	int unavailable;
};

struct MYSQL_RES
{
	int unavailable;
};

struct MYSQL_STMT
{
	int unavailable;
};

typedef char **MYSQL_ROW;

struct MYSQL_FIELD
{
	char *name;
};

enum enum_field_types
{
	MYSQL_TYPE_TINY,
	MYSQL_TYPE_SHORT,
	MYSQL_TYPE_LONG,
	MYSQL_TYPE_LONGLONG,
	MYSQL_TYPE_STRING,
	MYSQL_TYPE_BLOB
};

struct MYSQL_BIND
{
	enum_field_types buffer_type;
	void *buffer;
	unsigned long buffer_length;
	unsigned long *length;
	my_bool *is_null;
	my_bool *error;
	my_bool is_unsigned;
};

enum mysql_option
{
	MYSQL_OPT_CONNECT_TIMEOUT,
	MYSQL_OPT_READ_TIMEOUT,
	MYSQL_OPT_WRITE_TIMEOUT,
	MYSQL_OPT_RECONNECT,
	MYSQL_SET_CHARSET_NAME,
	MYSQL_OPT_SSL_ENFORCE,
	MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
	MYSQL_OPT_SSL_CA,
	MYSQL_OPT_SSL_MODE
};

#define CLIENT_SSL (1UL << 11)
#define CLIENT_MULTI_STATEMENTS (1UL << 16)
#define SSL_MODE_VERIFY_IDENTITY 5U
#define MYSQL_NO_DATA 100

static inline int mysql_library_init(int, char **, char **)
{
	return 1;
}

static inline int mysql_thread_init(void)
{
	return 1;
}

static inline void mysql_thread_end(void) {}

static inline MYSQL *mysql_init(MYSQL *)
{
	return NULL;
}

static inline MYSQL *mysql_real_connect(MYSQL *, const char *, const char *, const char *,
					const char *, unsigned int, const char *, unsigned long)
{
	return NULL;
}

static inline void mysql_close(MYSQL *) {}

static inline int mysql_options(MYSQL *, enum mysql_option, const void *)
{
	return 1;
}

static inline int mysql_set_character_set(MYSQL *, const char *)
{
	return 1;
}

static inline int mysql_real_query(MYSQL *, const char *, unsigned long)
{
	return 1;
}

static inline MYSQL_RES *mysql_store_result(MYSQL *)
{
	return NULL;
}

static inline MYSQL_RES *mysql_use_result(MYSQL *)
{
	return NULL;
}

static inline MYSQL_ROW mysql_fetch_row(MYSQL_RES *)
{
	return NULL;
}

static inline unsigned long *mysql_fetch_lengths(MYSQL_RES *)
{
	return NULL;
}

static inline MYSQL_FIELD *mysql_fetch_fields(MYSQL_RES *)
{
	return NULL;
}

static inline void mysql_free_result(MYSQL_RES *) {}

static inline my_ulonglong mysql_num_rows(MYSQL_RES *)
{
	return 0;
}

static inline unsigned int mysql_num_fields(MYSQL_RES *)
{
	return 0;
}

static inline unsigned int mysql_field_count(MYSQL *)
{
	return 0;
}

static inline my_ulonglong mysql_affected_rows(MYSQL *)
{
	return (my_ulonglong)-1;
}

static inline my_ulonglong mysql_insert_id(MYSQL *)
{
	return 0;
}

static inline unsigned int mysql_errno(MYSQL *)
{
	return ENOTSUP;
}

static inline const char *mysql_error(MYSQL *)
{
	return "MySQL client support is disabled";
}

static inline const char *mysql_sqlstate(MYSQL *)
{
	return "HY000";
}

static inline const char *mysql_get_server_info(MYSQL *)
{
	return "disabled";
}

static inline const char *mysql_get_ssl_cipher(MYSQL *)
{
	return NULL;
}

static inline int mysql_more_results(MYSQL *)
{
	return 0;
}

static inline int mysql_next_result(MYSQL *)
{
	return -1;
}

static inline unsigned long mysql_thread_id(MYSQL *)
{
	return 0;
}

static inline int mysql_rollback(MYSQL *)
{
	return 1;
}

static inline unsigned long mysql_real_escape_string(MYSQL *, char *output, const char *input,
						     unsigned long length)
{
	if (!output || !input)
		return 0;
	memcpy(output, input, length);
	output[length] = '\0';
	return length;
}

static inline MYSQL_STMT *mysql_stmt_init(MYSQL *)
{
	return NULL;
}

static inline int mysql_stmt_prepare(MYSQL_STMT *, const char *, unsigned long)
{
	return 1;
}

static inline int mysql_stmt_bind_param(MYSQL_STMT *, MYSQL_BIND *)
{
	return 1;
}

static inline int mysql_stmt_bind_result(MYSQL_STMT *, MYSQL_BIND *)
{
	return 1;
}

static inline int mysql_stmt_execute(MYSQL_STMT *)
{
	return 1;
}

static inline int mysql_stmt_store_result(MYSQL_STMT *)
{
	return 1;
}

static inline int mysql_stmt_fetch(MYSQL_STMT *)
{
	return MYSQL_NO_DATA;
}

static inline my_ulonglong mysql_stmt_affected_rows(MYSQL_STMT *)
{
	return (my_ulonglong)-1;
}

static inline unsigned int mysql_stmt_errno(MYSQL_STMT *)
{
	return ENOTSUP;
}

static inline int mysql_stmt_close(MYSQL_STMT *)
{
	return 0;
}

#endif
