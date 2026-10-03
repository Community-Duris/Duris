#include "economy/economic_accounting_intent.h"
#include "economy/economic_command_admission.h"
#include "economy/coin_transfer_command.h"
#include "economy/item_transfer_accounting.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "player/player_snapshot_codec.h"
#include "persistence/critical_command_coordinator.h"

#include <cassert>
#include <algorithm>
#include <atomic>
#include <barrier>
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
	item_transfer_payload duplicate_quest = room_creation;
	duplicate_quest.expected_from_revision = 2;
	duplicate_quest.expected_to_revision = 1;
	duplicate_quest.selected_item_uid = 8002;
	duplicate_quest.target_root_item_uid = 8002;
	duplicate_quest.items[0].item_uid = 8002;
	duplicate_quest.items[0].root_item_uid = 8002;
	room_item.object_uid = 8002;
	attach_item_blob(&duplicate_quest, { room_item });
	const auto duplicate_quest_command = accounted_item_command(
		duplicate_quest, 26, 42, lineage, epoch, economic_source_kind::quest_completion);
	assert(flatfile_item_repository_apply(root.string(), duplicate_quest_command).outcome ==
	       critical_apply_outcome::terminal_failure);
	uint64_t room_revision = 0;
	std::vector<flatfile_item_ownership_record> room_items;
	assert(flatfile_item_repository_load_owner(root.string(), room_creation.to_owner,
						   &room_revision, &room_items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(room_revision == 1 && room_items.size() == 1 && room_items[0].item_uid == 8001);

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

static void logical_creation_source_rejects_second_uid(const fs::path &base)
{
	const fs::path root = base / "accounted-logical-source";
	const critical_operation_id lineage = operation(41);
	const critical_operation_id epoch = operation(42);
	create_accounting_root(root);
	initialize_item_accounting_bucket(root, lineage, accounting_id(21));
	const item_owner_identity room = { item_owner_type::room, 51, 0 };
	item_transfer_payload creation = {};
	creation.from_owner = { item_owner_type::system, 0, 0 };
	creation.to_owner = room;
	creation.reason = item_transfer_reason::creation;
	creation.reason_id = 810;
	creation.logical_source_id = 90001;
	creation.expected_from_revision = 0;
	creation.expected_to_revision = 0;
	creation.selected_item_uid = 8101;
	creation.target_root_item_uid = 8101;
	creation.item_count = 1;
	creation.items[0] = { 8101, 8101,
			      0,    ITEM_TRANSFER_ABSENT_REVISION,
			      810,  item_custody_state::absent };
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = -1;
	item.object_uid = 8101;
	item.vnum = 810;
	item.type = 1;
	item.weight = 1;
	item.condition = 100;
	attach_item_blob(&creation, { item });
	const auto command = accounted_item_command(creation, 60, 42, lineage, epoch,
						    economic_source_kind::world_generation);
	assert(flatfile_item_repository_apply(root.string(), command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), command).outcome ==
	       critical_apply_outcome::already_applied);
	economic_accounting_item_reference reference = {};
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), command.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::ok);
	assert(reference.item_uid == 8101 && reference.before_revision == 0 &&
	       reference.after_revision == 1);
	creation.expected_from_revision = 1;
	creation.expected_to_revision = 1;
	creation.selected_item_uid = 8102;
	creation.target_root_item_uid = 8102;
	creation.items[0].item_uid = 8102;
	creation.items[0].root_item_uid = 8102;
	item.object_uid = 8102;
	attach_item_blob(&creation, { item });
	const auto duplicate = accounted_item_command(creation, 61, 42, lineage, epoch,
						      economic_source_kind::world_generation);
	assert(flatfile_item_repository_apply(root.string(), duplicate).outcome ==
	       critical_apply_outcome::terminal_failure);
	uint64_t revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	assert(flatfile_item_repository_load_owner(root.string(), room, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 1 && items.size() == 1 && items[0].item_uid == 8101);
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), duplicate.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::not_found);
}

