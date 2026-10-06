#include "telemetry/telemetry_outage_private.h"

#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <fcntl.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

std::atomic<bool> fail_write{ false };
std::atomic<bool> fail_rename{ false };
std::atomic<unsigned int> fail_fsync_call{ 0U };
extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_fsync(int);
extern "C" int __real_renameat(int, const char *, int, const char *);
extern "C" ssize_t __wrap_write(int fd, const void *data, size_t count)
{
	if (fail_write.load())
	{
		errno = ENOSPC;
		return -1;
	}
	return __real_write(fd, data, count);
}
extern "C" int __wrap_fsync(int fd)
{
	const auto remaining = fail_fsync_call.load();
	if (remaining != 0U && fail_fsync_call.fetch_sub(1U) == 1U)
	{
		errno = ENOSPC;
		return -1;
	}
	return __real_fsync(fd);
}
extern "C" int __wrap_renameat(int source, const char *from, int destination, const char *to)
{
	if (fail_rename.load())
	{
		errno = EIO;
		return -1;
	}
	return __real_renameat(source, from, destination, to);
}

namespace
{
using result = telemetry_outage_result;
using phase = telemetry_outage_phase;
namespace fs = std::filesystem;

struct fixture
{
	fs::path path;
	fixture()
	{
		char directory[] = "/tmp/duris-telemetry-outage-XXXXXX";
		assert(mkdtemp(directory) != nullptr);
		path = directory;
	}
	~fixture() { fs::remove_all(path); }
};

telemetry_outage_observation registration(std::uint64_t incarnation = 1U)
{
	telemetry_outage_observation value{};
	value.producer = { 9100U, incarnation };
	value.environment_id = 20U;
	value.season_id = 30U;
	value.registered_monotonic_usec = value.observed_monotonic_usec = 100U;
	value.registered_utc_usec = value.observed_utc_usec = 1000;
	return value;
}

telemetry_outage_observation sampled(std::uint64_t incarnation = 1U)
{
	auto value = registration(incarnation);
	value.observed_monotonic_usec = 400U;
	value.observed_utc_usec = TELEMETRY_UTC_UNKNOWN;
	value.record_kind_mask = (1U << 1U) | (1U << 6U) | (1U << 9U);
	value.health.admitted_detail = 8U;
	value.health.last_admitted_record_seq = 12U;
	value.health.applied_records = 4U;
	value.health.last_committed_record_seq = 6U;
	value.health.queue_depth = 4U;
	value.unattempted_records = 4U;
	value.health.dropped_detail = 3U;
	return value;
}

std::vector<unsigned char> bytes(const fs::path &file)
{
	std::ifstream input(file, std::ios::binary);
	assert(input.good());
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void replace(const fs::path &file, const std::vector<unsigned char> &data)
{
	std::ofstream output(file, std::ios::binary | std::ios::trunc);
	assert(output.good());
	output.write(reinterpret_cast<const char *>(data.data()),
		     static_cast<std::streamsize>(data.size()));
	output.close();
	assert(chmod(file.c_str(), 0600) == 0);
}

void word(std::vector<unsigned char> &data, std::size_t offset, std::uint64_t value)
{
	for (unsigned int index = 0; index < 8U; ++index)
		data[offset + index] = static_cast<unsigned char>(value >> (56U - index * 8U));
}

void checksum(std::vector<unsigned char> &data)
{
	assert(SHA256(data.data(), data.size() - SHA256_DIGEST_LENGTH,
		      data.data() + data.size() - SHA256_DIGEST_LENGTH) != nullptr);
}

void terminate_cleanly(telemetry_outage_journal &journal, std::uint64_t incarnation = 1U)
{
	auto value = registration(incarnation);
	value.observed_monotonic_usec = 500U;
	value.phase = phase::clean_drained;
	assert(telemetry_outage_checkpoint(&journal, value) == result::ready);
	telemetry_outage_close(&journal);
}

void lifecycle()
{
	fixture f;
	telemetry_outage_journal journal{};
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) == result::ready);
	assert(journal.count == 1U && journal.generation == 1U);
	const auto original = bytes(f.path / "outages.ledger");
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::invalid);
	assert(journal.directory_fd >= 0); // A mistaken double-open must not release ownership.
	telemetry_outage_journal rival{};
	assert(telemetry_outage_open(&rival, f.path.c_str(), registration(2)) ==
	       result::owned_elsewhere);
	const auto child = fork();
	assert(child >= 0);
	if (child == 0)
	{
		telemetry_outage_journal process_rival{};
		assert(telemetry_outage_open(&process_rival, f.path.c_str(), registration(2)) ==
		       result::owned_elsewhere);
		_exit(0);
	}
	int status = 0;
	assert(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
	       WEXITSTATUS(status) == 0);
	auto value = sampled();
	assert(telemetry_outage_terminal_phase(value) == phase::abandoned);
	assert(telemetry_outage_checkpoint(&journal, value) == result::ready);
	const auto observed = bytes(f.path / "outages.ledger");
	auto invalid = value;
	invalid.health.admitted_detail = 7U;
	assert(telemetry_outage_checkpoint(&journal, invalid) == result::invalid);
	invalid = value;
	invalid.season_id = 99U;
	assert(telemetry_outage_checkpoint(&journal, invalid) == result::invalid);
	assert(bytes(f.path / "outages.ledger") == observed);
	value.phase = phase::abandoned;
	assert(telemetry_outage_checkpoint(&journal, value) == result::ready);
	assert(telemetry_outage_checkpoint(&journal, value) == result::invalid);
	telemetry_outage_close(&journal);
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) == result::invalid);
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration(2)) == result::ready);
	assert(journal.observations[0].phase == phase::abandoned);
	terminate_cleanly(journal, 2);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	assert(journal.count == 2U && journal.observations[1].phase == phase::clean_drained);
	telemetry_outage_close(&journal);
	assert(original.size() == 384U);
	std::puts("outage lifecycle, immutable scopes and monotonic counters passed");
}

