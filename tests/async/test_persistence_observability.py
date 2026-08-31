#!/usr/bin/env python3
"""Runtime coverage for bounded, redacted persistence observability."""

from _paths import SRC
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "persistence/persistence_observability.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>

static int expect(int condition, int code)
{
	return condition ? 0 : code;
}

int main(void)
{
	struct persistence_query_metric metrics[PERSISTENCE_QUERY_SITE_CAPACITY];
	persistence_observability_reset_for_test();
	struct persistence_query_snapshot snapshot =
		persistence_query_snapshot_copy(metrics, PERSISTENCE_QUERY_SITE_CAPACITY);
	if (int failure = expect(snapshot.count == 0 && snapshot.total_calls == 0, 1))
		return failure;

	if (persistence_statement_kind_from_sql("  SELECT 1") != PERSISTENCE_STATEMENT_SELECT ||
	    persistence_statement_kind_from_sql("BEGIN") != PERSISTENCE_STATEMENT_TRANSACTION ||
	    persistence_statement_kind_from_sql("") != PERSISTENCE_STATEMENT_EMPTY)
		return 2;

	const uint64_t durations[] = {100, 101, 500, 501, 1000, 1001, 5000,
				      5001, 10000, 10001, 50000, 50001, 250000, 250001};
	const uint64_t expected_buckets[] = {1, 2, 2, 2, 2, 2, 2, 1};
	for (uint64_t duration : durations)
		persistence_query_record({"safe.cpp", "bucket_site", 10},
					 PERSISTENCE_QUERY_CONTEXT_MAIN,
					 PERSISTENCE_STATEMENT_SELECT, duration, 1, 0, "00000");
	snapshot = persistence_query_snapshot_copy(metrics, PERSISTENCE_QUERY_SITE_CAPACITY);
	if (snapshot.count != 1 || snapshot.total_calls != 14 || metrics[0].max_usec != 250001)
		return 3;
	for (size_t i = 0; i < PERSISTENCE_QUERY_LATENCY_BUCKETS; ++i)
		if (metrics[0].latency_buckets[i] != expected_buckets[i])
			return 4;

	persistence_observability_reset_for_test();
	std::vector<std::thread> workers;
	for (int worker = 0; worker < 8; ++worker)
		workers.emplace_back([]() {
			for (int call = 0; call < 1000; ++call)
				persistence_query_record({"concurrent.cpp", "shared", 20},
							 PERSISTENCE_QUERY_CONTEXT_EVENT_WORKER,
							 PERSISTENCE_STATEMENT_INSERT, 5, 1, 0,
							 "00000");
		});
	for (std::thread &worker : workers)
		worker.join();
	snapshot = persistence_query_snapshot_copy(metrics, PERSISTENCE_QUERY_SITE_CAPACITY);
	if (snapshot.count != 1 || snapshot.total_calls != 8000 || metrics[0].calls != 8000)
		return 5;

	persistence_observability_reset_for_test();
	persistence_query_record({"z.cpp", "tie", 1}, PERSISTENCE_QUERY_CONTEXT_MAIN,
				 PERSISTENCE_STATEMENT_UPDATE, 10, 1, 0, "00000");
	persistence_query_record({"a.cpp", "tie", 1}, PERSISTENCE_QUERY_CONTEXT_MAIN,
				 PERSISTENCE_STATEMENT_UPDATE, 10, 1, 0, "00000");
	snapshot = persistence_query_snapshot_copy(metrics, PERSISTENCE_QUERY_SITE_CAPACITY);
	if (snapshot.count != 2 || strcmp(metrics[0].site, "a.cpp:tie:1") != 0 ||
	    strcmp(metrics[1].site, "z.cpp:tie:1") != 0)
		return 6;
	persistence_observability_reset_for_test();
	persistence_query_record({"same.cpp", "tie", 2},
				 PERSISTENCE_QUERY_CONTEXT_LOCKER_WORKER,
				 PERSISTENCE_STATEMENT_UPDATE, 10, 1, 0, "00000");
	persistence_query_record({"same.cpp", "tie", 2}, PERSISTENCE_QUERY_CONTEXT_MAIN,
				 PERSISTENCE_STATEMENT_DELETE, 10, 1, 0, "00000");
	snapshot = persistence_query_snapshot_copy(metrics, PERSISTENCE_QUERY_SITE_CAPACITY);
	if (snapshot.count != 2 || metrics[0].context != PERSISTENCE_QUERY_CONTEXT_MAIN ||
	    metrics[1].context != PERSISTENCE_QUERY_CONTEXT_LOCKER_WORKER)
		return 12;

	persistence_observability_reset_for_test();
	for (int line = 1; line <= PERSISTENCE_QUERY_SITE_CAPACITY + 1; ++line)
		persistence_query_record({"overflow.cpp", "site", line},
					 PERSISTENCE_QUERY_CONTEXT_MAIN,
					 PERSISTENCE_STATEMENT_DELETE, 1, 1, 0, "00000");
	snapshot = persistence_query_snapshot_copy(metrics, PERSISTENCE_QUERY_SITE_CAPACITY);
	if (snapshot.count != PERSISTENCE_QUERY_SITE_CAPACITY ||
	    snapshot.total_calls != PERSISTENCE_QUERY_SITE_CAPACITY + 1 ||
	    snapshot.registry_overflow != 1)
		return 7;

	uint64_t saturating = UINT64_MAX - 2;
	persistence_counter_saturating_add(&saturating, 10);
	if (saturating != UINT64_MAX)
		return 8;

	const char *canaries[] = {
		"SELECT secret_canary FROM private_table",
		"password=correct-horse-battery-staple",
		"confirmation=849201",
		"ip=203.0.113.77",
		"description=private-item-description",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	};
	struct persistence_query_event event = {};
	event.operation_id = 42;
	event.site = {"safe.cpp", "redacted", 30};
	event.context = PERSISTENCE_QUERY_CONTEXT_LOCKER_WORKER;
	event.kind = PERSISTENCE_STATEMENT_REPLACE;
	event.duration_usec = 123;
	event.error_code = 1064;
	memcpy(event.sqlstate, "42000", 6);
	char diagnostic[512];
	if (persistence_query_event_format(diagnostic, sizeof(diagnostic), &event) < 0)
		return 9;
	for (const char *canary : canaries)
		if (strstr(diagnostic, canary))
			return 10;
	if (!strstr(diagnostic, "operation=42") || !strstr(diagnostic, "error_code=1064") ||
	    !strstr(diagnostic, "sqlstate=42000") || strstr(diagnostic, "SELECT"))
		return 11;

	puts("persistence observability runtime checks passed");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-persistence-observability-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pthread",
            f"-I{SRC}",
            str(harness),
            str(SRC / "persistence_observability.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("persistence observability tests passed")
