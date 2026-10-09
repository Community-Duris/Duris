#ifndef SMITH_NATIVE_ACCOUNTING_H
#define SMITH_NATIVE_ACCOUNTING_H

#include "economy/item_transfer_accounting.h"
#include "item/smith_native_command.h"

#include <algorithm>
#include <new>
#include <utility>

// Pure original Smith17 intent/effect projection. The atomic root supplies the
// authenticated installed PC mapping and locked custody (including real absent
// output witnesses). These values do not prove native lifetime, complete native
// or PC images, save ACK, source entitlement or either owner clock. No admission,
// factory, wallet/custody write, publication or execution support follows here.
namespace smith_native_accounting_detail
{
using error = economic_accounting_error;

inline bool source_equal(const economic_source_event &a, const economic_source_event &b) noexcept
{
	return a.kind == b.kind && a.source.bytes == b.source.bytes &&
	       a.generation.bytes == b.generation.bytes && a.sequence == b.sequence &&
	       a.slot == b.slot;
}

inline error decode(const critical_command &command, smith_native_compound_terms *terms) noexcept
{
	return smith_native_command_validate(command, terms);
}

inline bool wallet_matches(const smith_native_compound_terms &terms,
			   const critical_operation_id &lineage,
			   const economic_account_key &wallet) noexcept
{
	return economic_account_key_valid(wallet) && wallet.kind == economic_account_kind::wallet &&
	       wallet.context_id == 0 && wallet.lineage.bytes == lineage.bytes &&
	       wallet.authority_id == terms.player_wallet.wallet_mapping_id &&
	       terms.native_before.lineage.bytes == lineage.bytes;
}

inline bool metadata_matches(const economic_operation_metadata &metadata,
			     const smith_native_compound_terms &terms,
			     const economic_account_key &wallet) noexcept
{
	return economic_operation_metadata_validate(metadata) == error::ok &&
	       metadata.version == ECONOMIC_ACCOUNTING_VERSION &&
	       metadata.operation_id.bytes == terms.operation_id.bytes &&
	       critical_operation_id_is_zero(metadata.original_operation_id) &&
	       metadata.actor_kind == economic_actor_kind::domain &&
	       metadata.actor_id == terms.player_pid &&
	       metadata.writer_id == ECONOMIC_WRITER_ITEM_TRANSFER &&
	       metadata.policy_version == 1 && metadata.compiler_version == 1 &&
	       metadata.reason == economic_reason::crafting_cost && metadata.source_event &&
	       source_equal(*metadata.source_event, terms.source) &&
	       wallet_matches(terms, metadata.lineage, wallet);
}

inline bool plan_metadata_equal(const economic_plan_metadata &a,
				const economic_plan_metadata &b) noexcept
{
	return a.version == b.version && a.lineage.bytes == b.lineage.bytes &&
	       a.epoch.bytes == b.epoch.bytes && a.operation_id.bytes == b.operation_id.bytes &&
	       a.original_operation_id.bytes == b.original_operation_id.bytes &&
	       a.actor_kind == b.actor_kind && a.actor_id == b.actor_id &&
	       a.writer_id == b.writer_id && a.policy_version == b.policy_version &&
	       a.compiler_version == b.compiler_version && a.reason == b.reason && a.source_event &&
	       b.source_event && source_equal(*a.source_event, *b.source_event) &&
	       a.intent_digest == b.intent_digest && a.domain_digest == b.domain_digest;
}

inline size_t index_of(std::span<const economic_item_snapshot> items, uint64_t uid) noexcept
{
	const auto found = std::lower_bound(items.begin(), items.end(), uid,
					    [](const auto &item, uint64_t value)
					    { return item.uid < value; });
	return found == items.end() || found->uid != uid ? items.size() :
							   size_t(found - items.begin());
}
} // namespace smith_native_accounting_detail