void unsafe_storage()
{
	fixture f;
	telemetry_outage_journal journal{};
	assert(telemetry_outage_open(&journal, "relative", registration()) == result::invalid);
	assert(chmod(f.path.c_str(), 0755) == 0);
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) ==
	       result::unsafe_storage);
	assert(chmod(f.path.c_str(), 0700) == 0);
	const auto alias = f.path / "alias";
	fs::create_directory_symlink(f.path, alias);
	assert(telemetry_outage_open(&journal, alias.c_str(), registration()) != result::ready);
	assert(telemetry_outage_open(&journal, (alias.string() + "/").c_str(), registration()) !=
	       result::ready);
	fs::remove(alias);
	fs::create_symlink("outages.ledger", f.path / "outages.owner");
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) != result::ready);
	fs::remove(f.path / "outages.owner");
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) == result::ready);
	telemetry_outage_close(&journal);
	assert(chmod((f.path / "outages.ledger").c_str(), 0644) == 0);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::unsafe_storage);
	assert(chmod((f.path / "outages.ledger").c_str(), 0600) == 0);
	fs::create_hard_link(f.path / "outages.ledger", f.path / "duplicate");
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::unsafe_storage);
	fs::remove(f.path / "duplicate");
	fs::rename(f.path / "outages.ledger", f.path / "original");
	fs::create_symlink("original", f.path / "outages.ledger");
	assert(telemetry_outage_read(&journal, f.path.c_str()) != result::ready);
	std::puts("outage protected paths, ownership, symlinks and hard links passed");
}

