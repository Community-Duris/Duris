#include "flatfile/flatfile_item_repository.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "economy/item_transfer_accounting.h"
#include "item/craft_pouch_mutation.h"
#include "item/craft_recipe_continuation.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "world/vnum.obj.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;
	static constexpr auto initialize_evidence_bucket =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto commit = &flatfile_accounting_storage::commit;
};

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static critical_operation_id operation(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0x51;
	id.bytes.back() = discriminator;
	return id;
}

static player_item_snapshot snapshot(uint64_t uid, int32_t vnum, int32_t parent_index)
{
	player_item_snapshot item = {};
	item.parent_index = parent_index;
	item.equipment_slot = -1;
	item.object_uid = uid;
	item.vnum = vnum;
	item.string_mask = 1 | 2 | 4;
	item.name = "craft output name";
	item.short_description = "a crafted output";
	item.description = "A crafted output is here.";
	return item;
}

static critical_command craft_command(uint8_t discriminator, uint64_t pid, uint64_t revision,
				      const std::vector<item_transfer_entry> &inputs,
				      const std::vector<player_item_snapshot> &outputs)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::player, pid, 0 };
	payload.to_owner = payload.from_owner;
	payload.reason = item_transfer_reason::craft;
	payload.reason_id = 551;
	payload.expected_from_revision = revision;
	payload.expected_to_revision = revision;
	payload.multi_root = true;
	payload.item_count = static_cast<uint16_t>(inputs.size());
	for (size_t index = 0; index < inputs.size(); ++index)
		payload.items[index] = inputs[index];
	payload.selected_item_uid = outputs.empty() ? inputs.front().root_item_uid :
						      outputs.front().object_uid;
	if (!outputs.empty())
	{
		std::vector<uint8_t> encoded;
		require(player_item_snapshot_list_encode(outputs, &encoded) ==
					player_snapshot_codec_result::ok &&
				encoded.size() <= payload.item_blob.size(),
			"craft output snapshot did not encode");
		payload.item_blob_size = static_cast<uint32_t>(encoded.size());
		std::copy(encoded.begin(), encoded.end(), payload.item_blob.begin());
	}
	critical_command command = {};
	const bool built = item_transfer_command_build(&command, operation(discriminator), payload,
						       critical_source_site::command,
						       critical_deadline_class::interactive);
	require(built, "craft command did not build");
	command.accepted_at_usec = discriminator;
	return command;
}

static void establish(const fs::path &root, uint64_t pid,
		      const std::vector<flatfile_item_ownership_record> &items)
{
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	require(flatfile_item_repository_establish_owner(
			root.string(), { item_owner_type::player, pid, 0 }, items, &error) ==
			flatfile_item_baseline_result::applied,
		"craft owner baseline failed: " + error);
}

static std::vector<flatfile_item_ownership_record> load(const fs::path &root, uint64_t pid,
							uint64_t *revision)
{
	std::vector<flatfile_item_ownership_record> items;
	std::string error;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, pid, 0 }, revision, &items,
			&error) == flatfile_item_repository_result::ok,
		"craft owner load failed: " + error);
	return items;
}

static void activate(const fs::path &root)
{
	fs::create_directories(root / "economic-evidence");
	fs::permissions(root / "economic-evidence", fs::perms::owner_all,
			fs::perm_options::replace);
	std::string error;
	flatfile_authority_lock lock;
	require(lock.acquire(root.string(), &error), "craft accounting setup lock failed");
	std::vector<flatfile_authority_operation> operations;
	auto commit = [&]
	{
		require(flatfile_accounting_test_access::commit(root.string(), lock, operations,
								&error) ==
				flatfile_authority_transaction_result::ok,
			"craft accounting setup commit failed: " + error);
		operations.clear();
	};
	auto revision = [&]
	{
		flatfile_economic_control control;
		require(flatfile_economic_control_read(root.string(), lock, &control, &error) == 0,
			"craft accounting setup read failed: " + error);
		return control.revision;
	};
	require(flatfile_accounting_test_access::bootstrap(root.string(), lock, operation(71),
							   operation(70), &operations, &error) == 0,
		"craft lineage setup failed: " + error);
	commit();
	for (size_t bucket = 0; bucket < FLATFILE_ACCOUNTING_BUCKETS; ++bucket)
	{
		auto id = operation(75);
		id.bytes[1] = static_cast<uint8_t>(bucket);
		require(flatfile_accounting_test_access::initialize_evidence_bucket(
				root.string(), lock, revision(), bucket, id, &operations, &error) ==
				0,
			"craft accounting evidence bucket setup failed: " + error);
		commit();
	}
	flatfile_economic_epoch epoch;
	epoch.epoch = operation(72);
	epoch.creating_operation = operation(73);
	epoch.ordinal = 1;
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	require(flatfile_accounting_test_access::append_epoch(root.string(), lock, revision(),
							      epoch, &operations, &error) == 0,
		"craft epoch setup failed: " + error);
	commit();
	require(flatfile_accounting_test_access::select_epoch(root.string(), lock, revision(), true,
							      operation(74), &operations,
							      &error) == 0,
		"craft epoch selection failed: " + error);
	commit();
}

