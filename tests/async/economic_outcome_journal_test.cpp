#include "persistence/economic_outcome.h"
#include "persistence/critical_command_journal.h"

#include <cassert>
#include <cstdlib>
#include <cerrno>
#include <filesystem>
#include <iostream>
#include <new>
#include <thread>
#include <array>
#include <fcntl.h>
#include <zlib.h>
#include <unistd.h>

// Controlled composition boundaries only. These are not a genuine world,
// recovery owner, actor/forest or gameplay/admission qualification.
static const auto world_thread = std::this_thread::get_id();
static bool recovery_ready = false;
static size_t fail_allocation_size = 0;
void *operator new(size_t size)
{
	if (fail_allocation_size && size == fail_allocation_size)
	{
		fail_allocation_size = 0;
		throw std::bad_alloc();
	}
	if (void *value = std::malloc(size))
		return value;
	throw std::bad_alloc();
}
void operator delete(void *value) noexcept
{
	std::free(value);
}
void operator delete(void *value, size_t) noexcept
{
	std::free(value);
}
bool nevent_is_game_thread()
{
	return std::this_thread::get_id() == world_thread;
}
bool economic_outcome_recovery_admission_allowed() noexcept
{
	return recovery_ready;
}
static bool fail_sync = false;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd)
{
	if (fail_sync)
	{
		errno = EIO;
		return -1;
	}
	return __real_fsync(fd);
}

static critical_operation_id id(uint8_t value)
{
	critical_operation_id result{};
	result.bytes[0] = value;
	return result;
}

static economic_outcome_record value(uint8_t operation)
{
	economic_outcome_record record;
	record.domain = economic_outcome_domain::currency;
	record.logical_tick = 7;
	auto &command = record.command;
	command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	command.operation_id = id(operation);
	command.type = critical_command_type::test;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.accepted_at_usec = 100;
	command.keys = { { critical_entity_type::player, 7 } };
	command.expected_revisions = { { command.keys[0], 1 } };
	command.payload = { 1, 2 };
	assert(critical_command_normalize(&command));
	auto &plan = record.accounting;
	plan.metadata.lineage = id(1);
	plan.metadata.epoch = id(2);
	plan.metadata.operation_id = id(operation);
	plan.metadata.actor_kind = economic_actor_kind::domain;
	plan.metadata.actor_id = 7;
	plan.metadata.writer_id = 1;
	plan.metadata.reason = economic_reason::wallet_transfer;
	plan.metadata.intent_digest[0] = 11;
	plan.metadata.domain_digest[0] = 12;
	const auto wallet = [](uint64_t authority)
	{ return economic_account_key{ id(1), economic_account_kind::wallet, authority, 0 }; };
	plan.accounts = { { wallet(1), { 0, 0, 0, 1 }, { 3, 6, 8, 0 }, 4, 5 },
			  { wallet(2), {}, { 7, 3, 1, 0 }, 8, 9 } };
	plan.postings = { { 0, 0, 0, { 3, 6, 8, -1 }, -137 }, { 1, 1, 0, { 7, 3, 1, 0 }, 137 } };
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);
	record.result = { 4, 5, 6 };
	economic_outcome_effect effect;
	effect.key = command.keys[0];
	effect.owner_before = { item_owner_type::player, 7, 0 };
	effect.owner_after = effect.owner_before;
	effect.before_presence = economic_outcome_presence::present;
	effect.before_revision = 0; // Real base cut zero need not mean a birth.
	effect.before = { 11 };
	effect.after = { 12 };
	record.effects.push_back(effect);
	return record;
}

