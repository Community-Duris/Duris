#include "economy/economic_state_repository.h"
#include "economy/economic_state.h"
#include "player/player_checkpoint_ram.h"
#include "player/player_restitution_ram.h"
#include "player/artifact_restitution_ram.h"
#include "player/player_snapshot_codec.h"
#include "flatfile/flatfile_player_repository.h"
#include <openssl/sha.h>
#include <cmath>
#include <type_traits>
#include "economy/boon_ram.h"
#include "economy/collector_ram.h"
#include "economy/collector_runtime.h"
#include "economy/collector_repository.h"
#include "flatfile/flatfile_collector_repository.h"
#include "economy/collector_catalog_cache.h"
#include "economy/shop_trade_runtime.h"
#include "economy/auction_ram.h"
#include "persistence/ram_command.h"
#include "persistence/corpse_ram.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "world/zone_touch_command.h"
#include "item/item_ownership_runtime.h"
#include "flatfile/flatfile_auction_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"

#include <charconv>
#include <cstring>
#include <map>
#include <memory>

namespace
{
template <typename T> bool number(const char *text, T *value)
{
	if (!text || !*text)
		return false;
	const char *end = text + strlen(text);
	const auto parsed = std::from_chars(text, end, *value);
	if constexpr (std::is_floating_point_v<T>)
		if (!std::isfinite(*value))
			return false;
	return parsed.ec == std::errc{} && parsed.ptr == end;
}

bool seed_listing(const auction_ram_listing &listing)
{
	const item_owner_identity owner{ item_owner_type::auction, listing.id, 0 };
	uint64_t revision = 0;
	if (!item_ownership_runtime_owner_revision(owner, &revision))
		return false;
	for (const auto &item : listing.items)
		if (!item.claimed && !item_ownership_runtime_hydrate(
					     { item.uid, item.uid, 0, owner, item.revision,
					       revision, item.vnum, item_custody_state::active }))
			return false;
	return auction_ram_seed(listing);
}

#ifndef __NO_MYSQL__
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
result_ptr query(MYSQL *connection, const char *sql)
{
	return { mysql_real_query(connection, sql, strlen(sql)) ? nullptr :
								  mysql_store_result(connection),
		 mysql_free_result };
}

bool load_artifact_evidence(MYSQL *connection)
{
	std::map<uint32_t, artifact_restitution_state> states;
	auto rows = query(
		connection,
		"SELECT vnum,owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,bind_timer_epoch,item_uid,COALESCE(item_revision,0),revision,item_revision IS NOT NULL FROM artifact_domain_state");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		artifact_restitution_state s{};
		unsigned owned = 0;
		if (!number(row[0], &s.vnum) || !s.vnum || !number(row[1], &owned) || owned > 1 ||
		    !number(row[2], &s.domain.location_type) ||
		    !number(row[3], &s.domain.location) || !number(row[4], &s.domain.timer) ||
		    !number(row[5], &s.domain.type) || !number(row[6], &s.bind_owner) ||
		    !number(row[7], &s.bind_timer) || (row[8] && !number(row[8], &s.uid)) ||
		    !number(row[9], &s.item_revision) || !number(row[10], &s.revision))
			return false;
		s.domain.present = true;
		s.domain.owned = owned;
		s.uid_present = row[8] != nullptr;
		s.item_revision_present = row[11] && row[11][0] == '1';
		states.emplace(s.vnum, s);
	}
	rows = query(
		connection,
		"SELECT vnum,opening_timer_epoch,opening_bind_owner_pid,opening_bind_timer_epoch,opening_revision FROM artifact_domain_baseline");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t vnum = 0;
		if (!number(row[0], &vnum) || !vnum)
			return false;
		auto &s = states[vnum];
		s.vnum = vnum;
		s.baseline_present = true;
		if (!number(row[1], &s.opening_timer) || !number(row[2], &s.opening_bind_owner) ||
		    !number(row[3], &s.opening_bind_timer) || !number(row[4], &s.opening_revision))
			return false;
	}
	rows = query(connection,
		     "SELECT vnum,COALESCE(owner_pid,-1),COALESCE(timer,0) FROM artifact_bind");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t vnum = 0;
		if (!number(row[0], &vnum) || !vnum)
			return false;
		auto &s = states[vnum];
		s.vnum = vnum;
		s.bind_present = true;
		if (!number(row[1], &s.bind_row_owner) || !number(row[2], &s.bind_row_timer))
			return false;
	}
	for (bool mortal : { false, true })
	{
		rows = query(
			connection,
			mortal ?
				"SELECT vnum,owned='Y',locType,location,COALESCE(UNIX_TIMESTAMP(timer),0),type FROM artifacts_mortal" :
				"SELECT vnum,owned='Y',locType,location,COALESCE(UNIX_TIMESTAMP(timer),0),type FROM artifacts");
		if (!rows)
			return false;
		while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
		{
			uint32_t vnum = 0;
			unsigned owned = 0;
			if (!number(row[0], &vnum) || !vnum || !number(row[1], &owned) || owned > 1)
				return false;
			auto &s = states[vnum];
			s.vnum = vnum;
			auto &location = mortal ? s.mortal : s.god;
			location.present = true;
			location.owned = owned;
			if (!number(row[2], &location.location_type) ||
			    !number(row[3], &location.location) ||
			    !number(row[4], &location.timer) || !number(row[5], &location.type))
				return false;
		}
	}
	for (const auto &[vnum, s] : states)
		if (!artifact_restitution_ram_seed(s))
			return false;
	player_restitution_ram_artifact_callbacks(artifact_restitution_ram_prepare,
						  artifact_restitution_ram_publish);
	return true;
}

