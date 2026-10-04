#include "player/player_quarantine_recovery.h"

#include "player/player_snapshot_codec.h"
#include "persistence/critical_command.h"
#include "persistence/critical_command_repository.h"
#include "persistence/persistence_observability.h"
#include "player/player_snapshot_repository.h"
#include "player/player_sql_transaction_cleanup.h"
#include "flatfile/flatfile_player_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_mode.h"
#include "sql/sql_pool.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <cstring>
#include <memory>
#include "core/utility.h"

namespace
{
bool refuse(std::string *error, const char *message)
{
	if (error)
		*error = message;
	return false;
}

bool ordinary(const player_snapshot &snapshot)
{
	return snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION && !snapshot.death &&
	       snapshot.quest_xp_receipts.empty() && snapshot.spell_effect_receipts.empty() &&
	       snapshot.craft_receipts.empty();
}

bool complete(const player_load_result &current)
{
	std::vector<uint8_t> encoded;
	auto snapshot = current.snapshot;
	// Native SQL loads are projections, not queued save requests. Their byte
	// admission bound is set when creating the recovery checkpoint.
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	return current.outcome == player_load_outcome::applied && !current.degraded_components &&
	       !current.stale_item_rows && !current.missing_payload_rows &&
	       !current.promoted_item_rows && !current.repaired_item_rows &&
	       current.pending_quest_rewards.empty() &&
	       current.pending_quest_xp_entitlements.empty() &&
	       current.spell_effect_receipts.empty() && current.craft_receipts.empty() &&
	       ordinary(current.snapshot) && current.pid > 0 &&
	       current.snapshot.pid == current.pid && current.snapshot.revision &&
	       current.snapshot.components == PLAYER_CHECKPOINT_COMPONENT_ALL &&
	       current.item_identities.size() == current.snapshot.items.size() &&
	       current.pet_identities.size() == current.snapshot.pets.size() &&
	       player_snapshot_encode(snapshot, &encoded) == player_snapshot_codec_result::ok;
}

std::vector<uint8_t> authority(const player_load_result &current)
{
	std::vector<uint8_t> bytes;
	auto number = [&](uint64_t value)
	{
		for (unsigned int byte = 0; byte < 8; ++byte)
			bytes.push_back(static_cast<uint8_t>(value >> (byte * 8)));
	};
	const auto &domain = current.domains;
	number(domain.wallet_revision);
	number(domain.epic_revision);
	number(domain.frag_revision);
	number(domain.bank_revision);
	number(domain.base_stat_revision);
	for (auto value : domain.wallet)
		number(value);
	for (auto value : domain.bank)
		number(value);
	for (auto value : domain.base_stats)
		number(static_cast<uint64_t>(value));
	number(domain.epics);
	number(domain.frags);
	number(domain.old_frags);
	number(current.item_owner_revision);
	auto identities = current.item_identities;
	for (const auto &pet : current.pet_identities)
	{
		number(pet.pet_uid);
		number(pet.owner_revision);
		identities.insert(identities.end(), pet.item_identities.begin(),
				  pet.item_identities.end());
	}
	std::sort(identities.begin(), identities.end(),
		  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
	number(identities.size());
	for (const auto &item : identities)
	{
		number(item.item_uid);
		number(item.root_item_uid);
		number(item.parent_item_uid);
		number(static_cast<uint8_t>(item.owner.type));
		number(item.owner.id);
		number(item.owner.context_id);
		number(item.item_revision);
		number(item.owner_revision);
		number(static_cast<uint8_t>(item.state));
	}
	return bytes;
}

bool native_status(player_status_field field)
{
	return field == player_status_field::epics || field == player_status_field::frags ||
	       field == player_status_field::old_frags || field == player_status_field::copper ||
	       field == player_status_field::silver || field == player_status_field::gold ||
	       field == player_status_field::platinum ||
	       (field >= player_status_field::base_strength &&
		field <= player_status_field::base_luck);
}

player_snapshot canonical(player_snapshot snapshot)
{
	snapshot.save_intent = 0;
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	std::sort(snapshot.status_integers.begin(), snapshot.status_integers.end(),
		  [](const auto &a, const auto &b) { return a.field < b.field; });
	std::sort(snapshot.status_strings.begin(), snapshot.status_strings.end(),
		  [](const auto &a, const auto &b) { return a.field < b.field; });
	// These native SQL tables reload by logical key. Their row order is not
	// gameplay state; list-ordered affects, shapes, pets and item trees stay ordered.
	const auto indices = [](auto &rows)
	{
		std::sort(rows.begin(), rows.end(),
			  [](const auto &a, const auto &b) { return a.index < b.index; });
	};
	indices(snapshot.languages);
	indices(snapshot.introductions);
	indices(snapshot.timers);
	indices(snapshot.undead_slots);
	indices(snapshot.forged_items);
	std::sort(snapshot.skills.begin(), snapshot.skills.end(),
		  [](const auto &a, const auto &b) { return a.skill_id < b.skill_id; });
	std::sort(snapshot.trophies.begin(), snapshot.trophies.end(),
		  [](const auto &a, const auto &b) { return a.zone_number < b.zone_number; });
	return snapshot;
}

bool overlay(const player_snapshot &incoming, player_snapshot *target)
{
	const auto items = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
	if (!ordinary(incoming) || incoming.pid != target->pid || !incoming.components ||
	    (incoming.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) ||
	    ((incoming.components & items) && (incoming.components & items) != items))
		return false;
	if (incoming.components & PLAYER_COMPONENT_STATUS)
	{
		const auto name = [](const player_snapshot &value)
		{
			for (const auto &row : value.status_strings)
				if (row.field == player_status_string_field::name)
					return row.value;
			return std::string{};
		};
		if (name(incoming) != name(*target))
			return false;
		auto authoritative = target->status_integers;
		target->status_integers = incoming.status_integers;
		for (const auto &row : authoritative)
			if (native_status(row.field))
			{
				auto found = std::find_if(target->status_integers.begin(),
							  target->status_integers.end(),
							  [&](const auto &value)
							  { return value.field == row.field; });
				if (found == target->status_integers.end())
					target->status_integers.push_back(row);
				else
					*found = row;
			}
		target->status_strings = incoming.status_strings;
		target->conditions = incoming.conditions;
		target->quest_values = incoming.quest_values;
		target->output_preferences = incoming.output_preferences;
		target->room_vnum = incoming.room_vnum;
	}
	if (incoming.components & PLAYER_COMPONENT_LANGUAGES)
		target->languages = incoming.languages;
	if (incoming.components & PLAYER_COMPONENT_INTRODUCTIONS)
		target->introductions = incoming.introductions;
	if (incoming.components & PLAYER_COMPONENT_TIMERS)
		target->timers = incoming.timers;
	if (incoming.components & PLAYER_COMPONENT_UNDEAD_SLOTS)
		target->undead_slots = incoming.undead_slots;
	if (incoming.components & PLAYER_COMPONENT_FORGED_ITEMS)
		target->forged_items = incoming.forged_items;
	if (incoming.components & PLAYER_COMPONENT_GRANTED_COMMANDS)
		target->granted_commands = incoming.granted_commands;
	if (incoming.components & PLAYER_COMPONENT_SKILLS)
		target->skills = incoming.skills;
	if (incoming.components & PLAYER_COMPONENT_AFFECTS)
		target->affects = incoming.affects;
	if (incoming.components & PLAYER_COMPONENT_SHAPECHANGES)
		target->shapes = incoming.shapes;
	if (incoming.components & PLAYER_COMPONENT_TROPHIES)
		target->trophies = incoming.trophies;
	// Pet custody cannot be inferred from a player creation command. The first
	// slice preserves the complete native pet projection and refuses changes.
	if (incoming.components & PLAYER_COMPONENT_PETS)
	{
		auto expected = *target, supplied = *target;
		supplied.pets = incoming.pets;
		if (!player_quarantine_recovery_projection_equal(expected, supplied))
			return false;
	}
	return true;
}
} // namespace

bool player_quarantine_recovery_projection_equal(const player_snapshot &left,
						 const player_snapshot &right)
{
	std::vector<uint8_t> a, b;
	return player_snapshot_encode(canonical(left), &a) == player_snapshot_codec_result::ok &&
	       player_snapshot_encode(canonical(right), &b) == player_snapshot_codec_result::ok &&
	       a == b;
}

bool player_quarantine_recovery_state_matches(const player_load_result &current,
					      const player_save_recovery_record &record,
					      bool applied)
{
	return complete(current) && current.pid == record.replacement.pid &&
	       authority(current) == record.authority_evidence &&
	       player_quarantine_recovery_projection_equal(
		       current.snapshot, applied ? record.replacement : record.baseline);
}

bool player_quarantine_recovery_build(const player_load_result &current,
				      const std::vector<player_snapshot> &frames,
				      const std::vector<player_recovery_creation> &creations,
				      player_save_recovery_record *record, std::string *error)
{
	if (!record || !complete(current) || frames.empty() || creations.empty() ||
	    creations.size() > 64)
		return refuse(
			error,
			"incomplete native state, component/receipt obligation, or missing grant command");
	auto ordered = frames;
	std::sort(ordered.begin(), ordered.end(),
		  [](const auto &a, const auto &b) { return a.revision < b.revision; });
	player_revision_t revision = current.snapshot.revision;
	std::map<uint64_t, size_t> native_items;
	for (size_t index = 0; index < current.snapshot.items.size(); ++index)
		if (!current.snapshot.items[index].object_uid ||
		    !native_items.emplace(current.snapshot.items[index].object_uid, index).second)
			return refuse(error, "duplicate native item UID");
	std::set<uint64_t> created;
	record->creation_commands.clear();
	for (const auto &creation : creations)
	{
		item_transfer_payload payload{};
		if (!item_transfer_command_decode_payload(creation.command, &payload) ||
		    payload.reason != item_transfer_reason::creation ||
		    payload.from_owner.type != item_owner_type::system ||
		    payload.to_owner.type != item_owner_type::player ||
		    payload.to_owner.id != static_cast<uint64_t>(current.pid) ||
		    payload.to_owner.context_id || !payload.item_blob_size ||
		    payload.target_parent_item_uid ||
		    payload.continuation.kind != item_transfer_continuation_kind::none ||
		    creation.receipt.item_count != payload.item_count ||
		    creation.receipt.max_item_revision != 1 ||
		    creation.receipt.root_item_uid != payload.target_root_item_uid ||
		    payload.expected_from_revision == UINT64_MAX ||
		    payload.expected_to_revision == UINT64_MAX ||
		    creation.receipt.from_owner_revision != payload.expected_from_revision + 1 ||
		    creation.receipt.to_owner_revision != payload.expected_to_revision + 1)
			return refuse(error, "unsupported or conflicting creation receipt");
		std::vector<player_item_snapshot> frozen;
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &frozen) != player_snapshot_codec_result::ok ||
		    frozen.size() != payload.item_count)
			return refuse(error, "missing frozen grant payload");
		for (const auto &item : frozen)
		{
			const auto found = native_items.find(item.object_uid);
			if (found == native_items.end() || !created.insert(item.object_uid).second)
				return refuse(error, "missing or duplicated granted UID");
			const auto identity =
				std::find_if(current.item_identities.begin(),
					     current.item_identities.end(), [&](const auto &value)
					     { return value.item_uid == item.object_uid; });
			const auto proof = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &value)
				{ return value.item_uid == item.object_uid; });
			if (identity == current.item_identities.end() ||
			    proof == payload.items.begin() + payload.item_count ||
			    identity->root_item_uid != proof->root_item_uid ||
			    identity->parent_item_uid != proof->parent_item_uid)
				return refuse(
					error,
					"native grant topology differs from its original receipt");
			auto native = current.snapshot.items[found->second], expected = item;
			native.parent_index = expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			std::vector<uint8_t> a, b;
			if (player_item_snapshot_list_encode({ native }, &a) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode({ expected }, &b) !=
				    player_snapshot_codec_result::ok ||
			    a != b)
				return refuse(error, "grant payload changed after commit");
		}
		std::vector<uint8_t> encoded;
		if (critical_command_encode(creation.command, &encoded) !=
		    critical_command_codec_result::ok)
			return refuse(error, "invalid retained command envelope");
		record->creation_commands.push_back(std::move(encoded));
	}
	for (const auto &item : current.item_identities)
		if (item.item_revision != 1 || item.state != item_custody_state::active ||
		    item.owner.type != item_owner_type::player ||
		    item.owner.id != static_cast<uint64_t>(current.pid) || item.owner.context_id)
			return refuse(error, "later ownership revision or incompatible custody");
	player_snapshot replacement = current.snapshot;
	bool missing_grant = false;
	for (const auto &frame : ordered)
	{
		if (frame.revision <= revision || !overlay(frame, &replacement))
			return refuse(
				error,
				"ambiguous/stale component revision or unsupported obligation");
		revision = frame.revision;
		if (frame.components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		{
			std::set<uint64_t> present;
			for (const auto &item : frame.items)
			{
				const auto found = native_items.find(item.object_uid);
				if (found == native_items.end() ||
				    !present.insert(item.object_uid).second)
					return refuse(
						error,
						"archive graph contains conflicting or duplicate custody");
				const auto &native = replacement.items[found->second];
				const uint64_t archived_parent =
					item.parent_index < 0 ?
						0 :
					static_cast<size_t>(item.parent_index) <
							present.size() - 1 ?
						frame.items[item.parent_index].object_uid :
						UINT64_MAX;
				const uint64_t native_parent =
					native.parent_index < 0 ?
						0 :
						replacement.items[native.parent_index].object_uid;
				if (item.vnum != native.vnum ||
				    item.equipment_slot != native.equipment_slot ||
				    archived_parent != native_parent)
					return refuse(
						error,
						"archive topology or equipment conflicts with native custody");
				auto update = item;
				update.parent_index = native.parent_index;
				replacement.items[found->second] = std::move(update);
			}
			for (const auto &item : native_items)
				if (!present.count(item.first))
				{
					if (!created.count(item.first))
						return refuse(
							error,
							"omitted custody is not a proved grant");
					missing_grant = true;
				}
		}
	}
	if (!missing_grant || revision == std::numeric_limits<player_revision_t>::max())
		return refuse(error, "no proved omitted-grant projection or revision exhausted");
	replacement.revision = revision + 1;
	replacement.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	record->baseline = canonical(current.snapshot);
	record->account_name = current.account_name;
	record->replacement = canonical(std::move(replacement));
	record->authority_evidence = authority(current);
	std::vector<uint8_t> encoded;
	return player_snapshot_encode(record->replacement, &encoded) ==
		       player_snapshot_codec_result::ok ||
	       refuse(error, "replacement snapshot exceeds native codec bounds");
}

