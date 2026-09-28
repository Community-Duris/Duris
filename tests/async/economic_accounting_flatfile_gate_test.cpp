#include "economy/economic_accounting_intent.h"
#include "economy/economic_command_admission.h"
#include "economy/coin_transfer_command.h"
#include "economy/item_transfer_accounting.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "player/player_snapshot_codec.h"
#include "persistence/critical_command_coordinator.h"

#include <cassert>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using inventory = std::map<std::string, std::vector<char>>;

class flatfile_accounting_test_access
{
    public:
	static flatfile_accounting_status
	initialize_item_bucket(const std::string &root, const flatfile_authority_lock &lock,
			       const critical_operation_id &lineage, size_t bucket,
			       std::vector<flatfile_authority_operation> *operations,
			       std::string *error)
	{
		return flatfile_accounting_storage::initialize_bucket(root, lock, lineage, bucket,
								      operations, error);
	}
	static flatfile_authority_transaction_result
	commit(const std::string &root, const flatfile_authority_lock &lock,
	       const std::vector<flatfile_authority_operation> &operations, std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};

static inventory contents(const fs::path &root)
{
	inventory result;
	for (const auto &entry : fs::recursive_directory_iterator(root))
	{
		const auto relative = fs::relative(entry.path(), root).generic_string();
		if (entry.is_directory())
			result[relative + "/"] = {};
		else
		{
			assert(entry.is_regular_file());
			std::ifstream input(entry.path(), std::ios::binary);
			assert(input);
			result[relative] = { std::istreambuf_iterator<char>(input), {} };
			assert(!input.bad());
		}
	}
	return result;
}

static critical_operation_id operation(uint8_t value)
{
	critical_operation_id id = {};
	id.bytes[0] = value;
	return id;
}

static critical_command wallet(uint32_t pid, int64_t delta)
{
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = currency_reason_type::coin_transfer;
	std::strcpy(payload.account_name.data(), "gate-fixture");
	payload.wallet_delta.amount[0] = delta;
	critical_command command;
	assert(currency_command_build(&command, operation(1), payload, 0, 0,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	currency_command_payload decoded;
	assert(currency_command_decode_payload(command, &decoded));
	return command;
}

static critical_command item()
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.reason_id = 7;
	payload.selected_item_uid = 100;
	payload.target_root_item_uid = 100;
	payload.item_count = 1;
	payload.items[0] = { 100, 100,
			     0,	  ITEM_TRANSFER_ABSENT_REVISION,
			     500, item_custody_state::absent };
	critical_command command;
	assert(item_transfer_command_build(&command, operation(2), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	assert(item_transfer_command_decode_payload(command, &payload));
	return command;
}

static critical_command coin()
{
	coin_transfer_payload payload;
	payload.source.before[0] = 10;
	payload.source.after[0] = 9;
	payload.source.change = wallet(42, -1);
	payload.destination.before[0] = 0;
	payload.destination.after[0] = 1;
	payload.destination.change = wallet(43, 1);
	critical_command command;
	assert(coin_transfer_command_build(&command, operation(3), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	assert(coin_transfer_command_decode_payload(command, &payload));
	return command;
}

static critical_command accounting(critical_command command)
{
	economic_admission_facts facts;
	facts.metadata.lineage.bytes[0] = 2;
	facts.metadata.epoch.bytes[0] = 3;
	facts.metadata.actor_kind = economic_actor_kind::domain;
	facts.metadata.actor_id = 42;
	facts.metadata.writer_id = 1;
	facts.metadata.reason = economic_reason::wallet_transfer;
	assert(economic_intent_freeze(command, facts, &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = 2;
	std::vector<uint8_t> encoded;
	assert(critical_command_encode(command, &encoded) == critical_command_codec_result::ok);
	critical_command decoded;
	assert(critical_command_decode(encoded.data(), encoded.size(), &decoded) ==
	       critical_command_codec_result::ok);
	assert(critical_command_equal(command, decoded));
	assert(!critical_command_valid(decoded));
	return decoded;
}

static void refused(const critical_apply_result &result)
{
	assert(result.outcome == critical_apply_outcome::terminal_failure);
	assert(result.error_code == EINVAL || result.error_code == ENOTSUP);
	assert(result.result_size == 0);
}

static critical_operation_id accounting_id(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0xa5;
	id.bytes.back() = discriminator;
	return id;
}

static void create_accounting_root(const fs::path &root)
{
	for (const auto &directory : { root, root / "domains", root / "economic-evidence",
				       root / "accounting", root / "accounting/item_references" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
}

static void initialize_item_accounting_bucket(const fs::path &root,
					      const critical_operation_id &lineage,
					      const critical_operation_id &operation)
{
	flatfile_authority_lock lock;
	assert(lock.acquire(root.string(), nullptr));
	std::vector<flatfile_authority_operation> operations;
	assert(flatfile_accounting_test_access::initialize_item_bucket(
		       root.string(), lock, lineage, operation.bytes[0], &operations, nullptr) ==
	       flatfile_accounting_status::ok);
	assert(flatfile_accounting_test_access::commit(root.string(), lock, operations, nullptr) ==
	       flatfile_authority_transaction_result::ok);
}

static critical_command accounted_item_command(item_transfer_payload payload, uint8_t discriminator,
					       uint32_t actor_pid,
					       const critical_operation_id &lineage,
					       const critical_operation_id &epoch,
					       economic_source_kind source = {})
{
	critical_command command;
	assert(item_transfer_command_build(&command, accounting_id(discriminator), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = discriminator;
	std::vector<uint8_t> intent;
	assert(item_transfer_accounting_intent(command, lineage, epoch, actor_pid, &intent,
					       source) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	assert(item_transfer_accounting_command_supported(command));
	return command;
}

struct item_coordinator_context
{
	std::string root;
	std::atomic<unsigned> calls{ 0 };
};

static critical_apply_result apply_accounted_item(const critical_command &command, void *opaque)
{
	auto &context = *static_cast<item_coordinator_context *>(opaque);
	const auto call = context.calls.fetch_add(1);
	if (call == 0)
		assert(setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION", "1", 1) ==
		       0);
	const auto result = flatfile_item_repository_apply(context.root, command);
	if (call == 0)
	{
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION");
		assert(result.outcome == critical_apply_outcome::retryable_failure &&
		       result.error_code == EIO);
	}
	return result;
}

static critical_completion next_item_completion(const critical_operation_id &id)
{
	critical_completion result = {};
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
	while (critical_command_coordinator_pulse(&result, 1) != 1)
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	assert(critical_operation_id_equal(result.operation_id, id) &&
	       result.outcome == critical_apply_outcome::already_applied && !result.error_code);
	return result;
}

static void attach_item_blob(item_transfer_payload *payload,
			     const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> encoded;
	assert(payload &&
	       player_item_snapshot_list_encode(items, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       !encoded.empty() && encoded.size() <= payload->item_blob.size());
	payload->item_blob_size = static_cast<uint32_t>(encoded.size());
	std::copy(encoded.begin(), encoded.end(), payload->item_blob.begin());
}

static void integrated_item_accounting(const fs::path &base)
{
	const critical_operation_id lineage = operation(41);
	const critical_operation_id epoch = operation(42);
	const fs::path root = base / "accounted-items";
	create_accounting_root(root);
	const item_owner_identity player = { item_owner_type::player, 42, 0 };
	assert(flatfile_item_repository_establish_owner(
		       root.string(), player,
		       { { 5001, 5001, 0, player, 1, 501, item_custody_state::active } },
		       nullptr) == flatfile_item_baseline_result::applied);
	initialize_item_accounting_bucket(root, lineage, accounting_id(21));

	item_transfer_payload give = {};
	give.from_owner = player;
	give.to_owner = { item_owner_type::player, 43, 0 };
	give.reason = item_transfer_reason::player_give;
	give.expected_from_revision = 1;
	give.expected_to_revision = 0;
	give.selected_item_uid = 5001;
	give.target_root_item_uid = 5001;
	give.item_count = 1;
	give.items[0] = { 5001, 5001, 0, 1, 501, item_custody_state::active };
	const auto give_command = accounted_item_command(give, 21, 42, lineage, epoch);
	const auto given = flatfile_item_repository_apply(root.string(), give_command);
	assert(given.outcome == critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), give_command).outcome ==
	       critical_apply_outcome::already_applied);
	economic_accounting_item_reference reference = {};
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), give_command.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::ok);
	assert(reference.item_uid == 5001 && reference.before_revision == 1 &&
	       reference.after_revision == 2 && reference.child_index == 0);
	assert(flatfile_item_accounting_reference_verify_operation(
		       root.string(), give_command.operation_id, { &reference, 1 }, nullptr) ==
	       flatfile_item_accounting_status::ok);
	flatfile_accounting_record retained;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, give_command, &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	economic_accounting_plan plan;
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 && plan.item_events[0].uid == 5001);

	const fs::path ref_file = root / "accounting/item_references/a5.bin";
	fs::remove(ref_file);
	const auto missing_reference_replay =
		flatfile_item_repository_apply(root.string(), give_command);
	assert(missing_reference_replay.outcome == critical_apply_outcome::terminal_failure &&
	       missing_reference_replay.error_code == EILSEQ);
	assert(flatfile_item_accounting_reference_append(root.string(), reference, nullptr) ==
	       flatfile_item_accounting_status::ok);

	item_transfer_payload creation = {};
	creation.from_owner = { item_owner_type::system, 0, 0 };
	creation.to_owner = player;
	creation.reason = item_transfer_reason::creation;
	creation.reason_id = 77;
	creation.expected_from_revision = 0;
	creation.expected_to_revision = 2;
	creation.selected_item_uid = 6001;
	creation.target_root_item_uid = 6001;
	creation.item_count = 1;
	creation.items[0] = { 6001, 6001,
			      0,    ITEM_TRANSFER_ABSENT_REVISION,
			      601,  item_custody_state::absent };
	const auto creation_command = accounted_item_command(creation, 22, 42, lineage, epoch,
							     economic_source_kind::starter_grant);
	assert(flatfile_item_repository_apply(root.string(), creation_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), creation_command).outcome ==
	       critical_apply_outcome::already_applied);
	const auto source_claim = std::find_if(
		fs::directory_iterator(root / "economic-evidence"), fs::directory_iterator(),
		[](const fs::directory_entry &entry)
		{ return entry.path().filename().string().starts_with("source-claim-"); });
	assert(source_claim != fs::directory_iterator());
	fs::remove(source_claim->path());
	const auto missing_claim_replay =
		flatfile_item_repository_apply(root.string(), creation_command);
	assert(missing_claim_replay.outcome == critical_apply_outcome::terminal_failure &&
	       missing_claim_replay.error_code == EILSEQ);

	item_transfer_payload retirement = {};
	retirement.from_owner = player;
	retirement.to_owner = { item_owner_type::destruction, 0, 0 };
	retirement.reason = item_transfer_reason::destruction;
	retirement.reason_id = 601;
	retirement.expected_from_revision = 3;
	retirement.expected_to_revision = 0;
	retirement.selected_item_uid = 6001;
	retirement.target_root_item_uid = 6001;
	retirement.item_count = 1;
	retirement.items[0] = { 6001, 6001, 0, 1, 601, item_custody_state::active };
	const auto retirement_command = accounted_item_command(retirement, 24, 42, lineage, epoch,
							       economic_source_kind::item_action);
	const auto retired = flatfile_item_repository_apply(root.string(), retirement_command);
	assert(retired.outcome == critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), retirement_command).outcome ==
	       critical_apply_outcome::already_applied);
	economic_accounting_item_reference retirement_reference = {};
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), retirement_command.operation_id, 0, &retirement_reference,
		       nullptr) == flatfile_item_accounting_status::ok);
	assert(retirement_reference.item_uid == 6001 && retirement_reference.before_revision == 1 &&
	       retirement_reference.after_revision == 2);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, retirement_command,
						  &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 && plan.item_events[0].uid == 6001 &&
	       plan.item_events[0].after.state == item_custody_state::destroyed);

	item_transfer_payload room_creation = {};
	room_creation.from_owner = { item_owner_type::system, 0, 0 };
	room_creation.to_owner = { item_owner_type::room, 50, 0 };
	room_creation.reason = item_transfer_reason::creation;
	room_creation.reason_id = 88;
	room_creation.expected_from_revision = 1;
	room_creation.expected_to_revision = 0;
	room_creation.selected_item_uid = 8001;
	room_creation.target_root_item_uid = 8001;
	room_creation.item_count = 1;
	room_creation.items[0] = { 8001, 8001,
				   0,	 ITEM_TRANSFER_ABSENT_REVISION,
				   801,	 item_custody_state::absent };
	player_item_snapshot room_item = {};
	room_item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	room_item.equipment_slot = -1;
	room_item.object_uid = 8001;
	room_item.vnum = 801;
	room_item.type = 1;
	room_item.weight = 1;
	room_item.condition = 100;
	attach_item_blob(&room_creation, { room_item });
	const auto room_creation_command = accounted_item_command(
		room_creation, 25, 42, lineage, epoch, economic_source_kind::quest_completion);
	assert(flatfile_item_repository_apply(root.string(), room_creation_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), room_creation_command).outcome ==
	       critical_apply_outcome::already_applied);
	economic_accounting_item_reference room_reference = {};
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), room_creation_command.operation_id, 0, &room_reference,
		       nullptr) == flatfile_item_accounting_status::ok);
	assert(room_reference.item_uid == 8001 && room_reference.before_revision == 0 &&
	       room_reference.after_revision == 1);

	item_transfer_payload stale = give;
	stale.from_owner = { item_owner_type::player, 43, 0 };
	stale.to_owner = { item_owner_type::player, 44, 0 };
	stale.expected_from_revision = 0;
	stale.expected_to_revision = 0;
	stale.items[0].expected_item_revision = 2;
	const auto stale_command = accounted_item_command(stale, 23, 43, lineage, epoch);
	const auto rejected = flatfile_item_repository_apply(root.string(), stale_command);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure &&
	       rejected.error_code == ESTALE);
	assert(flatfile_item_repository_apply(root.string(), stale_command).error_code == ESTALE);

	const fs::path interrupted_root = base / "accounted-items-recovery";
	create_accounting_root(interrupted_root);
	assert(flatfile_item_repository_establish_owner(
		       interrupted_root.string(), player,
		       { { 7001, 7001, 0, player, 1, 701, item_custody_state::active } },
		       nullptr) == flatfile_item_baseline_result::applied);
	initialize_item_accounting_bucket(interrupted_root, lineage, accounting_id(31));
	give.items[0] = { 7001, 7001, 0, 1, 701, item_custody_state::active };
	give.selected_item_uid = 7001;
	give.target_root_item_uid = 7001;
	auto interrupted_command = accounted_item_command(give, 31, 42, lineage, epoch);
	interrupted_command.publication_required = true;
	assert(economic_flatfile_command_admission_supported(interrupted_command));
	const auto journal = base / "item-coordinator-journal";
	fs::create_directories(journal);
	fs::permissions(journal, fs::perms::owner_all, fs::perm_options::replace);
	item_coordinator_context context{ interrupted_root.string() };
	const auto start = [&]
	{
		assert(critical_command_coordinator_init(
			journal.c_str(), apply_accounted_item, &context, 1, nullptr, nullptr,
			economic_flatfile_command_admission_supported));
	};
	start();
	assert(critical_command_coordinator_submit_for_publication(interrupted_command) ==
	       critical_submit_result::awaiting_durability);
	const auto completed = next_item_completion(interrupted_command.operation_id);
	auto conflict = interrupted_command;
	++conflict.accepted_at_usec;
	assert(critical_command_coordinator_submit_for_publication(conflict) ==
	       critical_submit_result::identity_conflict);
	assert(context.calls == 2 && critical_command_journal_health_copy().checkpoints == 0 &&
	       critical_command_coordinator_health_copy().publication_pending == 1);
	assert(critical_command_coordinator_shutdown());
	start();
	const auto replayed = next_item_completion(interrupted_command.operation_id);
	assert(context.calls == 3 && replayed.durable_revision == completed.durable_revision &&
	       replayed.result_payload == completed.result_payload &&
	       critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_coordinator_acknowledge_publication(
		interrupted_command.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == 1);
	assert(critical_command_coordinator_shutdown());
	start();
	assert(critical_command_coordinator_health_copy().publication_pending == 0);
	assert(critical_command_coordinator_shutdown());
	assert(flatfile_item_repository_apply(interrupted_root.string(), interrupted_command)
		       .outcome == critical_apply_outcome::already_applied);
}