static void spell_component_batch_retains_tombstone_topology(const fs::path &base)
{
	const fs::path root = base / "accounted-spell-batch";
	const critical_operation_id lineage = operation(41);
	const critical_operation_id epoch = operation(42);
	create_accounting_root(root);
	initialize_item_accounting_bucket(root, lineage, accounting_id(21));
	const item_owner_identity player = { item_owner_type::player, 42, 0 };
	item_transfer_payload creation = {};
	creation.from_owner = { item_owner_type::system, 0, 0 };
	creation.to_owner = player;
	creation.reason = item_transfer_reason::creation;
	creation.reason_id = 820;
	creation.logical_source_id = 92001;
	creation.expected_from_revision = 0;
	creation.expected_to_revision = 0;
	creation.multi_root = true;
	creation.item_count = 3;
	creation.items[0] = { 8201, 8201,
			      0,    ITEM_TRANSFER_ABSENT_REVISION,
			      820,  item_custody_state::absent };
	creation.items[1] = { 8202, 8201,
			      8201, ITEM_TRANSFER_ABSENT_REVISION,
			      821,  item_custody_state::absent };
	creation.items[2] = { 8203, 8203,
			      0,    ITEM_TRANSFER_ABSENT_REVISION,
			      820,  item_custody_state::absent };
	player_item_snapshot first = {};
	first.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	first.equipment_slot = -1;
	first.object_uid = 8201;
	first.vnum = 820;
	first.type = 1;
	first.weight = 1;
	first.condition = 100;
	auto child = first;
	child.parent_index = 0;
	child.object_uid = 8202;
	child.vnum = 821;
	auto second = first;
	second.object_uid = 8203;
	attach_item_blob(&creation, { first, child, second });
	const auto issue = accounted_item_command(creation, 62, 42, lineage, epoch,
						  economic_source_kind::spell_creation);
	assert(flatfile_item_repository_apply(root.string(), issue).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), issue).outcome ==
	       critical_apply_outcome::already_applied);
	uint64_t revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 1 && items.size() == 3);

	auto consume = creation;
	consume.from_owner = player;
	consume.to_owner = { item_owner_type::destruction, 0, 0 };
	consume.reason = item_transfer_reason::destruction;
	consume.reason_id = 3401;
	consume.logical_source_id = 0;
	consume.expected_from_revision = 1;
	consume.expected_to_revision = 0;
	for (size_t index = 0; index < consume.item_count; ++index)
	{
		consume.items[index].expected_item_revision = 1;
		consume.items[index].expected_state = item_custody_state::active;
	}
	const auto retire = accounted_item_command(consume, 63, 42, lineage, epoch,
						   economic_source_kind::spell_consumption);
	assert(flatfile_item_repository_apply(root.string(), retire).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), retire).outcome ==
	       critical_apply_outcome::already_applied);
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 2 && items.empty());
	for (uint32_t index = 0; index < consume.item_count; ++index)
	{
		economic_accounting_item_reference reference = {};
		assert(flatfile_item_accounting_reference_find_by_legacy(
			       root.string(), retire.operation_id, index, &reference, nullptr) ==
		       flatfile_item_accounting_status::ok);
		assert(reference.item_uid == consume.items[index].item_uid &&
		       reference.before_revision == 1 && reference.after_revision == 2);
	}
	flatfile_accounting_record retained;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, retire, &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	economic_accounting_plan plan;
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 3 && plan.item_events[1].uid == 8202 &&
	       plan.item_events[1].before.root_uid == 8201 &&
	       plan.item_events[1].before.parent_uid == 8201 &&
	       plan.item_events[1].after.root_uid == 8201 &&
	       plan.item_events[1].after.parent_uid == 8201 &&
	       plan.item_events[1].after.state == item_custody_state::destroyed);
}