namespace
{
// Reserved inbox receipt discriminator. This is not a command admitted by the
// coordinator or replayed from its journal; only the stopped recovery owner
// writes it in the same transaction as the exact player replacement.
constexpr unsigned recovery_receipt_type = 65000;
int sql_query(MYSQL *connection, const char *sql)
{
	return mysql_real_query(connection, sql, std::strlen(sql));
}
struct sql_recovery_proof_context
{
	MYSQL *connection;
	unsigned long original_session;
	player_sql_cleanup *cleanup;
};
std::string hex_bytes(const uint8_t *data, size_t size)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string encoded;
	encoded.reserve(size * 2);
	for (size_t index = 0; index < size; ++index)
	{
		encoded.push_back(digits[data[index] >> 4]);
		encoded.push_back(digits[data[index] & 15]);
	}
	return encoded;
}
std::string identity_for(const critical_command &command, uint32_t backend)
{
	return std::string(backend == 1 ? "flatfile:" : "sql:") +
	       hex_bytes(command.operation_id.bytes.data(), command.operation_id.bytes.size());
}
bool sql_creations(MYSQL *connection, const std::vector<critical_command> &commands,
		   std::vector<player_recovery_creation> *verified)
{
	for (const auto &command : commands)
	{
		const auto result = critical_command_repository_verify_creation_in_transaction(
			connection, command);
		item_transfer_result receipt{};
		if (result.outcome != critical_apply_outcome::already_applied ||
		    result.error_code ||
		    !item_transfer_command_decode_result(result.result_payload.data(),
							 result.result_size, &receipt))
			return false;
		verified->push_back({ command, receipt });
	}
	return !verified->empty();
}
bool sql_proof(const player_save_recovery_record &record, void *context)
{
	auto *request = static_cast<sql_recovery_proof_context *>(context);
	if (!request || !request->cleanup)
		return false;
	auto *connection = request->connection;
	auto &proof = *request->cleanup;
	player_sql_transaction_cleanup owner(connection, proof);
	if (player_sql_idle_error(connection))
		return false;
	if (proof.original_session != request->original_session)
	{
		proof.disposition = player_sql_cleanup_disposition::retire_required;
		proof.cleanup_error = ENOTCONN;
		return false;
	}
	try
	{
		owner.starting();
		if (sql_query(connection, "START TRANSACTION") || !owner.same_session() ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
		{
			owner.finish();
			return false;
		}
		bool present = false;
		const bool receipt =
			player_quarantine_recovery_sql_receipt(connection, record, false,
							       &present) &&
			present &&
			player_quarantine_recovery_creation_proofs_sql(connection, record);
		const auto loaded = receipt && present ?
					    player_load_repository_quarantine_inspect(
						    connection,
						    player_quarantine_recovery_request(record),
						    &record) :
					    player_load_result{};
		player_save_recovery_record retained;
		bool resolved = false;
		const bool known = player_save_journal_recovery_read(record.replacement.pid,
								     &retained, &resolved) ==
				   player_save_journal_result::ok;
		const bool continuation =
			known && resolved && loaded.outcome == player_load_outcome::applied &&
			!loaded.degraded_components && !loaded.missing_payload_rows &&
			loaded.account_name == record.account_name &&
			loaded.snapshot.revision >= record.replacement.revision;
		const bool matches =
			receipt && present &&
			(continuation ||
			 player_quarantine_recovery_state_matches(loaded, record, true));
		owner.finish();
		return matches && proof.rollback_confirmed &&
		       proof.disposition == player_sql_cleanup_disposition::idle_verified &&
		       !proof.cleanup_error;
	}
	catch (...)
	{
		owner.finish();
		return false;
	}
}
}

