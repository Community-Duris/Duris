#include "telemetry/telemetry_queue_private.h"

#include <limits>

namespace telemetry_queue_private
{

void initialize(queue *value, std::uint32_t capacity, std::uint32_t control_reserve) noexcept
{
	if (value == nullptr)
		return;
	value->producer_head.store(0U, std::memory_order_relaxed);
	value->consumer_tail.store(0U, std::memory_order_relaxed);
	value->retained_count.store(0U, std::memory_order_relaxed);
	value->retained_control_count.store(0U, std::memory_order_relaxed);
	value->capacity = capacity;
	value->control_reserve = control_reserve;
}

void reset(queue *value) noexcept
{
	if (value == nullptr)
		return;
	value->producer_head.store(0U, std::memory_order_relaxed);
	value->consumer_tail.store(0U, std::memory_order_relaxed);
	value->retained_count.store(0U, std::memory_order_relaxed);
	value->retained_control_count.store(0U, std::memory_order_relaxed);
}

std::uint32_t physical_depth(const queue *value) noexcept
{
	if (value == nullptr || value->capacity == 0U)
		return 0U;
	const std::uint64_t head = value->producer_head.load(std::memory_order_acquire);
	const std::uint64_t tail = value->consumer_tail.load(std::memory_order_acquire);
	if (head < tail || head - tail > value->capacity)
		return 0U;
	return static_cast<std::uint32_t>(head - tail);
}

push_result try_push(queue *value, const telemetry_record &record,
		     telemetry_monotonic_usec admitted_at_monotonic_usec, bool admitted_at_valid,
		     bool control) noexcept
{
	push_result result{};
	if (value == nullptr || value->capacity == 0U || value->capacity > value->slots.size() ||
	    value->control_reserve >= value->capacity)
		return result;

	const std::uint32_t current_depth = value->retained_count.load(std::memory_order_acquire);
	const std::uint32_t limit = control ? value->capacity :
					      value->capacity - value->control_reserve;
	if (current_depth >= limit || current_depth >= value->capacity)
	{
		result.depth = current_depth;
		result.control_depth =
			value->retained_control_count.load(std::memory_order_acquire);
		return result;
	}

	const std::uint64_t head = value->producer_head.load(std::memory_order_relaxed);
	const std::uint64_t tail = value->consumer_tail.load(std::memory_order_acquire);
	if (head < tail || head - tail >= value->capacity ||
	    head == std::numeric_limits<std::uint64_t>::max())
	{
		result.depth = current_depth;
		result.control_depth =
			value->retained_control_count.load(std::memory_order_acquire);
		return result;
	}

	value->retained_count.fetch_add(1U, std::memory_order_release);
	if (control)
		value->retained_control_count.fetch_add(1U, std::memory_order_release);
	queue_slot &slot = value->slots[head % value->capacity];
	slot.record = record;
	slot.admitted_at_monotonic_usec = admitted_at_monotonic_usec;
	slot.admitted_at_valid = admitted_at_valid ? 1U : 0U;
	for (std::uint8_t &byte : slot.reserved)
		byte = 0U;
	value->producer_head.store(head + 1U, std::memory_order_release);

	result.accepted = true;
	result.depth = value->retained_count.load(std::memory_order_acquire);
	result.control_depth = value->retained_control_count.load(std::memory_order_acquire);
	return result;
}

bool peek(const queue *value, std::size_t offset, telemetry_record *record,
	  telemetry_monotonic_usec *admitted_at_monotonic_usec, bool *admitted_at_valid) noexcept
{
	if (value == nullptr || record == nullptr || value->capacity == 0U)
		return false;
	const std::uint64_t tail = value->consumer_tail.load(std::memory_order_relaxed);
	const std::uint64_t head = value->producer_head.load(std::memory_order_acquire);
	if (head < tail || offset >= head - tail || offset >= value->capacity)
		return false;
	const queue_slot &slot = value->slots[(tail + offset) % value->capacity];
	*record = slot.record;
	if (admitted_at_monotonic_usec != nullptr)
		*admitted_at_monotonic_usec = slot.admitted_at_monotonic_usec;
	if (admitted_at_valid != nullptr)
		*admitted_at_valid = slot.admitted_at_valid != 0U;
	return true;
}

bool commit(queue *value, std::size_t count, std::size_t control_count) noexcept
{
	if (value == nullptr || value->capacity == 0U || count == 0U || control_count > count)
		return false;
	const std::uint64_t tail = value->consumer_tail.load(std::memory_order_relaxed);
	const std::uint64_t head = value->producer_head.load(std::memory_order_acquire);
	const std::uint32_t retained = value->retained_count.load(std::memory_order_acquire);
	const std::uint32_t retained_control =
		value->retained_control_count.load(std::memory_order_acquire);
	if (head < tail || count > head - tail || count > retained ||
	    control_count > retained_control)
		return false;
	value->consumer_tail.store(tail + count, std::memory_order_release);
	value->retained_count.fetch_sub(static_cast<std::uint32_t>(count),
					std::memory_order_release);
	if (control_count != 0U)
		value->retained_control_count.fetch_sub(static_cast<std::uint32_t>(control_count),
							std::memory_order_release);
	return true;
}

std::uint32_t depth(const queue *value) noexcept
{
	return value == nullptr ? 0U : value->retained_count.load(std::memory_order_acquire);
}

std::uint32_t control_depth(const queue *value) noexcept
{
	return value == nullptr ? 0U :
				  value->retained_control_count.load(std::memory_order_acquire);
}

} // namespace telemetry_queue_private
