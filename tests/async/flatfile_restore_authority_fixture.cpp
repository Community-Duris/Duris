// Synthetic, private fixture encoded by the native metadata writers. Never
// selects an active epoch or runs a production lifecycle/activation owner.
#include "flatfile/flatfile_accounting_authority.h"
#include "economy/economic_currency_adapter.h"
#include "flatfile/flatfile_accounting_baseline.h"
#include "world/quest_mobile_native.h"
#include "../../scripts/qualify_flatfile_economic_records.h"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto initialize_native_bucket =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto create_mapping =
		&flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto retire_mapping =
		&flatfile_accounting_authority_storage::retire_mapping;
	static constexpr auto rename_bank = &flatfile_accounting_authority_storage::rename_bank;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto initialize_evidence_bucket =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto stage = &flatfile_accounting_storage::stage;
	static constexpr auto stage_source_claim = &flatfile_accounting_storage::stage_source_claim;
	static constexpr auto initialize_baseline =
		&flatfile_accounting_baseline_storage::initialize;
	static constexpr auto stage_baseline = &flatfile_accounting_baseline_storage::stage;
	static auto commit(const std::string &root, const flatfile_authority_lock &lock,
			   const std::vector<flatfile_authority_operation> &operations,
			   std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};
static critical_operation_id id(uint64_t value)
{
	critical_operation_id result = {};
	for (size_t i = 0; i < 8; ++i)
		result.bytes[i] = static_cast<uint8_t>(value >> (8 * i));
	return result;
}
static flatfile_accounting_record record(uint32_t sequence, bool large, bool source = false,
					 int32_t pid = 11, bool items = false)
{
	flatfile_accounting_record value;
	critical_operation_id operation = {};
	operation.bytes[0] = 1;
	for (size_t i = 0; i < 4; ++i)
		operation.bytes[15 - i] = static_cast<uint8_t>(sequence >> (8 * i));
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = source ? currency_reason_type::wallet_reward :
				  currency_reason_type::atm_deposit;
	payload.reason_id = source ? 3 : 0;
	strcpy(payload.account_name.data(), "renamed");
	payload.wallet_delta.amount[0] = source ? 10 : -10;
	payload.bank_delta.amount[0] = source ? 0 : 10;
	assert(currency_command_build(
		&value.command, operation, payload, UINT64_MAX, UINT64_MAX,
		source ? critical_source_site::recovery : critical_source_site::command,
		source ? critical_deadline_class::recovery : critical_deadline_class::interactive));
	value.command.accepted_at_usec = 1;
	economic_currency_authority state;
	state.epoch = id(source && sequence == 2 ? 51 : 50);
	state.wallet_account = { id(1), economic_account_kind::wallet, 3, 0 };
	state.bank_account = { id(1), economic_account_kind::bank, 2, 1 };
	state.player_fence = value.command.keys[0];
	state.bank_fence = value.command.keys[1];
	state.state.wallet.amount[0] = 100;
	state.state.bank.amount[0] = 50;
	const auto freeze = source ? economic_quest_wallet_reward_intent :
				     economic_bank_transfer_intent;
	assert(freeze(value.command, state.epoch, state.wallet_account, state.bank_account,
		      &value.command.accounting_intent) == economic_accounting_error::ok);
	value.command.schema_version = 2;
	economic_frozen_intent intent;
	assert(economic_intent_decode(value.command.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	if (items)
	{
		// Native-encoded structural witnesses only; no domain mutation or epoch
		// selection. Include equipment, retained destruction edges and creation.
		intent.admission.metadata.reason = economic_reason::item_move;
		value.command.schema_version = 1;
		value.command.accounting_intent.clear();
		assert(economic_intent_freeze(value.command, intent.admission,
					      &value.command.accounting_intent) ==
		       economic_accounting_error::ok);
		value.command.schema_version = 2;
		assert(economic_intent_decode(value.command.accounting_intent, &intent) ==
		       economic_accounting_error::ok);
		economic_accounting_plan plan;
		assert(economic_intent_plan_metadata(value.command, intent, &plan.metadata) ==
		       economic_accounting_error::ok);
		plan.items_before = { { 81,
					{ { item_owner_type::player, 7, 0 },
					  81,
					  0,
					  3,
					  item_custody_state::active,
					  5 } },
				      { 82,
					{ { item_owner_type::player, 7, 0 },
					  81,
					  81,
					  4,
					  item_custody_state::active,
					  0 } },
				      { 83, {} } };
		plan.items_after = plan.items_before;
		plan.items_after[0].position.revision = 4;
		plan.items_after[0].position.equipment_slot = 6;
		plan.items_after[1].position.owner = { item_owner_type::destruction, 0, 0 };
		plan.items_after[1].position.revision = 5;
		plan.items_after[1].position.state = item_custody_state::destroyed;
		plan.items_after[2].position = { { item_owner_type::player, 7, 0 }, 83, 0, 1,
						 item_custody_state::active,	    0 };
		for (size_t i = 0; i < plan.items_before.size(); ++i)
			plan.item_events.push_back(
				{ static_cast<uint32_t>(i), 0, plan.items_before[i].uid,
				  plan.items_before[i].position, plan.items_after[i].position });
		assert(economic_plan_encode(plan, &value.plan) == economic_accounting_error::ok);
		value.durable_revision = 1;
		return value;
	}
	if (large)
	{
		// Native storage's existing segment-crossing fixture technique: a
		// retained rejected command needs no game execution or activation.
		value.command.schema_version = 1;
		value.command.accounting_intent.clear();
		value.command.payload.resize(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES, 42);
		assert(economic_intent_freeze(value.command, intent.admission,
					      &value.command.accounting_intent) ==
		       economic_accounting_error::ok);
		value.command.schema_version = 2;
		value.result_code = ENOSPC;
		value.failure_stage = critical_failure_stage::coin_source_wallet_revision;
		value.result.resize(CRITICAL_COMPLETION_RESULT_MAX_BYTES, 7);
	}
	else
	{
		std::optional<economic_prepared_currency> prepared;
		const auto prepare = source ? economic_quest_wallet_reward_prepare :
					      economic_bank_transfer_prepare;
		assert(prepare(value.command, intent, state,
			       currency_revision_policy::flatfile_legacy,
			       &prepared) == economic_accounting_error::ok);
		assert(economic_plan_encode(prepared->plan(), &value.plan) ==
		       economic_accounting_error::ok);
		std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> result = {};
		assert(currency_command_encode_result(prepared->mutation().after(), &result));
		value.result.assign(result.begin(), result.end());
		value.durable_revision = 1;
	}
	return value;
}
int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string root = argv[1], mode = argv[2];
	if (mode == "hold-authority-lock" || mode == "pending-authority-journal")
	{
		flatfile_authority_lock lock;
		std::string error;
		assert(lock.acquire(root, &error));
		if (mode == "hold-authority-lock")
		{
			std::cout << "NATIVE_AUTHORITY_LOCK_HELD\n" << std::flush;
			std::string release;
			assert(std::getline(std::cin, release) && release == "release");
			return 0;
		}
		// The native writer publishes its encoded journal, then fails to replace
		// this private directory. No retained evidence or gameplay image changes.
		const auto target = std::filesystem::path(root) / "domains/audit-boundary-target";
		assert(std::filesystem::create_directory(target));
		flatfile_authority_commit_outcome outcome;
		const std::vector<flatfile_authority_operation> operations = {
			{ flatfile_authority_store::domains,
			  flatfile_authority_operation_kind::write,
			  "audit-boundary-target",
			  { 1, 2, 3 } }
		};
		assert(flatfile_authority_transaction_commit_operations_with_outcome(
			       root, lock, operations, &error, &outcome) ==
		       flatfile_authority_transaction_result::io_error);
		assert(outcome == flatfile_authority_commit_outcome::committed);
		assert(std::filesystem::is_regular_file(std::filesystem::path(root) /
							"domains/.critical-authority-transaction"));
		std::cout << "NATIVE_PENDING_AUTHORITY_JOURNAL\n";
		return 0;
	}
	if (mode == "compare-command-envelope")
	{
		critical_command command;
		command.schema_version = 2;
		command.operation_id = id(1);
		command.accounting_intent = { 1 }; // Envelope grammar only, not an EAI1 claim.
		command.accepted_at_usec = 1;
		command.source_site = critical_source_site::command;
		command.deadline_class = critical_deadline_class::interactive;
		command.keys = { { critical_entity_type::player, 1 } };
		size_t comparisons = 0;
		for (uint16_t type = 0; type <= 22; ++type)
			for (uint16_t version : { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, UINT16_MAX })
				for (bool publication : { false, true })
				{
					command.type = static_cast<critical_command_type>(type);
					command.payload_version = version;
					command.publication_required = publication;
					assert(critical_command_envelope_valid(command) ==
					       restore_economic_records::command_capability_valid(
						       type, version, publication));
					++comparisons;
				}
		command.type = critical_command_type::test;
		command.payload_version = 1;
		command.publication_required = false;
		for (uint8_t kind = 0; kind <= 16; ++kind)
			for (uint64_t key :
			     { uint64_t{ 0 }, uint64_t{ 1 }, UINT64_MAX - 1, UINT64_MAX })
			{
				command.keys = { { static_cast<critical_entity_type>(kind), key } };
				command.expected_revisions = { { command.keys.front(),
								 UINT64_MAX } };
				assert(critical_command_envelope_valid(command) ==
				       restore_economic_records::command_key_valid(kind, key));
				++comparisons;
			}
		std::cout << "NATIVE_INDEPENDENT_COMMAND_ENVELOPE_COMPARISONS " << comparisons
			  << '\n';
		return 0;
	}
	if (mode.starts_with("mobile-"))
	{
		assert(mode == "mobile-v1-live" || mode == "mobile-v1-retired" ||
		       mode == "mobile-v2-live" || mode == "mobile-v2-retired");
		const bool cash = mode.starts_with("mobile-v2"),
			   retired = mode.ends_with("retired");
		quest_mobile_native_reference reference;
		reference.mobile_instance_id = 42;
		reference.birth_operation = id(70);
		reference.birth_source = { economic_source_kind::npc_generation, id(71), id(72), 3,
					   5 };
		reference.mobile_vnum = 9001;
		reference.birthplace_vnum = 3000;
		reference.reset_zone_vnum = 30;
		reference.provenance = quest_mobile_birth_provenance::reset;
		reference.mobile_revision = 2;
		reference.stock_revision = 3;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> encoded_reference;
		assert(quest_mobile_native_reference_encode(reference, &encoded_reference) ==
		       player_snapshot_codec_result::ok);
		std::vector<player_item_snapshot> stock;
		if (!retired)
		{
			player_item_snapshot item{};
			item.object_uid = 81;
			item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			item.equipment_slot = 7;
			item.vnum = 9002;
			item.string_mask = 15;
			item.name = "private synthetic mobile item";
			item.short_description = "a private synthetic mobile item";
			item.description = "A private synthetic mobile item is here.";
			item.action_description = "private literal action";
			stock.push_back(item);
		}
		std::vector<uint8_t> encoded_stock;
		assert(player_item_snapshot_list_encode(stock, &encoded_stock) ==
		       player_snapshot_codec_result::ok);
		// Original native reference and stock bytes in explicit synthetic QMN
		// framing. No birth, source authentication, transition or storage owner.
		using namespace restore_economic_authority;
		bytes encoded{ 'Q', 'M', 'N', 'I', 'M', 'G', 0, 0 };
		put(encoded, cash ? 2 : 1, 2);
		put(encoded, retired ? 2 : 1, 1);
		put(encoded, 0, 1);
		put(encoded,
		    (cash ? QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD :
			    QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD) +
			    encoded_stock.size(),
		    4);
		encoded.insert(encoded.end(), encoded_reference.begin(), encoded_reference.end());
		const auto transition = id(73);
		encoded.insert(encoded.end(), transition.bytes.begin(), transition.bytes.end());
		if (cash)
		{
			put(encoded, 4, 8);
			for (uint64_t amount : { 5, 6, 7, 8 })
				put(encoded, retired ? 0 : amount, 8);
		}
		put(encoded, encoded_stock.size(), 4);
		encoded.insert(encoded.end(), encoded_stock.begin(), encoded_stock.end());
		const auto checksum = hash(encoded);
		encoded.insert(encoded.end(), checksum.begin(), checksum.end());
		std::ofstream output(std::filesystem::path(root) /
					     "domains/quest-mobile-native-42.qmn",
				     std::ios::binary);
		assert(output.write(reinterpret_cast<const char *>(encoded.data()),
				    encoded.size()));
		return 0;
	}
	if (mode == "compare-metadata")
	{
		size_t comparisons = 0;
		for (uint8_t reason = 1; reason <= 46; ++reason)
			for (uint8_t kind = 1; kind <= 23; ++kind)
			{
				std::vector<uint8_t> encoded(256);
				std::copy_n("EAI1", 4, encoded.begin());
				encoded[4] = encoded[7] = encoded[9] = 1;
				encoded[12] = encoded[16] = encoded[20] = encoded[28] = 1;
				encoded[24] = reason;
				encoded[26] = reason >= 38 && reason <= 42 ? 2 : 1;
				encoded[27] = 1;
				encoded[32] = 1;
				encoded[48] = 2;
				encoded[64] = 3;
				encoded[80] = 4;
				encoded[96] = 7;
				encoded[112] = kind;
				encoded[114] = 1;
				encoded[116] = 5;
				encoded[132] = 6;
				encoded[148] = 7;
				encoded[156] = 1;
				encoded[160] = encoded[192] = 1;
				economic_frozen_intent intent;
				const bool native = economic_intent_decode(encoded, &intent) ==
						    economic_accounting_error::ok;
				bool independent = true;
				try
				{
					restore_economic_records::intent_semantics(encoded);
				}
				catch (const std::runtime_error &)
				{
					independent = false;
				}
				assert(native == independent);
				++comparisons;
			}
		std::cout << "NATIVE_INDEPENDENT_METADATA_COMPARISONS " << comparisons << '\n';
		return 0;
	}
	if (mode == "decode-plan" || mode == "decode-intent" || mode == "decode-record")
	{
		std::ifstream input(root, std::ios::binary | std::ios::ate);
		assert(input);
		const auto size = input.tellg();
		assert(size >= 0 &&
		       static_cast<uint64_t>(size) <=
			       (mode == "decode-plan"	? ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES :
				mode == "decode-record" ? FLATFILE_ACCOUNTING_RECORD_MAX_BYTES :
							  ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES));
		std::vector<uint8_t> encoded(static_cast<size_t>(size));
		input.seekg(0);
		assert(input.read(reinterpret_cast<char *>(encoded.data()), size));
		if (mode == "decode-record")
		{
			flatfile_accounting_record value;
			const auto status = flatfile_accounting_record_decode(encoded, &value);
			std::cout << static_cast<unsigned>(status) << '\n';
			return status == flatfile_accounting_status::ok ? 0 : 1;
		}
		economic_accounting_plan plan;
		economic_frozen_intent intent;
		const auto status = mode == "decode-plan" ?
					    economic_plan_decode(encoded, &plan) :
					    economic_intent_decode(encoded, &intent);
		std::cout << static_cast<unsigned>(status) << '\n';
		return status == economic_accounting_error::ok ? 0 : 1;
	}
	for (auto name : { "domains", "economic-evidence" })
	{
		auto path = std::filesystem::path(root) / name;
		std::filesystem::create_directories(path);
		std::filesystem::permissions(path, std::filesystem::perms::owner_all);
	}
	flatfile_authority_lock lock;
	std::string error;
	assert(lock.acquire(root, &error));
	using access = flatfile_accounting_test_access;
	std::vector<flatfile_authority_operation> operations;
	auto commit = [&]
	{
		assert(access::commit(root, lock, operations, &error) ==
		       flatfile_authority_transaction_result::ok);
		operations.clear();
	};
	auto control = [&]
	{
		flatfile_economic_control value;
		assert(flatfile_economic_control_read(root, lock, &value, &error) == 0);
		return value;
	};
	assert(access::bootstrap(root, lock, id(1), id(2), &operations, &error) == 0);
	commit();
	if (mode == "bootstrap")
		return 0;
	if (mode == "evidence-bootstrap")
	{
		assert(access::initialize_evidence_bucket(root, lock, control().revision, 200,
							  id(8), &operations, &error) == 0);
		commit();
		assert(control().epoch_count == 0 &&
		       critical_operation_id_is_zero(control().active_epoch));
		return 0;
	}
	assert(mode == "lifetimes" || mode == "records" || mode == "item-records" ||
	       mode == "envelope-records" || mode == "source-claims" || mode == "retention" ||
	       mode == "baseline" || mode == "baseline-empty" || mode == "baseline-rich" ||
	       mode == "baseline-maximum" || mode == "baseline-full-index");
	for (size_t bucket = 0; bucket < 256; ++bucket)
	{
		assert(access::initialize_native_bucket(root, lock, control().revision, bucket,
							id(3), &operations, &error) == 0);
		commit();
	}
	auto create =
		[&](economic_account_kind kind, uint64_t context, flatfile_economic_locator locator)
	{
		flatfile_economic_mapping value;
		assert(access::create_mapping(root, lock, control().revision, kind, context,
					      locator, id(4), &value, &operations, &error) == 0);
		commit();
		return value;
	};
	const int32_t pid = mode == "retention" ? 1 : 11;
	auto wallet =
		create(economic_account_kind::wallet, 0, { 1, static_cast<uint64_t>(pid), {} });
	auto bank = create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	assert(access::rename_bank(root, lock, control().revision, bank.account, bank.revision,
				   "renamed", id(5), &operations, &error) == 0);
	commit();
	assert(access::retire_mapping(root, lock, control().revision, wallet.account,
				      wallet.revision, id(6), &operations, &error) == 0);
	commit();
	// A recreated native identity gets a new lifetime; the retired one remains.
	create(economic_account_kind::wallet, 0, { 1, static_cast<uint64_t>(pid), {} });
	create(economic_account_kind::auction_escrow, 0, { 4, UINT32_MAX, {} });
	create(economic_account_kind::pending_claim, 0, { 5, INT32_MAX, {} });
	create(economic_account_kind::treasury, 0, { 6, uint64_t{ UINT32_MAX } + 1, {} });
	flatfile_economic_epoch epoch;
	epoch.epoch = id(50);
	epoch.ordinal = 1;
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	epoch.creating_operation = id(7);
	assert(access::append_epoch(root, lock, control().revision, epoch, &operations, &error) ==
	       0);
	commit();
	epoch.predecessor = epoch.epoch;
	epoch.epoch = id(51);
	epoch.ordinal = 2;
	assert(access::append_epoch(root, lock, control().revision, epoch, &operations, &error) ==
	       0);
	commit();
	assert(critical_operation_id_is_zero(control().active_epoch));
	if (mode == "envelope-records")
	{
		assert(access::initialize_evidence_bucket(root, lock, control().revision, 1, id(8),
							  &operations, &error) == 0);
		commit();
		struct envelope
		{
			uint16_t type, version;
			bool publication;
			uint8_t kind;
			uint64_t key;
		};
		const envelope cases[] = {
			{ 18, 1, true, 13, 42 },	 { 21, 1, true, 15, UINT64_MAX - 1 },
			{ 21, 1, false, 15, 42 },	 { 15, 6, true, 1, 42 },
			{ 15, 7, true, 1, 42 },		 { 15, 8, true, 1, 42 },
			{ 15, 5, false, 1, 42 },	 { 3, 1, true, 1, 42 },
			{ 5, 1, true, 3, 42 },		 { 17, 1, true, 1, 42 },
			{ 20, 1, false, 9, UINT64_MAX },
		};
		uint32_t sequence = 0;
		for (const auto &entry : cases)
		{
			// Retained structural rejections exercise the exact native envelope.
			// They grant no writer capability and apply no gameplay/domain effect.
			auto value = record(++sequence, false);
			economic_frozen_intent intent;
			assert(economic_intent_decode(value.command.accounting_intent, &intent) ==
			       economic_accounting_error::ok);
			value.command.schema_version = 1;
			value.command.accounting_intent.clear();
			value.command.type = static_cast<critical_command_type>(entry.type);
			value.command.payload_version = entry.version;
			value.command.keys = { { static_cast<critical_entity_type>(entry.kind),
						 entry.key } };
			value.command.expected_revisions = { { value.command.keys.front(),
							       UINT64_MAX } };
			assert(economic_intent_freeze(value.command, intent.admission,
						      &value.command.accounting_intent) ==
			       economic_accounting_error::ok);
			value.command.schema_version = 2;
			value.command.publication_required = entry.publication;
			value.plan.clear();
			value.result_code = EINVAL;
			value.durable_revision = 0;
			value.result.clear();
			std::vector<uint8_t> encoded;
			flatfile_accounting_record decoded;
			assert(flatfile_accounting_record_encode(value, &encoded) ==
			       flatfile_accounting_status::ok);
			assert(flatfile_accounting_record_decode(encoded, &decoded) ==
			       flatfile_accounting_status::ok);
			assert(critical_command_equal(value.command, decoded.command));
			assert(access::stage(root, lock, value, &operations, &error) ==
			       flatfile_accounting_status::ok);
			commit();
		}
		assert(critical_operation_id_is_zero(control().active_epoch));
		std::cout << "NATIVE_ENVELOPE_RECORDS " << sequence << '\n';
		return 0;
	}
	if (mode.starts_with("baseline"))
	{
		economic_baseline_batch batch;
		batch.lineage = id(1);
		batch.epoch = id(50);
		batch.preparation_id = id(52);
		batch.actor_id = 7;
		batch.opening_account = { id(1), economic_account_kind::opening, 1, 0 };
		batch.boundary_digest.fill(1);
		batch.coverage_digest.fill(2);
		economic_digest native_digest;
		native_digest.fill(3);
		batch.holdings.push_back({ { id(1), economic_account_kind::wallet, 3, 0 },
					   { 100, 0, 0, 0 },
					   0,
					   native_digest });
		assert(access::initialize_baseline(root, lock, id(1), batch.epoch,
						   batch.opening_account, id(53), &operations,
						   &error) == flatfile_accounting_status::ok);
		commit();
		if (mode == "baseline-empty")
			return 0;
		auto item = [&](uint64_t uid, item_owner_identity owner, uint64_t root_uid,
				uint64_t parent, uint64_t revision, item_custody_state state)
		{
			economic_item_snapshot snapshot;
			snapshot.uid = uid;
			snapshot.position = { owner, root_uid, parent, revision, state, 0 };
			batch.items.push_back({ snapshot, native_digest });
		};
		if (mode == "baseline-rich")
		{
			batch.holdings.clear();
			// Deliberately unsorted inputs cross little-endian numeric boundaries.
			for (uint64_t lifetime : { 512u, 256u, 255u })
				batch.holdings.push_back(
					{ { id(1), economic_account_kind::wallet, lifetime, 0 },
					  { lifetime == 512 ? 0 : 10, 2, 3, 4 },
					  UINT64_MAX,
					  native_digest });
			batch.holdings.back().balance = {};
			batch.holdings.push_back(
				{ { id(1), economic_account_kind::bank, UINT64_MAX, UINT64_MAX },
				  { INT64_MAX, 0, 0, 0 },
				  UINT64_MAX,
				  native_digest });
			item(256, { item_owner_type::player, 7, 0 }, 255, 255, 0,
			     item_custody_state::quarantined);
			item(255, { item_owner_type::player, 7, 0 }, 255, 0, UINT64_MAX,
			     item_custody_state::active);
			item(512, { item_owner_type::destruction, 0, 0 }, UINT64_MAX, UINT64_MAX,
			     UINT64_MAX, item_custody_state::destroyed);
			item(513, { item_owner_type::pet, 8, INT32_MAX }, 513, 0, UINT64_MAX,
			     item_custody_state::active);
			item(514, { item_owner_type::collector, 9, 0 }, 514, 0, 0,
			     item_custody_state::quarantined);
			item(515, { item_owner_type::system, 0, 0 }, 515, 0, 0,
			     item_custody_state::active);
		}
		if (mode == "baseline-maximum")
		{
			batch.holdings.clear();
			for (uint64_t lifetime = 1; lifetime <= ECONOMIC_BASELINE_MAX_HOLDINGS;
			     ++lifetime)
				batch.holdings.push_back(
					{ { id(1), economic_account_kind::wallet, lifetime, 0 },
					  { 1, 2, 3, 4 },
					  UINT64_MAX,
					  native_digest });
			for (uint64_t uid = 10000;
			     uid < 10000 + ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++uid)
				item(uid, { item_owner_type::player, 7, 0 }, 10000,
				     uid == 10000 ? 0 : uid - 1, UINT64_MAX,
				     item_custody_state::active);
		}
		auto stage = [&]
		{
			std::optional<economic_prepared_baseline> prepared;
			assert(economic_baseline_prepare(batch, &prepared) ==
			       economic_accounting_error::ok);
			critical_command command;
			assert(economic_baseline_command_build(*prepared, UINT64_MAX, &command) ==
			       economic_accounting_error::ok);
			const auto bucket = command.operation_id.bytes[0];
			if (!(control().evidence_initialized[bucket / 8] & (1u << (bucket % 8))))
			{
				assert(access::initialize_evidence_bucket(
					       root, lock, control().revision, bucket, id(8),
					       &operations, &error) == 0);
				commit();
			}
			assert(access::stage_baseline(root, lock, command, *prepared, &operations,
						      &error) == flatfile_accounting_status::ok);
			commit();
		};
		if (mode == "baseline-full-index")
		{
			// A full native shard is 88 bytes larger than a generic 2 MiB file.
			for (uint64_t first = 1; first <= 65536;
			     first += ECONOMIC_BASELINE_MAX_HOLDINGS)
			{
				batch.batch_index = (first - 1) / ECONOMIC_BASELINE_MAX_HOLDINGS;
				batch.holdings.clear();
				for (uint64_t lifetime = first;
				     lifetime <= 65536 &&
				     lifetime < first + ECONOMIC_BASELINE_MAX_HOLDINGS;
				     ++lifetime)
					batch.holdings.push_back(
						{ { id(1), economic_account_kind::wallet,
						    lifetime * 16, 0 },
						  {},
						  UINT64_MAX,
						  native_digest });
				stage();
			}
		}
		else
			stage();
		if (mode == "baseline-rich")
		{
			const auto first = batch;
			batch.batch_index = 1;
			batch.holdings = { { { id(1), economic_account_kind::treasury, 1025, 0 },
					     {},
					     UINT64_MAX,
					     native_digest } };
			batch.items.clear();
			item(1025, { item_owner_type::room, 300, UINT64_MAX }, 1025, 0, 0,
			     item_custody_state::active);
			stage();
			batch.batch_index = UINT64_MAX;
			batch.holdings.clear();
			batch.items.clear();
			stage();
			batch = first;
			batch.preparation_id = id(54);
			batch.epoch = id(51);
			assert(access::initialize_baseline(root, lock, id(1), batch.epoch,
							   batch.opening_account, id(55),
							   &operations, &error) ==
			       flatfile_accounting_status::ok);
			commit();
			stage();
		}
		assert(critical_operation_id_is_zero(control().active_epoch));
		return 0;
	}
	if (mode == "records" || mode == "item-records" || mode == "source-claims" ||
	    mode == "retention")
	{
		const bool source = mode != "records" && mode != "item-records";
		for (size_t bucket : { 1, 2 })
		{
			assert(access::initialize_evidence_bucket(root, lock, control().revision,
								  bucket, id(8), &operations,
								  &error) == 0);
			commit();
		}
		// Records mode seals the first segment. Source mode retains two successful
		// claims and two claimless rejections. No native domain effect is applied.
		for (uint32_t sequence = 1; sequence <= (mode == "item-records" ? 1u :
							 mode == "records"	? 23u :
										  4u);
		     ++sequence)
		{
			auto value = record(sequence, mode == "records" && sequence != 1, source,
					    pid, mode == "item-records");
			if (source && sequence >= 3)
			{
				value.plan.clear();
				value.result_code = EEXIST;
				if (sequence == 4)
				{
					// A rejected retry may share a successful root's source.
					economic_frozen_intent intent, original;
					assert(economic_intent_decode(
						       value.command.accounting_intent, &intent) ==
					       economic_accounting_error::ok);
					assert(economic_intent_decode(
						       record(1, false, true, pid)
							       .command.accounting_intent,
						       &original) == economic_accounting_error::ok);
					intent.admission.metadata.source_event =
						original.admission.metadata.source_event;
					value.command.schema_version = 1;
					value.command.accounting_intent.clear();
					assert(economic_intent_freeze(
						       value.command, intent.admission,
						       &value.command.accounting_intent) ==
					       economic_accounting_error::ok);
					value.command.schema_version = 2;
				}
			}
			assert(access::stage(root, lock, value, &operations, &error) ==
			       flatfile_accounting_status::ok);
			assert(access::stage_source_claim(root, lock, value, &operations, &error) ==
			       flatfile_accounting_status::ok);
			commit();
		}
		assert(critical_operation_id_is_zero(control().active_epoch));
	}
}