static void check(const fs::path &missing, const fs::path &seeded, const inventory &before,
		  const critical_command &command)
{
	for (const auto &root : { missing, seeded })
	{
		const std::string path = root.string();
		refused(flatfile_critical_command_repository_apply_selected(
			command, const_cast<char *>(path.c_str())));
		assert(!fs::exists(missing));
		assert(contents(seeded) == before);
		if (command.type == critical_command_type::account_bank)
			refused(flatfile_player_domain_apply(path, command));
		if (command.type == critical_command_type::item_transfer)
			refused(flatfile_item_repository_apply(path, command));
		assert(!fs::exists(missing));
		assert(contents(seeded) == before);
	}
}

// Replace one embedded wire record, retaining the other endpoint and outer header.
static critical_command nested(critical_command command, bool destination)
{
	auto read32 = [&](size_t at)
	{
		uint32_t value = 0;
		for (unsigned byte = 0; byte < 4; ++byte)
			value |= uint32_t(command.payload.at(at + byte)) << (byte * 8);
		return value;
	};
	const size_t header = destination ? 36 + read32(32) : 0;
	const size_t size = read32(header + 32);
	const size_t start = header + 36;
	critical_command leg;
	assert(critical_command_decode(command.payload.data() + start, size, &leg) ==
	       critical_command_codec_result::ok);
	leg = accounting(leg);
	std::vector<uint8_t> encoded;
	assert(critical_command_encode(leg, &encoded) == critical_command_codec_result::ok);
	command.payload.erase(command.payload.begin() + start,
			      command.payload.begin() + start + size);
	command.payload.insert(command.payload.begin() + start, encoded.begin(), encoded.end());
	for (unsigned byte = 0; byte < 4; ++byte)
		command.payload[header + 32 + byte] = uint8_t(encoded.size() >> (byte * 8));
	assert(command.schema_version == 1 && command.accounting_intent.empty());
	assert(critical_command_valid(command));
	coin_transfer_payload rejected;
	assert(!coin_transfer_command_decode_payload(command, &rejected));
	return command;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const fs::path base(argv[1]), missing = base / "missing", seeded = base / "seeded";
	assert(!fs::exists(missing) && !fs::exists(seeded));
	fs::create_directories(seeded / "domains");
	fs::permissions(seeded, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(seeded / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const auto item_command = item();
	// Positive legacy control creates real authority state, not a synthetic sentinel.
	const auto applied = flatfile_item_repository_apply(seeded.string(), item_command);
	assert(applied.outcome == critical_apply_outcome::applied);
	const auto before = contents(seeded);
	assert(!before.empty());
	const auto coin_command = coin();
	for (const auto &legacy : { wallet(42, -1), item_command, coin_command })
	{
		auto changed = accounting(legacy);
		check(missing, seeded, before, changed);
		changed.schema_version = 1; // Legacy schema cannot hide frozen accounting bytes.
		check(missing, seeded, before, changed);
		changed = legacy;
		changed.schema_version = 99;
		check(missing, seeded, before, changed);
	}
	check(missing, seeded, before, nested(coin_command, false));
	check(missing, seeded, before, nested(coin_command, true));
	integrated_item_accounting(base);
	std::cout << "flatfile gates: valid account-bank/item/coin variants rejected; "
		     "both nested schema2 coin legs rejected; schema2 item movement, sourced room "
		     "creation, sourced retirement, exact references, replay and journal recovery "
		     "passed\n";
}