static void integrated_pet_custody(const fs::path &base)
{
	const critical_operation_id lineage = operation(51);
	const critical_operation_id epoch = operation(52);
	const fs::path root = base / "accounted-pet-custody";
	create_accounting_root(root);
	const item_owner_identity player = { item_owner_type::player, 52, 0 };
	const item_owner_identity pet = { item_owner_type::pet, 9301, 52 };
	assert(flatfile_item_repository_establish_owner(
		       root.string(), player,
		       { { 9101, 9101, 0, player, 1, 501, item_custody_state::active },
			 { 9102, 9101, 9101, player, 1, 502, item_custody_state::active } },
		       nullptr) == flatfile_item_baseline_result::applied);
	initialize_item_accounting_bucket(root, lineage, accounting_id(41));
	corpse_lifecycle_payload raised_pet = {};
	raised_pet.action = corpse_lifecycle_action::raise_follower;
	raised_pet.destination_player_pid = 52;
	raised_pet.pet_uid = 9301;
	raised_pet.pet_mob_vnum = 501;
	raised_pet.pet_hit = 20;
	raised_pet.pet_max_hit = 20;
	raised_pet.pet_mana = 10;
	raised_pet.pet_max_mana = 10;
	raised_pet.pet_vitality = 5;
	raised_pet.pet_max_vitality = 5;
	raised_pet.room_vnum = 500;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		flatfile_shop_trade_materialization_mutation materialization;
		assert(flatfile_corpse_resurrection_materialization_prepare(
			       root.string(), lock, operation(53), raised_pet, {}, &materialization,
			       nullptr) == flatfile_shop_trade_materialization_result::ok);
		const flatfile_authority_operation seed = {
			flatfile_authority_store::domains, flatfile_authority_operation_kind::write,
			materialization.after_image.filename, materialization.after_image.bytes
		};
		assert(flatfile_accounting_test_access::commit(root.string(), lock, { seed },
							       nullptr) ==
		       flatfile_authority_transaction_result::ok);
	}
	player_item_snapshot root_item = {};
	root_item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	root_item.equipment_slot = -1;
	root_item.object_uid = 9101;
	root_item.vnum = 501;
	root_item.name = "pet handoff root";
	root_item.affects[0] = { 1, 12 };
	root_item.extra_descriptions.push_back({ "mark", "persistent", false, {} });
	player_item_snapshot child_item = {};
	child_item.parent_index = 0;
	child_item.equipment_slot = -1;
	child_item.object_uid = 9102;
	child_item.vnum = 502;
	child_item.name = "pet handoff child";
	player_snapshot view = {};
	view.pid = 52;
	view.items = { root_item, child_item };

	item_transfer_payload give = {};
	give.from_owner = player;
	give.to_owner = pet;
	give.reason = item_transfer_reason::pet_give;
	give.reason_id = 9301;
	give.expected_from_revision = 1;
	give.expected_to_revision = 0;
	give.selected_item_uid = 9101;
	give.target_root_item_uid = 9101;
	give.item_count = 2;
	give.items[0] = { 9101, 9101, 0, 1, 501, item_custody_state::active };
	give.items[1] = { 9102, 9101, 9101, 1, 502, item_custody_state::active };
	attach_item_blob(&give, { root_item, child_item });
	const auto give_command = accounted_item_command(give, 41, 52, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), give_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), give_command).outcome ==
	       critical_apply_outcome::already_applied);
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	assert(flatfile_item_repository_load_owner(root.string(), player, &owner_revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(owner_revision == 2 && items.empty());
	assert(flatfile_item_repository_load_owner(root.string(), pet, &owner_revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(owner_revision == 1 && items.size() == 2 && items[0].item_revision == 2 &&
	       items[1].item_revision == 2 && items[1].root_item_uid == 9101 &&
	       items[1].parent_item_uid == 9101);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_shop_trade_materialization_reconcile(root.string(), lock, 52, {},
								     &view, nullptr) ==
		       flatfile_shop_trade_materialization_result::ok);
	}
	assert(view.items.empty() && view.pets.size() == 1 && view.pets[0].pet_uid == 9301 &&
	       view.pets[0].items.size() == 2 && view.pets[0].items[0].object_uid == 9101 &&
	       view.pets[0].items[1].parent_index == 0 &&
	       view.pets[0].items[0].affects[0][1] == 12 &&
	       view.pets[0].items[0].extra_descriptions.size() == 1);
	for (uint16_t index = 0; index < 2; ++index)
	{
		economic_accounting_item_reference reference = {};
		assert(flatfile_item_accounting_reference_find_by_legacy(
			       root.string(), give_command.operation_id, index, &reference,
			       nullptr) == flatfile_item_accounting_status::ok);
		assert(reference.item_uid == uint64_t{ 9101 } + index &&
		       reference.before_revision == 1 && reference.after_revision == 2);
	}

	item_transfer_payload returning = give;
	returning.from_owner = pet;
	returning.to_owner = player;
	returning.reason = item_transfer_reason::pet_return;
	returning.expected_from_revision = 1;
	returning.expected_to_revision = 2;
	returning.items[0].expected_item_revision = 2;
	returning.items[1].expected_item_revision = 2;
	const auto return_command = accounted_item_command(returning, 42, 52, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), return_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), return_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(flatfile_item_repository_load_owner(root.string(), pet, &owner_revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(owner_revision == 2 && items.empty());
	assert(flatfile_item_repository_load_owner(root.string(), player, &owner_revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(owner_revision == 3 && items.size() == 2 && items[0].item_revision == 3 &&
	       items[1].item_revision == 3 && items[1].root_item_uid == 9101 &&
	       items[1].parent_item_uid == 9101);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_shop_trade_materialization_reconcile(root.string(), lock, 52, items,
								     &view, nullptr) ==
		       flatfile_shop_trade_materialization_result::ok);
	}
	assert(view.items.size() == 2 && view.items[0].object_uid == 9101 &&
	       view.items[1].parent_index == 0 && view.pets.size() == 1 &&
	       view.pets[0].items.empty());
	for (uint16_t index = 0; index < 2; ++index)
	{
		economic_accounting_item_reference reference = {};
		assert(flatfile_item_accounting_reference_find_by_legacy(
			       root.string(), return_command.operation_id, index, &reference,
			       nullptr) == flatfile_item_accounting_status::ok);
		assert(reference.item_uid == uint64_t{ 9101 } + index &&
		       reference.before_revision == 2 && reference.after_revision == 3);
	}
}

static void integrated_locker_custody(const fs::path &base)
{
	const critical_operation_id lineage = operation(61);
	const critical_operation_id epoch = operation(62);
	const fs::path root = base / "accounted-locker-custody";
	create_accounting_root(root);
	const item_owner_identity player = { item_owner_type::player, 62, 0 };
	const item_owner_identity locker_owner = { item_owner_type::locker, 2, 11 };
	assert(flatfile_item_repository_establish_owner(
		       root.string(), player,
		       { { 9201, 9201, 0, player, 1, 601, item_custody_state::active },
			 { 9202, 9201, 9201, player, 1, 602, item_custody_state::active } },
		       nullptr) == flatfile_item_baseline_result::applied);
	flatfile_locker_chest_record chest = {};
	chest.chest_id = 11;
	chest.chest_name = "public";
	chest.is_public = true;
	chest.revision = 1;
	flatfile_locker_record locker = {};
	locker.locker_id = 2;
	locker.locker_name = "itemprovenance.locker";
	locker.owner_pid = 62;
	locker.revision = 1;
	locker.chests = { chest };
	assert(flatfile_locker_establish(root.string(), { locker }, {}, nullptr) ==
	       flatfile_locker_result::ok);
	assert(flatfile_item_repository_establish_owner(root.string(), locker_owner, {}, nullptr) ==
	       flatfile_item_baseline_result::applied);
	initialize_item_accounting_bucket(root, lineage, accounting_id(61));
	player_item_snapshot stored_root = {};
	stored_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	stored_root.equipment_slot = -1;
	stored_root.object_uid = 9201;
	stored_root.vnum = 601;
	stored_root.name = "accounted locker container";
	stored_root.affects[0] = { 1, 12 };
	stored_root.extra_descriptions.push_back({ "mark", "persistent", false, {} });
	player_item_snapshot stored_child = {};
	stored_child.parent_index = 0;
	stored_child.equipment_slot = -1;
	stored_child.object_uid = 9202;
	stored_child.vnum = 602;
	stored_child.name = "accounted locker child";
	item_transfer_payload deposit = {};
	deposit.from_owner = player;
	deposit.to_owner = locker_owner;
	deposit.reason = item_transfer_reason::locker_deposit;
	deposit.reason_id = 11;
	deposit.expected_from_revision = 1;
	deposit.expected_to_revision = 1;
	deposit.selected_item_uid = 9201;
	deposit.target_root_item_uid = 9201;
	deposit.item_count = 2;
	deposit.items[0] = { 9201, 9201, 0, 1, 601, item_custody_state::active };
	deposit.items[1] = { 9202, 9201, 9201, 1, 602, item_custody_state::active };
	attach_item_blob(&deposit, { stored_root, stored_child });
	item_transfer_payload missing_chest = deposit;
	missing_chest.to_owner.context_id = 12;
	missing_chest.reason_id = 12;
	const auto missing_chest_command =
		accounted_item_command(missing_chest, 61, 62, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), missing_chest_command).outcome ==
	       critical_apply_outcome::terminal_failure);
	uint64_t revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 1 && items.size() == 2 && items[0].item_revision == 1);
	const auto deposit_command = accounted_item_command(deposit, 62, 62, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), deposit_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), deposit_command).outcome ==
	       critical_apply_outcome::already_applied);
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> access;
	assert(flatfile_locker_list(root.string(), &lockers, &access, nullptr) ==
	       flatfile_locker_result::ok);
	assert(lockers.size() == 1 && lockers[0].revision == 2 &&
	       lockers[0].chests[0].revision == 2 && lockers[0].chests[0].items.size() == 2 &&
	       lockers[0].chests[0].items[0].object_uid == 9201 &&
	       lockers[0].chests[0].items[1].parent_index == 0 &&
	       lockers[0].chests[0].items[0].affects[0][1] == 12 &&
	       lockers[0].chests[0].items[0].extra_descriptions.size() == 1);
	assert(flatfile_item_repository_load_owner(root.string(), locker_owner, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 2 && items.size() == 2 && items[0].item_revision == 2 &&
	       items[1].parent_item_uid == 9201);
	for (uint16_t index = 0; index < 2; ++index)
	{
		economic_accounting_item_reference reference = {};
		assert(flatfile_item_accounting_reference_find_by_legacy(
			       root.string(), deposit_command.operation_id, index, &reference,
			       nullptr) == flatfile_item_accounting_status::ok);
		assert(reference.item_uid == uint64_t{ 9201 } + index &&
		       reference.before_revision == 1 && reference.after_revision == 2);
	}
	item_transfer_payload withdraw = deposit;
	withdraw.from_owner = locker_owner;
	withdraw.to_owner = player;
	withdraw.reason = item_transfer_reason::locker_withdraw;
	withdraw.expected_from_revision = 2;
	withdraw.expected_to_revision = 2;
	withdraw.items[0].expected_item_revision = 2;
	withdraw.items[1].expected_item_revision = 2;
	const auto withdraw_command = accounted_item_command(withdraw, 63, 62, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), withdraw_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), withdraw_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(flatfile_locker_list(root.string(), &lockers, &access, nullptr) ==
	       flatfile_locker_result::ok);
	assert(lockers[0].revision == 3 && lockers[0].chests[0].revision == 3 &&
	       lockers[0].chests[0].items.empty());
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 3 && items.size() == 2 && items[0].item_revision == 3 &&
	       items[1].parent_item_uid == 9201);
	for (uint16_t index = 0; index < 2; ++index)
	{
		economic_accounting_item_reference reference = {};
		assert(flatfile_item_accounting_reference_find_by_legacy(
			       root.string(), withdraw_command.operation_id, index, &reference,
			       nullptr) == flatfile_item_accounting_status::ok);
		assert(reference.item_uid == uint64_t{ 9201 } + index &&
		       reference.before_revision == 2 && reference.after_revision == 3);
	}
}

