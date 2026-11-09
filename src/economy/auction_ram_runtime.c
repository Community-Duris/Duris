#include "economy/auction_ram.h"
#include "economy/economic_state.h"
#include "item/item_ownership_runtime.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <map>
#include <memory>

namespace
{
std::map<uint32_t, auction_ram_listing> listings;
std::map<uint32_t, auction_ram_pickup> pickups;
uint64_t catalog_revision = 0;
bool flatfile_catalog = false;
uint32_t closing_fee_basis_points = 300;
uint32_t bid_extension_seconds = 300;
struct prepared_auction
{
	critical_operation_id id;
	auction_command_payload payload;
	auction_command_result result;
	auction_ram_listing next;
	item_ownership_runtime_patch custody;
	auction_ram_listing *destination = nullptr;
	auction_ram_pickup *refund = nullptr;
	auction_ram_pickup *seller = nullptr;
	auction_ram_pickup *claim = nullptr;
};
std::unique_ptr<prepared_auction> prepared;

bool credit_ok(auction_ram_pickup *pickup, int64_t amount)
{
	return pickup && amount >= 0 && amount <= UINT_MAX && pickup->amount >= 0 &&
	       pickup->amount <= UINT_MAX - amount && pickup->revision != UINT64_MAX;
}
} // namespace

bool auction_ram_seed(const auction_ram_listing &listing)
{
	return listing.id && listing.seller && listing.status >= 1 && listing.status <= 3 &&
	       listings.emplace(listing.id, listing).second;
}

bool auction_ram_seed_pickup(uint32_t pid, const auction_ram_pickup &pickup)
{
	return pid && pickup.amount >= 0 && pickup.amount <= UINT_MAX &&
	       pickups.emplace(pid, pickup).second;
}

void auction_ram_set_catalog_revision(uint64_t revision, bool flatfile)
{
	catalog_revision = revision;
	flatfile_catalog = flatfile;
}

void auction_ram_set_policy(uint32_t fee, uint32_t extension)
{
	closing_fee_basis_points = std::min(fee, UINT32_C(10000));
	bid_extension_seconds = extension;
}

bool auction_ram_find(uint32_t id, auction_ram_listing *listing)
{
	const auto found = listings.find(id);
	if (!listing || found == listings.end() || !found->second.id)
		return false;
	*listing = found->second;
	return true;
}

bool auction_ram_find_pickup(uint32_t pid, auction_ram_pickup *pickup)
{
	const auto found = pickups.find(pid);
	if (!pickup || found == pickups.end())
		return false;
	*pickup = found->second;
	return true;
}

size_t auction_ram_expired(uint64_t now, uint32_t *ids, size_t capacity) noexcept
{
	size_t count = 0;
	if (!ids)
		return 0;
	for (const auto &[id, listing] : listings)
	{
		if (count == capacity)
			break;
		if (listing.status == 1 && listing.end_time <= now)
			ids[count++] = id;
	}
	return count;
}

critical_apply_result auction_ram_prepare_command(const critical_command &command)
{
	prepared.reset();
	auto candidate = std::make_unique<prepared_auction>();
	auto &payload = candidate->payload;
	if (!auction_command_decode_payload(command, &payload))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	candidate->id = command.operation_id;
	currency_command_result balances{};
	if (payload.actor_pid &&
	    !economic_state_balances(payload.actor_pid, payload.account_name.data(),
				     payload.racewar, &balances))
		return { critical_apply_outcome::terminal_failure, 0, EACCES };
	const uint32_t id = payload.action == auction_action::list ?
				    auction_command_listing_id(command.operation_id) :
				    payload.auction_id;
	auto found = listings.find(id);
	const auto *current = found == listings.end() || !found->second.id ? nullptr :
									     &found->second;
	auction_ram_pickup pickup{};
	auction_ram_find_pickup(payload.actor_pid, &pickup);
	uint64_t player_owner_revision = 0, auction_owner_revision = 0;
	if (payload.item_count)
	{
		const item_owner_identity player{ item_owner_type::player, payload.actor_pid, 0 };
		const item_owner_identity auction{ item_owner_type::auction, id, 0 };
		if (!item_ownership_runtime_owner_revision(player, &player_owner_revision) ||
		    !item_ownership_runtime_owner_revision(auction, &auction_owner_revision))
			return { critical_apply_outcome::terminal_failure, 0, ESTALE };
		const auto &owner = payload.action == auction_action::list ? player : auction;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			item_ownership_runtime_entry item{};
			const auto &entry = payload.items[i];
			if (!item_ownership_runtime_lookup(entry.item_uid, &item) ||
			    !item_owner_identity_equal(owner, item.owner) || item.parent_item_uid ||
			    item.root_item_uid != item.item_uid || item.vnum != entry.vnum ||
			    item.item_revision != entry.expected_item_revision ||
			    item.state != item_custody_state::active)
				return { critical_apply_outcome::terminal_failure, 0, ESTALE };
		}
	}
	critical_command decision = command;
	if (command.source_site == critical_source_site::website)
	{
		// Request identity contains only the caller's intent. The current server
		// policy is frozen in the outcome, so retries survive policy changes.
		payload.closing_fee_basis_points = closing_fee_basis_points;
		payload.bid_extension_seconds = bid_extension_seconds;
		if (!auction_command_encode_payload(payload, &decision.payload))
			return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	}
	if (auto error = auction_ram_prepare(decision, current, balances, pickup,
					     player_owner_revision, auction_owner_revision,
					     &candidate->next, &candidate->result))
		return { critical_apply_outcome::terminal_failure, 0, error };
	auto &result = candidate->result;
	if (payload.item_count)
	{
		const item_owner_identity player{ item_owner_type::player, payload.actor_pid, 0 };
		const item_owner_identity auction{ item_owner_type::auction, id, 0 };
		const bool listing = payload.action == auction_action::list;
		candidate->custody.owners = { { player, result.player_owner_revision },
					      { auction, result.auction_owner_revision } };
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			item_ownership_runtime_entry before{};
			if (!item_ownership_runtime_lookup(payload.items[i].item_uid, &before))
				return { critical_apply_outcome::terminal_failure, 0, ESTALE };
			candidate->custody.before.push_back(before);
			auto after = before;
			after.owner = listing ? auction : player;
			after.item_revision = result.item_revisions[i];
			after.owner_revision = listing ? result.auction_owner_revision :
							 result.player_owner_revision;
			candidate->custody.after.push_back(after);
		}
	}

	if (flatfile_catalog)
	{
		if (catalog_revision == UINT64_MAX)
			return { critical_apply_outcome::terminal_failure, 0, ERANGE };
		result.catalog_revision = catalog_revision + 1;
	}
	if (payload.action == auction_action::claim_money)
		candidate->claim = &pickups.at(payload.actor_pid);
	else
		candidate->destination = &listings.try_emplace(id).first->second;
	if (result.refund)
	{
		const uint32_t pid = payload.action == auction_action::remove ?
					     result.winner_pid :
					     result.previous_bidder_pid;
		candidate->refund = &pickups.try_emplace(pid).first->second;
		if (!credit_ok(candidate->refund, result.refund))
			return { critical_apply_outcome::terminal_failure, 0, ERANGE };
	}
	if (result.event_type == auction_event_type::sold)
	{
		candidate->seller = &pickups.try_emplace(result.seller_pid).first->second;
		if (!credit_ok(candidate->seller, result.seller_proceeds))
			return { critical_apply_outcome::terminal_failure, 0, ERANGE };
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> bytes{};
	if (!auction_command_encode_result(result, &bytes))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	critical_apply_result completion{
		critical_apply_outcome::applied,
		std::max({ result.catalog_revision, result.auction_revision, result.wallet_revision,
			   result.bank_revision, result.player_owner_revision,
			   result.auction_owner_revision }),
		0
	};
	completion.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), completion.result_payload.begin());
	prepared = std::move(candidate);
	return completion;
}

