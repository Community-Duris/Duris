#ifndef DURIS_TELEMETRY_QUEUE_PRIVATE_H
#define DURIS_TELEMETRY_QUEUE_PRIVATE_H

#include "telemetry/telemetry_types.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

/*
 * Private SPSC storage for #262.  The producer is the game/runtime owner and
 * the consumer is the one transport worker.  The fixed slots are never
 * allocated, moved, or overwritten while a retained prefix is in flight.
 */
namespace telemetry_queue_private
{

struct queue_slot
{
	telemetry_record record;
	telemetry_monotonic_usec admitted_at_monotonic_usec;
	std::uint8_t admitted_at_valid;
	std::uint8_t reserved[7];
};

struct queue
{
	/* producer_head is published after the slot copy; consumer_tail is advanced
	 * only after the worker's immutable batch has resolved. */
	std::atomic<std::uint64_t> producer_head{ 0U };
	std::atomic<std::uint64_t> consumer_tail{ 0U };
	std::atomic<std::uint32_t> retained_count{ 0U };
	std::atomic<std::uint32_t> retained_control_count{ 0U };
	std::uint32_t capacity = 0U;
	std::uint32_t control_reserve = 0U;
	std::array<queue_slot, TELEMETRY_QUEUE_CAPACITY_PROPOSAL> slots{};
};

struct push_result
{
	bool accepted;
	std::uint32_t depth;
	std::uint32_t control_depth;
};

void initialize(queue *value, std::uint32_t capacity, std::uint32_t control_reserve) noexcept;
void reset(queue *value) noexcept;
push_result try_push(queue *value, const telemetry_record &record,
		     telemetry_monotonic_usec admitted_at_monotonic_usec, bool admitted_at_valid,
		     bool control) noexcept;
bool peek(const queue *value, std::size_t offset, telemetry_record *record,
	  telemetry_monotonic_usec *admitted_at_monotonic_usec, bool *admitted_at_valid) noexcept;
bool commit(queue *value, std::size_t count, std::size_t control_count) noexcept;
std::uint32_t depth(const queue *value) noexcept;
std::uint32_t control_depth(const queue *value) noexcept;
std::uint32_t physical_depth(const queue *value) noexcept;

} // namespace telemetry_queue_private

#endif