static void integrated_nested_item_accounting(const fs::path &base)
{
	const critical_operation_id lineage = operation(51);
	const critical_operation_id epoch = operation(52);
	const fs::path root = base / "accounted-nested-items";
	create_accounting_root(root);
	const item_owner_identity player = { item_owner_type::player, 42, 0 };
	assert(flatfile_item_repository_establish_owner(
		       root.string(), player,
		       { { 9001, 9001, 0, player, 1, 901, item_custody_state::active },
			 { 9002, 9002, 0, player, 1, 902, item_custody_state::active } },
		       nullptr) == flatfile_item_baseline_result::applied);
	initialize_item_accounting_bucket(root, lineage, accounting_id(51));

	item_transfer_payload put = {};
	put.from_owner = player;
	put.to_owner = player;
	put.reason = item_transfer_reason::player_put;
	put.reason_id = 9001;
	put.expected_from_revision = 1;
	put.expected_to_revision = 1;
	put.selected_item_uid = 9002;
	put.target_root_item_uid = 9001;
	put.target_parent_item_uid = 9001;
	put.expected_target_parent_revision = 1;
	put.item_count = 1;
	put.items[0] = { 9002, 9002, 0, 1, 902, item_custody_state::active };
	const auto put_command = accounted_item_command(put, 51, 42, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), put_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), put_command).outcome ==
	       critical_apply_outcome::already_applied);
	economic_accounting_item_reference reference = {};
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), put_command.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::ok);
	assert(reference.item_uid == 9002 && reference.before_revision == 1 &&
	       reference.after_revision == 2);
	flatfile_accounting_record retained;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, put_command, &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	economic_accounting_plan plan;
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 && plan.item_events[0].uid == 9002 &&
	       plan.item_events[0].before.root_uid == 9002 &&
	       plan.item_events[0].after.root_uid == 9001 &&
	       plan.item_events[0].after.parent_uid == 9001);

	item_transfer_payload get = put;
	get.reason = item_transfer_reason::player_get;
	get.reason_id = 0;
	get.expected_from_revision = 2;
	get.expected_to_revision = 2;
	get.target_root_item_uid = 9002;
	get.target_parent_item_uid = 0;
	get.expected_target_parent_revision = 0;
	get.items[0] = { 9002, 9001, 9001, 2, 902, item_custody_state::active };
	const auto get_command = accounted_item_command(get, 52, 42, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), get_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), get_command.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::ok);
	assert(reference.item_uid == 9002 && reference.before_revision == 2 &&
	       reference.after_revision == 3);
}

