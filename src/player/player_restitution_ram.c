#include "player/player_restitution_ram.h"
#include "player/player_checkpoint_ram.h"
#include <algorithm>
#include <cerrno>
#include <set>

namespace
{
std::set<uint64_t> delivered, staged_delivered;
item_ownership_runtime_patch custody;
critical_operation_id staged_id{};
critical_apply_result staged_result{};
restitution_artifact_prepare_fn artifact_prepare = nullptr;
restitution_artifact_publish_fn artifact_publish = nullptr;
critical_apply_result reject(unsigned error)
{
	return { critical_apply_outcome::terminal_failure, 0, error };
}
}
void player_restitution_ram_artifact_callbacks(restitution_artifact_prepare_fn prepare,
					       restitution_artifact_publish_fn publish)
{
	artifact_prepare = prepare;
	artifact_publish = publish;
}
void player_restitution_ram_reset()
{
	delivered.clear();
	staged_delivered.clear();
	custody = {};
	staged_id = {};
	staged_result = {};
}
bool player_restitution_ram_seed_delivery(uint64_t uid)
{
	return uid && delivered.insert(uid).second;
}
critical_apply_result player_restitution_ram_prepare(const critical_command &command)
{
	staged_delivered.clear();
	custody = {};
	staged_id = {};
	staged_result = {};
	player_death_restitution_plan plan;
	if (!player_death_restitution_command_decode_payload(command, &plan))
		return reject(EINVAL);
	uint64_t save_revision = 0;
	if (!player_checkpoint_ram_revision(plan.recipient_pid, &save_revision) ||
	    save_revision != plan.expected_recipient_save_revision)
		return reject(ESTALE);
	const auto proof = player_checkpoint_ram_death(plan.source_pid, plan.death_revision);
	if (!proof || proof->digest != plan.payload_digest || proof->epoch != plan.loss_epoch ||
	    !critical_operation_id_equal(proof->operation_id, plan.death_operation_id))
		return reject(ESTALE);
	const item_owner_identity source{ item_owner_type::player, plan.source_pid, 0 };
	const item_owner_identity recipient{ item_owner_type::player, plan.recipient_pid, 0 };
	uint64_t source_revision = 0, recipient_revision = 0;
	if (!item_ownership_runtime_owner_revision(source, &source_revision) ||
	    !item_ownership_runtime_owner_revision(recipient, &recipient_revision) ||
	    source_revision != plan.expected_source_owner_revision ||
	    recipient_revision != plan.expected_recipient_owner_revision ||
	    source_revision == UINT64_MAX || recipient_revision == UINT64_MAX)
		return reject(ESTALE);
	player_death_restitution_result result{};
	result.restitution_id = plan.restitution_id;
	result.source_pid = plan.source_pid;
	result.recipient_pid = plan.recipient_pid;
	result.delivery_epoch = command.accepted_at_usec / 1000000;
	result.candidate_count = plan.items.size();
	result.durable_revision = std::max(source_revision, recipient_revision) + 1;
	for (const auto &item : plan.items)
	{
		if (item.disposition != player_death_restitution_disposition::deliver)
		{
			++result.unresolved_count;
			continue;
		}
		const auto captured = proof->custody.find(item.item_uid);
		if (captured == proof->custody.end())
			return reject(ESTALE);
		const auto &evidence = captured->second;
		if (evidence.item.root_item_uid != item.source_root_item_uid ||
		    evidence.item.parent_item_uid != item.source_parent_item_uid ||
		    evidence.item.expected_item_revision != item.custody_item_revision ||
		    evidence.item.vnum != static_cast<int32_t>(item.vnum) ||
		    static_cast<uint8_t>(evidence.item.expected_state) != item.custody_state ||
		    static_cast<uint8_t>(evidence.owner.type) != item.custody_owner_type ||
		    evidence.owner.id != item.custody_owner_id ||
		    evidence.owner.context_id != item.custody_owner_context_id ||
		    evidence.owner_revision != item.custody_owner_revision)
			return reject(ESTALE);
		item_ownership_runtime_entry current;
		if (!item_ownership_runtime_lookup(item.item_uid, &current) ||
		    !item_owner_identity_equal(current.owner, source) ||
		    current.root_item_uid != item.source_root_item_uid ||
		    current.parent_item_uid != item.source_parent_item_uid ||
		    current.item_revision != item.expected_item_revision ||
		    static_cast<uint8_t>(current.state) != item.expected_owner_state ||
		    current.vnum != static_cast<int32_t>(item.vnum) ||
		    current.item_revision == UINT64_MAX)
			return reject(ESTALE);
		if (delivered.contains(item.item_uid))
			return reject(EEXIST);
		if (item.artifact_vnum &&
		    (!artifact_prepare || !artifact_publish ||
		     !item_ownership_runtime_unique_vnum(item.artifact_vnum, item.item_uid) ||
		     !player_checkpoint_ram_artifact_unique(item.artifact_vnum, item.item_uid)))
			return reject(ESTALE);
		custody.before.push_back(current);
		current.owner = recipient;
		current.owner_revision = recipient_revision + 1;
		++current.item_revision;
		current.root_item_uid = item.delivered_root_item_uid;
		current.parent_item_uid = item.delivered_parent_item_uid;
		current.state = item_custody_state::active;
		custody.after.push_back(current);
		staged_delivered.insert(item.item_uid);
		++result.delivered_count;
		result.durable_revision = std::max(result.durable_revision, current.item_revision);
	}
	if (!result.delivered_count)
		return reject(ENOTSUP);
	if (artifact_prepare && !artifact_prepare(plan, result.delivery_epoch))
		return reject(ESTALE);
	custody.owners.emplace_back(source, source_revision + 1);
	if (plan.source_pid != plan.recipient_pid)
		custody.owners.emplace_back(recipient, recipient_revision + 1);
	result.mutation_applied = true;
	std::array<uint8_t, PLAYER_DEATH_RESTITUTION_RESULT_BYTES> bytes;
	if (!player_death_restitution_command_encode_result(result, &bytes) ||
	    !item_ownership_runtime_patch_ready(custody))
		return reject(EINVAL);
	staged_id = command.operation_id;
	staged_result = { critical_apply_outcome::applied, result.durable_revision, 0 };
	staged_result.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), staged_result.result_payload.begin());
	return staged_result;
}
bool player_restitution_ram_can_publish(const critical_command &command)
{
	return command.schema_version == CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION &&
	       critical_operation_id_equal(staged_id, command.operation_id) &&
	       critical_command_outcome_matches(command, staged_result.durable_revision, 0,
						staged_result.result_payload.data(),
						staged_result.result_size) &&
	       item_ownership_runtime_patch_ready(custody);
}
bool player_restitution_ram_publish(const critical_command &command)
{
	if (!player_restitution_ram_can_publish(command) ||
	    !item_ownership_runtime_patch_publish(custody))
		return false;
	delivered.merge(staged_delivered);
	if (artifact_publish)
		artifact_publish();
	staged_id = {};
	return true;
}
