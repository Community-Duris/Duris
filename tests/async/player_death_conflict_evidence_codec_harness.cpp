#include "player/player_snapshot_codec.h"
#include "classes/necromancy.h"
#include "core/files.h"
#include "world/vnum.obj.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
void require(bool value, const char *message)
{
	if (!value)
	{
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

player_snapshot legacy_death()
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.pid = 7;
	snapshot.revision = 11;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = RENT_DEATH;
	snapshot.encoded_size_bound = 4096;
	snapshot.death.emplace();
	auto &death = *snapshot.death;
	death.operation_id.bytes[0] = 17;
	death.corpse_room_vnum = 3001;
	death.wallet_revision = 5;
	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 200;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_PID] = snapshot.pid;
	corpse.values[CORPSE_SAVEID] = 5001;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	death.corpse.push_back(corpse);
	player_item_snapshot item = {};
	item.parent_index = 0;
	item.object_uid = 201;
	item.vnum = 15;
	death.corpse.push_back(item);
	player_death_custody_snapshot custody = {};
	custody.item.item_uid = item.object_uid;
	custody.item.root_item_uid = item.object_uid;
	custody.item.expected_item_revision = 3;
	custody.item.vnum = item.vnum;
	custody.item.expected_state = item_custody_state::active;
	custody.owner.type = item_owner_type::player;
	custody.owner.id = snapshot.pid;
	custody.owner_revision = 2;
	death.custody.push_back(custody);
	return snapshot;
}

player_snapshot with_evidence()
{
	auto snapshot = legacy_death();
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_EVIDENCE_SCHEMA_VERSION;
	snapshot.death->conflict_evidence.emplace();
	auto &evidence = *snapshot.death->conflict_evidence;
	evidence.player_items.columns = { "id",	  "pid",	 "obj_uid",    "container_id",
					  "vnum", "custom_name", "description" };
	// Duplicate observed UIDs and malformed/conflicting topology are retained,
	// not normalized into a new authoritative inventory or silently deduplicated.
	evidence.player_items.rows = { { "88", "7", "9000000000000000002", std::nullopt, "15",
					 std::string("\0'\\\xff", 4), "" },
				       { "89", "7", "9000000000000000002", "", "15", "",
					 std::nullopt } };
	evidence.player_item_affects.columns = { "id", "item_id", "location", "modifier" };
	evidence.player_item_affects.rows = { { "300", "89", "7", "-40" } };
	evidence.player_item_extra_descr.columns = { "id", "item_id", "keyword", "description" };
	evidence.player_item_extra_descr.rows = { { "301", "88", "sigil",
						    std::string("mark\0tail", 9) },
						  { "302", "89", "inscription", std::nullopt } };
	evidence.item_current_owner.columns = { "item_uid",	 "owner_type",
						"owner_id",	 "owner_context_id",
						"root_item_uid", "parent_item_uid",
						"item_revision", "vnum",
						"state" };
	evidence.item_current_owner.rows = { { "9000000000000000002", "1", "99", "0",
					       "9000000000000000001", std::nullopt,
					       "18446744073709551615", "15", "1" } };
	evidence.item_owner_revision.columns = { "owner_type", "owner_id", "owner_context_id",
						 "revision" };
	evidence.item_owner_revision.rows = { { "1", "99", "0", "123" } };
	return snapshot;
}

bool same_table(const player_death_evidence_table &left, const player_death_evidence_table &right)
{
	return left.columns == right.columns && left.rows == right.rows;
}

void rejected_encode(const player_snapshot &snapshot)
{
	std::vector<uint8_t> output = { 91, 92 };
	require(player_snapshot_encode(snapshot, &output) != player_snapshot_codec_result::ok,
		"invalid or unrepresentable evidence accepted");
	require(output == std::vector<uint8_t>({ 91, 92 }), "failed encode changed output");
}

void rejected_decode(const std::vector<uint8_t> &bytes, size_t size)
{
	player_snapshot output = {};
	output.pid = 999;
	require(player_snapshot_decode(bytes.data(), size, &output) !=
			player_snapshot_codec_result::ok,
		"malformed evidence accepted");
	require(output.pid == 999 && !output.death, "failed decode published partial output");
}

void set_u32(std::vector<uint8_t> &bytes, size_t offset, uint32_t value)
{
	for (size_t index = 0; index < sizeof(value); ++index)
	{
		bytes.at(offset + index) = value & 0xff;
		value >>= 8;
	}
}
} // namespace

