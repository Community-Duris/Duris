// Weak stubs for the server functions that native harnesses most often do not link.
//
// Compile this file with a harness instead of copying a stub into it. Every definition is
// weak: the real source, when the harness links it, or a harness's own definition (one
// that counts calls, keeps the output or returns a value) replaces it at link time. A
// harness therefore keeps a local stub only where it must behave differently.
#include "core/prototypes.h"
#include "persistence/persistence_log.h"
#include "player/player_save_worker.h"

#include <cstdlib>
#include <cerrno>
#include <cstdarg>
#include <cstdio>

#define HARNESS_STUB __attribute__((weak))

HARNESS_STUB int checked_snprintf(char *buffer, size_t size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vsnprintf(buffer, size, format, args);
	va_end(args);
	return ret;
}

HARNESS_STUB void logit(const char *, const char *, ...) {}
HARNESS_STUB bool sql_async_booting(void)
{
	return true;
}
HARNESS_STUB player_save_submit_result persistence_writer_submit(persistence_job_kind, uint64_t,
								 size_t, persistence_job_write_fn)
{
	errno = ESHUTDOWN;
	return player_save_submit_result::unavailable;
}
HARNESS_STUB void debug(const char *, ...) {}
HARNESS_STUB void wizlog(int, const char *, ...) {}
HARNESS_STUB void statuslog(int, const char *, ...) {}
HARNESS_STUB bool persistence_log_submit(const char *)
{
	return true;
}
HARNESS_STUB void sql_log(P_char, const char *, const char *, ...) {}
HARNESS_STUB void persistence_alert(int, const char *, const char *, const char *, const char *,
				    const char *, const char *, ...)
{
}
HARNESS_STUB bool persistence_trace_enabled(void)
{
	return false;
}
HARNESS_STUB void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
HARNESS_STUB int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

HARNESS_STUB void send_to_char(const char *, P_char) {}
HARNESS_STUB void send_to_char(const char *, P_char, int) {}
HARNESS_STUB void act(const char *, int, P_char, P_obj, void *, int) {}
HARNESS_STUB void update_pos(P_char) {}
HARNESS_STUB void updateShortAffects(P_char) {}
HARNESS_STUB int process_proxy_protocol(P_desc)
{
	return 0;
}
HARNESS_STUB P_char get_linked_char(P_char, ush_int)
{
	return nullptr;
}
HARNESS_STUB bool has_innate(P_char, int)
{
	return false;
}
HARNESS_STUB bool notch_skill(P_char, int, float)
{
	return false;
}
HARNESS_STUB bool affected_by_spell(P_char, int)
{
	return false;
}
HARNESS_STUB bool isname(const char *, const char *)
{
	return false;
}

// sql/sql_pool.h needs <mysql.h>; the pool's functions have C linkage, so these need not.
extern "C"
{
	typedef struct st_mysql MYSQL;
	HARNESS_STUB MYSQL *sql_pool_acquire(void)
	{
		return nullptr;
	}
	HARNESS_STUB void sql_pool_release(MYSQL *) {}
	HARNESS_STUB MYSQL *sql_pool_replace_connection(MYSQL *)
	{
		return nullptr;
	}
}
