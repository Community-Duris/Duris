// Independent shopkeeper literals and custody comparison; no native storage calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_SHOPKEEPER_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_SHOPKEEPER_H

#include "qualify_flatfile_native_custody.h"

namespace restore_native_shopkeeper
{
using namespace restore_native_custody;
struct keeper
{
	owner location;
	int32_t mobile, room;
	int64_t saved_at, cash;
	uint64_t revision;
	bool roaming;
	std::span<const uint8_t> affects;
	std::vector<item_literal> items;
};
struct keeper_catalog
{
	uint32_t version;
	uint64_t revision;
	std::vector<keeper> records;
};
// Item/affect spans borrow encoded. Complete native fields are validated.
inline keeper_catalog decode_shopkeeper(std::span<const uint8_t> encoded)
{
	const auto file = unwrap(encoded, "DURSHOP", 256 * 1024 * 1024, 2);
	reader in{ file.body };
	keeper_catalog result{ file.version, file.revision, {} };
	const auto count = in.number(4);
	need(count <= 262144);
	std::set<uint64_t> uids;
	uint64_t previous_id = 0;
	for (size_t i = 0; i < count; ++i)
	{
		const auto id = in.number(4);
		const auto mobile = signed32(in), room = signed32(in);
		const auto saved_at = std::bit_cast<int64_t>(in.number(8));
		const auto revision = in.number(8);
		const auto cash = file.version == 2 ? std::bit_cast<int64_t>(in.number(8)) : -1;
		const auto roaming = file.version == 2 ? in.number(1) : 0;
		const auto affect_count = in.number(4);
		need((i == 0 || id > previous_id) && mobile > 0 && room > 0 && saved_at >= 0 &&
		     revision && cash >= -1 && cash <= INT32_MAX && roaming <= 1 &&
		     affect_count <= 4096);
		using affect_key =
			std::tuple<int32_t, int32_t, int32_t, int32_t, std::array<uint64_t, 5>>;
		std::optional<affect_key> previous_affect;
		const auto begin = in.offset;
		for (size_t j = 0; j < affect_count; ++j)
		{
			audit_checkpoint();
			const auto type = signed32(in), duration = signed32(in),
				   modifier = signed32(in), location = signed32(in);
			std::array<uint64_t, 5> bits{};
			for (auto &value : bits)
				value = in.number(8);
			const affect_key key{ type, location, modifier, duration, bits };
			need(!previous_affect || *previous_affect <= key);
			previous_affect = key;
		}
		const auto affects = file.body.subspan(begin, in.offset - begin);
		const auto size = in.number(4);
		need(size && size <= 4 * 1024 * 1024);
		auto items = decode_items(in.take(size));
		std::set<int16_t> slots;
		for (const auto &row : items)
			need(row.uid && row.vnum > 0 && row.equipment >= 0 &&
			     row.equipment <= 255 && (row.parent < 0 || !row.equipment) &&
			     (!row.equipment || slots.insert(row.equipment).second) &&
			     uids.insert(row.uid).second);
		// Native owner transform promotes before adding; shop ID zero is valid.
		result.records.push_back({ { 9, id + 1, 0 },
					   mobile,
					   room,
					   saved_at,
					   cash,
					   revision,
					   roaming != 0,
					   affects,
					   std::move(items) });
		previous_id = id;
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
	bool shop_present = false, custody_present = false;
	uint32_t shop_version = 0;
	uint64_t shop_revision = 0, retained_cash = 0;
	size_t keepers = 0, affects = 0, cash_known = 0, cash_legacy = 0, shop_items = 0,
	       shop_coins = 0, compared_items = 0, custody_shop_items = 0, other_custody_items = 0,
	       finding_count = 0;
	std::map<std::string, size_t> finding_counts;
	std::vector<finding> findings;
	bool valid() const { return !finding_count; }
	bool verified() const { return shop_present && custody_present && valid(); }
};
inline result audit(const std::filesystem::path &root, audit_budget &budget,
		    size_t detail_limit = 100)
{
	need(detail_limit <= 100);
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	const auto domains = root / "domains";
	const auto read = [&](const char *name, size_t maximum)
	{
		struct stat info = {};
		if (lstat((domains / name).c_str(), &info) != 0)
		{
			need(errno == ENOENT);
			return bytes{};
		}
		need(lock.locked());
		auto encoded = file_bytes(domains, name, maximum);
		need(!encoded.empty());
		return encoded;
	};
	const auto custody_bytes = read("item_ownership", catalog_limit);
	const auto shop_bytes = read("shopkeeper_catalog", 256 * 1024 * 1024);
	result output;
	output.custody_present = !custody_bytes.empty();
	output.shop_present = !shop_bytes.empty();
	restore_native_custody::catalog custody{};
	keeper_catalog shops{};
	if (output.custody_present)
		custody = decode_catalog(custody_bytes);
	if (output.shop_present)
		shops = decode_shopkeeper(shop_bytes);
	output.shop_version = shops.version;
	output.shop_revision = shops.revision;
	output.keepers = shops.records.size();
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
		if (row.location.type == 9)
			++output.custody_shop_items;
		else
			++output.other_custody_items;
	}
	std::set<uint64_t> seen;
	for (const auto &record : shops.records)
	{
		output.affects += record.affects.size() / 56;
		if (record.cash < 0)
			++output.cash_legacy;
		else
		{
			++output.cash_known;
			output.retained_cash += record.cash;
		}
		for (size_t i = 0; i < record.items.size(); ++i)
		{
			audit_checkpoint();
			const auto &literal = record.items[i];
			++output.shop_items;
			seen.insert(literal.uid);
			if (literal.type == 20)
			{
				++output.shop_coins;
				if (std::any_of(literal.values.begin(), literal.values.begin() + 4,
						[](auto value) { return value < 0; }))
					issue("shop_negative_coin_value", literal.uid);
			}
			const auto found = owned.find(literal.uid);
			if (found == owned.end())
			{
				issue("shop_uid_unadmitted", literal.uid);
				continue;
			}
			++output.compared_items;
			const auto &row = *found->second;
			if (row.state != 1)
				issue("shop_uid_not_active", row.uid);
			if (row.location != record.location)
				issue("shop_owner_mismatch", row.uid);
			size_t root_index = i;
			while (record.items[root_index].parent >= 0)
				root_index = record.items[root_index].parent;
			const auto parent = literal.parent < 0 ? 0 :
								 record.items[literal.parent].uid;
			if (row.root != record.items[root_index].uid || row.parent != parent)
				issue("shop_item_topology_mismatch", row.uid);
			if (row.vnum != literal.vnum)
				issue("shop_item_vnum_mismatch", row.uid);
			// Native keeper slots differ from this custody field's owner policy.
			if (row.equipment)
				issue("shop_item_equipment_mismatch", row.uid);
			if (!row.coin_payload.empty() &&
			    !same(row.coin_payload.subspan(8), literal.encoded.subspan(4)))
				issue("shop_coin_literal_mismatch", row.uid);
		}
	}
	if (output.shop_items && !output.custody_present)
		issue("shop_custody_catalog_missing");
	if (output.custody_shop_items && !output.shop_present)
		issue("custody_shop_catalog_missing");
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		if (row.state != 2 && row.location.type == 9 && !seen.contains(row.uid))
			issue("shop_uid_missing_literal", row.uid);
	}
	lock.no_pending_player_domains();
	lock.finish();
	budget.checkpoint();
	return output;
}
} // namespace restore_native_shopkeeper
#endif