int main(int argc, char **argv)
{
	auto legacy = legacy_death();
	if (argc == 2 && std::string(argv[1]) == "legacy-normal")
	{
		legacy.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		legacy.death.reset();
	}
	std::vector<uint8_t> legacy_bytes;
	require(player_snapshot_encode(legacy, &legacy_bytes) == player_snapshot_codec_result::ok,
		"legacy encode failed");
	if (argc == 2)
	{
		if (std::string(argv[1]) == "conflict-evidence")
		{
			legacy_bytes.clear();
			require(player_snapshot_encode(with_evidence(), &legacy_bytes) ==
					player_snapshot_codec_result::ok,
				"conflict fixture encode failed");
		}
		std::fwrite(legacy_bytes.data(), 1, legacy_bytes.size(), stdout);
		return 0;
	}
	player_snapshot old_readback = {};
	require(player_snapshot_decode(legacy_bytes.data(), legacy_bytes.size(), &old_readback) ==
				player_snapshot_codec_result::ok &&
			!old_readback.death->conflict_evidence,
		"legacy death decode changed");
	auto snapshot = with_evidence();
	std::vector<uint8_t> bytes;
	require(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok,
		"death conflict evidence encoding failed");
	player_snapshot decoded = {};
	require(player_snapshot_decode(bytes.data(), bytes.size(), &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded.death->conflict_evidence,
		"death conflict evidence decoding failed");
	const auto &before = *snapshot.death->conflict_evidence;
	const auto &after = *decoded.death->conflict_evidence;
	require(same_table(before.player_items, after.player_items) &&
			same_table(before.player_item_affects, after.player_item_affects) &&
			same_table(before.player_item_extra_descr, after.player_item_extra_descr) &&
			same_table(before.item_current_owner, after.item_current_owner) &&
			same_table(before.item_owner_revision, after.item_owner_revision),
		"raw payload/custody bytes or NULL semantics changed");
	require(decoded.pid == snapshot.pid && decoded.revision == snapshot.revision &&
			decoded.death->operation_id.bytes == snapshot.death->operation_id.bytes &&
			decoded.items.empty() && decoded.death->corpse.size() == 2 &&
			decoded.death->custody.size() == 1,
		"evidence lost operation binding or was promoted into inventory/custody");
	std::vector<uint8_t> replay;
	require(player_snapshot_encode(decoded, &replay) == player_snapshot_codec_result::ok &&
			replay == bytes,
		"evidence encoding is not replay-stable");

	// A downgrade must reject rather than silently discard retained payloads.
	for (bool evidence : { false, true })
	{
		auto with_receipt = evidence ? snapshot : legacy;
		with_receipt.schema_version =
			evidence ? PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION :
				   PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
		player_spell_effect_receipt_snapshot receipt = {};
		receipt.operation_id.bytes[0] = 77;
		receipt.effect_id = 6;
		with_receipt.spell_effect_receipts.push_back(receipt);
		std::vector<uint8_t> extended;
		require(player_snapshot_encode(with_receipt, &extended) ==
				player_snapshot_codec_result::ok,
			"death spell receipt encode failed");
		player_snapshot readback;
		require(player_snapshot_decode(extended.data(), extended.size(), &readback) ==
					player_snapshot_codec_result::ok &&
				readback.spell_effect_receipts.size() == 1 &&
				readback.spell_effect_receipts[0].operation_id.bytes ==
					receipt.operation_id.bytes &&
				readback.spell_effect_receipts[0].effect_id == 6 &&
				readback.death->conflict_evidence.has_value() == evidence,
			"death or evidence lost its exact spell receipt");
		require(player_snapshot_encode(readback, &replay) ==
					player_snapshot_codec_result::ok &&
				replay == extended,
			"death spell receipt replay bytes changed");
		auto downgrade = with_receipt;
		downgrade.schema_version = evidence ?
						   PLAYER_SNAPSHOT_DEATH_EVIDENCE_SCHEMA_VERSION :
						   PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
		rejected_encode(downgrade);
		downgrade = with_receipt;
		downgrade.spell_effect_receipts.clear();
		rejected_encode(downgrade);
		for (size_t size = 0; size < extended.size(); ++size)
			rejected_decode(extended, size);
	}
	for (bool evidence : { false, true })
		for (bool spell : { false, true })
		{
			auto with_xp = evidence ? snapshot : legacy;
			with_xp.schema_version =
				evidence ? PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION :
					   PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
			player_quest_xp_receipt_snapshot xp = {};
			xp.offering_operation.bytes[0] = 88;
			xp.reward_index = 0;
			xp.amount = 75;
			with_xp.quest_xp_receipts.push_back(xp);
			if (spell)
			{
				player_spell_effect_receipt_snapshot receipt = {};
				receipt.operation_id.bytes[0] = 77;
				receipt.effect_id = 6;
				with_xp.spell_effect_receipts.push_back(receipt);
			}
			std::vector<uint8_t> extended;
			require(player_snapshot_encode(with_xp, &extended) ==
					player_snapshot_codec_result::ok,
				"death XP receipt encode failed");
			player_snapshot readback;
			require(player_snapshot_decode(extended.data(), extended.size(),
						       &readback) ==
						player_snapshot_codec_result::ok &&
					readback.quest_xp_receipts.size() == 1 &&
					readback.quest_xp_receipts[0].offering_operation.bytes ==
						xp.offering_operation.bytes &&
					readback.quest_xp_receipts[0].amount == 75 &&
					readback.spell_effect_receipts.size() ==
						(spell ? 1U : 0U) &&
					readback.death->conflict_evidence.has_value() == evidence,
				"death XP or optional spell receipt was lost");
			require(player_snapshot_encode(readback, &replay) ==
						player_snapshot_codec_result::ok &&
					replay == extended,
				"death XP receipt replay bytes changed");
			auto downgrade = with_xp;
			downgrade.schema_version =
				evidence ? PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION :
					   PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
			rejected_encode(downgrade);
			downgrade = with_xp;
			downgrade.quest_xp_receipts.clear();
			rejected_encode(downgrade);
			for (size_t size = 0; size < extended.size(); ++size)
				rejected_decode(extended, size);
		}
	auto invalid = snapshot;
	invalid.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence.reset();
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.columns[1] = "id";
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.columns[0] = "id;DROP";
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.columns[1] = "missing_pid";
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->item_current_owner.columns.clear();
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.rows[0].pop_back();
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.columns.resize(
		PLAYER_DEATH_EVIDENCE_MAX_COLUMNS + 1, "column");
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.rows.resize(PLAYER_SNAPSHOT_MAX_ROWS + 1);
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.rows[0][5] =
		std::string(PLAYER_SNAPSHOT_MAX_BYTES + 1, 'x');
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.rows[0][5] =
		std::string(PLAYER_SNAPSHOT_MAX_BYTES / 2, 'x');
	invalid.death->conflict_evidence->player_items.rows[1][5] =
		std::string(PLAYER_SNAPSHOT_MAX_BYTES / 2, 'y');
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_items.rows.clear();
	invalid.death->conflict_evidence->player_item_affects.rows.clear();
	invalid.death->conflict_evidence->player_item_extra_descr.rows.clear();
	invalid.death->conflict_evidence->item_current_owner.rows.clear();
	invalid.death->conflict_evidence->item_owner_revision.rows.clear();
	rejected_encode(invalid);

	invalid = snapshot;
	invalid.death->conflict_evidence->player_item_affects.columns.clear();
	rejected_encode(invalid);
	invalid = snapshot;
	invalid.death->conflict_evidence->player_item_extra_descr.columns[1] = "missing_item_id";
	rejected_encode(invalid);
	auto long_description = snapshot;
	long_description.death->conflict_evidence->player_item_extra_descr.rows[0][3] =
		std::string(65535, 'x');
	(*long_description.death->conflict_evidence->player_item_extra_descr.rows[0][3])[4096] =
		'\0';
	require(player_snapshot_encode(long_description, &replay) ==
			player_snapshot_codec_result::ok,
		"SQL TEXT-sized evidence was truncated to the ordinary string bound");
	require(player_snapshot_decode(replay.data(), replay.size(), &decoded) ==
				player_snapshot_codec_result::ok &&
			same_table(
				long_description.death->conflict_evidence->player_item_extra_descr,
				decoded.death->conflict_evidence->player_item_extra_descr),
		"large SQL TEXT evidence did not round trip exactly");
	// Missing authority is evidence of absence, not an invented owner row.
	auto absent_owner = snapshot;
	absent_owner.death->conflict_evidence->item_current_owner.rows.clear();
	absent_owner.death->conflict_evidence->item_owner_revision.rows.clear();
	require(player_snapshot_encode(absent_owner, &replay) == player_snapshot_codec_result::ok,
		"absent authority observations rejected");
	// An entirely missing player payload may still have retained authority.
	auto absent_payload = snapshot;
	absent_payload.death->conflict_evidence->player_items.rows.clear();
	require(player_snapshot_encode(absent_payload, &replay) == player_snapshot_codec_result::ok,
		"missing payload observations rejected");

	for (size_t size = 0; size < bytes.size(); ++size)
		rejected_decode(bytes, size);
	auto corrupt = bytes;
	corrupt.push_back(0);
	rejected_decode(corrupt, corrupt.size());
	corrupt = bytes;
	set_u32(corrupt, 0, 9); // Unassigned ordinary format is not death evidence.
	rejected_decode(corrupt, corrupt.size());
	const size_t evidence_offset = legacy_bytes.size();
	corrupt = bytes;
	set_u32(corrupt, evidence_offset, PLAYER_DEATH_EVIDENCE_MAX_COLUMNS + 1);
	rejected_decode(corrupt, corrupt.size());
	size_t rows_offset = evidence_offset + sizeof(uint32_t);
	for (const auto &column : before.player_items.columns)
		rows_offset += sizeof(uint32_t) + column.size();
	corrupt = bytes;
	set_u32(corrupt, rows_offset, UINT32_MAX);
	rejected_decode(corrupt, corrupt.size());
	corrupt = bytes;
	corrupt.at(rows_offset + sizeof(uint32_t)) = 2;
	rejected_decode(corrupt, corrupt.size());
	std::printf("PASS death evidence: exact bytes, auxiliary payloads, SQL TEXT, "
		    "NULL/empty, foreign/absent authority, "
		    "operation binding, no inventory grant, bounds, %zu truncated prefixes\n",
		    bytes.size());
	return 0;
}