void legacy_and_battle_versions()
{
	fixture f;
	telemetry_outage_journal journal{};
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) == result::ready);
	assert(telemetry_outage_checkpoint(&journal, sampled()) == result::ready);
	telemetry_outage_close(&journal);
	const auto current = bytes(f.path / "outages.ledger");
	assert(std::memcmp(current.data(), "DMSTLJ07", 8U) == 0);
	auto result_version = current;
	result_version[7] = '6';
	word(result_version, 32U + 9U * 8U, sampled().record_kind_mask | (1U << 14U));
	checksum(result_version);
	replace(f.path / "outages.ledger", result_version);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	telemetry_outage_close(&journal);
	assert(bytes(f.path / "outages.ledger") == result_version);
	for (const auto version : { '1', '2', '3', '4', '5', '6' })
		for (const auto kind : { 15U, 16U })
			for (const auto field : { 9U, 36U })
			{
				auto incompatible = current;
				incompatible[7] = version;
				word(incompatible, 32U + field * 8U, 1U << kind);
				checksum(incompatible);
				replace(f.path / "outages.ledger", incompatible);
				assert(telemetry_outage_read(&journal, f.path.c_str()) ==
				       result::corrupt);
				assert(bytes(f.path / "outages.ledger") == incompatible);
			}
	auto control_version = current;
	control_version[7] = '5';
	word(control_version, 32U + 9U * 8U, sampled().record_kind_mask | (1U << 13U));
	checksum(control_version);
	replace(f.path / "outages.ledger", control_version);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	telemetry_outage_close(&journal);
	assert(bytes(f.path / "outages.ledger") == control_version);
	for (const auto version : { '1', '2', '3', '4', '5' })
		for (const auto field : { 9U, 36U })
		{
			auto incompatible = current;
			incompatible[7] = version;
			word(incompatible, 32U + field * 8U, 1U << 14U);
			checksum(incompatible);
			replace(f.path / "outages.ledger", incompatible);
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
			assert(bytes(f.path / "outages.ledger") == incompatible);
		}
	auto build_version = current;
	build_version[7] = '4';
	word(build_version, 32U + 9U * 8U, sampled().record_kind_mask | (1U << 12U));
	checksum(build_version);
	replace(f.path / "outages.ledger", build_version);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	telemetry_outage_close(&journal);
	assert(bytes(f.path / "outages.ledger") == build_version);
	for (const auto version : { '1', '2', '3', '4' })
		for (const auto field : { 9U, 36U })
		{
			auto incompatible = current;
			incompatible[7] = version;
			word(incompatible, 32U + field * 8U, 1U << 13U);
			checksum(incompatible);
			replace(f.path / "outages.ledger", incompatible);
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
			assert(bytes(f.path / "outages.ledger") == incompatible);
		}
	auto contribution_version = current;
	contribution_version[7] = '3';
	word(contribution_version, 32U + 9U * 8U, sampled().record_kind_mask | (1U << 11U));
	checksum(contribution_version);
	replace(f.path / "outages.ledger", contribution_version);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	telemetry_outage_close(&journal);
	assert(bytes(f.path / "outages.ledger") == contribution_version);
	for (const auto version : { '1', '2', '3' })
		for (const auto field : { 9U, 36U })
		{
			auto incompatible = current;
			incompatible[7] = version;
			word(incompatible, 32U + field * 8U, 1U << 12U);
			checksum(incompatible);
			replace(f.path / "outages.ledger", incompatible);
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
			assert(bytes(f.path / "outages.ledger") == incompatible);
		}
	auto battle_version = current;
	battle_version[7] = '2';
	word(battle_version, 32U + 9U * 8U, sampled().record_kind_mask | (1U << 10U));
	checksum(battle_version);
	replace(f.path / "outages.ledger", battle_version);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	telemetry_outage_close(&journal);
	assert(bytes(f.path / "outages.ledger") == battle_version);
	for (const auto version : { '1', '2' })
		for (const auto field : { 9U, 36U })
		{
			auto incompatible = current;
			incompatible[7] = version;
			word(incompatible, 32U + field * 8U, 1U << 11U);
			checksum(incompatible);
			replace(f.path / "outages.ledger", incompatible);
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
			assert(bytes(f.path / "outages.ledger") == incompatible);
		}
	auto legacy = current;
	legacy[7] = '1';
	checksum(legacy);
	replace(f.path / "outages.ledger", legacy);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	assert(journal.observations[0].record_kind_mask == sampled().record_kind_mask);
	telemetry_outage_close(&journal);
	assert(bytes(f.path / "outages.ledger") == legacy);
	for (const auto field : { 9U, 36U })
	{
		auto incompatible = legacy;
		word(incompatible, 32U + field * 8U, 1U << 10U);
		checksum(incompatible);
		replace(f.path / "outages.ledger", incompatible);
		assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
		assert(bytes(f.path / "outages.ledger") == incompatible);
	}
	replace(f.path / "outages.ledger", legacy);
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration(2)) == result::ready);
	assert(journal.observations[0].record_kind_mask == sampled().record_kind_mask);
	auto battle = sampled(2);
	battle.record_kind_mask |= (1U << 10U) | (1U << 11U) | (1U << 12U) | (1U << 13U) |
				   (1U << 14U) | (1U << 15U) | (1U << 16U);
	assert(telemetry_outage_checkpoint(&journal, battle) == result::ready);
	telemetry_outage_close(&journal);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	assert(journal.count == 2U &&
	       journal.observations[1].record_kind_mask == battle.record_kind_mask);
	assert(journal.observations[0].record_kind_mask == sampled().record_kind_mask);
	telemetry_outage_close(&journal);
	assert(std::memcmp(bytes(f.path / "outages.ledger").data(), "DMSTLJ07", 8U) == 0);
	std::puts(
		"outage v1/v2/v3/v4/v5/v6 read-only compatibility, v7 progression evidence and atomic upgrade passed");
}