static critical_command accounted(critical_command command, uint32_t pid)
{
	command.accepted_at_usec = 0;
	std::vector<uint8_t> intent;
	require(item_transfer_accounting_intent(command, operation(71), operation(72), pid, &intent,
						economic_source_kind::crafting) ==
			economic_accounting_error::ok,
		"craft accounting intent did not freeze");
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	command.accepted_at_usec = 1;
	return command;
}

static critical_command alchemy(critical_command command, craft_recipe_discipline discipline,
				uint32_t count)
{
	item_transfer_payload payload = {};
	require(item_transfer_command_decode_payload(command, &payload),
		"alchemy payload did not decode");
	craft_recipe_continuation terms;
	terms.player_pid = static_cast<uint32_t>(payload.from_owner.id);
	terms.discipline = discipline;
	terms.recipe_vnum = static_cast<uint32_t>(payload.reason_id);
	terms.output_uid = payload.selected_item_uid;
	terms.output_count = count;
	if (payload.continuation.kind == item_transfer_continuation_kind::craft_pouch_usage)
		terms.pouch_mutation = std::move(payload.continuation.data);
	payload.continuation.kind = item_transfer_continuation_kind::craft_recipe;
	require(craft_recipe_continuation_encode(terms, &payload.continuation.data),
		"alchemy terms did not freeze");
	require(item_transfer_command_build(&command, command.operation_id, payload,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"alchemy root did not build");
	return command;
}

static player_item_snapshot reconcile_pouch(const fs::path &root, uint64_t *revision)
{
	const auto owned = load(root, 99, revision);
	player_snapshot player;
	player.pid = 99;
	flatfile_authority_lock lock;
	std::string error;
	require(lock.acquire(root.string(), &error), "pouch reconciliation lock failed");
	require(flatfile_shop_trade_materialization_reconcile(root.string(), lock, 99, owned,
							      &player, &error) ==
			flatfile_shop_trade_materialization_result::ok,
		"pouch reconstruction failed: " + error);
	for (auto &item : player.items)
		if (item.object_uid == 7002)
		{
			require(item.parent_index >= 0 &&
					player.items[item.parent_index].object_uid == 7001,
				"pouch did not retain its original container");
			item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			return item;
		}
	require(false, "retained pouch missing on cold reconstruction");
	return {};
}

static void pouch_seed(const fs::path &root)
{
	const item_owner_identity owner = { item_owner_type::player, 99, 0 };
	establish(root, 99,
		  {
			  { 7000, 7000, 0, owner, 1, 501, item_custody_state::active },
			  { 7003, 7003, 0, owner, 1, 400000, item_custody_state::active },
			  { 7004, 7004, 0, owner, 1, 400000, item_custody_state::active },
			  { 7005, 7005, 0, owner, 1, 9000, item_custody_state::active },
			  { 7006, 7006, 0, owner, 1, 9000, item_custody_state::active },
		  });
	auto pouch = snapshot(7002, VOBJ_CHAOS_CRAFT_POUCH, 0);
	pouch.generated_key = 93;
	pouch.values[2] = 84;
	pouch.extra_descriptions.push_back(
		{ "inscription", "preserve native pouch inscription", false, {} });
	const auto seed = craft_command(80, 99, 1,
					{ { 7000, 7000, 0, 1, 501, item_custody_state::active } },
					{ snapshot(7001, 800, -1), pouch });
	require(flatfile_item_repository_apply(root.string(), seed).outcome ==
			critical_apply_outcome::applied,
		"native pouch seed did not apply");
	activate(root);
}

static critical_command pouch_command(uint8_t discriminator, uint64_t revision,
				      uint64_t material_uid, uint64_t pouch_revision,
				      const player_item_snapshot &before,
				      chaos_pouch_usage_mode mode,
				      const std::vector<player_item_snapshot> &outputs = {})
{
	const int vnum = mode == chaos_pouch_usage_mode::collected ? 400000 : 9000;
	auto base = craft_command(
		discriminator, 99, revision,
		{ { material_uid, material_uid, 0, 1, vnum, item_custody_state::active } },
		outputs);
	item_transfer_payload payload = {};
	require(item_transfer_command_decode_payload(base, &payload),
		"pouch base command did not decode");
	payload.reason_id = mode == chaos_pouch_usage_mode::collected ? VOBJ_CHAOS_CRAFT_POUCH :
									400291;
	payload.item_count = 2;
	payload.items[1] = payload.items[0];
	payload.items[0] = {
		7002, 7001, 7001, pouch_revision, VOBJ_CHAOS_CRAFT_POUCH, item_custody_state::active
	};
	craft_pouch_mutation mutation;
	mutation.before = before;
	mutation.mode = mode;
	mutation.usage = { { mode == chaos_pouch_usage_mode::collected ? 400000 : 400291, 1 } };
	require(chaos_pouch_ledger_prepare(before, mutation.usage, mode, &mutation.after) ==
			chaos_pouch_ledger_result::ok,
		"pouch mutation did not freeze");
	payload.continuation.kind = item_transfer_continuation_kind::craft_pouch_usage;
	require(craft_pouch_mutation_encode(mutation, &payload.continuation.data),
		"pouch envelope did not encode");
	require(item_transfer_command_build(&base, operation(discriminator), payload,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"pouch compound command did not build");
	if (mode == chaos_pouch_usage_mode::generated)
		base = alchemy(std::move(base),
			       outputs.empty() ? craft_recipe_discipline::encrust_failure :
						 craft_recipe_discipline::encrust,
			       static_cast<uint32_t>(outputs.size()));
	return accounted(std::move(base), 99);
}

static void retained_pouch_updates_are_atomic(const fs::path &root)
{
	pouch_seed(root);
	uint64_t revision = 0;
	auto before = reconcile_pouch(root, &revision);
	require(revision == 2, "pouch seed owner revision mismatch");
	const auto collect =
		pouch_command(81, 2, 7003, 1, before, chaos_pouch_usage_mode::collected);
	auto result = flatfile_item_repository_apply(root.string(), collect);
	require(result.outcome == critical_apply_outcome::applied,
		"pouch collection did not commit: " + std::to_string(result.error_code));
	auto first = reconcile_pouch(root, &revision);
	require(revision == 3 && first.object_uid == before.object_uid &&
			first.generated_key == 93 && first.values[2] == 84 &&
			first.extra_descriptions.back().description == "0:0:1;",
		"pouch collection lost its native state or counters");
	require(flatfile_item_repository_apply(root.string(), collect).outcome ==
			critical_apply_outcome::already_applied,
		"pouch collection replay was not exact once");
	const auto second = pouch_command(82, 3, 7004, 2, first, chaos_pouch_usage_mode::collected);
	require(flatfile_item_repository_apply(root.string(), second).outcome ==
			critical_apply_outcome::applied,
		"retained pouch was incorrectly reused as the issuance source");
	auto twice = reconcile_pouch(root, &revision);
	require(revision == 4 && twice.extra_descriptions.back().description == "0:0:2;",
		"second collection did not advance exactly once");
	const auto encrust = pouch_command(83, 4, 7005, 3, twice, chaos_pouch_usage_mode::generated,
					   { snapshot(7100, 1251, -1) });
	require(flatfile_item_repository_apply(root.string(), encrust).outcome ==
			critical_apply_outcome::applied,
		"virtual-material craft did not commit with its retained pouch");
	auto generated = reconcile_pouch(root, &revision);
	require(revision == 5 &&
			generated.extra_descriptions.back().description == "0:0:2;210:1:0;",
		"generated pouch counter did not commit with craft output");
	const auto failed =
		pouch_command(84, 5, 7006, 4, generated, chaos_pouch_usage_mode::generated);
	require(flatfile_item_repository_apply(root.string(), failed).outcome ==
				critical_apply_outcome::applied &&
			flatfile_item_repository_apply(root.string(), failed).outcome ==
				critical_apply_outcome::already_applied,
		"zero-output alchemy/pouch publication did not commit and replay");
	const auto broken = reconcile_pouch(root, &revision);
	require(revision == 6 && broken.object_uid == before.object_uid &&
			broken.extra_descriptions.back().description == "0:0:2;210:2:0;",
		"failed Encrust lost the retained pouch or repeated its counter");
	for (const char *fault : { "DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT",
				   "DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL",
				   "DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION" })
	{
		const fs::path interrupted_root = root.string() + fault;
		pouch_seed(interrupted_root);
		const pid_t child = fork();
		require(child >= 0, "pouch fault fixture did not fork");
		if (!child)
		{
			setenv(fault, "1", 1);
			const auto interrupted =
				flatfile_item_repository_apply(interrupted_root.string(), collect);
			_exit(interrupted.outcome == critical_apply_outcome::retryable_failure ? 0 :
												 1);
		}
		int status = 0;
		require(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
				!WEXITSTATUS(status),
			"pouch fault did not report uncertainty");
		const bool committed = std::string(fault) !=
				       "DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT";
		auto recovered = reconcile_pouch(interrupted_root, &revision);
		require(revision == (committed ? 3u : 2u) &&
				chaos_pouch_ledger_counters_equal(recovered,
								  committed ? first : before),
			"pouch crash exposed counter/custody partial commit");
		const auto resumed =
			flatfile_item_repository_apply(interrupted_root.string(), collect);
		require(resumed.outcome == (committed ? critical_apply_outcome::already_applied :
							critical_apply_outcome::applied),
			"pouch interrupted retry duplicated counters");
	}
}

int main(int argc, char **argv)
{
	if (argc == 3 && std::string(argv[1]) == "--activate-fixture")
	{
		activate(argv[2]);
		return 0;
	}
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const item_owner_identity owner = { item_owner_type::player, 77, 0 };
	establish(root, 77,
		  { { 1000, 1000, 0, owner, 1, 500, item_custody_state::active },
		    { 1001, 1001, 0, owner, 1, 501, item_custody_state::active } });
	const std::vector<item_transfer_entry> inputs = {
		{ 1000, 1000, 0, 1, 500, item_custody_state::active },
		{ 1001, 1001, 0, 1, 501, item_custody_state::active },
	};
	const auto success = craft_command(1, 77, 1, inputs,
					   { snapshot(2000, 800, PLAYER_SNAPSHOT_NO_PARENT),
					     snapshot(2001, 801, 0) });
	critical_apply_result applied = flatfile_item_repository_apply(root.string(), success);
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0,
		"multi-output craft did not apply");
	uint64_t revision = 0;
	auto items = load(root, 77, &revision);
	require(revision == 2 && items.size() == 2 && items[0].item_uid == 2000 &&
			items[1].item_uid == 2001 && items[1].parent_item_uid == 2000,
		"multi-output craft did not publish exact output custody");
	applied = flatfile_item_repository_apply(root.string(), success);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"craft replay was not idempotent");

	const std::vector<item_transfer_entry> stale_input = {
		{ 2000, 2000, 0, 1, 800, item_custody_state::active },
	};
	const auto stale = craft_command(2, 77, 1, stale_input, { snapshot(3000, 900, -1) });
	applied = flatfile_item_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure,
		"stale craft was accepted");
	items = load(root, 77, &revision);
	require(revision == 2 && items.size() == 2 && items[0].item_uid == 2000 &&
			items[1].item_uid == 2001,
		"stale craft mutated authoritative custody");

	const fs::path failure_root = root.string() + "-failure";
	const item_owner_identity failure_owner = { item_owner_type::player, 88, 0 };
	establish(failure_root, 88,
		  { { 4000, 4000, 0, failure_owner, 1, 600, item_custody_state::active } });
	const auto failure = craft_command(
		3, 88, 1, { { 4000, 4000, 0, 1, 600, item_custody_state::active } }, {});
	applied = flatfile_item_repository_apply(failure_root.string(), failure);
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0,
		"zero-output craft failure did not apply");
	items = load(failure_root, 88, &revision);
	require(revision == 2 && items.empty(),
		"zero-output craft failure did not retire its consumed input atomically");

	const fs::path accounted_root = root.string() + "-accounted";
	const item_owner_identity accounted_owner = { item_owner_type::player, 99, 0 };
	establish(accounted_root, 99,
		  { { 5000, 5000, 0, accounted_owner, 1, 500, item_custody_state::active, {}, 7 },
		    { 5001, 5000, 5000, accounted_owner, 1, 501, item_custody_state::active } });
	activate(accounted_root);
	const auto accounted_success = accounted(
		alchemy(craft_command(5, 99, 1,
				      { { 5000, 5000, 0, 1, 500, item_custody_state::active },
					{ 5001, 5000, 5000, 1, 501, item_custody_state::active } },
				      { snapshot(6000, 800, -1), snapshot(6001, 801, 0) }),
			craft_recipe_discipline::encrust, 1),
		99);
	applied = flatfile_item_repository_apply(accounted_root.string(), accounted_success);
	require(applied.outcome == critical_apply_outcome::applied,
		"accounted craft did not commit: " + std::to_string(applied.error_code));
	std::string error;
	for (uint16_t index = 0; index < 4; ++index)
	{
		economic_accounting_item_reference ref = {};
		require(flatfile_item_accounting_reference_find_by_legacy(
				accounted_root.string(), accounted_success.operation_id, index,
				&ref, &error) == flatfile_item_accounting_status::ok &&
				ref.event_index == index &&
				ref.before_revision == (index < 2 ? 1 : 0) &&
				ref.after_revision == (index < 2 ? 2 : 1),
			"craft reference omitted an input or output: " + error);
	}
	require(flatfile_item_repository_apply(accounted_root.string(), accounted_success).outcome ==
			critical_apply_outcome::already_applied,
		"accounted craft replay failed");
	items = load(accounted_root, 99, &revision);
	require(revision == 2 && items.size() == 2 && items[0].item_uid == 6000 &&
			items[1].parent_item_uid == 6000,
		"accounted output custody mismatch");
	for (const char *fault : { "DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT",
				   "DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL",
				   "DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION" })
	{
		const fs::path crash_root = root.string() + "-" + fault;
		establish(crash_root, 99,
			  { { 5000,
			      5000,
			      0,
			      accounted_owner,
			      1,
			      500,
			      item_custody_state::active,
			      {},
			      7 },
			    { 5001, 5000, 5000, accounted_owner, 1, 501,
			      item_custody_state::active } });
		activate(crash_root);
		const pid_t child = fork();
		require(child >= 0, "craft crash fixture did not fork");
		if (!child)
		{
			setenv(fault, "1", 1);
			const auto interrupted = flatfile_item_repository_apply(crash_root.string(),
										accounted_success);
			_exit(interrupted.outcome == critical_apply_outcome::retryable_failure ? 0 :
												 1);
		}
		int status = 0;
		require(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
				WEXITSTATUS(status) == 0,
			"craft commit interruption did not report uncertainty");
		const bool committed = std::string(fault) !=
				       "DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT";
		// The failed worker has exited. Loading now must recover a complete bundle
		// or retain the original inputs when no commit journal reached disk.
		items = load(crash_root, 99, &revision);
		require(revision == (committed ? 2u : 1u) && items.size() == 2 &&
				items[0].item_uid == (committed ? 6000u : 5000u),
			"craft restart exposed partial custody");
		const auto resumed =
			flatfile_item_repository_apply(crash_root.string(), accounted_success);
		require(resumed.outcome == (committed ? critical_apply_outcome::already_applied :
							critical_apply_outcome::applied),
			"craft interrupted retry was not exact once");
		for (uint16_t index = 0; index < 4; ++index)
		{
			economic_accounting_item_reference ref = {};
			require(flatfile_item_accounting_reference_find_by_legacy(
					crash_root.string(), accounted_success.operation_id, index,
					&ref, &error) == flatfile_item_accounting_status::ok &&
					ref.item_uid == (index < 2 ? 5000u + index : 5998u + index),
				"craft restart lost exact consumed/output accounting references");
		}
	}
	const auto consumed_failure = accounted(
		alchemy(craft_command(6, 99, 2,
				      { { 6000, 6000, 0, 1, 800, item_custody_state::active },
					{ 6001, 6000, 6000, 1, 801, item_custody_state::active } },
				      {}),
			craft_recipe_discipline::encrust_failure, 0),
		99);
	const auto intended_failure =
		flatfile_item_repository_apply(accounted_root.string(), consumed_failure);
	require(intended_failure.outcome == critical_apply_outcome::applied,
		"accounted intended failure did not commit: " +
			std::to_string(intended_failure.error_code));
	items = load(accounted_root, 99, &revision);
	require(revision == 3 && items.empty(),
		"accounted intended failure left live input custody");

	retained_pouch_updates_are_atomic(root.string() + "-pouch");
	std::cout << "Issue 551 flat-file craft and retained pouch conservation passed\n";
	return 0;
}
