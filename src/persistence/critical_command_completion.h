#ifndef CRITICAL_COMMAND_COMPLETION_H
#define CRITICAL_COMMAND_COMPLETION_H

#include "net/network_wakeup.h"
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

// In-process delivery authority only; this is not part of a command, journal
// frame or stored receipt. Only proven admission refusal may bypass publication
// ACK. Uncertain append and every executed/replayed result remain execution.
enum class critical_completion_disposition : uint8_t
{
	execution = 0,
	never_admitted,
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
	// Reconstructed from durable command entity keys after restart; diagnostics only.
	std::array<char, 33> recovery_correlation = {};
	critical_completion_disposition disposition = critical_completion_disposition::execution;
};

inline bool critical_completion_disposition_valid(const critical_completion &completion) noexcept
{
	if (completion.disposition == critical_completion_disposition::execution)
		return true;
	if (completion.disposition != critical_completion_disposition::never_admitted ||
	    completion.outcome != critical_apply_outcome::terminal_failure ||
	    !completion.error_code || completion.durable_revision || completion.started_at_usec ||
	    completion.failure_stage != critical_failure_stage::none || completion.result_size)
		return false;
	for (uint8_t byte : completion.result_payload)
		if (byte)
			return false;
	return true;
}

// Completion delivery is serialized by the coordinator mutex. It owns bounded
// retention and queue operations, but it does not decide whether an operation
// should be retried or whether a domain may publish a live result.
enum class critical_completion_channel : uint8_t
{
	execution,
	admission_failure,
};

// Actual fixed-receipt queues retain the original deque algorithms/allocator.
// This newly owning data-free type can inspect its genuine protected base;
// existing objects are never cast or interpreted through guessed layout.
class critical_completion_storage_queue final : public std::deque<critical_completion>
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using storage_base =
		std::_Deque_base<critical_completion, std::allocator<critical_completion>>;
#endif
    public:
	using std::deque<critical_completion>::deque;
	using std::deque<critical_completion>::operator=;
	critical_completion_storage_queue() = default;
	critical_completion_storage_queue(const critical_completion_storage_queue &) = default;
	critical_completion_storage_queue(critical_completion_storage_queue &&) = default;
	critical_completion_storage_queue &
	operator=(const critical_completion_storage_queue &) = default;
	critical_completion_storage_queue &
	operator=(critical_completion_storage_queue &&) = default;
	bool current_heap_bytes(size_t *output) const noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (!output)
			return false;
		const auto &actual =
			this->std::_Deque_base<critical_completion,
					       std::allocator<critical_completion>>::_M_impl;
		if (!actual._M_map || !actual._M_map_size ||
		    actual._M_map_size > SIZE_MAX / sizeof(critical_completion *))
			return false;
		size_t bytes = actual._M_map_size * sizeof(critical_completion *);
		const size_t blocks =
			static_cast<size_t>(actual._M_finish._M_node - actual._M_start._M_node) + 1;
		const size_t block_bytes = std::__deque_buf_size(sizeof(critical_completion)) *
					   sizeof(critical_completion);
		if (blocks > (SIZE_MAX - bytes) / block_bytes)
			return false;
		bytes += blocks * block_bytes;
		// Receipts contain fixed scalar/array values; no nested heap is omitted.
		*output = bytes;
		return true;
#else
		(void)output;
		return false;
#endif
	}
};
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && !defined(_GLIBCXX_DEBUG)
static_assert(sizeof(critical_completion_storage_queue) == sizeof(std::deque<critical_completion>));
#endif

class critical_completion_delivery
{
    public:
	bool has_capacity() const noexcept { return size() < CRITICAL_COORDINATOR_MAX_RESULTS; }

	// Caller holds the actual coordinator mutex, as for all ordinary delivery
	// methods. Actual allocated deque map/blocks survive clear/pop as observed;
	// logical receipt count is not physical storage. Fixed object is separate.
	bool current_heap_bytes(size_t *output) const noexcept
	{
		if (!output)
			return false;
		size_t execution = 0, refusal = 0;
		if (!execution_results.current_heap_bytes(&execution) ||
		    !admission_failures.current_heap_bytes(&refusal) ||
		    refusal > SIZE_MAX - execution)
			return false;
		*output = execution + refusal;
		return true;
	}

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
			network_wakeup_notify();
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
		network_wakeup_notify();
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

	critical_completion_storage_queue execution_results;
	critical_completion_storage_queue admission_failures;
};

#endif