player_load_request player_quarantine_recovery_request(const player_save_recovery_record &record)
{
	player_load_request request;
	request.request_id = 1;
	request.pid = record.replacement.pid;
	request.account_name = record.account_name;
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	return request;
}

bool player_quarantine_recovery_receipt_bytes(const player_save_recovery_record &record,
					      std::vector<uint8_t> *bytes)
{
	std::array<uint8_t, 32> digest;
	if (!bytes || !player_save_journal_recovery_fingerprint(record, &digest))
		return false;
	std::vector<uint8_t> next = { 'D', 'P', 'R', 'P', 'R', 'O', 'O', 'F' };
	next.insert(next.end(), record.identity.begin(), record.identity.end());
	next.insert(next.end(), digest.begin(), digest.end());
	for (uint64_t value :
	     { static_cast<uint64_t>(record.replacement.pid), record.replacement.revision })
		for (unsigned byte = 0; byte < 8; ++byte)
			next.push_back(static_cast<uint8_t>(value >> (byte * 8)));
	bytes->swap(next);
	return true;
}

std::string player_quarantine_recovery_receipt_filename(const player_save_recovery_record &record)
{
	return "player-recovery-" + std::to_string(record.replacement.pid) + "-" +
	       hex_bytes(record.identity.data(), record.identity.size()) + ".receipt";
}

