#ifndef __PERSISTENCE_OBSERVABILITY_H_INCLUDED__
#define __PERSISTENCE_OBSERVABILITY_H_INCLUDED__

#include <stddef.h>
#include <stdint.h>

#define PERSISTENCE_QUERY_SITE_CAPACITY 256
#define PERSISTENCE_QUERY_SITE_NAME_MAX 192
#define PERSISTENCE_QUERY_LATENCY_BUCKETS 8
#define PERSISTENCE_SQLSTATE_LEN 5

#define PERSISTENCE_STRINGIFY_INNER(value) #value
#define PERSISTENCE_STRINGIFY(value) PERSISTENCE_STRINGIFY_INNER(value)
#define PERSISTENCE_QUERY_SITE               \
	{                                    \
		__FILE__, __func__, __LINE__ \
	}

enum persistence_query_context
{
	PERSISTENCE_QUERY_CONTEXT_MAIN = 0,
	PERSISTENCE_QUERY_CONTEXT_CHILD,
	PERSISTENCE_QUERY_CONTEXT_EVENT_WORKER,
	PERSISTENCE_QUERY_CONTEXT_LOCKER_WORKER,
	PERSISTENCE_QUERY_CONTEXT_PLAYER_SAVE_WORKER,
	PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER,
	PERSISTENCE_QUERY_CONTEXT_COUNT
};

enum persistence_statement_kind
{
	PERSISTENCE_STATEMENT_EMPTY = 0,
	PERSISTENCE_STATEMENT_SELECT,
	PERSISTENCE_STATEMENT_INSERT,
	PERSISTENCE_STATEMENT_UPDATE,
	PERSISTENCE_STATEMENT_DELETE,
	PERSISTENCE_STATEMENT_REPLACE,
	PERSISTENCE_STATEMENT_TRANSACTION,
	PERSISTENCE_STATEMENT_OTHER,
	PERSISTENCE_STATEMENT_KIND_COUNT
};

struct persistence_query_site
{
	const char *file;
	const char *function;
	int line;
};

struct persistence_query_event
{
	uint64_t operation_id;
	struct persistence_query_site site;
	enum persistence_query_context context;
	enum persistence_statement_kind kind;
	uint64_t duration_usec;
	unsigned int error_code;
	char sqlstate[PERSISTENCE_SQLSTATE_LEN + 1];
	int success;
};

struct persistence_query_metric
{
	char site[PERSISTENCE_QUERY_SITE_NAME_MAX];
	enum persistence_query_context context;
	enum persistence_statement_kind kind;
	uint64_t calls;
	uint64_t failures;
	uint64_t total_usec;
	uint64_t max_usec;
	uint64_t latency_buckets[PERSISTENCE_QUERY_LATENCY_BUCKETS];
};

struct persistence_query_snapshot
{
	size_t count;
	uint64_t registry_overflow;
	uint64_t total_calls;
	uint64_t total_failures;
	uint64_t generated_at_usec;
};

struct persistence_deferred_save_snapshot
{
	uint64_t pending;
	uint64_t scheduled;
	uint64_t failed_unscheduled;
	uint64_t attempts;
	uint64_t failures;
	uint64_t oldest_age_msec;
};

struct persistence_dirty_save_snapshot
{
	int enabled;
	int available;
	uint64_t active_count;
	uint64_t inflight_count;
	uint64_t active_oldest_age_msec;
	uint64_t inflight_oldest_age_msec;
};

struct persistence_queue_health_snapshot
{
	uint64_t pending;
	uint64_t dropped;
	uint64_t written;
	uint64_t failures;
	uint64_t heartbeat_age_msec;
	int running;
	int stop_pending;
	int heartbeat_available;
};

uint64_t persistence_observability_now_usec(void);
enum persistence_statement_kind persistence_statement_kind_from_sql(const char *sql);
const char *persistence_query_context_name(enum persistence_query_context context);
const char *persistence_statement_kind_name(enum persistence_statement_kind kind);
uint64_t persistence_query_record(struct persistence_query_site site,
				  enum persistence_query_context context,
				  enum persistence_statement_kind kind, uint64_t duration_usec,
				  int success, unsigned int error_code, const char *sqlstate);
struct persistence_query_snapshot
persistence_query_snapshot_copy(struct persistence_query_metric *metrics, size_t capacity);
struct persistence_deferred_save_snapshot persistence_deferred_save_snapshot_copy(void);
int persistence_query_event_format(char *out, size_t out_size,
				   const struct persistence_query_event *event);
void persistence_counter_saturating_add(uint64_t *counter, uint64_t value);
void persistence_observability_reset_for_test(void);

#endif