bool load_death_evidence(MYSQL *connection)
{
	std::map<std::pair<uint32_t, uint64_t>, player_death_ram_evidence> deaths;
	auto rows = query(
		connection,
		"SELECT pid,save_revision,payload,FLOOR(UNIX_TIMESTAMP(recorded_at)) FROM player_death_disposition");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t pid = 0;
		uint64_t revision = 0;
		player_death_ram_evidence evidence;
		player_snapshot snapshot;
		const auto lengths = mysql_fetch_lengths(rows.get());
		if (!lengths || !number(row[0], &pid) || !number(row[1], &revision) || !row[2] ||
		    !number(row[3], &evidence.epoch) ||
		    player_snapshot_decode(reinterpret_cast<const uint8_t *>(row[2]), lengths[2],
					   &snapshot) != player_snapshot_codec_result::ok ||
		    !snapshot.death || snapshot.pid != static_cast<int32_t>(pid) ||
		    snapshot.revision != revision)
			return false;
		evidence.operation_id = snapshot.death->operation_id;
		SHA256(reinterpret_cast<const uint8_t *>(row[2]), lengths[2],
		       evidence.digest.data());
		if (!deaths.emplace(std::make_pair(pid, revision), std::move(evidence)).second)
			return false;
	}
	rows = query(
		connection,
		"SELECT pid,save_revision,item_uid,root_item_uid,COALESCE(parent_item_uid,0),item_revision,vnum,state,owner_type,owner_id,owner_context_id,owner_revision FROM player_death_custody");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t pid = 0;
		uint64_t revision = 0;
		unsigned state = 0, owner = 0;
		player_death_custody_snapshot custody{};
		if (!number(row[0], &pid) || !number(row[1], &revision) ||
		    !number(row[2], &custody.item.item_uid) ||
		    !number(row[3], &custody.item.root_item_uid) ||
		    !number(row[4], &custody.item.parent_item_uid) ||
		    !number(row[5], &custody.item.expected_item_revision) ||
		    !number(row[6], &custody.item.vnum) || !number(row[7], &state) ||
		    !number(row[8], &owner) || !number(row[9], &custody.owner.id) ||
		    !number(row[10], &custody.owner.context_id) ||
		    !number(row[11], &custody.owner_revision))
			return false;
		custody.item.expected_state = static_cast<item_custody_state>(state);
		custody.owner.type = static_cast<item_owner_type>(owner);
		const auto death = deaths.find({ pid, revision });
		if (death == deaths.end() ||
		    !death->second.custody.emplace(custody.item.item_uid, custody).second)
			return false;
	}
	for (auto &[key, evidence] : deaths)
		if (!player_checkpoint_ram_seed_death(key.first, key.second, std::move(evidence)))
			return false;
	rows = query(connection, "SELECT item_uid FROM player_death_restitution_delivery");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint64_t uid = 0;
		if (!number(row[0], &uid) || !player_restitution_ram_seed_delivery(uid))
			return false;
	}
	return true;
}