bool player_quarantine_recovery_sql_receipt(MYSQL *connection,
					    const player_save_recovery_record &record, bool write,
					    bool *present)
{
	std::array<uint8_t, 32> digest;
	std::vector<uint8_t> receipt;
	if (!connection || !present || record.backend != 2 ||
	    !player_save_journal_recovery_matches(record) ||
	    !player_save_journal_recovery_fingerprint(record, &digest) ||
	    !player_quarantine_recovery_receipt_bytes(record, &receipt))
		return false;
#ifndef __NO_MYSQL__
	if (!(connection->server_status & SERVER_STATUS_IN_TRANS))
		return false;
#endif
	const std::string id = hex_bytes(record.identity.data(), record.identity.size());
	const std::string predicate =
		"command_type=" + std::to_string(recovery_receipt_type) +
		" AND schema_version=1 AND payload_version=1 AND status=1 AND result_code=0 AND failure_stage=0" +
		" AND durable_revision=" + std::to_string(record.replacement.revision) +
		" AND command_hash=UNHEX('" + hex_bytes(digest.data(), digest.size()) +
		"') AND keys_hash=UNHEX('" +
		hex_bytes(record.archive_digest.data(), record.archive_digest.size()) +
		"') AND result_payload=UNHEX('" + hex_bytes(receipt.data(), receipt.size()) + "')";
	const std::string query = "SELECT (" + predicate +
				  ") FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
				  id + "') FOR UPDATE";
	if (sql_query(connection, query.c_str()))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) > 1)
		return false;
	*present = mysql_num_rows(rows.get()) == 1;
	if (*present)
	{
		const auto row = mysql_fetch_row(rows.get());
		return row && row[0] && std::strcmp(row[0], "1") == 0;
	}
	rows.reset();
	if (!write)
		return true;
	const std::string insert =
		"INSERT INTO critical_operation_inbox (operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_code,failure_stage,durable_revision,result_payload,committed_at) VALUES (UNHEX('" +
		id + "'),UNHEX('" + hex_bytes(digest.data(), digest.size()) + "'),UNHEX('" +
		hex_bytes(record.archive_digest.data(), record.archive_digest.size()) + "')," +
		std::to_string(recovery_receipt_type) + ",1,1,1,0,0," +
		std::to_string(record.replacement.revision) + ",UNHEX('" +
		hex_bytes(receipt.data(), receipt.size()) + "'),CURRENT_TIMESTAMP(6))";
	if (sql_query(connection, insert.c_str()))
		return false;
	*present = true;
	return true;
}

