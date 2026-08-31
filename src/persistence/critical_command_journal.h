#ifndef CRITICAL_COMMAND_JOURNAL_H
#define CRITICAL_COMMAND_JOURNAL_H

#include "persistence/critical_command.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA = 256 * 1024 * 1024;
constexpr size_t CRITICAL_COMMAND_JOURNAL_MAX_RECORDS = 4096;

enum class critical_command_journal_result : uint8_t
{
	ok,
	not_initialized,
	invalid,
	io_failure,
	unsafe_permissions,
	quota_exceeded,
	corrupt_data,
	replay_blocked,
};

struct critical_command_journal_health
{
	uint64_t records;
	uint64_t bytes;
	uint64_t oldest_age_msec;
	uint64_t appends;
	uint64_t checkpoints;
	uint64_t replays;
	uint64_t duplicates;
	uint64_t corrupt_records;
	uint64_t io_failures;
	critical_command_journal_result last_result;
	bool quota_exceeded;
	bool initialized;
};

using critical_command_replay_fn = bool (*)(critical_command command, void *context);

bool critical_command_journal_init(const char *directory,
				   size_t quota_bytes = CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA);
void critical_command_journal_shutdown(void);
critical_command_journal_result critical_command_journal_append(const critical_command &command);
critical_command_journal_result
critical_command_journal_checkpoint(const critical_operation_id &operation_id);
critical_command_journal_result critical_command_journal_replay(critical_command_replay_fn replay,
								void *context);
critical_command_journal_health critical_command_journal_health_copy(void);
const char *critical_command_journal_result_name(critical_command_journal_result result);
void critical_command_journal_reset_for_tests(void);

#endif
