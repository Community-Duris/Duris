#include "economy/collector_ram.h"
#include "economy/collector_runtime.h"
#include "economy/collector_eligibility.h"
#include "economy/economic_state.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <map>

namespace
{
bool flatfile = false;
collector::catalog candidate;
std::vector<collector_death_snapshot> deaths;
critical_operation_id staged_id{};
critical_apply_result sealed{};
bool staged = false;

unsigned int policy_error(collector::outcome value)
{
	switch (value)
	{
	case collector::outcome::applied:
		return 0;
	case collector::outcome::not_due:
		return EAGAIN;
	case collector::outcome::conflict:
		return ESTALE;
	case collector::outcome::overflow:
		return ERANGE;
	case collector::outcome::forbidden:
		return EACCES;
	case collector::outcome::insufficient_funds:
		return ENOSPC;
	case collector::outcome::capacity:
		return ENOBUFS;
	default:
		return EINVAL;
	}
}

bool stage(const critical_command &command)
{
	if (staged)
		return critical_operation_id_equal(staged_id, command.operation_id);
	if (!collector_runtime_snapshot(&candidate) || !collector_runtime_deaths(&deaths))
		return false;
	staged_id = command.operation_id;
	staged = true;
	return true;
}

unsigned int advance(size_t count)
{
	const size_t increment = flatfile ? (count != 0) : count;
	if (candidate.revision > UINT64_MAX - increment)
		return ERANGE;
	candidate.revision += increment;
	return 0;
}

critical_apply_result failure(unsigned int error)
{
	return { critical_apply_outcome::terminal_failure, 0, error };
}
}

void collector_ram_set_flatfile(bool value)
{
	flatfile = value;
}
void collector_ram_begin()
{
	staged = false;
	sealed = {};
}