static void integrated_equipment_item_accounting(const fs::path &base)
{
	const critical_operation_id lineage = operation(71);
	const critical_operation_id epoch = operation(72);
	const fs::path root = base / "accounted-equipment-items";
	create_accounting_root(root);
	const item_owner_identity player = { item_owner_type::player, 42, 0 };
	assert(flatfile_item_repository_establish_owner(
		       root.string(), player,
		       { { 9701, 9701, 0, player, 1, 971, item_custody_state::active },
			 { 9702, 9702, 0, player, 1, 972, item_custody_state::active } },
		       nullptr) == flatfile_item_baseline_result::applied);
	initialize_item_accounting_bucket(root, lineage, accounting_id(71));

	player_item_snapshot snapshot = {};
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.equipment_slot = 5;
	snapshot.object_uid = 9701;
	snapshot.vnum = 971;
	snapshot.type = 1;
	snapshot.weight = 1;
	snapshot.condition = 100;
	item_transfer_payload wear = {};
	wear.from_owner = player;
	wear.to_owner = player;
	wear.reason = item_transfer_reason::player_wear;
	wear.reason_id = 5;
	wear.expected_from_revision = 1;
	wear.expected_to_revision = 1;
	wear.selected_item_uid = 9701;
	wear.target_root_item_uid = 9701;
	wear.item_count = 1;
	wear.items[0] = { 9701, 9701, 0, 1, 971, item_custody_state::active };
	attach_item_blob(&wear, { snapshot });
	const auto wear_command = accounted_item_command(wear, 71, 42, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), wear_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), wear_command).outcome ==
	       critical_apply_outcome::already_applied);
	uint64_t revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 2 && items.size() == 2 && items[0].equipment_slot == 5);
	economic_accounting_item_reference reference = {};
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), wear_command.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::ok);
	assert(reference.item_uid == 9701 && reference.before_revision == 1 &&
	       reference.after_revision == 2);
	flatfile_accounting_record retained;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, wear_command, &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	economic_accounting_plan plan;
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 && plan.item_events[0].before.equipment_slot == 0 &&
	       plan.item_events[0].after.equipment_slot == 5);
	player_snapshot saved = {};
	saved.pid = 42;
	auto stale_snapshot = snapshot;
	stale_snapshot.equipment_slot = 0;
	saved.items.push_back(stale_snapshot);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_shop_trade_materialization_reconcile(root.string(), lock, 42, items,
								     &saved, nullptr) ==
		       flatfile_shop_trade_materialization_result::ok);
	}
	assert(saved.items.size() == 1 && saved.items[0].equipment_slot == 5);

	item_transfer_payload occupied = wear;
	occupied.selected_item_uid = 9702;
	occupied.target_root_item_uid = 9702;
	occupied.expected_from_revision = 2;
	occupied.expected_to_revision = 2;
	occupied.items[0] = { 9702, 9702, 0, 1, 972, item_custody_state::active };
	snapshot.object_uid = 9702;
	snapshot.vnum = 972;
	attach_item_blob(&occupied, { snapshot });
	const auto occupied_command = accounted_item_command(occupied, 72, 42, lineage, epoch);
	const auto occupied_result =
		flatfile_item_repository_apply(root.string(), occupied_command);
	assert(occupied_result.outcome == critical_apply_outcome::terminal_failure &&
	       occupied_result.error_code == ESTALE);

	item_transfer_payload remove = wear;
	remove.reason = item_transfer_reason::player_remove;
	remove.expected_from_revision = 2;
	remove.expected_to_revision = 2;
	remove.items[0].expected_item_revision = 2;
	snapshot.object_uid = 9701;
	snapshot.vnum = 971;
	snapshot.equipment_slot = 0;
	attach_item_blob(&remove, { snapshot });
	const auto remove_command = accounted_item_command(remove, 73, 42, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), remove_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), remove_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 3 && items[0].equipment_slot == 0);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, remove_command, &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 && plan.item_events[0].before.equipment_slot == 5 &&
	       plan.item_events[0].after.equipment_slot == 0);
	player_snapshot stale_remove = {};
	stale_remove.pid = 42;
	snapshot.equipment_slot = 5;
	stale_remove.items.push_back(snapshot);
	player_snapshot legacy_wear = {};
	legacy_wear.pid = 42;
	snapshot.object_uid = 9702;
	snapshot.vnum = 972;
	legacy_wear.items.push_back(snapshot);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_shop_trade_materialization_reconcile(root.string(), lock, 42, items,
								     &stale_remove, nullptr) ==
		       flatfile_shop_trade_materialization_result::ok);
		assert(flatfile_shop_trade_materialization_reconcile(root.string(), lock, 42, items,
								     &legacy_wear, nullptr) ==
		       flatfile_shop_trade_materialization_result::ok);
	}
	assert(stale_remove.items.size() == 1 && stale_remove.items[0].equipment_slot == 0);
	const auto legacy_item = std::find_if(legacy_wear.items.begin(), legacy_wear.items.end(),
					      [](const auto &item)
					      { return item.object_uid == 9702; });
	assert(legacy_item != legacy_wear.items.end() && legacy_item->equipment_slot == 5);

	item_transfer_payload rewear = wear;
	rewear.expected_from_revision = 3;
	rewear.expected_to_revision = 3;
	rewear.items[0].expected_item_revision = 3;
	snapshot.object_uid = 9701;
	snapshot.vnum = 971;
	snapshot.equipment_slot = 5;
	attach_item_blob(&rewear, { snapshot });
	const auto rewear_command = accounted_item_command(rewear, 74, 42, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), rewear_command).outcome ==
	       critical_apply_outcome::applied);

	item_transfer_payload forced = rewear;
	forced.to_owner = { item_owner_type::room, 50, 0 };
	forced.reason = item_transfer_reason::combat_fumble;
	forced.expected_from_revision = 4;
	forced.expected_to_revision = 0;
	forced.items[0].expected_item_revision = 4;
	snapshot.equipment_slot = 0;
	attach_item_blob(&forced, { snapshot });
	forced.reason_id = 6;
	const auto wrong_slot = accounted_item_command(forced, 75, 42, lineage, epoch);
	const auto wrong_slot_result = flatfile_item_repository_apply(root.string(), wrong_slot);
	assert(wrong_slot_result.outcome == critical_apply_outcome::terminal_failure &&
	       wrong_slot_result.error_code == ESTALE);
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 4 && items[0].equipment_slot == 5);
	forced.reason_id = 5;
	const auto forced_command = accounted_item_command(forced, 76, 42, lineage, epoch);
	assert(flatfile_item_repository_apply(root.string(), forced_command).outcome ==
	       critical_apply_outcome::applied);
	assert(flatfile_item_repository_apply(root.string(), forced_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(flatfile_item_repository_load_owner(root.string(), player, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 5 && items.size() == 1 && items[0].item_uid == 9702);
	const auto player_items = items;
	const item_owner_identity room = { item_owner_type::room, 50, 0 };
	assert(flatfile_item_repository_load_owner(root.string(), room, &revision, &items,
						   nullptr) == flatfile_item_repository_result::ok);
	assert(revision == 1 && items.size() == 1 && items[0].item_uid == 9701 &&
	       items[0].item_revision == 5 && items[0].equipment_slot == 0);
	std::vector<flatfile_room_item_record> rooms;
	assert(flatfile_world_item_list_rooms(root.string(), &rooms, nullptr) ==
	       flatfile_world_item_result::ok);
	assert(rooms.size() == 1 && rooms[0].room_vnum == 50 && rooms[0].items.size() == 1 &&
	       rooms[0].items[0].object_uid == 9701 && rooms[0].items[0].equipment_slot == -1);
	assert(flatfile_item_accounting_reference_find_by_legacy(
		       root.string(), forced_command.operation_id, 0, &reference, nullptr) ==
	       flatfile_item_accounting_status::ok);
	assert(reference.item_uid == 9701 && reference.before_revision == 4 &&
	       reference.after_revision == 5);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_accounting_lookup(root.string(), lock, forced_command, &retained,
						  nullptr) == flatfile_accounting_status::ok);
	}
	assert(economic_plan_decode(retained.plan, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 && plan.item_events[0].before.equipment_slot == 5 &&
	       plan.item_events[0].after.equipment_slot == 0);
	player_snapshot stale_drop = {};
	stale_drop.pid = 42;
	snapshot.equipment_slot = 5;
	stale_drop.items.push_back(snapshot);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), nullptr));
		assert(flatfile_shop_trade_materialization_reconcile(
			       root.string(), lock, 42, player_items, &stale_drop, nullptr) ==
		       flatfile_shop_trade_materialization_result::ok);
	}
	assert(stale_drop.items.empty());
}

