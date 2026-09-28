#include "economy/economic_accounting_intent.h"
#include "persistence/critical_command_coordinator.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <thread>

namespace
{
std::atomic<unsigned> applied = 0;
critical_apply_result apply(const critical_command &command, void *)
{
	assert(critical_command_valid(command));
	++applied;
	return { critical_apply_outcome::applied, 1, 0 };
}
bool observe(const critical_command &command, void *raw)
{
	assert(critical_command_valid(command));
	++*static_cast<unsigned *>(raw);
	return true;
}
using file_snapshot = std::map<std::string, std::vector<uint8_t>>;
file_snapshot snapshot(const std::filesystem::path &root)
{
	file_snapshot files;
	for (const auto &entry : std::filesystem::recursive_directory_iterator(root))
	{
		if (!entry.is_regular_file())
			continue;
		std::ifstream input(entry.path(), std::ios::binary);
		assert(input);
		files.emplace(entry.path().lexically_relative(root).string(),
			      std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {}));
		assert(!input.bad());
	}
	return files;
}
struct replay_expectation
{
	std::vector<critical_command> commands;
	size_t seen = 0;
};
bool retained(critical_command command, void *raw)
{
	auto &expected = *static_cast<replay_expectation *>(raw);
	assert(expected.seen < expected.commands.size());
	const auto &original = expected.commands[expected.seen++];
	std::vector<uint8_t> before, after;
	assert(critical_command_encode(original, &before) == critical_command_codec_result::ok);
	assert(critical_command_encode(command, &after) == critical_command_codec_result::ok);
	assert(before == after);
	assert(critical_command_valid(command) == critical_command_valid(original));
	return true;
}
void seed(const std::string &path, const std::vector<critical_command> &commands)
{
	assert(critical_command_journal_init(path.c_str()));
	for (const auto &command : commands)
		assert(critical_command_journal_append(command) ==
		       critical_command_journal_result::ok);
	assert(critical_command_journal_health_copy().records == commands.size());
	critical_command_journal_shutdown();
}
void blocked_replay(const std::string &path, const std::vector<critical_command> &commands)
{
	seed(path, commands);
	const auto original_files = snapshot(path);
	unsigned observed = 0;
	assert(!critical_command_coordinator_init(path.c_str(), apply, nullptr, 1, observe,
						  &observed));
	assert(applied == 0 && !critical_command_coordinator_health_copy().initialized);
	// Startup callers shut down the failed coordinator to discard queued legacy state.
	critical_command_coordinator_shutdown();
	const auto stopped = critical_command_coordinator_health_copy();
	assert(!stopped.initialized && !stopped.running && !stopped.accepting);
	assert(stopped.queued == 0 && stopped.inflight == 0 && stopped.fenced_keys == 0);
	assert(snapshot(path) == original_files);
	assert(critical_command_journal_init(path.c_str()));
	assert(critical_command_journal_health_copy().records == commands.size());
	assert(critical_command_journal_health_copy().checkpoints == 0);
	replay_expectation expected{ commands, 0 };
	assert(critical_command_journal_replay(retained, &expected) ==
	       critical_command_journal_result::ok);
	assert(expected.seen == commands.size());
	assert(critical_command_journal_health_copy().records == commands.size());
	assert(critical_command_journal_health_copy().checkpoints == 0);
	critical_command_journal_shutdown();
	assert(snapshot(path) == original_files);
	// Observers run per record before full validation; no apply worker starts on failure.
	std::cout << std::filesystem::path(path).filename().string()
		  << ": apply callbacks=0, replay observers=" << observed
		  << ", original journal bytes retained\n";
}
} // namespace
int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::filesystem::path root(argv[1]);
	critical_command legacy = {};
	legacy.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	legacy.operation_id.bytes[0] = 1;
	legacy.type = critical_command_type::account_bank;
	legacy.payload_version = 1;
	legacy.source_site = critical_source_site::command;
	legacy.deadline_class = critical_deadline_class::interactive;
	legacy.accepted_at_usec = 123;
	legacy.keys = { { critical_entity_type::player, 7 } };
	legacy.payload = { 1, 2, 3 };
	assert(critical_command_valid(legacy));
	critical_command accounting = legacy;
	accounting.operation_id.bytes[0] = 2;
	economic_admission_facts facts;
	facts.metadata.lineage.bytes[0] = 2;
	facts.metadata.epoch.bytes[0] = 3;
	facts.metadata.actor_kind = economic_actor_kind::domain;
	facts.metadata.actor_id = 7;
	facts.metadata.writer_id = 1;
	facts.metadata.reason = economic_reason::wallet_transfer;
	assert(economic_intent_freeze(accounting, facts, &accounting.accounting_intent) ==
	       economic_accounting_error::ok);
	accounting.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(accounting) && !critical_command_valid(accounting));
	const auto admission_path = (root / "fresh-admission").string();
	assert(critical_command_coordinator_init(admission_path.c_str(), apply, nullptr, 1));
	assert(critical_command_coordinator_submit(accounting) == critical_submit_result::invalid);
	assert(critical_command_coordinator_submit_for_publication(accounting) ==
	       critical_submit_result::invalid);
	critical_command hidden = accounting;
	hidden.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	assert(critical_command_coordinator_submit(hidden) == critical_submit_result::invalid);
	assert(critical_command_coordinator_submit_for_publication(hidden) ==
	       critical_submit_result::invalid);
	assert(critical_command_coordinator_health_copy().accepted == 0);
	assert(critical_command_journal_health_copy().records == 0);
	critical_command_coordinator_shutdown();
	assert(applied == 0);
	blocked_replay((root / "accounting-only").string(), { accounting });
	blocked_replay((root / "legacy-first").string(), { legacy, accounting });
	blocked_replay((root / "accounting-first").string(), { accounting, legacy });
	const auto legacy_path = (root / "legacy-only").string();
	seed(legacy_path, { legacy });
	unsigned observed = 0;
	assert(critical_command_coordinator_init(legacy_path.c_str(), apply, nullptr, 1, observe,
						 &observed));
	assert(observed == 1);
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	critical_completion completion = {};
	bool completed = false;
	while (std::chrono::steady_clock::now() < deadline)
	{
		if (critical_command_coordinator_pulse(&completion, 1) == 1)
		{
			completed = true;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	assert(completed && completion.outcome == critical_apply_outcome::applied);
	assert(critical_operation_id_equal(completion.operation_id, legacy.operation_id));
	assert(applied == 1);
	critical_command_coordinator_shutdown();
	std::cout
		<< "both admission APIs reject accounting envelopes; legacy-only replay succeeds\n";
}