critical_apply_result collector_ram_prepare(const critical_command &command)
{
	collector_command_payload payload{};
	if (!collector_command_decode_payload(command, &payload))
		return failure(EINVAL);
	if (!stage(command))
		return failure(ENOMEM);
	auto found = std::find_if(candidate.records.begin(), candidate.records.end(),
				  [&](const auto &entry)
				  { return entry.listing == payload.listing; });
	if (found == candidate.records.end())
		return failure(ENOENT);
	auto &entry = *found;
	if (payload.action != collector_action::hint_ack &&
	    entry.revision != payload.expected_listing_revision)
		return failure(ESTALE);
	collector_command_result result{};
	result.action = payload.action;
	result.record_present = true;
	if (payload.action == collector_action::hint ||
	    payload.action == collector_action::hint_ack)
	{
		auto death =
			std::find_if(deaths.begin(), deaths.end(),
				     [&](const auto &value)
				     {
					     char hex[CRITICAL_COMMAND_ID_HEX_SIZE]{};
					     return critical_operation_id_to_hex(
							    value.operation_id, hex, sizeof(hex)) &&
						    !strcmp(hex, entry.death_operation.data());
				     });
		if (death == deaths.end())
			return failure(ENOENT);
		if (payload.action == collector_action::hint)
		{
			if (entry.status != collector::state::available || entry.holding_paused ||
			    payload.observed_at < entry.available_at ||
			    payload.observed_at >= entry.expires_at)
				return failure(ESTALE);
			if (death->hint_state != COLLECTOR_HINT_NONE)
				return failure(EALREADY);
			death->hint_state = COLLECTOR_HINT_PENDING;
			death->hint_revision = entry.revision;
		}
		else
		{
			if (death->hint_state == COLLECTOR_HINT_DELIVERED)
				return failure(EALREADY);
			if (death->hint_state != COLLECTOR_HINT_PENDING ||
			    death->hint_revision != payload.expected_listing_revision)
				return failure(ESTALE);
			death->hint_state = COLLECTOR_HINT_DELIVERED;
		}
	}
	else
	{
		currency_command_result wallet{};
		int64_t value = 0;
		if (payload.action == collector_action::purchase)
		{
			if (!economic_state_balances(payload.actor_pid, payload.account_name.data(),
						     payload.racewar, &wallet))
				return failure(ENOENT);
			if (wallet.wallet_revision != payload.expected_wallet_revision ||
			    wallet.bank_revision != payload.expected_bank_revision)
				return failure(ESTALE);
			if (wallet.wallet_revision == UINT64_MAX ||
			    wallet.bank_revision == UINT64_MAX)
				return failure(ERANGE);
			value = wallet.wallet.amount[0] + 10 * wallet.wallet.amount[1] +
				100 * wallet.wallet.amount[2] + 1000 * wallet.wallet.amount[3];
		}
		player_item_snapshot snapshot;
		if (payload.item_count)
		{
			uint64_t from = 0, to = 0;
			item_ownership_runtime_peek_owner_revision(payload.from_owner, &from);
			item_ownership_runtime_peek_owner_revision(payload.to_owner, &to);
			if (from != payload.expected_from_owner_revision ||
			    to != payload.expected_to_owner_revision)
				return failure(ESTALE);
			if (from == UINT64_MAX || to == UINT64_MAX)
				return failure(ERANGE);
			std::vector<item_ownership_runtime_entry> owned;
			if (!item_ownership_runtime_snapshot_owner(payload.from_owner, SIZE_MAX,
								   &owned))
				return failure(ENOMEM);
			size_t root_count = 0;
			for (const auto &item : owned)
				if (item.root_item_uid == payload.items[0].root_item_uid)
					++root_count;
			if (root_count != payload.item_count)
				return failure(ESTALE);
			bool selected = false;
			for (size_t i = 0; i < payload.item_count; ++i)
			{
				const auto &expected = payload.items[i];
				item_ownership_runtime_entry current;
				if (!item_ownership_runtime_lookup(expected.item_uid, &current) ||
				    !item_owner_identity_equal(current.owner, payload.from_owner) ||
				    current.item_revision != expected.expected_item_revision ||
				    current.state != expected.expected_state ||
				    current.root_item_uid != expected.root_item_uid ||
				    current.parent_item_uid != expected.parent_item_uid ||
				    current.vnum != expected.vnum)
					return failure(ESTALE);
				if (current.item_revision == UINT64_MAX)
					return failure(ERANGE);
				selected = selected ||
					   current.item_uid == payload.selected_item_uid;
			}
			std::vector<player_item_snapshot> items;
			if (!selected ||
			    player_item_snapshot_list_decode(payload.item_blob.data(),
							     payload.item_blob_size, &items) !=
				    player_snapshot_codec_result::ok ||
			    items.size() != 1 || items[0].object_uid != payload.selected_item_uid ||
			    items[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
			    items[0].equipment_slot != 0)
				return failure(EBADMSG);
			snapshot = std::move(items[0]);
			result.from_owner_revision = from + 1;
			result.to_owner_revision = to + 1;
		}
		collector::outcome next = collector::outcome::invalid;
		switch (payload.action)
		{
		case collector_action::collect:
		{
			const auto selected = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &item)
				{ return item.item_uid == payload.selected_item_uid; });
			if (selected == payload.items.begin() + payload.item_count)
				return failure(EINVAL);
			next = collector::collect(&entry, payload.expected_listing_revision,
						  selected->expected_item_revision,
						  selected->expected_item_revision, true,
						  snapshot.cost, payload.observed_at);
			break;
		}
		case collector_action::activate:
			next = collector::activate(&entry, payload.expected_listing_revision,
						   payload.observed_at);
			break;
		case collector_action::purchase:
			next = collector::purchase(&entry, payload.expected_listing_revision,
						   payload.actor_pid, value,
						   payload.capacity_admitted, payload.observed_at);
			break;
		case collector_action::expire:
			next = collector::expire(&entry, payload.expected_listing_revision,
						 payload.observed_at);
			break;
		case collector_action::cancel:
			next = collector::cancel(&entry, payload.expected_listing_revision,
						 payload.cancel_reason);
			break;
		case collector_action::pause:
			next = collector::pause(&entry, payload.expected_listing_revision,
						payload.observed_at);
			break;
		case collector_action::resume:
			next = collector::resume(&entry, payload.expected_listing_revision,
						 payload.observed_at);
			break;
		default:
			break;
		}
		if (auto error = policy_error(next))
			return failure(error);
		if (payload.action == collector_action::purchase)
		{
			const auto remaining = value - entry.price_value;
			result.wallet.amount = { static_cast<int64_t>(remaining % 10),
						 static_cast<int64_t>((remaining / 10) % 10),
						 static_cast<int64_t>((remaining / 100) % 10),
						 static_cast<int64_t>(remaining / 1000) };
			result.bank = wallet.bank;
			result.wallet_revision = wallet.wallet_revision + 1;
			result.bank_revision = wallet.bank_revision + 1;
		}
		if (auto error = advance(1))
			return failure(error);
	}
	result.entry = entry;
	result.catalog_revision = candidate.revision;
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> bytes{};
	if (!collector_command_encode_result(result, &bytes))
		return failure(EINVAL);
	critical_apply_result prepared{
		critical_apply_outcome::applied,
		std::max({ result.catalog_revision, result.from_owner_revision,
			   result.to_owner_revision, result.wallet_revision, result.bank_revision,
			   result.entry.revision, result.entry.item_revision }),
		0
	};
	prepared.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), prepared.result_payload.begin());
	return prepared;
}

