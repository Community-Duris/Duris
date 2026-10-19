#ifndef CRITICAL_COMMAND_COMPLETION_H
#define CRITICAL_COMMAND_COMPLETION_H

#include "persistence/critical_command.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <new>

constexpr size_t CRITICAL_COORDINATOR_MAX_RESULTS = 2048;
// Match the durable inbox result bound; boon rewards already encode 2080 bytes.
constexpr size_t CRITICAL_COMPLETION_RESULT_MAX_BYTES = 4096;

enum class critical_apply_outcome : uint8_t
{
	applied,
	already_applied,
	retryable_failure,
	ambiguous_commit,
	terminal_failure,
};

struct critical_apply_result
{
	critical_apply_outcome outcome;
	uint64_t durable_revision;
	unsigned int error_code;
	critical_failure_stage failure_stage = critical_failure_stage::none;
	uint16_t result_size = 0;
	std::array<uint8_t, CRITICAL_COMPLETION_RESULT_MAX_BYTES> result_payload = {};
	std::array<char, 1024> failure_detail = {};
};

struct critical_completion
{
	critical_operation_id operation_id;
	critical_command_type type = {};
	critical_apply_outcome outcome;
	uint64_t durable_revision;
	unsigned int error_code;
	unsigned int attempt;
	uint64_t queued_at_usec;
	uint64_t started_at_usec;
	uint64_t completed_at_usec;
	critical_failure_stage failure_stage = critical_failure_stage::none;
	uint16_t result_size = 0;
	std::array<uint8_t, CRITICAL_COMPLETION_RESULT_MAX_BYTES> result_payload = {};
	// Reconstructed from durable command entity keys after restart; diagnostics only.
	std::array<char, 33> recovery_correlation = {};
	std::array<char, 2048> failure_detail = {};
};

// Completion delivery is serialized by the coordinator mutex. It owns bounded
// retention and queue operations, but it does not decide whether an operation
// should be retried or whether a domain may publish a live result.
enum class critical_completion_channel : uint8_t
{
	execution,
	admission_failure,
};

class critical_completion_delivery
{
	struct ring
	{
		std::array<critical_completion, CRITICAL_COORDINATOR_MAX_RESULTS> entries{};
		size_t head = 0;
		size_t count = 0;
	};

    public:
	bool has_capacity() const noexcept { return size() < CRITICAL_COORDINATOR_MAX_RESULTS; }
	size_t size() const noexcept { return execution_results.count + admission_failures.count; }
	size_t size(critical_completion_channel channel) const noexcept
	{
		return queue(channel).count;
	}
	bool try_enqueue(critical_completion_channel channel,
			 const critical_completion &completion) noexcept
	{
		if (!has_capacity())
			return false;
		auto &selected = queue(channel);
		selected.entries[(selected.head + selected.count) % selected.entries.size()] =
			completion;
		++selected.count;
		return true;
	}
	void enqueue(critical_completion_channel channel, const critical_completion &completion)
	{
		if (!try_enqueue(channel, completion))
			throw std::bad_alloc();
	}
	const critical_completion *front(critical_completion_channel channel) const noexcept
	{
		const auto &selected = queue(channel);
		return selected.count ? &selected.entries[selected.head] : nullptr;
	}
	void pop_front(critical_completion_channel channel) noexcept
	{
		auto &selected = queue(channel);
		if (selected.count)
		{
			selected.head = (selected.head + 1) % selected.entries.size();
			--selected.count;
		}
	}
	void clear() noexcept
	{
		execution_results.head = execution_results.count = 0;
		admission_failures.head = admission_failures.count = 0;
	}

    private:
	ring &queue(critical_completion_channel channel) noexcept
	{
		return channel == critical_completion_channel::execution ? execution_results :
									   admission_failures;
	}
	const ring &queue(critical_completion_channel channel) const noexcept
	{
		return channel == critical_completion_channel::execution ? execution_results :
									   admission_failures;
	}
	ring execution_results;
	ring admission_failures;
};

#endif