static void simultaneous_first_claimants(const fs::path &base)
{
	const fs::path root = base / "simultaneous-first-claimants";
	const auto lineage = operation(81);
	const auto epoch = operation(82);
	create_accounting_root(root);
	initialize_item_accounting_bucket(root, lineage, accounting_id(81));
	item_transfer_payload first = {};
	first.from_owner = { item_owner_type::system, 0, 0 };
	first.to_owner = { item_owner_type::player, 42, 0 };
	first.reason = item_transfer_reason::creation;
	first.reason_id = 81;
	first.selected_item_uid = 9801;
	first.target_root_item_uid = 9801;
	first.item_count = 1;
	first.items[0] = { 9801, 9801,
			   0,	 ITEM_TRANSFER_ABSENT_REVISION,
			   981,	 item_custody_state::absent };
	item_transfer_payload second = first;
	second.to_owner = { item_owner_type::player, 43, 0 };
	const std::array<critical_command, 2> commands = {
		accounted_item_command(first, 81, 42, lineage, epoch,
				       economic_source_kind::world_generation),
		accounted_item_command(second, 82, 43, lineage, epoch,
				       economic_source_kind::world_generation),
	};
	std::array<critical_apply_result, 2> results = {};
	std::barrier start(3);
	auto claimant = [&](size_t index)
	{
		start.arrive_and_wait();
		results[index] = flatfile_item_repository_apply(root.string(), commands[index]);
	};
	std::thread one(claimant, 0);
	std::thread two(claimant, 1);
	start.arrive_and_wait();
	one.join();
	two.join();
	const auto applied = [](const critical_apply_result &result)
	{ return result.outcome == critical_apply_outcome::applied; };
	assert(static_cast<int>(applied(results[0])) + static_cast<int>(applied(results[1])) == 1);
	uint64_t revision = 0;
	std::vector<flatfile_item_ownership_record> first_items, second_items;
	const auto first_loaded = flatfile_item_repository_load_owner(
		root.string(), first.to_owner, &revision, &first_items, nullptr);
	const auto second_loaded = flatfile_item_repository_load_owner(
		root.string(), second.to_owner, &revision, &second_items, nullptr);
	assert((first_loaded == flatfile_item_repository_result::ok ||
		second_loaded == flatfile_item_repository_result::ok) &&
	       first_items.size() + second_items.size() == 1);
	std::vector<economic_accounting_item_reference> history;
	assert(flatfile_item_accounting_reference_find_history(root.string(), 9801, &history,
							       nullptr) ==
		       flatfile_item_accounting_status::ok &&
	       history.size() == 1);
	const size_t claims = std::count_if(
		fs::directory_iterator(root / "economic-evidence"), fs::directory_iterator(),
		[](const auto &entry)
		{ return entry.path().filename().string().starts_with("source-claim-"); });
	assert(claims == 1);
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
	logical_creation_source_rejects_second_uid(base);
	spell_component_batch_retains_tombstone_topology(base);
	integrated_pet_custody(base);
	integrated_locker_custody(base);
	integrated_nested_item_accounting(base);
	integrated_equipment_item_accounting(base);
	simultaneous_first_claimants(base);
	std::cout
		<< "flatfile gates: invalid envelopes and nested coin legs rejected; item "
		   "movement, sourced room creation and retirement, nested pet and locker custody, "
		   "duplicate quest and world sources refused, spell component batch retired, same-owner nesting and equipment slots, exact references, "
		   "simultaneous first-claim refusal, replay and journal recovery passed\n";
}