void corruption()
{
	fixture f;
	telemetry_outage_journal journal{};
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) == result::ready);
	telemetry_outage_close(&journal);
	const auto original = bytes(f.path / "outages.ledger");
	for (const auto size : { std::size_t{ 0U }, std::size_t{ 63U }, original.size() - 1U,
				 original.size() + 1U, TELEMETRY_OUTAGE_MAX_BYTES + 1U })
	{
		auto altered = original;
		altered.resize(size);
		replace(f.path / "outages.ledger", altered);
		assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
		assert(bytes(f.path / "outages.ledger") == altered);
	}
	auto altered = original;
	altered[42] ^= 1U;
	replace(f.path / "outages.ledger", altered);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
	for (const auto field : { 8U, 9U, 24U, 27U, 32U })
	{
		altered = original;
		word(altered, 32U + field * 8U, UINT64_MAX);
		checksum(altered);
		replace(f.path / "outages.ledger", altered);
		assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
	}
	std::puts("outage checksum, truncation, size and semantic corruption passed");
}

void publication_faults()
{
	for (unsigned int fault = 0U; fault < 4U; ++fault)
	{
		fixture f;
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) ==
		       result::ready);
		const auto original = bytes(f.path / "outages.ledger");
		fail_write = fault == 0U;
		fail_rename = fault == 1U;
		fail_fsync_call = fault >= 2U ? fault - 1U : 0U;
		assert(telemetry_outage_checkpoint(&journal, sampled()) == result::io_failure);
		assert(journal.error_code == (fault == 1U ? EIO : ENOSPC));
		assert(journal.observations[0].health.admitted_detail == 0U);
		assert(telemetry_outage_checkpoint(&journal, sampled()) == result::invalid);
		fail_write = fail_rename = false;
		fail_fsync_call = 0U;
		telemetry_outage_close(&journal);
		if (fault == 0U)
		{
			assert(bytes(f.path / "outages.ledger") == original);
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
			assert(fs::file_size(f.path / "outages.pending") == 0U);
		}
		else
		{
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
			assert(journal.generation == 2U &&
			       journal.observations[0].health.admitted_detail == 8U);
			assert(!fs::exists(f.path / "outages.pending"));
			telemetry_outage_close(&journal);
			const auto recovered = bytes(f.path / "outages.ledger");
			assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
			assert(journal.generation == 2U &&
			       bytes(f.path / "outages.ledger") == recovered);
			telemetry_outage_close(&journal);
		}
	}
	std::puts(
		"outage disk-full, fsync, interrupted publication and idempotent recovery passed");
}

