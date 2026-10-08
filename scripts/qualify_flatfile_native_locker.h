// Independent locker literals and custody comparison; no native storage calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_LOCKER_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_LOCKER_H

#include "qualify_flatfile_native_custody.h"

namespace restore_native_locker
{
using namespace restore_native_custody;
struct chest
{
	owner location;
	uint64_t locker_revision, revision;
	std::vector<item_literal> items;
};
struct locker_catalog
{
	uint32_t version;
	uint64_t revision;
	size_t lockers, access;
	std::vector<chest> chests;
};
inline std::string canonical_name(reader &in, size_t maximum)
{
	const auto value = text(in, maximum);
	need(!value.empty());
	for (auto c : value)
		need(c >= 0x21 && c <= 0x7e && (c < 'A' || c > 'Z'));
	return { reinterpret_cast<const char *>(value.data()), value.size() };
}
// Item spans borrow encoded. Policy names/passwords/sort bytes are never exported.
inline locker_catalog decode_locker(std::span<const uint8_t> encoded)
{
	const auto file = unwrap(encoded, "DURLOCK", catalog_limit, 2);
	reader in{ file.body };
	locker_catalog result{ file.version,
			       file.revision,
			       static_cast<size_t>(in.number(4)),
			       static_cast<size_t>(in.number(4)),
			       {} };
	need(result.lockers <= 65536 && result.access <= 1048576);
	std::set<uint64_t> chest_ids, uids;
	std::set<int32_t> player_owners, association_owners;
	std::set<std::pair<std::string, uint64_t>> account_owners;
	std::set<std::string> locker_names;
	uint64_t previous_locker = 0;
	for (size_t i = 0; i < result.lockers; ++i)
	{
		const auto id = in.number(4);
		const auto name = canonical_name(in, 100);
		const auto pid = signed32(in), association = signed32(in);
		const auto account_present = file.version == 2 ? in.number(1) : 0;
		need(account_present <= 1);
		std::string account;
		uint64_t account_side = 0;
		if (account_present)
		{
			account = canonical_name(in, 50);
			account_side = in.number(1);
			need(account_side <= 4 &&
			     account_owners.emplace(account, account_side).second);
		}
		const auto racewar = in.number(1);
		(void)in.number(1); // Native signed race; unconstrained in historical catalogs.
		const auto revision = in.number(8), count = in.number(4);
		need(id > previous_locker && revision && pid >= 0 && association >= 0 &&
		     (static_cast<unsigned>(pid > 0) + static_cast<unsigned>(association > 0) +
		      account_present) == 1 &&
		     locker_names.insert(name).second && count &&
		     count <= 262144 - result.chests.size());
		if (pid)
			need(player_owners.insert(pid).second);
		if (association)
			need(association_owners.insert(association).second);
		if (account_present)
			need(racewar == account_side &&
			     name == "account." + account + "." + std::to_string(account_side) +
					     ".locker");
		else
			need(!name.starts_with("account."));
		uint64_t previous_chest = 0;
		size_t public_count = 0;
		std::set<std::string> chest_names;
		for (size_t j = 0; j < count; ++j)
		{
			const auto chest_id = in.number(4);
			const auto chest_name = canonical_name(in, 32);
			const auto password = text(in, 64);
			const auto is_public = in.number(1);
			(void)text(in, 4096);
			const auto chest_revision = in.number(8), size = in.number(4);
			need(chest_id > previous_chest && chest_revision && is_public <= 1 &&
			     (!is_public || password.empty()) &&
			     chest_ids.insert(chest_id).second &&
			     chest_names.insert(chest_name).second && size &&
			     size <= 4 * 1024 * 1024);
			auto items = decode_items(in.take(size));
			for (const auto &row : items)
				need(row.uid && row.vnum > 0 && row.equipment == -1 &&
				     uids.insert(row.uid).second);
			result.chests.push_back(
				{ { 5, id, chest_id }, revision, chest_revision, std::move(items) });
			public_count += is_public;
			previous_chest = chest_id;
		}
		need(public_count == 1);
		previous_locker = id;
	}
	std::optional<std::pair<std::string, std::string>> previous_access;
	for (size_t i = 0; i < result.access; ++i)
	{
		const auto owner_name = canonical_name(in, 255), visitor = canonical_name(in, 255);
		const auto revision = in.number(8);
		const auto entry = std::make_pair(owner_name, visitor);
		need(revision && locker_names.contains(owner_name) &&
		     (!previous_access || *previous_access < entry));
		previous_access = entry;
	}
	in.done();
	return result;
}
struct finding
{
	const char *code;
	uint64_t uid;
};
struct result
{
	bool locker_present = false, custody_present = false;
	uint32_t locker_version = 0;
	uint64_t locker_revision = 0;
	size_t lockers = 0, chests = 0, access = 0, locker_items = 0, locker_coins = 0,
	       compared_items = 0, custody_locker_items = 0, other_custody_items = 0,
	       finding_count = 0;
	std::map<std::string, size_t> finding_counts;
	std::vector<finding> findings;
	bool valid() const { return !finding_count; }
	bool verified() const { return locker_present && custody_present && valid(); }
};
inline result audit(const std::filesystem::path &root, audit_budget &budget,
		    size_t detail_limit = 100)
{
	need(detail_limit <= 100);
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	const auto domains = root / "domains";
	const auto read = [&](const char *name)
	{
		struct stat info = {};
		if (lstat((domains / name).c_str(), &info) != 0)
		{
			need(errno == ENOENT);
			return bytes{};
		}
		need(lock.locked());
		auto encoded = file_bytes(domains, name, catalog_limit);
		need(!encoded.empty());
		return encoded;
	};
	const auto custody_bytes = read("item_ownership"), locker_bytes = read("locker_catalog");
	result output;
	output.custody_present = !custody_bytes.empty();
	output.locker_present = !locker_bytes.empty();
	restore_native_custody::catalog custody{};
	locker_catalog lockers{};
	if (output.custody_present)
		custody = decode_catalog(custody_bytes);
	if (output.locker_present)
		lockers = decode_locker(locker_bytes);
	output.locker_version = lockers.version;
	output.locker_revision = lockers.revision;
	output.lockers = lockers.lockers;
	output.chests = lockers.chests.size();
	output.access = lockers.access;
	const auto issue = [&](const char *code, uint64_t uid = 0)
	{
		++output.finding_count;
		++output.finding_counts[code];
		if (output.findings.size() < detail_limit)
			output.findings.push_back({ code, uid });
	};
	std::map<uint64_t, const item *> owned;
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		owned.emplace(row.uid, &row);
		if (row.state == 2)
			continue;
		if (row.location.type == 5)
			++output.custody_locker_items;
		else
			++output.other_custody_items;
	}
	std::set<uint64_t> seen;
	for (const auto &record : lockers.chests)
		for (size_t i = 0; i < record.items.size(); ++i)
		{
			audit_checkpoint();
			const auto &literal = record.items[i];
			++output.locker_items;
			seen.insert(literal.uid);
			if (literal.type == 20)
			{
				++output.locker_coins;
				if (std::any_of(literal.values.begin(), literal.values.begin() + 4,
						[](auto value) { return value < 0; }))
					issue("locker_negative_coin_value", literal.uid);
			}
			const auto found = owned.find(literal.uid);
			if (found == owned.end())
			{
				issue("locker_uid_unadmitted", literal.uid);
				continue;
			}
			++output.compared_items;
			const auto &row = *found->second;
			if (row.state != 1)
				issue("locker_uid_not_active", row.uid);
			if (row.location != record.location)
				issue("locker_owner_mismatch", row.uid);
			size_t root_index = i;
			while (record.items[root_index].parent >= 0)
				root_index = record.items[root_index].parent;
			const auto parent = literal.parent < 0 ? 0 :
								 record.items[literal.parent].uid;
			if (row.root != record.items[root_index].uid || row.parent != parent)
				issue("locker_item_topology_mismatch", row.uid);
			if (row.vnum != literal.vnum)
				issue("locker_item_vnum_mismatch", row.uid);
			if (row.equipment)
				issue("locker_item_equipment_mismatch", row.uid);
			if (!row.coin_payload.empty() &&
			    !same(row.coin_payload.subspan(8), literal.encoded.subspan(4)))
				issue("locker_coin_literal_mismatch", row.uid);
		}
	if (output.locker_items && !output.custody_present)
		issue("locker_custody_catalog_missing");
	if (output.custody_locker_items && !output.locker_present)
		issue("custody_locker_catalog_missing");
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		if (row.state != 2 && row.location.type == 5 && !seen.contains(row.uid))
			issue("locker_uid_missing_literal", row.uid);
	}
	lock.no_pending_player_domains();
	lock.finish();
	budget.checkpoint();
	return output;
}
} // namespace restore_native_locker
#endif