// Original schema1 preparation only: structurally freeze the supplied original
// crafting source and exact Smith command, independently of closed support17.
// The caller authenticates lineage/epoch/PID mapping; this helper issues none.
inline economic_accounting_error smith_native_accounting_intent(
	const critical_command &command, const critical_operation_id &lineage,
	const critical_operation_id &epoch, uint32_t actor_pid,
	const economic_account_key &original_player_wallet, std::vector<uint8_t> *encoded) noexcept
{
	using namespace smith_native_accounting_detail;
	if (!encoded)
		return error::corrupt_evidence;
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !command.accounting_intent.empty())
		return error::invalid_version;
	if (critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return error::invalid_identity;
	try
	{
		smith_native_compound_terms terms;
		auto status = decode(command, &terms);
		if (status != error::ok)
			return status;
		if (actor_pid != terms.player_pid ||
		    !wallet_matches(terms, lineage, original_player_wallet))
			return error::unauthorized;
		economic_admission_facts facts;
		facts.metadata.lineage = lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.operation_id = command.operation_id;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_ITEM_TRANSFER;
		facts.metadata.reason = economic_reason::crafting_cost;
		facts.metadata.source_event = terms.source;
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// Compile expected effects only from the original frozen schema2 intent and
// actual locked UID positions. Keep unrelated supplied witnesses unchanged.
// Replace only effect vectors in a local copy; metadata/children and caller
// output remain unchanged on refusal. The original atomic participant must
// advance native stock/item-owner and PC item-owner once, verify NPC cash stayed
// unchanged, and record these effects together with the same source/result.
inline economic_accounting_error
smith_native_accounting_effects(const critical_command &command,
				const economic_account_key &original_player_wallet,
				std::span<const economic_item_snapshot> locked_custody,
				economic_accounting_plan *plan) noexcept
{
	using namespace smith_native_accounting_detail;
	if (!plan)
		return error::corrupt_evidence;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.accounting_intent.empty())
		return error::invalid_version;
	if (locked_custody.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
		return error::capacity;
	try
	{
		smith_native_compound_terms terms;
		auto status = decode(command, &terms);
		if (status != error::ok)
			return status;
		economic_frozen_intent intent;
		status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		if (intent.admission.facts_version != 1 || !intent.admission.facts.empty() ||
		    !metadata_matches(intent.admission.metadata, terms, original_player_wallet))
			return error::unauthorized;
		economic_plan_metadata expected_metadata;
		status = economic_intent_plan_metadata(command, intent, &expected_metadata);
		if (status != error::ok)
			return status;
		if (!plan_metadata_equal(plan->metadata, expected_metadata))
			return error::payload_conflict;

		auto candidate = *plan;
		candidate.accounts.clear();
		candidate.postings.clear();
		candidate.item_events.clear();
		candidate.items_before.assign(locked_custody.begin(), locked_custody.end());
		std::sort(candidate.items_before.begin(), candidate.items_before.end(),
			  [](const auto &a, const auto &b) { return a.uid < b.uid; });
		for (size_t i = 0; i < candidate.items_before.size(); ++i)
			if (!candidate.items_before[i].uid ||
			    (i &&
			     candidate.items_before[i - 1].uid == candidate.items_before[i].uid))
				return error::invalid_identity;
		candidate.items_after = candidate.items_before;

		const auto &cost = terms.player_wallet;
		economic_coin_vector before{}, after{}, debit{}, credit{};
		for (size_t i = 0; i < before.size(); ++i)
		{
			before[i] = cost.before[i];
			after[i] = cost.after[i];
		}
		status = economic_coin_delta(before, after, &debit);
		if (status != error::ok)
			return status;
		int64_t copper = 0;
		status = economic_coin_value(debit, &copper);
		if (status != error::ok)
			return status;
		if (copper != -int64_t(terms.fee))
			return error::corrupt_evidence;
		for (size_t i = 0; i < debit.size(); ++i)
		{
			if (debit[i] == INT64_MIN)
				return error::overflow;
			credit[i] = -debit[i];
		}
		const economic_account_key sink{ expected_metadata.lineage,
						 economic_account_kind::sink,
						 uint64_t(economic_reason::crafting_cost), 0 };
		candidate.accounts = { { original_player_wallet, before, after,
					 cost.before_revision, cost.after_revision },
				       { sink, {}, {}, 0, 0 } };
		candidate.postings = { { 0, 0, 0, debit, copper }, { 1, 1, 0, credit, -copper } };

		const item_owner_identity native_owner{
			item_owner_type::native_mobile,
			terms.native_before.reference.mobile_instance_id, 0
		};
		for (const auto &entry : terms.selected_custody)
		{
			const size_t index = index_of(candidate.items_before, entry.item_uid);
			if (index == candidate.items_before.size())
				return error::incomplete_coverage;
			const auto &position = candidate.items_before[index].position;
			if (!item_owner_identity_equal(position.owner, native_owner) ||
			    position.root_uid != entry.root_item_uid ||
			    position.parent_uid != entry.parent_item_uid ||
			    position.revision != entry.expected_item_revision ||
			    position.state != entry.expected_state || position.equipment_slot)
				return error::corrupt_evidence;
		}
		// Every observed live descendant in a selected tree must be in the
		// original selected forest. The backend still owns full-world/SQL absence
		// proof: a supplied span cannot prove that an unobserved row is absent.
		for (const auto &item : candidate.items_before)
		{
			const auto &position = item.position;
			if (position.state != item_custody_state::active &&
			    position.state != item_custody_state::quarantined)
				continue;
			const auto entry = std::lower_bound(terms.selected_custody.begin(),
							    terms.selected_custody.end(), item.uid,
							    [](const auto &row, uint64_t uid)
							    { return row.item_uid < uid; });
			if (std::find(terms.selected_root_order.begin(),
				      terms.selected_root_order.end(),
				      position.root_uid) != terms.selected_root_order.end() &&
			    (entry == terms.selected_custody.end() || entry->item_uid != item.uid))
				return error::incomplete_coverage;
		}

		const economic_item_position absent{ { item_owner_type::unknown, 0, 0 }, 0, 0, 0,
						     item_custody_state::absent,	 0 };
		std::vector<uint64_t> output_roots;
		output_roots.reserve(terms.frozen_outputs.size());
		for (const auto &output : terms.frozen_outputs)
		{
			const size_t index = index_of(candidate.items_before, output.object_uid);
			if (index == candidate.items_before.size())
				return error::incomplete_coverage;
			const auto &prior = candidate.items_before[index].position;
			if (!economic_item_position_equal(prior, absent))
				return error::corrupt_evidence;
			const bool root = output.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
			const uint64_t root_uid = root ? output.object_uid :
							 output_roots[output.parent_index];
			const uint64_t parent_uid =
				root ? 0 : terms.frozen_outputs[output.parent_index].object_uid;
			output_roots.push_back(root_uid);
			const economic_item_position created{ { item_owner_type::player,
								terms.player_pid, 0 },
							      root_uid,
							      parent_uid,
							      1,
							      item_custody_state::active,
							      0 };
			candidate.items_after[index].position = created;
			candidate.item_events.push_back({ uint32_t(candidate.item_events.size()), 0,
							  output.object_uid, prior, created });
		}

		// Original Smith grants its output, then calls extract_obj on ore roots
		// in reverse selection order. extract_obj traverses original contains
		// siblings in order, retiring each complete child tree before its parent.
		const auto &inputs = terms.selected_native_items;
		std::vector<std::vector<size_t>> children(inputs.size());
		for (size_t i = 0; i < inputs.size(); ++i)
			if (inputs[i].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				children[inputs[i].parent_index].push_back(i);
		struct cursor
		{
			size_t index, next;
		};
		std::vector<cursor> stack;
		stack.reserve(PLAYER_SNAPSHOT_MAX_DEPTH);
		size_t retired = 0;
		for (size_t root = terms.selected_root_order.size(); root-- > 0;)
		{
			const auto found = std::find_if(
				inputs.begin(), inputs.end(),
				[&](const auto &item)
				{
					return item.object_uid == terms.selected_root_order[root] &&
					       item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
				});
			if (found == inputs.end())
				return error::corrupt_evidence;
			stack.push_back({ size_t(found - inputs.begin()), 0 });
			while (!stack.empty())
			{
				auto &current = stack.back();
				if (current.next < children[current.index].size())
				{
					const size_t child =
						children[current.index][current.next++];
					stack.push_back({ child, 0 });
					continue;
				}
				const uint64_t uid = inputs[current.index].object_uid;
				const size_t index = index_of(candidate.items_before, uid);
				if (index == candidate.items_before.size())
					return error::incomplete_coverage;
				const auto &prior = candidate.items_before[index].position;
				auto after_position = prior;
				after_position.owner = { item_owner_type::destruction, 0, 0 };
				after_position.state = item_custody_state::destroyed;
				after_position.equipment_slot = 0;
				++after_position.revision;
				candidate.items_after[index].position = after_position;
				candidate.item_events.push_back(
					{ uint32_t(candidate.item_events.size()), 0, uid, prior,
					  after_position });
				++retired;
				stack.pop_back();
			}
		}
		if (retired != inputs.size())
			return error::incomplete_coverage;
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		*plan = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

#endif