void changed_storage()
{
	for (unsigned int change = 0U; change < 3U; ++change)
	{
		fixture f;
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) ==
		       result::ready);
		const auto original = bytes(f.path / "outages.ledger");
		if (change == 0U)
			assert(chmod(f.path.c_str(), 0755) == 0);
		else if (change == 1U)
			fs::remove(f.path / "outages.owner");
		else
			assert(chmod((f.path / "outages.ledger").c_str(), 0644) == 0);
		assert(telemetry_outage_checkpoint(&journal, sampled()) == result::unsafe_storage);
		assert(bytes(f.path / "outages.ledger") == original &&
		       !fs::exists(f.path / "outages.pending"));
		telemetry_outage_close(&journal);
	}
	std::puts("outage changed permissions or replaced ownership refuse mutation passed");
}

void pending_chain()
{
	fixture f;
	telemetry_outage_journal journal{};
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) == result::ready);
	assert(telemetry_outage_checkpoint(&journal, sampled()) == result::ready);
	telemetry_outage_close(&journal);
	const auto original = bytes(f.path / "outages.ledger");
	for (const auto field : { 0U, 2U, 4U, 6U, 10U, 12U, 19U })
	{
		auto pending = original;
		word(pending, 8U, 3U);
		word(pending, 32U + field * 8U, field < 6U ? 999U : 0U);
		checksum(pending);
		replace(f.path / "outages.pending", pending);
		assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
		assert(bytes(f.path / "outages.ledger") == original);
		assert(bytes(f.path / "outages.pending") == pending);
	}
	fs::remove(f.path / "outages.pending");
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration(2)) == result::ready);
	assert(journal.observations[0].observed_monotonic_usec == 400U);
	assert(journal.observations[0].phase == phase::unknown_tail);
	terminate_cleanly(journal, 2);
	const auto historical = bytes(f.path / "outages.ledger");
	auto pending = historical;
	word(pending, 8U, 5U);
	word(pending, 32U + 10U * 8U, 9U);
	checksum(pending);
	replace(f.path / "outages.pending", pending);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::corrupt);
	assert(bytes(f.path / "outages.ledger") == historical);
	std::puts("outage pending chain refuses altered identities and rewritten history passed");
}

void wait_success(pid_t child)
{
	int status = 0;
	assert(waitpid(child, &status, 0) == child);
	assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

void abrupt_kill()
{
	fixture f;
	int pipe_fds[2];
	assert(pipe(pipe_fds) == 0);
	const auto child = fork();
	assert(child >= 0);
	if (child == 0)
	{
		close(pipe_fds[0]);
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) ==
		       result::ready);
		auto value = sampled();
		value.inflight_records = 2U;
		value.unattempted_records = 2U;
		value.health.inflight_active = 1U;
		value.health.inflight_first_record_seq = 8U;
		value.health.inflight_last_record_seq = 9U;
		value.health.ambiguous_commits = 1U;
		value.health.last_failure_class = telemetry_failure_class::commit_ambiguous;
		assert(telemetry_outage_terminal_phase(value) == phase::unknown_tail);
		assert(telemetry_outage_checkpoint(&journal, value) == result::ready);
		assert(write(pipe_fds[1], "!", 1) == 1);
		for (;;)
			pause();
	}
	close(pipe_fds[1]);
	char signal = 0;
	assert(read(pipe_fds[0], &signal, 1) == 1);
	close(pipe_fds[0]);
	assert(kill(child, SIGKILL) == 0);
	int status = 0;
	assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status));
	telemetry_outage_journal journal{};
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration(2)) == result::ready);
	assert(journal.count == 2U && journal.observations[0].phase == phase::unknown_tail);
	const auto &before_kill = journal.observations[0];
	assert(before_kill.observed_monotonic_usec == 400U);
	assert(before_kill.observed_utc_usec == TELEMETRY_UTC_UNKNOWN);
	assert(before_kill.inflight_records == 2U && before_kill.unattempted_records == 2U);
	assert(before_kill.health.ambiguous_commits == 1U);
	terminate_cleanly(journal, 2);
	std::puts(
		"outage real SIGKILL retains ambiguous last sample without a crash boundary passed");
}

