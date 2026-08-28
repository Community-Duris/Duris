#include "flatfile_shop_trade_materialization.h"

#include "flatfile_store.h"
#include "player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'S', 'H', 'M', 'A', 'T' };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_events = 262144;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr const char *catalog_filename = "shop_trade_materializations";

struct materialization_event
{
	critical_operation_id operation_id = {};
	shop_trade_action action = shop_trade_action::unknown;
	uint32_t player_pid = 0;
	std::vector<uint8_t> item_blob;
};

struct materialization_catalog
{
	uint64_t revision = 0;
	std::vector<materialization_event> events;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		if (!valid)
			return;
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		try
		{
			for (size_t index = 0; index < sizeof(T); ++index)
			{
				bytes.push_back(static_cast<uint8_t>(bits & 0xff));
				bits >>= 8;
			}
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	void raw(const uint8_t *data, size_t size)
	{
		if (!valid || (!data && size) || bytes.size() > catalog_maximum_bytes ||
		    size > catalog_maximum_bytes - bytes.size())
		{
			valid = false;
			return;
		}
		try
		{
			bytes.insert(bytes.end(), data, data + size);
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}
};

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;

	template <typename T> bool number(T *value)
	{
		if (!value || offset > size || size - offset < sizeof(T))
			return false;
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<unsigned_type>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}

	bool raw(uint8_t *output, size_t count)
	{
		if (!output || offset > size || count > size - offset)
			return false;
		memcpy(output, data + offset, count);
		offset += count;
		return true;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool inbound(shop_trade_action action)
{
	return action == shop_trade_action::buy_existing ||
	       action == shop_trade_action::buy_produced;
}

bool valid_action(shop_trade_action action)
{
	return inbound(action) || action == shop_trade_action::sell_store ||
	       action == shop_trade_action::sell_destroy;
}

bool decode_items(const materialization_event &event, std::vector<player_item_snapshot> *items)
{
	return !event.item_blob.empty() &&
	       player_item_snapshot_list_decode(event.item_blob.data(), event.item_blob.size(),
						items) == player_snapshot_codec_result::ok &&
	       !items->empty() && items->size() <= SHOP_TRADE_MAX_ITEMS;
}

bool encode_catalog(const materialization_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || catalog.events.empty() ||
	    catalog.events.size() > catalog_maximum_events)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.events.size());
	for (const auto &event : catalog.events)
	{
		std::vector<player_item_snapshot> items;
		if (critical_operation_id_is_zero(event.operation_id) ||
		    !valid_action(event.action) || !event.player_pid || event.item_blob.empty() ||
		    event.item_blob.size() > SHOP_TRADE_ITEM_BLOB_MAX_BYTES ||
		    !decode_items(event, &items))
			return false;
		payload.raw(event.operation_id.bytes.data(), event.operation_id.bytes.size());
		payload.number<uint8_t>(static_cast<uint8_t>(event.action));
		payload.number(event.player_pid);
		payload.number<uint32_t>(event.item_blob.size());
		payload.raw(event.item_blob.data(), event.item_blob.size());
	}
	if (!payload.valid || payload.bytes.size() > catalog_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(catalog_magic.data(), catalog_magic.size());
	file.number(catalog_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > catalog_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, materialization_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + catalog_magic.size(), bytes.size() - catalog_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != catalog_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || !count || count > catalog_maximum_events)
		return false;
	materialization_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.events.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &event : decoded.events)
	{
		uint8_t action = 0;
		uint32_t blob_size = 0;
		if (!payload.raw(event.operation_id.bytes.data(),
				 event.operation_id.bytes.size()) ||
		    !payload.number(&action) || !payload.number(&event.player_pid) ||
		    !payload.number(&blob_size) || !blob_size ||
		    blob_size > SHOP_TRADE_ITEM_BLOB_MAX_BYTES || payload.offset > payload.size ||
		    blob_size > payload.size - payload.offset)
			return false;
		event.action = static_cast<shop_trade_action>(action);
		try
		{
			event.item_blob.resize(blob_size);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		std::vector<player_item_snapshot> items;
		if (!payload.raw(event.item_blob.data(), event.item_blob.size()) ||
		    critical_operation_id_is_zero(event.operation_id) || !event.player_pid ||
		    !valid_action(event.action) || !decode_items(event, &items))
			return false;
	}
	if (payload.offset != payload.size)
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_shop_trade_materialization_result
load_catalog(const std::string &root, materialization_catalog *catalog, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
	{
		*catalog = {};
		return flatfile_shop_trade_materialization_result::ok;
	}
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "shop trade materialization catalog is corrupt";
		return loaded == flatfile_read_result::io_error ?
			       flatfile_shop_trade_materialization_result::io_error :
			       flatfile_shop_trade_materialization_result::invalid;
	}
	return flatfile_shop_trade_materialization_result::ok;
}

bool payload_items_match(const shop_trade_payload &payload,
			 const std::vector<player_item_snapshot> &items)
{
	if (items.size() != payload.item_count ||
	    items.front().object_uid != payload.selected_item_uid)
		return false;
	std::unordered_set<uint64_t> payload_uids;
	try
	{
		payload_uids.reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
			payload_uids.insert(payload.items[index].item_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return payload_uids.size() == items.size() &&
	       std::all_of(items.begin(), items.end(), [&](const auto &item)
			   { return payload_uids.contains(item.object_uid); });
}

struct snapshot_node
{
	player_item_snapshot item;
	uint64_t parent_uid = 0;
};

bool normalize_items(const std::vector<player_item_snapshot> &original,
		     const std::vector<player_item_snapshot> &additions,
		     const std::unordered_set<uint64_t> &mentioned,
		     const std::unordered_map<uint64_t, flatfile_item_ownership_record> &owned,
		     std::vector<player_item_snapshot> *normalized)
{
	if (!normalized)
		return false;
	std::vector<snapshot_node> nodes;
	std::unordered_map<uint64_t, size_t> positions;
	try
	{
		nodes.reserve(original.size() + additions.size());
		positions.reserve(original.size() + additions.size());
		for (size_t index = 0; index < original.size(); ++index)
		{
			const auto &item = original[index];
			uint64_t parent_uid = 0;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return false;
				parent_uid =
					original[static_cast<size_t>(item.parent_index)].object_uid;
			}
			const auto owner = owned.find(item.object_uid);
			if (mentioned.contains(item.object_uid))
			{
				if (owner == owned.end())
					continue;
				parent_uid = owner->second.parent_item_uid;
			}
			if (!item.object_uid ||
			    !positions.emplace(item.object_uid, nodes.size()).second)
				return false;
			nodes.push_back({ item, parent_uid });
		}
		for (const auto &item : additions)
		{
			const auto owner = owned.find(item.object_uid);
			if (owner == owned.end() || owner->second.vnum != item.vnum ||
			    !positions.emplace(item.object_uid, nodes.size()).second)
				return false;
			nodes.push_back({ item, owner->second.parent_item_uid });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::vector<uint8_t> states;
	std::vector<size_t> order;
	try
	{
		states.resize(nodes.size());
		order.reserve(nodes.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	auto visit = [&](auto &&self, size_t index) -> bool
	{
		if (states[index] == 2)
			return true;
		if (states[index] == 1)
			return false;
		states[index] = 1;
		if (nodes[index].parent_uid)
		{
			const auto parent = positions.find(nodes[index].parent_uid);
			if (parent == positions.end() || !self(self, parent->second))
				return false;
		}
		states[index] = 2;
		order.push_back(index);
		return true;
	};
	for (size_t index = 0; index < nodes.size(); ++index)
		if (!visit(visit, index))
			return false;
	std::vector<player_item_snapshot> result;
	std::unordered_map<uint64_t, int32_t> new_positions;
	try
	{
		result.reserve(nodes.size());
		new_positions.reserve(nodes.size());
		for (size_t index : order)
		{
			auto item = nodes[index].item;
			if (nodes[index].parent_uid)
			{
				const auto parent = new_positions.find(nodes[index].parent_uid);
				if (parent == new_positions.end())
					return false;
				item.parent_index = parent->second;
			}
			else
				item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			new_positions.emplace(item.object_uid, static_cast<int32_t>(result.size()));
			result.push_back(std::move(item));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*normalized = std::move(result);
	return true;
}
} // namespace

flatfile_shop_trade_materialization_result flatfile_shop_trade_materialization_prepare(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation_id, const shop_trade_payload &payload,
	flatfile_shop_trade_materialization_mutation *mutation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || critical_operation_id_is_zero(operation_id) ||
	    !mutation || !payload.player_pid || !valid_action(payload.action) ||
	    !payload.item_blob_size || payload.item_blob_size > payload.item_blob.size())
		return flatfile_shop_trade_materialization_result::invalid;
	materialization_event event;
	event.operation_id = operation_id;
	event.action = payload.action;
	event.player_pid = payload.player_pid;
	try
	{
		event.item_blob.assign(payload.item_blob.begin(),
				       payload.item_blob.begin() + payload.item_blob_size);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shop_trade_materialization_result::io_error;
	}
	std::vector<player_item_snapshot> items;
	if (!decode_items(event, &items) || !payload_items_match(payload, items))
		return flatfile_shop_trade_materialization_result::invalid;
	materialization_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_shop_trade_materialization_result::ok)
		return loaded;
	if (std::any_of(
		    catalog.events.begin(), catalog.events.end(), [&](const auto &existing)
		    { return critical_operation_id_equal(existing.operation_id, operation_id); }) ||
	    catalog.events.size() >= catalog_maximum_events ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_shop_trade_materialization_result::invalid;
	try
	{
		catalog.events.push_back(std::move(event));
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shop_trade_materialization_result::io_error;
	}
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_shop_trade_materialization_result::invalid;
	mutation->after_image = { catalog_filename, std::move(bytes) };
	return flatfile_shop_trade_materialization_result::ok;
}

flatfile_shop_trade_materialization_result flatfile_shop_trade_materialization_reconcile(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t player_pid,
	const std::vector<flatfile_item_ownership_record> &owned, player_snapshot *snapshot,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !player_pid || !snapshot ||
	    snapshot->pid != static_cast<int32_t>(player_pid))
		return flatfile_shop_trade_materialization_result::invalid;
	materialization_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_shop_trade_materialization_result::ok || catalog.events.empty())
		return loaded;
	std::unordered_map<uint64_t, flatfile_item_ownership_record> owner_records;
	std::unordered_set<uint64_t> mentioned;
	std::unordered_map<uint64_t, player_item_snapshot> latest_inbound;
	std::unordered_set<uint64_t> existing;
	try
	{
		owner_records.reserve(owned.size());
		for (const auto &record : owned)
			if (!owner_records.emplace(record.item_uid, record).second)
				return flatfile_shop_trade_materialization_result::invalid;
		for (const auto &event : catalog.events)
		{
			if (event.player_pid != player_pid)
				continue;
			std::vector<player_item_snapshot> items;
			if (!decode_items(event, &items))
				return flatfile_shop_trade_materialization_result::invalid;
			for (const auto &item : items)
			{
				mentioned.insert(item.object_uid);
				if (inbound(event.action))
					latest_inbound[item.object_uid] = item;
			}
		}
		if (mentioned.empty())
			return flatfile_shop_trade_materialization_result::ok;
		for (const auto &item : snapshot->items)
			if (!existing.insert(item.object_uid).second)
				return flatfile_shop_trade_materialization_result::invalid;
		for (const auto &pet : snapshot->pets)
			for (const auto &item : pet.items)
				if (!existing.insert(item.object_uid).second)
					return flatfile_shop_trade_materialization_result::invalid;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shop_trade_materialization_result::io_error;
	}
	std::vector<player_item_snapshot> additions;
	try
	{
		for (uint64_t uid : mentioned)
		{
			if (owner_records.contains(uid) && !existing.contains(uid))
			{
				const auto source = latest_inbound.find(uid);
				if (source == latest_inbound.end())
					return flatfile_shop_trade_materialization_result::invalid;
				additions.push_back(source->second);
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shop_trade_materialization_result::io_error;
	}
	player_snapshot reconciled;
	try
	{
		reconciled = *snapshot;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shop_trade_materialization_result::io_error;
	}
	if (!normalize_items(snapshot->items, additions, mentioned, owner_records,
			     &reconciled.items))
		return flatfile_shop_trade_materialization_result::invalid;
	for (size_t index = 0; index < snapshot->pets.size(); ++index)
		if (!normalize_items(snapshot->pets[index].items, {}, mentioned, owner_records,
				     &reconciled.pets[index].items))
			return flatfile_shop_trade_materialization_result::invalid;
	size_t total_items = reconciled.items.size();
	if (total_items > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return flatfile_shop_trade_materialization_result::invalid;
	for (const auto &pet : reconciled.pets)
	{
		if (pet.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - total_items)
			return flatfile_shop_trade_materialization_result::invalid;
		total_items += pet.items.size();
	}
	*snapshot = std::move(reconciled);
	return flatfile_shop_trade_materialization_result::ok;
}

flatfile_shop_trade_materialization_result
flatfile_shop_trade_materialization_prepare_player_remove(const std::string &root,
							  const flatfile_authority_lock &lock,
							  uint32_t player_pid,
							  flatfile_authority_operation *operation,
							  std::string *error)
{
	if (root.empty() || !lock.matches(root) || !player_pid || !operation)
		return flatfile_shop_trade_materialization_result::invalid;
	*operation = {};
	materialization_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_shop_trade_materialization_result::ok)
		return loaded;
	const size_t original_size = catalog.events.size();
	catalog.events.erase(std::remove_if(catalog.events.begin(), catalog.events.end(),
					    [&](const auto &event)
					    { return event.player_pid == player_pid; }),
			     catalog.events.end());
	if (catalog.events.size() == original_size)
		return flatfile_shop_trade_materialization_result::unchanged;
	operation->store = flatfile_authority_store::domains;
	operation->filename = catalog_filename;
	if (catalog.events.empty())
	{
		operation->kind = flatfile_authority_operation_kind::remove;
		return flatfile_shop_trade_materialization_result::ok;
	}
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_shop_trade_materialization_result::invalid;
	++catalog.revision;
	if (!encode_catalog(catalog, &operation->bytes))
		return flatfile_shop_trade_materialization_result::invalid;
	return flatfile_shop_trade_materialization_result::ok;
}
