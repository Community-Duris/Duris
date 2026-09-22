// The wrapper supplies the existing phase8 helper definitions, excluding main.
// This reuses its native identity, metadata, account and retained-record fixtures.
#include "phase8_bank_fixture.h"
#include "flatfile/flatfile_accounting_dispatch.h"
#include "flatfile/flatfile_item_repository.h"
#include "economy/economic_command_admission.h"
#include "persistence/persistence_mode.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

const char *persistence_mode_flatfile_root()
{
	return nullptr;
}
critical_apply_result flatfile_critical_command_repository_apply_selected(const critical_command &,
									  void *)
{
	assert(false && "native accounting journey must never enter legacy dispatcher");
	return { outcome::terminal_failure, 0, EINVAL };
}
struct native_execution
{
	std::string root;
	std::atomic<unsigned> calls{ 0 };
	std::mutex mutex;
	critical_command exact;
};
critical_apply_result native_apply(const critical_command &cmd, void *context)
{
	auto &execution = *static_cast<native_execution *>(context);
	{
		std::lock_guard lock(execution.mutex);
		execution.exact = cmd;
	}
	++execution.calls;
	return flatfile_accounting_apply_selected(cmd, execution.root.data());
}
critical_completion await_completion()
{
	critical_completion completion{};
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
	while (!critical_command_coordinator_pulse(&completion, 1))
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return completion;
}
critical_command executed(native_execution &execution)
{
	std::lock_guard lock(execution.mutex);
	return execution.exact;
}
void assert_completion_same(const critical_completion &a, const critical_completion &b)
{
	assert(critical_operation_id_equal(a.operation_id, b.operation_id) &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.failure_stage == b.failure_stage && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload);
}
struct journal_observation
{
	critical_command expected;
	unsigned calls = 0;
};
bool observe_journal(critical_command cmd, void *context)
{
	auto &value = *static_cast<journal_observation *>(context);
	assert(critical_command_equal(cmd, value.expected));
	++value.calls;
	return true;
}
void journal_contains(const std::string &path, const critical_command &expected, unsigned count)
{
	assert(critical_command_journal_init(path.c_str()));
	journal_observation observation{ expected };
	assert(critical_command_journal_replay(observe_journal, &observation) ==
	       critical_command_journal_result::ok);
	assert(observation.calls == count);
	critical_command_journal_shutdown();
}
int main(int argc, char **argv)
{
	assert(argc == 2);
	const fs::path base = argv[1];
	fs::create_directories(base);
	fs::permissions(base, fs::perms::owner_all);
	native_execution execution;
	execution.root = (base / "authority").string();
	setup(execution.root);
	const auto journal = (base / "journal").string();
	auto cmd = command(execution.root, 100);
	cmd.accepted_at_usec = 0;
	assert(critical_command_coordinator_init(journal.c_str(), native_apply, &execution, 1,
						 nullptr, nullptr,
						 economic_command_admission_supported));
	assert(critical_command_coordinator_submit_for_publication(cmd) ==
	       critical_submit_result::awaiting_durability);
	const auto first = await_completion();
	assert(first.outcome == outcome::applied && execution.calls == 1);
	const auto exact = executed(execution);
	assert(exact.accepted_at_usec && exact.accounting_intent == cmd.accounting_intent &&
	       critical_operation_id_equal(exact.operation_id, cmd.operation_id));
	assert(critical_command_equal(retained(execution.root, exact).command, exact));
	const auto first_state = state(execution.root);
	assert(first_state.domains.wallet[0] == 101 && first_state.domains.bank[0] == 99);
	assert(critical_command_journal_health_copy().checkpoints == 0);
	for (const auto &key : exact.keys)
		assert(critical_command_coordinator_is_fenced(key, nullptr));
	critical_command_coordinator_shutdown(); // Deliberately no publication acknowledgement.
	journal_contains(journal, exact, 1);

	assert(critical_command_coordinator_init(journal.c_str(), native_apply, &execution, 1,
						 nullptr, nullptr,
						 economic_command_admission_supported));
	const auto replay = await_completion();
	assert(replay.outcome == outcome::already_applied && execution.calls == 2);
	assert(critical_command_equal(executed(execution), exact));
	assert_completion_same(first, replay);
	const auto replay_state = state(execution.root);
	assert(replay_state.domains.wallet == first_state.domains.wallet &&
	       replay_state.domains.bank == first_state.domains.bank &&
	       replay_state.domains.wallet_revision == first_state.domains.wallet_revision &&
	       replay_state.domains.bank_revision == first_state.domains.bank_revision);
	// Current coordinator replay does not retain the publication flag. This is
	// evidence of automatic retirement, NOT proof of restart-safe publication.
	// Publication/save acknowledgement must be fixed before gameplay activation.
	assert(critical_command_journal_health_copy().checkpoints == 1);
	assert(!critical_command_coordinator_acknowledge_publication(exact.operation_id));
	for (const auto &key : exact.keys)
		assert(!critical_command_coordinator_is_fenced(key, nullptr));
	critical_command_coordinator_shutdown();
	journal_contains(journal, exact, 0);

	assert(critical_command_coordinator_init(journal.c_str(), native_apply, &execution, 1,
						 nullptr, nullptr,
						 economic_command_admission_supported));
	assert(execution.calls == 2);
	auto fresh = command(execution.root, 101);
	assert(critical_command_coordinator_submit_for_publication(fresh) ==
	       critical_submit_result::awaiting_durability);
	const auto fresh_result = await_completion();
	assert(fresh_result.outcome == outcome::applied && execution.calls == 3);
	const auto checkpoints = critical_command_journal_health_copy().checkpoints;
	for (const auto &key : fresh.keys)
		assert(critical_command_coordinator_is_fenced(key, nullptr));
	assert(critical_command_coordinator_acknowledge_publication(fresh.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == checkpoints + 1);
	for (const auto &key : fresh.keys)
		assert(!critical_command_coordinator_is_fenced(key, nullptr));
	critical_command_coordinator_shutdown();
	journal_contains(journal, fresh, 0);
	assert(critical_command_coordinator_init(journal.c_str(), native_apply, &execution, 1,
						 nullptr, nullptr,
						 economic_command_admission_supported));
	assert(execution.calls == 3);
	critical_command_coordinator_shutdown();
	const auto final_state = state(execution.root);
	assert(final_state.domains.wallet[0] == 102 && final_state.domains.bank[0] == 98);

	// Structurally valid durable schema2 work in a still-closed family must not
	// execute or acquire a checkpoint merely because flatfile bank is admitted.
	auto unsupported = command(execution.root, 102);
	economic_frozen_intent intent;
	assert(economic_intent_decode(unsupported.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	unsupported.schema_version = 1;
	unsupported.accounting_intent.clear();
	unsupported.type = critical_command_type::coin_transfer;
	assert(economic_intent_freeze(unsupported, intent.admission,
				      &unsupported.accounting_intent) ==
	       economic_accounting_error::ok);
	unsupported.schema_version = 2;
	assert(critical_command_envelope_valid(unsupported) &&
	       !economic_command_admission_supported(unsupported));
	const auto refused_journal = (base / "unsupported-journal").string();
	assert(critical_command_journal_init(refused_journal.c_str()));
	assert(critical_command_journal_append(unsupported) == critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	assert(!critical_command_coordinator_init(refused_journal.c_str(), native_apply, &execution,
						  1, nullptr, nullptr,
						  economic_command_admission_supported));
	critical_command_coordinator_shutdown();
	assert(execution.calls == 3);
	assert(critical_command_journal_init(refused_journal.c_str()));
	assert(critical_command_journal_health_copy().checkpoints == 0);
	journal_observation observation{ unsupported };
	assert(critical_command_journal_replay(observe_journal, &observation) ==
		       critical_command_journal_result::ok &&
	       observation.calls == 1);
	critical_command_journal_shutdown();
	assert(state(execution.root).domains.wallet == final_state.domains.wallet);
	std::cout
		<< "native flatfile admission: exact original-ID replay changes balances once; fresh ack retires; unsupported durable work stays uncheckpointed\n";
	std::cout
		<< "known publication gap: replay auto-retires without restoring publication retention\n";
}