void process_exec(const char *executable)
{
	fixture f;
	const auto child = fork();
	assert(child >= 0);
	if (child == 0)
	{
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, f.path.c_str(), registration()) ==
		       result::ready);
		assert(telemetry_outage_checkpoint(&journal, sampled()) == result::ready);
		// Do not close: successful replacement proves O_CLOEXEC drops ownership.
		execl(executable, executable, "--after-exec", f.path.c_str(), nullptr);
		_exit(1);
	}
	wait_success(child);
	telemetry_outage_journal journal{};
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	assert(journal.count == 2U && journal.observations[0].phase == phase::unknown_tail);
	assert(journal.observations[1].phase == phase::clean_drained);
	telemetry_outage_close(&journal);
	std::puts("outage real process exec, restart and descriptor ownership passed");
}

void quota()
{
	fixture f;
	telemetry_outage_journal journal{};
	for (std::size_t index = 0U; index < TELEMETRY_OUTAGE_MAX_PRODUCERS; ++index)
	{
		assert(telemetry_outage_open(&journal, f.path.c_str(), registration(index + 1U)) ==
		       result::ready);
		terminate_cleanly(journal, index + 1U);
	}
	const auto full = bytes(f.path / "outages.ledger");
	assert(full.size() == TELEMETRY_OUTAGE_MAX_BYTES);
	assert(telemetry_outage_open(&journal, f.path.c_str(), registration(257)) == result::quota);
	assert(bytes(f.path / "outages.ledger") == full);
	assert(telemetry_outage_read(&journal, f.path.c_str()) == result::ready);
	assert(journal.count == TELEMETRY_OUTAGE_MAX_PRODUCERS);
	telemetry_outage_close(&journal);
	std::puts("outage bounded producer quota preserves all evidence passed");
}
} // namespace

int main(int argc, char **argv)
{
	if (argc == 3 && std::strcmp(argv[1], "--make-fixture") == 0)
	{
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, argv[2], registration()) == result::ready);
		auto battle = sampled();
		battle.record_kind_mask |= (1U << 10U) | (1U << 11U) | (1U << 12U) | (1U << 13U) |
					   (1U << 14U) | (1U << 15U) | (1U << 16U);
		assert(telemetry_outage_checkpoint(&journal, battle) == result::ready);
		telemetry_outage_close(&journal);
		assert(telemetry_outage_open(&journal, argv[2], registration(2)) == result::ready);
		terminate_cleanly(journal, 2);
		return 0;
	}
	if (argc == 3 && std::strcmp(argv[1], "--upgrade-fixture") == 0)
	{
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, argv[2], registration(3)) == result::ready);
		terminate_cleanly(journal, 3);
		return 0;
	}
	if (argc == 3 && std::strcmp(argv[1], "--hold-fixture") == 0)
	{
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, argv[2], registration()) == result::ready);
		std::puts("held");
		std::fflush(stdout);
		for (;;)
			pause();
	}
	if (argc == 3 && std::strcmp(argv[1], "--after-exec") == 0)
	{
		telemetry_outage_journal journal{};
		assert(telemetry_outage_open(&journal, argv[2], registration(2)) == result::ready);
		terminate_cleanly(journal, 2);
		return 0;
	}
	lifecycle();
	unsafe_storage();
	legacy_and_battle_versions();
	corruption();
	publication_faults();
	changed_storage();
	pending_chain();
	abrupt_kill();
	process_exec(argv[0]);
	quota();
	std::puts("telemetry durable outage evidence qualification passed");
}