bool load_sql(MYSQL *connection)
{
	if (!connection)
		return false;
	auto rows = query(
		connection,
		"SELECT owner_type,owner_id,owner_context_id,revision FROM item_owner_revision");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		item_owner_identity owner{};
		unsigned int type = 0;
		uint64_t revision = 0;
		if (!number(row[0], &type) || !number(row[1], &owner.id) ||
		    !number(row[2], &owner.context_id) || !number(row[3], &revision))
			return false;
		owner.type = static_cast<item_owner_type>(type);
		if (!item_ownership_runtime_hydrate_owner(owner, revision))
			return false;
	}
	rows = query(
		connection,
		"SELECT i.item_uid,i.root_item_uid,COALESCE(i.parent_item_uid,0),i.owner_type,i.owner_id,i.owner_context_id,i.item_revision,r.revision,i.vnum,i.state FROM item_current_owner i LEFT JOIN item_owner_revision r ON r.owner_type=i.owner_type AND r.owner_id=i.owner_id AND r.owner_context_id=i.owner_context_id");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		item_ownership_runtime_entry entry{};
		unsigned int owner = 0, state = 0;
		if (!number(row[0], &entry.item_uid) || !number(row[1], &entry.root_item_uid) ||
		    !number(row[2], &entry.parent_item_uid) || !number(row[3], &owner) ||
		    !number(row[4], &entry.owner.id) || !number(row[5], &entry.owner.context_id) ||
		    !number(row[6], &entry.item_revision) ||
		    !number(row[7], &entry.owner_revision) || !number(row[8], &entry.vnum) ||
		    !number(row[9], &state))
			return false;
		entry.owner.type = static_cast<item_owner_type>(owner);
		entry.state = static_cast<item_custody_state>(state);
		if (!item_ownership_runtime_hydrate(entry))
			return false;
	}
	rows = query(
		connection,
		"SELECT p.pid,p.account_name,p.name,p.racewar,p.copper,p.silver,p.gold,p.platinum,"
		"p.wallet_revision,COALESCE(b.bank_copper,0),COALESCE(b.bank_silver,0),COALESCE(b.bank_gold,0),COALESCE(b.bank_platinum,0),COALESCE(b.bank_revision,0),"
		"p.epics,p.epic_revision,p.frags,p.oldfrags,p.frag_revision,p.save_revision "
		"FROM player_data p JOIN accounts a ON a.account_name=p.account_name LEFT JOIN account_banks b ON b.account_name=p.account_name AND b.racewar=p.racewar");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		economic_player_state player;
		uint64_t save_revision = 0;
		if (!row[1] || !row[2] || !number(row[0], &player.pid) ||
		    !number(row[3], &player.racewar))
			return false;
		player.account = row[1];
		player.name = row[2];
		for (size_t i = 0; i < 4; ++i)
			if (!number(row[4 + i], &player.balances.wallet.amount[i]) ||
			    !number(row[9 + i] ? row[9 + i] : "0", &player.balances.bank.amount[i]))
				return false;
		if (!number(row[8], &player.balances.wallet_revision) ||
		    !number(row[13] ? row[13] : "0", &player.balances.bank_revision) ||
		    !number(row[14], &player.progress.epics) ||
		    !number(row[15], &player.progress.epic_revision) ||
		    !number(row[16], &player.progress.frags) ||
		    !number(row[17], &player.progress.old_frags) ||
		    !number(row[18], &player.progress.frag_revision) ||
		    !number(row[19], &save_revision) ||
		    !player_checkpoint_ram_seed_revision(player.pid, save_revision) ||
		    !economic_state_seed(player))
			return false;
	}
	if (!load_death_evidence(connection) ||
	    !economic_state_repository_refresh_artifacts(connection))
		return false;
	rows = query(
		connection,
		"SELECT id,seller_pid,winning_bidder_pid,status+0,cur_price,buy_price,"
		"auction_revision,UNIX_TIMESTAMP(end_time),COALESCE(p.account_name,'') FROM auctions a "
		"LEFT JOIN player_data p ON p.pid=a.seller_pid WHERE custody_state=1");
	if (!rows)
		return false;
	std::map<uint32_t, auction_ram_listing> listings;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		auction_ram_listing listing;
		if (!number(row[0], &listing.id) || !number(row[1], &listing.seller) ||
		    !number(row[2], &listing.winner) || !number(row[3], &listing.status) ||
		    !number(row[4], &listing.price) || !number(row[5], &listing.buy_price) ||
		    !number(row[6], &listing.revision) || !number(row[7], &listing.end_time))
			return false;
		listing.seller_account = row[8] ? row[8] : "";
		listings.emplace(listing.id, std::move(listing));
	}
	rows = query(
		connection,
		"SELECT auction_id,item_uid,item_revision,vnum,COALESCE(claim_pid,0),"
		"claimed_at IS NOT NULL,obj_blob FROM auction_item_custody ORDER BY auction_id,slot");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t id = 0, claimed = 0;
		auction_ram_item item;
		if (!number(row[0], &id) || !number(row[1], &item.uid) ||
		    !number(row[2], &item.revision) || !number(row[3], &item.vnum) ||
		    !number(row[4], &item.claimant) || !number(row[5], &claimed))
			return false;
		item.claimed = claimed != 0;
		if (auto found = listings.find(id); found != listings.end())
		{
			found->second.items.push_back(item);
			const auto *lengths = mysql_fetch_lengths(rows.get());
			if (!lengths || !row[6] || !lengths[6])
				return false;
			std::vector<uint8_t> blob(row[6], row[6] + lengths[6]);
			if (!found->second.object_blob.empty() && found->second.object_blob != blob)
				return false;
			found->second.object_blob = std::move(blob);
		}
	}
	for (const auto &[id, listing] : listings)
		if (!seed_listing(listing))
			return false;
	rows = query(connection, "SELECT pid,money,claim_revision FROM auction_money_pickups");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t pid = 0;
		auction_ram_pickup pickup;
		if (!number(row[0], &pid) || !number(row[1], &pickup.amount) ||
		    !number(row[2], &pickup.revision) || !auction_ram_seed_pickup(pid, pickup))
			return false;
	}
	rows = query(connection, "SELECT COALESCE(MAX(id),0) FROM pkill_event");
	if (!rows)
		return false;
	MYSQL_ROW maximum = mysql_fetch_row(rows.get());
	uint64_t last_event = 0;
	if (!maximum || !number(maximum[0], &last_event) || last_event >= UINT32_MAX)
		return false;
	ram_command_set_combat_sequence(last_event + 1, false);
	rows = query(
		connection,
		"SELECT i.result_payload FROM epic_stone_claim c JOIN critical_operation_inbox i ON i.operation_id=c.operation_id");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		const auto *lengths = mysql_fetch_lengths(rows.get());
		zone_touch_result result;
		if (!lengths || !row[0] ||
		    !zone_touch_command_decode_result(reinterpret_cast<const uint8_t *>(row[0]),
						      lengths[0], &result) ||
		    !ram_command_seed_stone(result))
			return false;
	}
	boon_ram_state boons;
	rows = query(
		connection,
		"SELECT id,time,duration,racewar,type,opt,criteria,criteria2,bonus,bonus2,random,active,pid,rpt,author FROM boons");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		flatfile_boon_definition entry;
		unsigned int random = 0, active = 0, repeat = 0;
		if (!number(row[0], &entry.id) || !number(row[1], &entry.start_time) ||
		    !number(row[2], &entry.duration) || !number(row[3], &entry.racewar) ||
		    !number(row[4], &entry.type) || !number(row[5], &entry.option) ||
		    !number(row[6], &entry.criteria) || !number(row[7], &entry.criteria2) ||
		    !number(row[8], &entry.bonus) || !number(row[9], &entry.bonus2) ||
		    !number(row[10], &random) || !number(row[11], &active) ||
		    !number(row[12], &entry.target_pid) || !number(row[13], &repeat))
			return false;
		entry.random = random != 0;
		entry.active = active != 0;
		entry.repeat = repeat != 0;
		entry.author = row[14] ? row[14] : "";
		boons.definitions.emplace(entry.id, std::move(entry));
	}
	rows = query(connection, "SELECT boonid,pid,counter FROM boons_progress ORDER BY id");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t boon = 0, pid = 0;
		double counter = 0;
		if (!number(row[0], &boon) || !number(row[1], &pid) || !number(row[2], &counter))
			return false;
		boons.progress.emplace(std::pair{ boon, pid }, counter);
	}
	rows = query(connection, "SELECT pid,points,stats FROM boons_shop");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t pid = 0;
		flatfile_boon_player_projection shop;
		if (!number(row[0], &pid) || !number(row[1], &shop.points) ||
		    !number(row[2], &shop.stats))
			return false;
		boons.shops.emplace(pid, shop);
	}
	rows = query(connection, "SELECT pid,base_stat_revision FROM player_data");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t pid = 0;
		uint64_t revision = 0;
		if (!number(row[0], &pid) || !number(row[1], &revision))
			return false;
		boons.stat_revisions.emplace(pid, revision);
	}
	boon_ram_seed(std::move(boons));
	rows = query(connection, "SELECT shop_id,revision FROM shop_trade_state");
	if (!rows)
		return false;
	shop_trade_runtime_reset_for_tests();
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint32_t id = 0;
		uint64_t revision = 0;
		if (!number(row[0], &id) || !number(row[1], &revision) ||
		    !shop_trade_runtime_seed_revision(id, revision))
			return false;
	}
	rows = query(connection,
		     "SELECT catalog_revision FROM corpse_catalog_state WHERE state_id=1");
	if (!rows)
		return false;
	MYSQL_ROW clock = mysql_fetch_row(rows.get());
	uint64_t corpse_clock = 0;
	if (!clock || !number(clock[0], &corpse_clock) || !corpse_clock)
		return false;
	corpse_ram_reset(corpse_clock, false);
	rows = query(
		connection,
		"SELECT value3,save_id,player_name,corpse_revision,room_vnum,COALESCE(value0,0),COALESCE(value1,0),COALESCE(value2,0),COALESCE(value4,0),COALESCE(value5,0),COALESCE(value7,0) FROM corpses");
	if (!rows)
		return false;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		corpse_lifecycle_payload corpse;
		uint64_t revision = 0;
		if (!number(row[0], &corpse.owner_pid) || !number(row[1], &corpse.save_id) ||
		    !row[2] || !number(row[3], &revision) || !number(row[4], &corpse.room_vnum))
			return false;
		corpse.owner_name = row[2];
		const size_t indices[]{ 0, 1, 2, 4, 5, 7 };
		for (size_t i = 0; i < 6; ++i)
			if (!number(row[5 + i], &corpse.values[indices[i]]))
				return false;
		corpse.values[3] = corpse.owner_pid;
		corpse.values[6] = corpse.save_id;
		if (!corpse_ram_seed(corpse, revision))
			return false;
	}
	collector_bootstrap_snapshot collector;
	if (!collector_repository_read_bootstrap(connection, &collector) ||
	    !collector_runtime_rebuild_authoritative(
		    collector.catalog, collector.held_items.data(), collector.held_items.size(),
		    collector.deaths.data(), collector.deaths.size()))
		return false;
	collector_ram_set_flatfile(false);
	collector_catalog_cache_use_ram(true);
	return true;
}
#endif

