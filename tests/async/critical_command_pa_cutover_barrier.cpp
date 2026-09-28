#include "persistence/critical_command_coordinator.h"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <unistd.h>

static int close_fault = 0;

extern "C" int __real_close(int);
extern "C" int __wrap_close(int fd)
{
	if (close_fault)
	{
		--close_fault;
		const int result = __real_close(fd);
		assert(result == 0);
		errno = EIO;
		return -1;
	}
	return __real_close(fd);
}

static critical_command make_command(uint8_t tag)
{
	critical_command command = {};
	command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	assert(critical_operation_id_generate(&command.operation_id));
	command.type = critical_command_type::test;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.accepted_at_usec = 1700000000000000ULL + tag;
	command.keys = { { critical_entity_type::player, 100ULL + tag } };
	command.payload = { tag };
	assert(critical_command_normalize(&command));
	return command;
}

static critical_apply_result apply(const critical_command &, void *)
{
	return { critical_apply_outcome::applied, 1, 0 };
}

static bool reject_replay(const critical_command &, void *)
{
	return false;
}

template <typename Predicate> static void wait_for(Predicate condition)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!condition())
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

static void test_clean_boundary(const std::string &directory)
{
	assert(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1));
	assert(!critical_command_coordinator_cutover_ready());
	critical_command_coordinator_quiesce();
	assert(critical_command_coordinator_drain(3000));
	assert(critical_command_coordinator_cutover_ready());
	critical_command_coordinator_reset_for_tests();
}

static void test_publication_hold(const std::string &directory)
{
	assert(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1));
	const critical_command command = make_command(1);
	assert(critical_command_coordinator_submit_for_publication(command) ==
	       critical_submit_result::awaiting_durability);
	wait_for(
		[]
		{
			critical_completion completion[8] = {};
			critical_command_coordinator_pulse(completion, 8);
			return critical_command_coordinator_health_copy().publication_pending == 1;
		});

	critical_command_coordinator_quiesce();
	assert(!critical_command_coordinator_drain(10));
	assert(!critical_command_coordinator_cutover_ready());
	assert(critical_command_coordinator_health_copy().publication_pending == 1);
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::player, 101 },
						      nullptr));
	assert(critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_coordinator_drain(3000));
	assert(critical_command_coordinator_cutover_ready());
	critical_command_coordinator_reset_for_tests();
}

static void test_uncertain_append(const std::string &directory)
{
	assert(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1));
	const critical_command command = make_command(2);
	close_fault = 2; // append-close and rollback-close both report failure
	assert(critical_command_coordinator_submit(command) ==
	       critical_submit_result::awaiting_durability);
	wait_for(
		[&]
		{
			return critical_command_coordinator_durability(command.operation_id) ==
			       critical_command_durability::uncertain;
		});
	close_fault = 0;
	critical_command_coordinator_quiesce();
	assert(!critical_command_coordinator_drain(10));
	assert(!critical_command_coordinator_cutover_ready());
	const critical_coordinator_health coordinator = critical_command_coordinator_health_copy();
	const critical_command_journal_health journal = critical_command_journal_health_copy();
	assert(coordinator.blocked == 1 && coordinator.fenced_keys == 1);
	assert(journal.append_uncertain);
	critical_operation_id fenced = {};
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::player, 102 },
						      &fenced));
	assert(critical_operation_id_equal(fenced, command.operation_id));
	critical_command_coordinator_reset_for_tests();
}

static void test_replay_blocked_frame(const std::string &directory)
{
	assert(critical_command_journal_init(directory.c_str()));
	const critical_command command = make_command(3);
	assert(critical_command_journal_append(command) == critical_command_journal_result::ok);
	critical_command_journal_shutdown();

	assert(!critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1,
						  reject_replay, nullptr));
	assert(!critical_command_coordinator_cutover_ready());
	critical_command_journal_health journal = critical_command_journal_health_copy();
	assert(!journal.initialized && journal.records == 1);
	assert(journal.last_result == critical_command_journal_result::replay_blocked);

	// Reopening must find the original durable frame; a failed replay cannot
	// erase or checkpoint it merely to make the cutover preflight pass.
	assert(critical_command_journal_init(directory.c_str()));
	journal = critical_command_journal_health_copy();
	assert(journal.initialized && journal.records == 1);
	critical_command_journal_shutdown();
	critical_command_journal_reset_for_tests();
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::filesystem::path root(argv[1]);
	std::filesystem::remove_all(root);
	assert(std::filesystem::create_directories(root));
	test_clean_boundary((root / "clean").string());
	test_publication_hold((root / "publication").string());
	test_uncertain_append((root / "uncertain").string());
	test_replay_blocked_frame((root / "replay-blocked").string());
	return 0;
}