struct publication_state
{
	unsigned int published = 0;
	unsigned int fenced = 0;
	uint64_t expected_cut = 0;
	economic_outcome_record *probe = nullptr;
	const economic_outcome_publication *callback = nullptr;
	bool refuse = false;
};
static bool current(void *opaque) noexcept
{
	auto &state = *static_cast<publication_state *>(opaque);
	uint64_t cut = 0, snapshot = 0;
	assert(!economic_outcome_capture_cut(&snapshot));
	assert(economic_outcome_current_observation_cut(&snapshot) &&
	       snapshot == state.expected_cut);
	assert(economic_outcome_clock_matches(id(1), id(2), snapshot));
	assert(!economic_outcome_clock_matches(id(9), id(2), snapshot));
	assert(!economic_outcome_clock_matches(id(1), id(9), snapshot));
	assert(!economic_outcome_clock_matches(id(1), id(2), snapshot + 1));
	assert(economic_outcome_commit(*state.probe, *state.callback).status ==
	       economic_outcome_status::unavailable);
	return economic_outcome_entity_cut({ critical_entity_type::player, 7 }, &cut) &&
	       cut == state.expected_cut;
}
static bool publish(const economic_outcome_record &record, void *opaque) noexcept
{
	auto &state = *static_cast<publication_state *>(opaque);
	if (state.refuse)
		return false;
	assert(record.sequence && record.effects[0].after_revision == record.sequence);
	uint64_t snapshot = 0;
	assert(!economic_outcome_capture_cut(&snapshot));
	uint64_t observation = 999;
	assert(!economic_outcome_current_observation_cut(&observation) && observation == 999);
	assert(!economic_outcome_clock_matches(id(1), id(2), record.sequence));
	++state.published;
	return true;
}
static void fence(uint64_t sequence, void *opaque) noexcept
{
	assert(sequence);
	++static_cast<publication_state *>(opaque)->fenced;
}
static bool verify(uint64_t, void *) noexcept
{
	return true;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const auto directory = std::filesystem::path(argv[1]);
	std::filesystem::create_directories(directory);
	std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
	assert(!economic_outcome_journal_init(directory.c_str(),
					      CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
					      CRITICAL_COMMAND_JOURNAL_MAX_RECORDS));
	assert(!std::filesystem::exists(directory / "economic-outcomes.journal"));
	assert(!economic_outcome_journal_init_namespace(
		directory.c_str(), CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
		CRITICAL_COMMAND_JOURNAL_MAX_RECORDS, id(1), {}, 0));
	assert(economic_outcome_journal_init_namespace(
		directory.c_str(), CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
		CRITICAL_COMMAND_JOURNAL_MAX_RECORDS, id(1), id(2), 0));
	auto first = value(3);
	publication_state native;
	const economic_outcome_publication callback{ current, publish, fence, &native };
	native.callback = &callback;
	native.probe = &first;
	assert(economic_outcome_commit(first, callback).status ==
	       economic_outcome_status::unavailable);
	assert(native.published == 0);
	assert(economic_outcome_install_recovered_cut(0, verify, nullptr));
	recovery_ready = true;
	assert(economic_outcome_commit(first, callback).status == economic_outcome_status::durable);
	assert(first.sequence == 1 && native.published == 1);
	assert(economic_outcome_commit(first, callback).status == economic_outcome_status::durable);
	assert(native.published == 1);
	auto conflict = first;
	conflict.result.push_back(9);
	assert(economic_outcome_commit(conflict, callback).status ==
	       economic_outcome_status::conflict);
	assert(native.published == 1);
	std::vector<uint8_t> encoded;
	assert(economic_outcome_encode(first, &encoded));
	for (size_t n = 0; n < encoded.size(); ++n)
	{
		economic_outcome_record sentinel;
		sentinel.sequence = 999;
		assert(!economic_outcome_decode(std::span(encoded).first(n), &sentinel));
		assert(sentinel.sequence == 999);
	}
	economic_outcome_record decoded;
	assert(economic_outcome_decode(encoded, &decoded));
	std::vector<uint8_t> exact;
	assert(economic_outcome_encode(decoded, &exact) && exact == encoded);
	assert(decoded.effects[0].before_presence == economic_outcome_presence::present &&
	       decoded.effects[0].before_revision == 0 && !decoded.effects[0].before.empty());
	size_t effect_at = 26; // EOR1 fixed header through result_version.
	for (unsigned int blob = 0; blob < 3; ++blob)
	{
		uint32_t length = 0;
		for (unsigned int i = 0; i < 4; ++i)
			length |= static_cast<uint32_t>(encoded[effect_at + i]) << (8 * i);
		effect_at += 4 + length;
	}
	effect_at += 4; // Effect count.
	for (const auto owner :
	     { item_owner_identity{ item_owner_type::pet, 0, 0 },
	       item_owner_identity{ item_owner_type::pet, 7, 0 },
	       item_owner_identity{ item_owner_type::pet, 7, UINT64_MAX },
	       item_owner_identity{ item_owner_type::native_mobile, UINT64_MAX, 1 },
	       item_owner_identity{ item_owner_type::destruction, 7, 0 } })
	{
		auto malformed = first;
		malformed.effects[0].owner_after = owner;
		std::vector<uint8_t> unchanged{ 99 };
		assert(!economic_outcome_encode(malformed, &unchanged));
		assert(unchanged == std::vector<uint8_t>{ 99 });
		auto malformed_wire = encoded;
		const size_t after_owner = effect_at + 1 + 8 + 2 + 8 + 8 + 2 + 17;
		malformed_wire[after_owner] = static_cast<uint8_t>(owner.type);
		for (unsigned int i = 0; i < 8; ++i)
		{
			malformed_wire[after_owner + 1 + i] =
				static_cast<uint8_t>(owner.id >> (8 * i));
			malformed_wire[after_owner + 9 + i] =
				static_cast<uint8_t>(owner.context_id >> (8 * i));
		}
		economic_outcome_record sentinel;
		sentinel.sequence = 999;
		assert(!economic_outcome_decode(malformed_wire, &sentinel) &&
		       sentinel.sequence == 999);
	}
	uint64_t cut = 99;
	assert(economic_outcome_capture_cut(&cut) && cut == 1);
	uint64_t observation = 999;
	assert(!economic_outcome_current_observation_cut(&observation) && observation == 999);
	assert(economic_outcome_clock_matches(id(1), id(2), cut));
	assert(!economic_outcome_clock_matches({}, id(2), cut));
	assert(!economic_outcome_clock_matches(id(1), {}, cut));
	auto second = value(4);
	native.expected_cut = 1;
	second.effects[0].before_revision = 1;
	fail_sync = true;
	assert(economic_outcome_commit(second, callback).status ==
	       economic_outcome_status::uncertain);
	assert(native.published == 1 && native.fenced == 1);
	assert(!economic_outcome_capture_cut(&cut));
	assert(!economic_outcome_clock_matches(id(1), id(2), 1));
	assert(!economic_outcome_journal_reconcile());
	fail_sync = false;
	assert(economic_outcome_journal_reconcile());
	unsigned int recovered = 0;
	const auto replay = +[](const economic_outcome_record &record, void *opaque) noexcept
	{
		auto &count = *static_cast<unsigned int *>(opaque);
		assert(record.sequence == 2 && record.effects[0].after_revision == 2);
		++count;
		return true;
	};
	assert(economic_outcome_journal_replay(1, replay, &recovered) && recovered == 1);
	assert(economic_outcome_install_recovered_cut(2, verify, nullptr));
	assert(economic_outcome_commit(second, callback).status ==
	       economic_outcome_status::durable);
	assert(native.published == 1); // Controlled recovery installed exact cut.
	auto third = value(5);
	native.expected_cut = 2;
	third.effects[0].before_revision = 2;
	native.refuse = true;
	assert(economic_outcome_commit(third, callback).status ==
	       economic_outcome_status::uncertain);
	assert(native.published == 1 && native.fenced == 2);
	assert(!economic_outcome_capture_cut(&cut));
	assert(!economic_outcome_journal_retire_prefix(3, verify, nullptr));
	economic_outcome_journal_shutdown();
	assert(!economic_outcome_journal_init_namespace(
		directory.c_str(), CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
		CRITICAL_COMMAND_JOURNAL_MAX_RECORDS, {}, id(9), 0));
	assert(!economic_outcome_journal_health_copy().initialized);
	fail_sync = true;
	assert(!economic_outcome_journal_init(directory.c_str(),
					      CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
					      CRITICAL_COMMAND_JOURNAL_MAX_RECORDS));
	assert(!economic_outcome_journal_health_copy().initialized);
	fail_sync = false;
	fail_allocation_size = encoded.size();
	assert(!economic_outcome_journal_init(directory.c_str(),
					      CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
					      CRITICAL_COMMAND_JOURNAL_MAX_RECORDS));
	assert(fail_allocation_size == 0 && !economic_outcome_journal_health_copy().initialized);
	assert(economic_outcome_journal_init(directory.c_str(),
					     CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
					     CRITICAL_COMMAND_JOURNAL_MAX_RECORDS));
	assert(economic_outcome_journal_health_copy().durable_sequence == 3);
	unsigned int replayed = 0;
	const auto replay_with_observers =
		+[](const economic_outcome_record &record, void *raw) noexcept
	{
		assert(economic_outcome_journal_health_copy().initialized);
		critical_operation_id lineage{}, epoch{};
		assert(economic_outcome_journal_namespace(&lineage, &epoch));
		assert(economic_outcome_journal_append(record).status ==
		       economic_outcome_status::unavailable);
		++*static_cast<unsigned int *>(raw);
		return true;
	};
	assert(economic_outcome_journal_replay(0, replay_with_observers, &replayed) &&
	       replayed == 3);
	economic_outcome_journal_shutdown();
	const auto foreign = directory / "foreign";
	std::filesystem::create_directory(foreign);
	std::filesystem::permissions(foreign, std::filesystem::perms::owner_all);
	assert(economic_outcome_journal_init_namespace(
		foreign.c_str(), CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
		CRITICAL_COMMAND_JOURNAL_MAX_RECORDS, id(1), id(2), 0));
	const int fd = open((foreign / "economic-outcomes.journal").c_str(), O_RDWR);
	assert(fd >= 0);
	std::array<uint8_t, 60> header{};
	assert(pread(fd, header.data(), header.size(), 0) == static_cast<ssize_t>(header.size()));
	header[8] = 9;
	const uint32_t crc = crc32(0, header.data(), 56);
	for (unsigned int i = 0; i < 4; ++i)
		header[56 + i] = static_cast<uint8_t>(crc >> (8 * i));
	assert(pwrite(fd, header.data(), header.size(), 0) == static_cast<ssize_t>(header.size()));
	assert(close(fd) == 0);
	auto other = value(6);
	other.sequence = 1;
	other.effects[0].after_revision = 1;
	assert(economic_outcome_journal_append(other).status == economic_outcome_status::uncertain);
	assert(economic_outcome_journal_health_copy().uncertain);
	std::cout
		<< "PASS: bounded canonical outcome and real fsync/replay/once-publication composition; real gameplay/base/admission/crash qualification remains open\n";
	economic_outcome_journal_shutdown();
	return 0;
}