bool load_flat(const char *root, std::string *error)
{
	std::vector<std::pair<item_owner_identity, uint64_t>> owners;
	std::vector<item_ownership_runtime_entry> custody;
	if (!flatfile_item_repository_ram_bootstrap(root, &owners, &custody, error))
		return false;
	for (const auto &[owner, revision] : owners)
		if (!item_ownership_runtime_hydrate_owner(owner, revision))
			return false;
	if (!item_ownership_runtime_hydrate_many_atomic(custody.data(), custody.size()))
		return false;
	uint64_t world_revision = 0;
	std::vector<flatfile_corpse_record> corpses;
	if (!flatfile_world_item_ram_bootstrap(root, &world_revision, &corpses, error))
		return false;
	corpse_ram_reset(world_revision, true);
	for (const auto &record : corpses)
	{
		corpse_lifecycle_payload corpse;
		corpse.owner_pid = record.owner_pid;
		corpse.save_id = record.save_id;
		corpse.owner_name = record.owner_name;
		corpse.room_vnum = record.room_vnum;
		corpse.values = record.values;
		corpse.money = record.money;
		if (!corpse_ram_seed(corpse, record.revision))
			return false;
	}
	boon_ram_state boons;
	if (!flatfile_boon_load_ram(root, &boons, error))
		return false;
	std::vector<flatfile_identity_record> identities;
	if (flatfile_identity_list_all(root, &identities, error) != flatfile_identity_result::ok)
		return false;
	for (const auto &identity : identities)
	{
		player_snapshot snapshot;
		const auto snapshot_loaded =
			flatfile_player_snapshot_load(root, identity.pid, &snapshot, error);
		if (snapshot_loaded == flatfile_player_load_result::ok &&
		    !player_checkpoint_ram_seed_revision(identity.pid, snapshot.revision))
			return false;
		flatfile_player_domain_record record;
		const auto loaded = flatfile_player_domain_load(
			root, identity.pid, identity.account, identity.racewar, &record, error);
		if (loaded == flatfile_player_domain_result::not_found)
			continue; // Identity allocated but character creation not yet committed.
		if (loaded != flatfile_player_domain_result::ok)
			return false;
		economic_player_state player{ static_cast<uint32_t>(identity.pid),
					      identity.account,
					      identity.name,
					      static_cast<uint8_t>(identity.racewar),
					      {} };
		for (size_t i = 0; i < 4; ++i)
		{
			player.balances.wallet.amount[i] = record.domains.wallet[i];
			player.balances.bank.amount[i] = record.domains.bank[i];
		}
		player.balances.wallet_revision = record.domains.wallet_revision;
		player.balances.bank_revision = record.domains.bank_revision;
		boons.stat_revisions.emplace(identity.pid, record.domains.base_stat_revision);
		player.progress = { record.domains.epics, record.domains.epic_revision,
				    record.domains.frags, record.domains.old_frags,
				    record.domains.frag_revision };
		if (!economic_state_seed(player))
			return false;
	}
	std::vector<auction_ram_listing> listings;
	std::vector<std::pair<uint32_t, auction_ram_pickup>> pickups;
	uint64_t revision = 0;
	if (!flatfile_auction_load_ram(root, &listings, &pickups, &revision, error))
		return false;
	for (const auto &listing : listings)
		if (!seed_listing(listing))
			return false;
	for (const auto &[pid, pickup] : pickups)
		if (!auction_ram_seed_pickup(pid, pickup))
			return false;
	auction_ram_set_catalog_revision(revision, true);
	ram_command_set_combat_sequence(1, true);
	boon_ram_seed(std::move(boons));
	collector_bootstrap_snapshot collector;
	const auto loaded = flatfile_collector_repository_read_bootstrap(root, &collector, error);
	if (loaded != flatfile_collector_repository_result::ok &&
	    loaded != flatfile_collector_repository_result::not_found)
		return false;
	if (!collector_runtime_rebuild_authoritative(
		    collector.catalog, collector.held_items.data(), collector.held_items.size(),
		    collector.deaths.data(), collector.deaths.size()))
		return false;
	collector_ram_set_flatfile(true);
	collector_catalog_cache_use_ram(true);
	return true;
}
} // namespace

