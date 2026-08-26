#include "persistence_observability.h"

#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

struct persistence_query_registry_entry
{
	int used;
	struct persistence_query_metric metric;
};

static pthread_mutex_t persistence_query_registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct persistence_query_registry_entry
	persistence_query_registry[PERSISTENCE_QUERY_SITE_CAPACITY];
static uint64_t persistence_query_next_operation_id;
static uint64_t persistence_query_registry_overflow;
static uint64_t persistence_query_total_calls;
static uint64_t persistence_query_total_failures;

static const uint64_t persistence_query_bucket_limits_usec[PERSISTENCE_QUERY_LATENCY_BUCKETS - 1] = {
	100, 500, 1000, 5000, 10000, 50000, 250000
};

void persistence_counter_saturating_add(uint64_t *counter, uint64_t value)
{
	if (!counter)
		return;
	if (UINT64_MAX - *counter < value)
		*counter = UINT64_MAX;
	else
		*counter += value;
}

uint64_t persistence_observability_now_usec(void)
{
	struct timespec now = {};
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}

static int persistence_keyword_matches(const char *sql, const char *keyword)
{
	while (sql && *sql && isspace((unsigned char)*sql))
		++sql;
	if (!sql)
		return 0;
	for (; *keyword; ++keyword, ++sql)
	{
		if (toupper((unsigned char)*sql) != (unsigned char)*keyword)
			return 0;
	}
	return !*sql || isspace((unsigned char)*sql) || *sql == '(';
}

enum persistence_statement_kind persistence_statement_kind_from_sql(const char *sql)
{
	while (sql && *sql && isspace((unsigned char)*sql))
		++sql;
	if (!sql || !*sql)
		return PERSISTENCE_STATEMENT_EMPTY;
	if (persistence_keyword_matches(sql, "SELECT"))
		return PERSISTENCE_STATEMENT_SELECT;
	if (persistence_keyword_matches(sql, "INSERT"))
		return PERSISTENCE_STATEMENT_INSERT;
	if (persistence_keyword_matches(sql, "UPDATE"))
		return PERSISTENCE_STATEMENT_UPDATE;
	if (persistence_keyword_matches(sql, "DELETE"))
		return PERSISTENCE_STATEMENT_DELETE;
	if (persistence_keyword_matches(sql, "REPLACE"))
		return PERSISTENCE_STATEMENT_REPLACE;
	if (persistence_keyword_matches(sql, "START") ||
	    persistence_keyword_matches(sql, "BEGIN") ||
	    persistence_keyword_matches(sql, "COMMIT") ||
	    persistence_keyword_matches(sql, "ROLLBACK"))
		return PERSISTENCE_STATEMENT_TRANSACTION;
	return PERSISTENCE_STATEMENT_OTHER;
}

const char *persistence_query_context_name(enum persistence_query_context context)
{
	switch (context)
	{
	case PERSISTENCE_QUERY_CONTEXT_MAIN:
		return "main";
	case PERSISTENCE_QUERY_CONTEXT_CHILD:
		return "child";
	case PERSISTENCE_QUERY_CONTEXT_EVENT_WORKER:
		return "event-worker";
	case PERSISTENCE_QUERY_CONTEXT_LOCKER_WORKER:
		return "locker-worker";
	case PERSISTENCE_QUERY_CONTEXT_PLAYER_SAVE_WORKER:
		return "player-save-worker";
	case PERSISTENCE_QUERY_CONTEXT_COUNT:
		break;
	}
	return "unknown";
}

const char *persistence_statement_kind_name(enum persistence_statement_kind kind)
{
	switch (kind)
	{
	case PERSISTENCE_STATEMENT_EMPTY:
		return "empty";
	case PERSISTENCE_STATEMENT_SELECT:
		return "select";
	case PERSISTENCE_STATEMENT_INSERT:
		return "insert";
	case PERSISTENCE_STATEMENT_UPDATE:
		return "update";
	case PERSISTENCE_STATEMENT_DELETE:
		return "delete";
	case PERSISTENCE_STATEMENT_REPLACE:
		return "replace";
	case PERSISTENCE_STATEMENT_TRANSACTION:
		return "transaction";
	case PERSISTENCE_STATEMENT_OTHER:
		return "other";
	case PERSISTENCE_STATEMENT_KIND_COUNT:
		break;
	}
	return "unknown";
}

static void persistence_query_site_format(char *out, size_t out_size,
					  struct persistence_query_site site)
{
	const char *file = site.file ? site.file : "unknown";
	const char *function = site.function ? site.function : "unknown";
	snprintf(out, out_size, "%s:%s:%d", file, function, site.line);
}

static size_t persistence_query_bucket(uint64_t duration_usec)
{
	for (size_t i = 0; i < PERSISTENCE_QUERY_LATENCY_BUCKETS - 1; ++i)
	{
		if (duration_usec <= persistence_query_bucket_limits_usec[i])
			return i;
	}
	return PERSISTENCE_QUERY_LATENCY_BUCKETS - 1;
}

