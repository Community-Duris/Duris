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
};

struct critical_completion
{
	critical_operation_id operation_id;
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
    public:
	bool has_capacity() const noexcept { return size() < CRITICAL_COORDINATOR_MAX_RESULTS; }

	size_t size() const noexcept
	{
		return execution_results.size() + admission_failures.size();
	}

	size_t size(critical_completion_channel channel) const noexcept
	{
		return channel == critical_completion_channel::execution ?
			       execution_results.size() :
			       admission_failures.size();
	}

	bool try_enqueue(critical_completion_channel channel,
			 const critical_completion &completion) noexcept
	{
		if (!has_capacity())
			return false;
		try
		{
			queue(channel).push_back(completion);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}

	// The execution worker calls this only after waiting for capacity. Keeping
	// the non-throwing admission path above separate means a completed command
	// is never silently discarded if allocation fails at this boundary.
	void enqueue(critical_completion_channel channel, const critical_completion &completion)
	{
		queue(channel).push_back(completion);
	}

	const critical_completion *front(critical_completion_channel channel) const noexcept
	{
		const auto &queue = channel == critical_completion_channel::execution ?
					    execution_results :
					    admission_failures;
		return queue.empty() ? nullptr : &queue.front();
	}

	void pop_front(critical_completion_channel channel) noexcept
	{
		auto &queue = this->queue(channel);
		if (!queue.empty())
			queue.pop_front();
	}

	void clear() noexcept
	{
		execution_results.clear();
		admission_failures.clear();
	}

    private:
	std::deque<critical_completion> &queue(critical_completion_channel channel) noexcept
	{
		return channel == critical_completion_channel::execution ? execution_results :
									   admission_failures;
	}

	std::deque<critical_completion> execution_results;
	std::deque<critical_completion> admission_failures;
};

#endif