unsigned int
collector_ram_prepare_boundary(const critical_command &command, collector::reason reason,
			       const std::vector<collector_custody_boundary_item> &items,
			       bool *changed, const char *excluded_death)
{
	if (!changed)
		return EINVAL;
	*changed = false;
	if (reason == collector::reason::none || items.empty())
		return 0;
	if (!staged)
	{
		std::vector<uint64_t> uids;
		for (const auto &item : items)
			uids.push_back(item.item_uid);
		if (!collector_runtime_has_candidates(uids))
			return 0;
	}
	if (!stage(command))
		return ENOMEM;
	size_t cancelled = 0;
	for (auto &entry : candidate.records)
	{
		if (entry.status != collector::state::candidate ||
		    (excluded_death && !strcmp(entry.death_operation.data(), excluded_death)))
			continue;
		auto item = std::find_if(items.begin(), items.end(), [&](const auto &value)
					 { return value.item_uid == entry.uid; });
		if (item == items.end())
			continue;
		if (item->post_item_revision < entry.item_revision)
			return ESTALE;
		if (auto error = policy_error(collector::cancel(&entry, entry.revision, reason)))
			return error;
		entry.item_revision = item->post_item_revision;
		++cancelled;
	}
	if (auto error = advance(cancelled))
		return error;
	*changed = cancelled != 0;
	return 0;
}