uint64_t persistence_query_record(struct persistence_query_site site,
				  enum persistence_query_context context,
				  enum persistence_statement_kind kind, uint64_t duration_usec,
				  int success, unsigned int error_code, const char *sqlstate)
{
	char site_name[PERSISTENCE_QUERY_SITE_NAME_MAX];
	persistence_query_site_format(site_name, sizeof(site_name), site);

	pthread_mutex_lock(&persistence_query_registry_mutex);
	persistence_counter_saturating_add(&persistence_query_next_operation_id, 1);
	uint64_t operation_id = persistence_query_next_operation_id;
	persistence_counter_saturating_add(&persistence_query_total_calls, 1);
	if (!success)
		persistence_counter_saturating_add(&persistence_query_total_failures, 1);

	struct persistence_query_registry_entry *entry = NULL;
	for (size_t i = 0; i < PERSISTENCE_QUERY_SITE_CAPACITY; ++i)
	{
		if (persistence_query_registry[i].used &&
		    persistence_query_registry[i].metric.context == context &&
		    persistence_query_registry[i].metric.kind == kind &&
		    strcmp(persistence_query_registry[i].metric.site, site_name) == 0)
		{
			entry = &persistence_query_registry[i];
			break;
		}
		if (!entry && !persistence_query_registry[i].used)
			entry = &persistence_query_registry[i];
	}

	if (entry && !entry->used)
	{
		entry->used = 1;
		snprintf(entry->metric.site, sizeof(entry->metric.site), "%s", site_name);
		entry->metric.context = context;
		entry->metric.kind = kind;
	}
	if (entry)
	{
		persistence_counter_saturating_add(&entry->metric.calls, 1);
		if (!success)
			persistence_counter_saturating_add(&entry->metric.failures, 1);
		persistence_counter_saturating_add(&entry->metric.total_usec, duration_usec);
		if (duration_usec > entry->metric.max_usec)
			entry->metric.max_usec = duration_usec;
		persistence_counter_saturating_add(
			&entry->metric.latency_buckets[persistence_query_bucket(duration_usec)], 1);
	}
	else
	{
		persistence_counter_saturating_add(&persistence_query_registry_overflow, 1);
	}
	pthread_mutex_unlock(&persistence_query_registry_mutex);

	(void)error_code;
	(void)sqlstate;
	return operation_id;
}

static int persistence_query_metric_before(const struct persistence_query_metric *left,
					   const struct persistence_query_metric *right)
{
	if (left->total_usec != right->total_usec)
		return left->total_usec > right->total_usec;
	if (left->calls != right->calls)
		return left->calls > right->calls;
	int site_order = strcmp(left->site, right->site);
	if (site_order != 0)
		return site_order < 0;
	if (left->context != right->context)
		return left->context < right->context;
	return left->kind < right->kind;
}

struct persistence_query_snapshot
persistence_query_snapshot_copy(struct persistence_query_metric *metrics, size_t capacity)
{
	struct persistence_query_snapshot snapshot = {};
	snapshot.generated_at_usec = persistence_observability_now_usec();

	pthread_mutex_lock(&persistence_query_registry_mutex);
	snapshot.registry_overflow = persistence_query_registry_overflow;
	snapshot.total_calls = persistence_query_total_calls;
	snapshot.total_failures = persistence_query_total_failures;
	if (metrics)
	{
		for (size_t i = 0; i < PERSISTENCE_QUERY_SITE_CAPACITY && snapshot.count < capacity;
		     ++i)
		{
			if (persistence_query_registry[i].used)
				metrics[snapshot.count++] = persistence_query_registry[i].metric;
		}
	}
	pthread_mutex_unlock(&persistence_query_registry_mutex);

	for (size_t i = 1; i < snapshot.count; ++i)
	{
		struct persistence_query_metric value = metrics[i];
		size_t j = i;
		while (j > 0 && persistence_query_metric_before(&value, &metrics[j - 1]))
		{
			metrics[j] = metrics[j - 1];
			--j;
		}
		metrics[j] = value;
	}
	return snapshot;
}

static void persistence_sqlstate_copy(char out[PERSISTENCE_SQLSTATE_LEN + 1], const char *sqlstate)
{
	for (size_t i = 0; i < PERSISTENCE_SQLSTATE_LEN; ++i)
	{
		unsigned char c = sqlstate && sqlstate[i] ? (unsigned char)sqlstate[i] : '0';
		out[i] = isalnum(c) ? (char)c : '0';
	}
	out[PERSISTENCE_SQLSTATE_LEN] = '\0';
}

int persistence_query_event_format(char *out, size_t out_size,
				   const struct persistence_query_event *event)
{
	if (!out || out_size == 0 || !event)
		return -1;
	char formatted[512];
	char site[PERSISTENCE_QUERY_SITE_NAME_MAX];
	char sqlstate[PERSISTENCE_SQLSTATE_LEN + 1];
	persistence_query_site_format(site, sizeof(site), event->site);
	persistence_sqlstate_copy(sqlstate, event->sqlstate);
	int written = snprintf(formatted, sizeof(formatted),
			       "persistence-query site=%s operation=%llu context=%s kind=%s "
			       "duration_usec=%llu outcome=%s error_code=%u sqlstate=%s",
			       site, (unsigned long long)event->operation_id,
			       persistence_query_context_name(event->context),
			       persistence_statement_kind_name(event->kind),
			       (unsigned long long)event->duration_usec,
			       event->success ? "success" : "failure", event->error_code, sqlstate);
	if (written < 0 || (size_t)written >= sizeof(formatted))
	{
		out[0] = '\0';
		return -1;
	}
	size_t copy_size = (size_t)written + 1;
	if (copy_size > out_size)
	{
		out[0] = '\0';
		return -1;
	}
	memcpy(out, formatted, copy_size);
	return written;
}

void persistence_observability_reset_for_test(void)
{
	pthread_mutex_lock(&persistence_query_registry_mutex);
	memset(persistence_query_registry, 0, sizeof(persistence_query_registry));
	persistence_query_next_operation_id = 0;
	persistence_query_registry_overflow = 0;
	persistence_query_total_calls = 0;
	persistence_query_total_failures = 0;
	pthread_mutex_unlock(&persistence_query_registry_mutex);
}
