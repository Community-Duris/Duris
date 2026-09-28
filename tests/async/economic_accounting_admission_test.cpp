#include "economy/economic_command_admission.h"
#include "economy/economic_currency_adapter.h"
#include "persistence/critical_command_coordinator.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

using error = economic_accounting_error;
critical_operation_id id(uint8_t n)
{
	critical_operation_id value = {};
	value.bytes[0] = n;
	return value;
}
critical_command bank(bool withdraw)
{
	currency_command_payload payload = {};
	payload.pid = 7;
	payload.racewar = 1;
	payload.reason = withdraw ? currency_reason_type::atm_withdraw :
				    currency_reason_type::atm_deposit;
	std::strcpy(payload.account_name.data(), "fixture");
	payload.wallet_delta.amount[0] = withdraw ? 7 : -7;
	payload.bank_delta.amount[0] = -payload.wallet_delta.amount[0];
	critical_command command;
	assert(currency_command_build(&command, id(3), payload, 4, 9, critical_source_site::command,
				      critical_deadline_class::interactive));
	assert(economic_bank_transfer_intent(command, id(2),
					     { id(1), economic_account_kind::wallet, 1001, 0 },
					     { id(1), economic_account_kind::bank, 2001, 1 },
					     &command.accounting_intent) == error::ok);
	command.schema_version = 2;
	command.accepted_at_usec = 123;
	return command;
}
void malformed(const critical_command &command)
{
	assert(economic_command_admission_supported(command));
	assert(!critical_command_valid(command)); // Legacy mutation entrypoints stay closed.
	for (size_t size = 0; size < command.accounting_intent.size(); ++size)
	{
		auto bad = command;
		bad.accounting_intent.resize(size);
		assert(!economic_command_admission_supported(bad));
	}
	for (size_t offset :
	     { size_t(12), size_t(16), size_t(20), size_t(24), size_t(26), size_t(28), size_t(64),
	       size_t(80), size_t(96), size_t(160), size_t(192), size_t(224) })
	{
		auto bad = command;
		bad.accounting_intent[offset] ^= 1;
		assert(!economic_command_admission_supported(bad));
	}
	auto bad = command;
	bad.payload.back() ^= 1;
	assert(!economic_command_admission_supported(bad));
	bad = command;
	if (bad.expected_revisions.empty())
		bad.expected_revisions.push_back({ bad.keys[0], 1 });
	else
		bad.expected_revisions[0].revision++;
	assert(!economic_command_admission_supported(bad));
	bad = command;
	for (uint16_t type = static_cast<uint16_t>(critical_command_type::test);
	     type <= static_cast<uint16_t>(critical_command_type::player_death_restitution); ++type)
	{
		if (type == static_cast<uint16_t>(critical_command_type::account_bank))
			continue;
		bad.type = static_cast<critical_command_type>(type);
		assert(!economic_command_admission_supported(bad));
	}
	bad = command;
	std::swap(bad.keys.front(), bad.keys.back());
	assert(!economic_command_admission_supported(bad));
	bad = command;
	bad.schema_version = 1;
	assert(!economic_command_admission_supported(bad));
	bad = command;
	bad.accepted_at_usec++;
	assert(economic_command_admission_supported(bad));
}
struct execution
{
	critical_command expected;
	std::mutex mutex;
	std::condition_variable changed;
	unsigned calls = 0;
	bool entered = false;
	bool release = false;
	bool unresolved = false;
};
critical_apply_result apply(const critical_command &command, void *context)
{
	auto &state = *static_cast<execution *>(context);
	std::unique_lock lock(state.mutex);
	if (!state.expected.accepted_at_usec)
	{
		assert(command.accepted_at_usec != 0);
		state.expected.accepted_at_usec = command.accepted_at_usec;
	}
	assert(critical_command_equal(command, state.expected));
	assert(economic_command_admission_supported(command));
	++state.calls;
	state.entered = true;
	state.changed.notify_all();
	assert(state.changed.wait_for(lock, std::chrono::seconds(5),
				      [&] { return state.release; }));
	if (state.unresolved)
		return { critical_apply_outcome::ambiguous_commit, 0, 5 };
	return { critical_apply_outcome::already_applied, 11, 0 };
}
template <class Predicate> void wait_for(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
bool retained(critical_command command, void *context)
{
	assert(critical_command_equal(command, *static_cast<critical_command *>(context)));
	return true;
}
bool wrong_backend(const critical_command &) noexcept
{
	return false;
}
critical_apply_result must_not_apply(const critical_command &, void *)
{
	assert(false && "backend without accounting support received a command");
	return { critical_apply_outcome::terminal_failure, 0, EINVAL };
}
void exercise(const std::string &path, critical_command command, bool assign_acceptance = false)
{
	malformed(command);
	if (assign_acceptance)
		command.accepted_at_usec = 0;
	execution state;
	state.expected = command;
	assert(critical_command_coordinator_init(path.c_str(), apply, &state, 1, nullptr, nullptr,
						 economic_command_admission_supported));
	auto bad = command;
	bad.accounting_intent[12] ^= 1;
	assert(critical_command_coordinator_submit(bad) == critical_submit_result::invalid);
	assert(critical_command_journal_health_copy().records == 0);
	assert(critical_command_coordinator_submit(command) ==
	       critical_submit_result::awaiting_durability);
	{
		std::unique_lock lock(state.mutex);
		assert(state.changed.wait_for(lock, std::chrono::seconds(5),
					      [&] { return state.entered; }));
		if (assign_acceptance)
		{
			assert(state.expected.accepted_at_usec != 0);
			assert(state.expected.accounting_intent == command.accounting_intent);
			// All retained replay comparisons below use the exact first timestamp.
			command = state.expected;
		}
	}
	assert(critical_command_coordinator_submit(command) == critical_submit_result::attached);
	auto retry = command;
	retry.accepted_at_usec = 0;
	assert(critical_command_coordinator_submit(retry) == critical_submit_result::attached);
	// A valid, differently bound lifetime/epoch decision conflicts under the original ID.
	economic_frozen_intent changed;
	assert(economic_intent_decode(command.accounting_intent, &changed) == error::ok);
	changed.admission.metadata.epoch = id(42);
	auto conflict = command;
	assert(economic_intent_encode(changed, &conflict.accounting_intent) == error::ok);
	assert(economic_command_admission_supported(conflict));
	assert(critical_command_coordinator_submit(conflict) ==
	       critical_submit_result::identity_conflict);
	for (const auto &key : command.keys)
		assert(critical_command_coordinator_is_fenced(key, nullptr));
	{
		std::lock_guard lock(state.mutex);
		state.release = true;
		state.unresolved = true;
		state.changed.notify_all();
	}
	wait_for(
		[&]
		{
			critical_completion completion;
			critical_command_coordinator_pulse(&completion, 1);
			return critical_command_coordinator_health_copy().blocked == 1;
		});
	for (const auto &key : command.keys)
		assert(critical_command_coordinator_is_fenced(key, nullptr));
	assert(critical_command_journal_health_copy().checkpoints == 0);
	critical_command_coordinator_shutdown();
	assert(critical_command_journal_init(path.c_str()));
	assert(critical_command_journal_replay(retained, &command) ==
	       critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	// A backend that declines this route must not forward retained work or
	// checkpoint it. The default registration remains closed as well.
	assert(!critical_command_coordinator_init(path.c_str(), must_not_apply, nullptr, 1, nullptr,
						  nullptr, wrong_backend));
	critical_command_coordinator_shutdown();
	assert(critical_command_journal_init(path.c_str()));
	assert(critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_journal_replay(retained, &command) ==
	       critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	// A caller without the matching executor refuses replay without checkpointing.
	assert(!critical_command_coordinator_init(path.c_str(), apply, &state, 1));
	critical_command_coordinator_shutdown();
	state.unresolved = false;
	state.calls = 0;
	assert(critical_command_coordinator_init(path.c_str(), apply, &state, 1, nullptr, nullptr,
						 economic_command_admission_supported));
	critical_completion completed = {};
	wait_for([&] { return critical_command_coordinator_pulse(&completed, 1) == 1; });
	assert(completed.outcome == critical_apply_outcome::already_applied &&
	       completed.durable_revision == 11);
	assert(state.calls == 1);
	assert(critical_command_journal_health_copy().checkpoints == 1);
	for (const auto &key : command.keys)
		assert(!critical_command_coordinator_is_fenced(key, nullptr));
	assert(critical_command_coordinator_submit(retry) == critical_submit_result::attached);
	assert(critical_command_coordinator_submit(conflict) ==
	       critical_submit_result::identity_conflict);
	critical_command_coordinator_shutdown();
	// Shutdown must clear the extension registration, including fresh admission.
	assert(critical_command_coordinator_init(path.c_str(), apply, &state, 1));
	assert(critical_command_coordinator_submit(command) == critical_submit_result::invalid);
	critical_command_coordinator_shutdown();
	const std::string wrong = path + "-wrong-backend";
	assert(critical_command_coordinator_init(wrong.c_str(), must_not_apply, nullptr, 1, nullptr,
						 nullptr, wrong_backend));
	assert(critical_command_coordinator_submit(command) == critical_submit_result::invalid);
	assert(critical_command_coordinator_submit_for_publication(command) ==
	       critical_submit_result::invalid);
	assert(critical_command_journal_health_copy().records == 0);
	for (const auto &key : command.keys)
		assert(!critical_command_coordinator_is_fenced(key, nullptr));
	critical_command_coordinator_shutdown();
	const std::string unsupported = path + "-unsupported";
	assert(critical_command_journal_init(unsupported.c_str()));
	// Journal input retains the assigned envelope timestamp even when its intent is invalid.
	bad.accepted_at_usec = command.accepted_at_usec;
	assert(critical_command_journal_append(bad) == critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	assert(!critical_command_coordinator_init(unsupported.c_str(), apply, &state, 1, nullptr,
						  nullptr, economic_command_admission_supported));
	critical_command_coordinator_shutdown();
	assert(critical_command_journal_init(unsupported.c_str()));
	assert(critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_journal_replay(retained, &bad) ==
	       critical_command_journal_result::ok);
	critical_command_journal_shutdown();
}
void publication_acknowledgement(const std::string &path)
{
	auto command = bank(false);
	execution state;
	state.expected = command;
	state.release = true;
	assert(critical_command_coordinator_init(path.c_str(), apply, &state, 1, nullptr, nullptr,
						 economic_command_admission_supported));
	assert(critical_command_coordinator_submit_for_publication(command) ==
	       critical_submit_result::awaiting_durability);
	critical_completion completed = {};
	wait_for([&] { return critical_command_coordinator_pulse(&completed, 1) == 1; });
	assert(completed.outcome == critical_apply_outcome::already_applied &&
	       completed.durable_revision == 11);
	assert(critical_operation_id_equal(completed.operation_id, command.operation_id));
	assert(state.calls == 1);
	assert(critical_command_journal_health_copy().checkpoints == 0);
	for (const auto &key : command.keys)
		assert(critical_command_coordinator_is_fenced(key, nullptr));
	assert(critical_command_coordinator_submit(command) ==
	       critical_submit_result::identity_conflict);
	auto retry = command;
	retry.accepted_at_usec = 0;
	assert(critical_command_coordinator_submit_for_publication(retry) ==
	       critical_submit_result::attached);
	assert(!critical_command_coordinator_acknowledge_publication(id(99)));
	assert(critical_command_journal_health_copy().checkpoints == 0);
	for (const auto &key : command.keys)
		assert(critical_command_coordinator_is_fenced(key, nullptr));
	// This exercises coordinator acknowledgement only, not native live publication.
	assert(critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == 1);
	for (const auto &key : command.keys)
		assert(!critical_command_coordinator_is_fenced(key, nullptr));
	assert(critical_command_coordinator_get_completed(command.operation_id, &completed));
	assert(completed.outcome == critical_apply_outcome::already_applied &&
	       completed.durable_revision == 11);
	assert(!critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_coordinator_submit_for_publication(retry) ==
	       critical_submit_result::attached);
	assert(state.calls == 1);
	critical_command_coordinator_shutdown();
}
int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string path = argv[1];
	std::filesystem::create_directories(path);
	exercise(path + "/deposit", bank(false));
	exercise(path + "/withdraw", bank(true));
	exercise(path + "/assigned-time", bank(false), true);
	publication_acknowledgement(path + "/publication");
	std::cout
		<< "bank accounting coordinator: typed admission, backend refusal, attachment/conflict, unresolved fences, assigned timestamp replay and publication acknowledgement passed\n";
}
