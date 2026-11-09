#include "persistence/critical_command_coordinator.h"
#include "player/player_save_worker.h"

#include <cassert>
#include <deque>
#include <map>
#include <new>
#include <set>
#include <string>

static bool fail_next_allocation = false;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (fail_next_allocation)
	{
		fail_next_allocation = false;
		throw std::bad_alloc();
	}
	return __real__Znwm(size);
}

namespace
{
std::deque<persistence_job_write_fn> jobs;
bool release = false;
unsigned int attempts = 0;
critical_apply_outcome transient;

std::string identity(const critical_operation_id &id)
{
	return std::string(reinterpret_cast<const char *>(id.bytes.data()), id.bytes.size());
}

critical_command request(unsigned int tag)
{
	critical_command command{};
	command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	assert(critical_operation_id_generate(&command.operation_id));
	command.type = critical_command_type::test;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.keys = { { critical_entity_type::player, tag + 1 } };
	command.payload = { static_cast<uint8_t>(tag) };
	return command;
}

critical_apply_result apply(const critical_command &command, void *)
{
	const auto tag = command.payload[0];
	if (tag == 2)
	{
		++attempts;
		if (!release)
			return { transient, 73, 123 };
	}
	critical_apply_result result{ tag % 2 ? critical_apply_outcome::terminal_failure :
						critical_apply_outcome::applied,
				      73, 0 };
	if (tag == 202)
		result.outcome = critical_apply_outcome::already_applied;
	if (tag == 200 || tag == 202)
	{
		result.result_size = 4096;
		for (size_t index = 0; index < result.result_size; ++index)
			result.result_payload[index] = static_cast<uint8_t>((index * 7 + 3) % 251);
	}
	return result;
}

bool pump_one()
{
	assert(!jobs.empty());
	if (jobs.front()().outcome == player_save_apply_outcome::retryable_failure)
		return false;
	jobs.pop_front();
	return true;
}
}

player_save_submit_result persistence_writer_submit(persistence_job_kind, uint64_t, size_t,
						    persistence_job_write_fn write)
{
	jobs.push_back(std::move(write));
	return player_save_submit_result::accepted;
}

int main()
{
	for (auto outcome : { critical_apply_outcome::retryable_failure,
			      critical_apply_outcome::ambiguous_commit })
		for (size_t capacity : { 0, 1, 64 })
		{
			release = false;
			attempts = 0;
			transient = outcome;
			assert(critical_command_coordinator_init(apply, nullptr));
			const auto blocked = request(2);
			assert(critical_command_coordinator_submit(blocked) ==
			       critical_submit_result::accepted);
			std::map<std::string, critical_apply_outcome> expected;
			expected.emplace(identity(blocked.operation_id),
					 critical_apply_outcome::applied);
			for (size_t index = 0; index < capacity; ++index)
			{
				const auto other = request(10 + index);
				expected.emplace(identity(other.operation_id),
						 index % 2 ?
							 critical_apply_outcome::terminal_failure :
							 critical_apply_outcome::applied);
				assert(critical_command_coordinator_submit(other) ==
				       critical_submit_result::accepted);
			}
			critical_completion completions[64]{};
			// Failure while checking the head's fence must retain its writer
			// reservation, without running the backend or fabricating a receipt.
			fail_next_allocation = true;
			assert(!pump_one());
			assert(!fail_next_allocation && attempts == 0);
			assert(critical_command_coordinator_pulse(completions, 64) == 0);
			// Exhausting the retired retry budget cannot turn an uncertain operation
			// into a final failure or let a later write overtake it.
			for (unsigned int index = 0; index < 20; ++index)
			{
				assert(!pump_one());
				assert(critical_command_coordinator_pulse(completions, 64) == 0);
				assert(!critical_command_coordinator_get_completed(
					blocked.operation_id, completions));
				assert(critical_command_coordinator_is_fenced(blocked.keys[0],
									      nullptr));
				assert(jobs.size() == capacity + 1);
			}
			assert(attempts == 20);
			release = true;
			while (!jobs.empty())
				assert(pump_one());
			std::set<std::string> seen;
			const auto consume = [&](size_t count)
			{
				for (size_t index = 0; index < count; ++index)
				{
					const auto id = identity(completions[index].operation_id);
					assert(expected.at(id) == completions[index].outcome);
					assert(seen.insert(id).second);
				}
			};
			assert(critical_command_coordinator_pulse(capacity ? completions : nullptr,
								  capacity) == capacity);
			consume(capacity);
			for (int index = 0; index < 3; ++index)
				assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
			assert(critical_command_coordinator_pulse(completions, 1) == 1);
			consume(1);
			assert(seen.size() == expected.size());
			assert(critical_command_coordinator_pulse(completions, 64) == 0);
			const auto health = critical_command_coordinator_health_copy();
			assert(!health.inflight && !health.fenced_keys && health.retries == 20);
			assert(health.ambiguous ==
			       (outcome == critical_apply_outcome::ambiguous_commit ? 20 : 0));
			critical_command_coordinator_shutdown();
		}

	assert(critical_command_coordinator_init(apply, nullptr));
	for (unsigned int tag : { 200, 202 })
	{
		const auto large = request(tag);
		assert(critical_command_coordinator_submit(large) ==
		       critical_submit_result::accepted);
		assert(pump_one());
		critical_completion delivered{}, cached{};
		assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
		assert(critical_command_coordinator_pulse(&delivered, 1) == 1);
		assert(delivered.result_size == 4096 && delivered.durable_revision == 73);
		for (size_t index = 0; index < delivered.result_size; ++index)
			assert(delivered.result_payload[index] ==
			       static_cast<uint8_t>((index * 7 + 3) % 251));
		assert(critical_command_coordinator_get_completed(large.operation_id, &cached));
		assert(cached.result_size == delivered.result_size &&
		       cached.result_payload == delivered.result_payload);
		assert(critical_command_coordinator_submit(large) ==
		       critical_submit_result::attached);
		assert(jobs.empty());
	}
	critical_command_coordinator_shutdown();
}