unsigned int auction_ram_next_claim(uint32_t pid, auction_command_payload *payload)
{
	if (!payload || !pid || payload->actor_pid != pid)
		return EINVAL;
	const auto money = pickups.find(pid);
	if (money != pickups.end() && money->second.amount > 0)
	{
		payload->action = auction_action::claim_money;
		return 0;
	}
	for (const auto &[id, listing] : listings)
	{
		const bool pending = std::any_of(listing.items.begin(), listing.items.end(),
						 [&](const auto &item)
						 { return item.claimant == pid && !item.claimed; });
		if (!pending)
			continue;
		if (listing.object_blob.empty() ||
		    listing.object_blob.size() >= payload->object_blob.size())
			return EILSEQ;
		payload->action = auction_action::claim_item;
		payload->auction_id = id;
		payload->item_count = 0;
		payload->object_blob_size = listing.object_blob.size();
		std::copy(listing.object_blob.begin(), listing.object_blob.end(),
			  payload->object_blob.begin());
		for (const auto &item : listing.items)
			if (item.claimant == pid && !item.claimed)
			{
				if (payload->item_count == payload->items.size())
					return EILSEQ;
				payload->items[payload->item_count++] = { item.uid, item.revision,
									  item.vnum };
			}
		return 0;
	}
	return ENOENT;
}

bool auction_ram_can_publish(const critical_command &command) noexcept
{
	if (command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION || !prepared ||
	    !item_ownership_runtime_patch_ready(prepared->custody) ||
	    !critical_operation_id_equal(prepared->id, command.operation_id))
		return false;
	const auto &result = prepared->result;
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> bytes{};
	const uint64_t revision =
		std::max({ result.catalog_revision, result.auction_revision, result.wallet_revision,
			   result.bank_revision, result.player_owner_revision,
			   result.auction_owner_revision });
	if (!auction_command_encode_result(result, &bytes) ||
	    !critical_command_outcome_matches(command, revision, 0, bytes.data(), bytes.size()))
		return false;
	return !prepared->payload.actor_pid ||
	       economic_state_can_publish(prepared->payload.actor_pid,
					  prepared->payload.account_name.data(),
					  prepared->payload.racewar,
					  { result.wallet, result.bank, result.wallet_revision,
					    result.bank_revision });
}

bool auction_ram_publish(const critical_command &command) noexcept
{
	if (!auction_ram_can_publish(command))
		return false;
	auto &entry = *prepared;
	const auto &result = entry.result;
	if (!item_ownership_runtime_patch_publish(entry.custody))
		return false;
	if (entry.payload.actor_pid &&
	    !economic_state_publish(entry.payload.actor_pid, entry.payload.account_name.data(),
				    entry.payload.racewar,
				    { result.wallet, result.bank, result.wallet_revision,
				      result.bank_revision }))
		return false;
	if (entry.destination)
		*entry.destination = std::move(entry.next);
	if (entry.refund)
	{
		entry.refund->amount += result.refund;
		++entry.refund->revision;
	}
	if (entry.seller)
	{
		entry.seller->amount += result.seller_proceeds;
		++entry.seller->revision;
	}
	if (entry.claim)
	{
		entry.claim->amount = 0;
		++entry.claim->revision;
	}
	if (flatfile_catalog)
		catalog_revision = result.catalog_revision;
	prepared.reset();
	return true;
}

void auction_ram_reset()
{
	prepared.reset();
	listings.clear();
	pickups.clear();
	catalog_revision = 0;
	flatfile_catalog = false;
}