bool player_quarantine_recovery_creation_proofs_sql(MYSQL *connection,
						    const player_save_recovery_record &record)
{
	std::vector<critical_command> commands;
	for (const auto &bytes : record.creation_commands)
	{
		critical_command command;
		if (critical_command_decode(bytes.data(), bytes.size(), &command) !=
		    critical_command_codec_result::ok)
			return false;
		commands.push_back(std::move(command));
	}
	std::vector<player_recovery_creation> verified;
	return !commands.empty() && record.backend_identity == identity_for(commands.front(), 2) &&
	       sql_creations(connection, commands, &verified);
}

static bool prepare_sql_owned(MYSQL *connection, int pid,
			      const std::vector<critical_command> &commands,
			      const std::string &backend_identity,
			      player_save_recovery_record *record, std::string *error,
			      player_sql_transaction_cleanup &owner, player_sql_cleanup &proof)
{
	std::vector<player_snapshot> frames;
	std::array<uint8_t, 32> archive;
	if (!connection || !record || commands.empty() || commands.size() > 64 ||
	    player_save_journal_recovery_inspect(pid, &frames, &archive) !=
		    player_save_journal_result::ok)
		return refuse(error, "missing or unsupported archive/command evidence");
	player_save_recovery_record next;
	next.backend = 2;
	next.backend_identity = identity_for(commands.front(), 2);
	if (!backend_identity.empty() && backend_identity != next.backend_identity)
		return refuse(error, "backend identity mismatch");
	const std::string query =
		"SELECT COALESCE(account_name,(SELECT ac.account_name FROM account_characters ac WHERE ac.pid=player_data.pid AND ac.deleted_at IS NULL LIMIT 1)) FROM player_data WHERE pid=" +
		std::to_string(pid);
	if (sql_query(connection, query.c_str()))
		return refuse(error, "account identity read failed");
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	const auto row = rows && mysql_num_rows(rows.get()) == 1 ? mysql_fetch_row(rows.get()) :
								   nullptr;
	if (!row || !row[0] || std::strlen(row[0]) > 50)
		return refuse(error, "missing account identity");
	next.account_name = row[0];
	next.replacement.pid = pid;
	rows.reset();
	if (!owner.same_session() || player_sql_idle_error(connection))
	{
		proof.disposition = player_sql_cleanup_disposition::retire_required;
		proof.cleanup_error = ENOTCONN;
		return refuse(error, "backend session changed before verification");
	}
	owner.starting();
	if (sql_query(connection, "START TRANSACTION") || !owner.same_session() ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return refuse(error, "verification transaction failed");
	std::vector<player_recovery_creation> verified;
	const bool proved = sql_creations(connection, commands, &verified);
	owner.finish();
	if (!proof.rollback_confirmed ||
	    proof.disposition != player_sql_cleanup_disposition::idle_verified ||
	    proof.cleanup_error)
		return refuse(error,
			      "verification cleanup uncertain; retain fence and retire session");
	if (!proved)
		return refuse(error, "original grant receipt/source/history mismatch");
	// Inspection starts its own read transaction. The earlier creation-proof
	// rollback is not cleanup evidence for this second phase or its exceptions.
	owner.starting();
	const auto current = player_load_repository_quarantine_inspect(
		connection, player_quarantine_recovery_request(next));
	owner.finish();
	if (!proof.rollback_confirmed ||
	    proof.disposition != player_sql_cleanup_disposition::idle_verified ||
	    proof.cleanup_error)
	{
		return refuse(error,
			      "inspection cleanup uncertain; retain fence and retire session");
	}
	if (!player_quarantine_recovery_build(current, frames, verified, &next, error))
		return false;
	critical_operation_id identity;
	if (!critical_operation_id_generate(&identity))
		return refuse(error, "recovery identity generation failed");
	next.identity = identity.bytes;
	next.archive_digest = archive;
	if (player_save_journal_recovery_prepare(next) != player_save_journal_result::ok)
		return refuse(error, "prepared record not durable");
	*record = std::move(next);
	return true;
}

bool player_quarantine_recovery_prepare_sql(MYSQL *connection, int pid,
					    const std::vector<critical_command> &commands,
					    const std::string &backend_identity,
					    player_save_recovery_record *record, std::string *error,
					    player_sql_cleanup *cleanup)
{
	player_sql_cleanup local;
	auto &proof = cleanup ? *cleanup : local;
	player_sql_transaction_cleanup owner(connection, proof);
	if (player_sql_idle_error(connection))
		return refuse(error, "connection is not an isolated transaction owner");
	try
	{
		const bool prepared = prepare_sql_owned(connection, pid, commands, backend_identity,
							record, error, owner, proof);
		owner.finish();
		return prepared;
	}
	catch (...)
	{
		owner.finish();
		return refuse(error, "recovery preparation failed; retain archive and fence");
	}
}

bool player_quarantine_recovery_prepare_sql(MYSQL *connection, int pid,
					    const std::vector<critical_command> &commands,
					    const std::string &backend_identity,
					    player_save_recovery_record *record, std::string *error)
{
	return player_quarantine_recovery_prepare_sql(connection, pid, commands, backend_identity,
						      record, error, nullptr);
}

static bool resume_sql_owned(MYSQL *connection, int pid, const std::string &backend_identity,
			     std::string *error, sql_recovery_proof_context &context)
{
	player_save_recovery_record record;
	bool resolved = false;
	if (!connection ||
	    player_save_journal_recovery_read(pid, &record, &resolved) !=
		    player_save_journal_result::ok ||
	    record.backend != 2 ||
	    (!backend_identity.empty() && backend_identity != record.backend_identity))
		return refuse(error, "no matching durable SQL preparation");
	if (resolved)
		return player_save_journal_recovery_resolve(record, sql_proof, &context) ==
			       player_save_journal_result::ok ||
		       refuse(error, "resolved recovery does not match this backend generation");
	const auto result =
		player_snapshot_repository_recovery_apply(connection, record, context.cleanup);
	if (result.outcome != player_save_apply_outcome::applied &&
	    result.outcome != player_save_apply_outcome::already_applied)
		return refuse(
			error,
			"native replacement refused or commit uncertain; retain fence and resume");
	if (context.cleanup->original_session != context.original_session ||
	    context.cleanup->disposition != player_sql_cleanup_disposition::idle_verified ||
	    context.cleanup->cleanup_error)
		return refuse(error, "native replacement cleanup uncertain; retain fence");
	return player_save_journal_recovery_resolve(record, sql_proof, &context) ==
		       player_save_journal_result::ok ||
	       refuse(error, "exact backend result or durable resolution missing");
}

bool player_quarantine_recovery_resume_sql(MYSQL *connection, int pid,
					   const std::string &backend_identity, std::string *error,
					   player_sql_cleanup *cleanup)
{
	player_sql_cleanup local;
	auto &proof = cleanup ? *cleanup : local;
	proof = {};
	// Keep the call's original session independent of phase cleanup evidence.
	player_sql_cleanup boundary_proof;
	player_sql_transaction_cleanup boundary(connection, boundary_proof);
	proof.original_session = boundary_proof.original_session;
	if (player_sql_idle_error(connection))
		return refuse(error, "connection is not an isolated transaction owner");
	sql_recovery_proof_context context{ connection, boundary_proof.original_session, &proof };
	try
	{
		const bool resumed =
			resume_sql_owned(connection, pid, backend_identity, error, context);
		if (!boundary.same_session() || player_sql_idle_error(connection))
		{
			proof.disposition = player_sql_cleanup_disposition::retire_required;
			proof.cleanup_error = ENOTCONN;
			// sql_proof establishes cleanup before durable journal resolution.
			// Later lease retirement cannot undo that result or recreate a fence.
			return resumed;
		}
		return resumed;
	}
	catch (...)
	{
		// Phase owners clean their own transactions during unwinding. In the
		// absence of explicit proof the pooled caller must retire its lease.
		proof.disposition = player_sql_cleanup_disposition::retire_required;
		return refuse(error, "recovery verification failed; retain fence");
	}
}

bool player_quarantine_recovery_resume_sql(MYSQL *connection, int pid,
					   const std::string &backend_identity, std::string *error)
{
	return player_quarantine_recovery_resume_sql(connection, pid, backend_identity, error,
						     nullptr);
}

bool player_quarantine_recovery_prepare_flatfile(const std::string &root, int pid,
						 const std::vector<critical_command> &commands,
						 const std::string &backend_identity,
						 player_save_recovery_record *record,
						 std::string *error)
{
	std::vector<player_snapshot> frames;
	std::array<uint8_t, 32> archive;
	if (!record || commands.empty() || commands.size() > 64 ||
	    player_save_journal_recovery_inspect(pid, &frames, &archive) !=
		    player_save_journal_result::ok)
		return refuse(error, "missing or unsupported archive/command evidence");
	flatfile_identity_record identity;
	if (flatfile_identity_lookup_pid(root, pid, &identity, error) !=
		    flatfile_identity_result::ok ||
	    !identity.active || identity.blocked)
		return refuse(error, "missing or blocked account identity");
	player_save_recovery_record next;
	next.backend = 1;
	next.backend_identity = identity_for(commands.front(), 1);
	if (!backend_identity.empty() && backend_identity != next.backend_identity)
		return refuse(error, "backend identity mismatch");
	next.account_name = identity.account;
	next.replacement.pid = pid;
	std::vector<player_recovery_creation> verified;
	{
		flatfile_authority_lock authority;
		if (!authority.acquire(root, error) ||
		    flatfile_authority_transaction_recover(root, authority, error) !=
			    flatfile_authority_transaction_result::ok)
			return refuse(error, "authority recovery failed");
		for (const auto &command : commands)
		{
			const auto proof = flatfile_item_repository_verify_creation_locked(
				root, authority, command, error);
			item_transfer_result receipt;
			if (proof.outcome != critical_apply_outcome::already_applied ||
			    proof.error_code ||
			    !item_transfer_command_decode_result(proof.result_payload.data(),
								 proof.result_size, &receipt))
				return refuse(error,
					      "original grant receipt/source/history mismatch");
			verified.push_back({ command, receipt });
		}
	}
	const auto current =
		flatfile_player_quarantine_inspect(root, player_quarantine_recovery_request(next));
	if (!player_quarantine_recovery_build(current, frames, verified, &next, error))
		return false;
	critical_operation_id operation;
	if (!critical_operation_id_generate(&operation))
		return refuse(error, "recovery identity generation failed");
	next.identity = operation.bytes;
	next.archive_digest = archive;
	if (player_save_journal_recovery_prepare(next) != player_save_journal_result::ok)
		return refuse(error, "prepared record not durable");
	*record = std::move(next);
	return true;
}

namespace
{
bool flatfile_proof(const player_save_recovery_record &record, void *context)
{
	const auto &root = *static_cast<const std::string *>(context);
	player_save_recovery_record retained;
	bool resolved = false;
	std::string error;
	return player_save_journal_recovery_read(record.replacement.pid, &retained, &resolved) ==
		       player_save_journal_result::ok &&
	       flatfile_player_quarantine_verify(root, record, resolved, &error);
}
}

bool player_quarantine_recovery_resume_flatfile(const std::string &root, int pid,
						const std::string &backend_identity,
						std::string *error)
{
	player_save_recovery_record record;
	bool resolved = false;
	if (player_save_journal_recovery_read(pid, &record, &resolved) !=
		    player_save_journal_result::ok ||
	    record.backend != 1 ||
	    (!backend_identity.empty() && backend_identity != record.backend_identity))
		return refuse(error, "no matching durable flatfile preparation");
	if (!resolved)
	{
		const auto applied = flatfile_player_quarantine_apply(root, record, error);
		if (applied.outcome != player_save_apply_outcome::applied &&
		    applied.outcome != player_save_apply_outcome::already_applied)
			return refuse(
				error,
				"native replacement refused or commit uncertain; retain fence and resume");
	}
	return player_save_journal_recovery_resolve(record, flatfile_proof,
						    const_cast<std::string *>(&root)) ==
		       player_save_journal_result::ok ||
	       refuse(error, "exact backend result or durable resolution missing");
}

void player_quarantine_recovery_revalidate_selected()
{
	std::vector<player_save_recovery_record> records;
	if (!player_save_journal_resolved_recoveries(&records))
		return;
	const bool flatfile = persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY;
	const uint64_t deadline = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	size_t verified = 0;
	for (const auto &record : records)
	{
		// Boot verification is bounded. Unverified PIDs remain fenced and never
		// impede unrelated player admission; stopped recovery can retry them.
		if (persistence_observability_now_usec() >= deadline)
			break;
		std::string error;
		try
		{
			if (flatfile && record.backend == 1)
				verified += player_quarantine_recovery_resume_flatfile(
					persistence_mode_flatfile_root(), record.replacement.pid,
					record.backend_identity, &error);
			else if (!flatfile && record.backend == 2)
			{
				player_sql_pool_lease lease(sql_pool_acquire());
				if (!lease.get())
					break;
				player_sql_cleanup cleanup;
				try
				{
					verified += player_quarantine_recovery_resume_sql(
						lease.get(), record.replacement.pid,
						record.backend_identity, &error, &cleanup);
					lease.reuse(cleanup);
				}
				catch (...)
				{
				}
			}
		}
		catch (...)
		{
		}
	}
	if (verified != records.size())
		logit("logs/log/status", "player recovery revalidation incomplete");
}