bool economic_state_repository_load(MYSQL *connection, const char *root, std::string *error)
try
{
	economic_state_reset();
	player_checkpoint_ram_reset();
	player_restitution_ram_reset();
	artifact_restitution_ram_reset();
	player_restitution_ram_artifact_callbacks(nullptr, nullptr);
	auction_ram_reset();
	if (root && *root)
		return load_flat(root, error);
#ifndef __NO_MYSQL__
	return load_sql(connection);
#else
	(void)connection;
	return false;
#endif
}

catch (...)
{
	if (error)
		*error = "Could not load the complete economic state";
	return false;
}

bool economic_state_repository_refresh_artifacts(MYSQL *connection)
try
{
#ifdef __NO_MYSQL__
	(void)connection;
	return true;
#else
	artifact_restitution_ram_reset();
	// Old maintenance changes must advance the same canonical clock as logged
	// feeds. Otherwise the next RAM acceptance could never pass replay's CAS.
	const char *sync =
		"UPDATE artifact_domain_state s JOIN artifacts a ON a.vnum=s.vnum "
		"LEFT JOIN artifact_bind b ON b.vnum=s.vnum LEFT JOIN item_current_owner i ON i.item_uid=s.item_uid SET s.owned=(a.owned='Y'),"
		"s.loc_type=a.locType,s.location=a.location,s.timer_epoch=COALESCE(UNIX_TIMESTAMP(a.timer),0),"
		"s.artifact_type=a.type,s.bind_owner_pid=COALESCE(b.owner_pid,0),s.bind_timer_epoch=COALESCE(b.timer,0),s.item_revision=IF(s.item_uid IS NULL,s.item_revision,i.item_revision),s.revision=s.revision+1 "
		"WHERE s.owned<>(a.owned='Y') OR s.loc_type<>a.locType OR s.location<>a.location OR "
		"s.timer_epoch<>COALESCE(UNIX_TIMESTAMP(a.timer),0) OR s.artifact_type<>a.type OR "
		"s.bind_owner_pid<>COALESCE(b.owner_pid,0) OR s.bind_timer_epoch<>COALESCE(b.timer,0) OR (s.item_uid IS NOT NULL AND NOT(s.item_revision<=>i.item_revision))";
	const char *adopt =
		"INSERT IGNORE INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,bind_timer_epoch) "
		"SELECT a.vnum,a.owned='Y',a.locType,COALESCE(a.location,0),COALESCE(UNIX_TIMESTAMP(a.timer),0),a.type,COALESCE(b.owner_pid,0),COALESCE(b.timer,0) "
		"FROM artifacts a LEFT JOIN artifact_bind b ON b.vnum=a.vnum WHERE a.vnum>0";
	return connection && !mysql_real_query(connection, adopt, strlen(adopt)) &&
	       !mysql_real_query(connection, sync, strlen(sync)) &&
	       load_artifact_evidence(connection);
#endif
}
catch (...)
{
	artifact_restitution_ram_reset();
	return false;
}