unsigned int collector_ram_prepare_item(const critical_command &command,
					const item_transfer_payload &payload,
					item_transfer_result *result)
{
	if (!result)
		return EINVAL;
	if (!payload.collector.present)
	{
		if (collector_item_transfer_boundary_reason(payload) == collector::reason::none)
			return 0;
		std::vector<uint64_t> uids;
		for (size_t i = 0; i < payload.item_count; ++i)
			uids.push_back(payload.items[i].item_uid);
		if (!collector_runtime_has_candidates(uids))
			return 0;
	}
	if (!stage(command))
		return ENOMEM;
	std::vector<collector_custody_boundary_item> boundaries;
	char death_hex[CRITICAL_COMMAND_ID_HEX_SIZE]{};
	if (payload.collector.present &&
	    !critical_operation_id_to_hex(payload.collector.death_operation, death_hex,
					  sizeof(death_hex)))
		return EINVAL;
	// Existing candidates from this same death are intentionally not cancelled.
	for (size_t i = 0; i < payload.item_count; ++i)
	{
		const auto &item = payload.items[i];
		boundaries.push_back(
			{ item.item_uid, item.expected_state == item_custody_state::absent ?
						 1 :
						 item.expected_item_revision + 1 });
	}
	if (auto error = collector_ram_prepare_boundary(
		    command, collector_item_transfer_boundary_reason(payload), boundaries,
		    &result->collector_catalog_changed,
		    payload.collector.present ? death_hex : nullptr))
		return error;
	if (!payload.collector.present)
		return 0;
	std::vector<player_item_snapshot> snapshots;
	std::vector<uint64_t> eligible;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok ||
	    snapshots.size() != payload.item_count)
		return EBADMSG;
	for (const auto &snapshot : snapshots)
		if (collector_death_item_snapshot_eligible(snapshot))
			eligible.push_back(snapshot.object_uid);
	std::sort(eligible.begin(), eligible.end());
	if (eligible != payload.collector.eligible_item_uids)
		return EBADMSG;
	auto death =
		std::find_if(deaths.begin(), deaths.end(),
			     [&](const auto &value) {
				     return critical_operation_id_equal(
					     value.operation_id, payload.collector.death_operation);
			     });
	if (death == deaths.end())
	{
		if (!critical_operation_id_equal(command.operation_id,
						 payload.collector.death_operation))
			return ESTALE;
		collector_death_snapshot value;
		value.operation_id = payload.collector.death_operation;
		value.beneficiary_pid = payload.collector.beneficiary_pid;
		value.death_time = payload.collector.death_time;
		const auto &policy = payload.collector.policy;
		value.policy = { true,
				 policy.collection_delay,
				 policy.sale_delay,
				 policy.holding_duration,
				 policy.price_percent,
				 policy.minimum_value };
		if (!collector::valid_rules(value.policy))
			return EINVAL;
		result->collector_catalog_changed = true;
		deaths.push_back(value);
		death = std::prev(deaths.end());
	}
	if (death->beneficiary_pid != payload.collector.beneficiary_pid ||
	    death->death_time != payload.collector.death_time)
		return ESTALE;
	size_t enrolled = 0;
	for (uint64_t uid : eligible)
	{
		if (std::any_of(candidate.records.begin(), candidate.records.end(),
				[&](const auto &entry) {
					return entry.uid == uid &&
					       !strcmp(entry.death_operation.data(), death_hex);
				}))
			continue;
		if (candidate.next_listing == UINT64_MAX ||
		    candidate.records.size() == collector::catalog_max_records)
			return ENOSPC;
		const auto item = std::find_if(payload.items.begin(),
					       payload.items.begin() + payload.item_count,
					       [&](const auto &value)
					       { return value.item_uid == uid; });
		if (item == payload.items.begin() + payload.item_count)
			return EBADMSG;
		collector::record entry;
		const auto revision = item->expected_state == item_custody_state::absent ?
					      1 :
					      item->expected_item_revision + 1;
		if (auto error = policy_error(collector::enroll(
			    candidate.next_listing, death_hex, death->beneficiary_pid, uid,
			    revision, death->death_time, death->policy, &entry)))
			return error;
		candidate.records.push_back(entry);
		++candidate.next_listing;
		++enrolled;
	}
	if (enrolled)
	{
		if (auto error = advance(1))
			return error;
		result->collector_catalog_changed = true;
	}
	return 0;
}

uint64_t collector_ram_revision()
{
	return staged ? candidate.revision : 0;
}

bool collector_ram_seal(const critical_command &command, const critical_apply_result &result)
{
	if (!staged)
		return true;
	if (!critical_operation_id_equal(staged_id, command.operation_id))
		return false;
	if (result.outcome != critical_apply_outcome::applied ||
	    !collector_runtime_stage(candidate, deaths))
	{
		staged = false;
		return false;
	}
	sealed = result;
	return true;
}

bool collector_ram_can_publish(const critical_command &command)
{
	return command.schema_version == CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION &&
	       (!staged || (critical_operation_id_equal(staged_id, command.operation_id) &&
			    critical_command_outcome_matches(
				    command, sealed.durable_revision, sealed.error_code,
				    sealed.result_payload.data(), sealed.result_size)));
}

bool collector_ram_publish(const critical_command &command)
{
	if (!collector_ram_can_publish(command))
		return false;
	if (staged)
		collector_runtime_commit_staged();
	staged = false;
	return true;
}
