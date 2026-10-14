#include "persistence/economic_outcome.h"
#include "persistence/critical_command_journal.h"

#include <algorithm>
#include <limits>

namespace
{
constexpr size_t outcome_limit = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;

template <typename T> void put(std::vector<uint8_t> &out, T value)
{
	for (size_t i = 0; i < sizeof(T); ++i)
		out.push_back(static_cast<uint8_t>(value >> (i * 8)));
}

struct reader
{
	std::span<const uint8_t> bytes;
	size_t at = 0;
	template <typename T> bool get(T &value)
	{
		if (at > bytes.size() || sizeof(T) > bytes.size() - at)
			return false;
		value = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			value |= static_cast<T>(bytes[at++]) << (i * 8);
		return true;
	}
	bool blob(std::vector<uint8_t> &value)
	{
		uint32_t length = 0;
		if (!get(length) || at > bytes.size() || length > bytes.size() - at)
			return false;
		value.assign(bytes.begin() + at, bytes.begin() + at + length);
		at += length;
		return true;
	}
};

bool append_blob(std::vector<uint8_t> &out, std::span<const uint8_t> value)
{
	if (out.size() > outcome_limit || sizeof(uint32_t) > outcome_limit - out.size() ||
	    value.size() > outcome_limit - out.size() - sizeof(uint32_t) ||
	    value.size() > std::numeric_limits<uint32_t>::max())
		return false;
	put<uint32_t>(out, static_cast<uint32_t>(value.size()));
	out.insert(out.end(), value.begin(), value.end());
	return true;
}

void put_owner(std::vector<uint8_t> &out, const item_owner_identity &owner)
{
	put<uint8_t>(out, static_cast<uint8_t>(owner.type));
	put<uint64_t>(out, owner.id);
	put<uint64_t>(out, owner.context_id);
}

bool get_owner(reader &in, item_owner_identity &owner)
{
	uint8_t type = 0;
	if (!in.get(type) || type > static_cast<uint8_t>(item_owner_type::native_mobile) ||
	    !in.get(owner.id) || !in.get(owner.context_id))
		return false;
	owner.type = static_cast<item_owner_type>(type);
	return owner.type == item_owner_type::unknown ? !owner.id && !owner.context_id :
							item_owner_identity_valid(owner);
}

bool valid(const economic_outcome_record &record)
{
	const auto domain = static_cast<uint16_t>(record.domain);
	if (record.version != 1 || !record.sequence || !domain ||
	    domain > static_cast<uint16_t>(economic_outcome_domain::escrow) ||
	    !record.result_version || record.result.size() > outcome_limit ||
	    record.effects.empty() ||
	    record.effects.size() > CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS ||
	    !critical_command_envelope_valid(record.command) ||
	    critical_operation_id_is_zero(record.command.operation_id) ||
	    !critical_operation_id_equal(record.command.operation_id,
					 record.accounting.metadata.operation_id) ||
	    economic_plan_validate_structure(record.accounting) != economic_accounting_error::ok)
		return false;
	for (size_t i = 0; i < record.effects.size(); ++i)
	{
		const auto &effect = record.effects[i];
		const auto type = static_cast<uint8_t>(effect.key.type);
		if (!type || type > static_cast<uint8_t>(critical_entity_type::native_mobile) ||
		    !effect.key.id || !effect.codec_version ||
		    effect.after_revision <= effect.before_revision ||
		    effect.before.size() > outcome_limit || effect.after.size() > outcome_limit ||
		    (effect.before_presence != economic_outcome_presence::present &&
		     effect.before_presence != economic_outcome_presence::absent) ||
		    (effect.before_presence == economic_outcome_presence::present) !=
			    !effect.before.empty() ||
		    (effect.after_presence != economic_outcome_presence::present &&
		     effect.after_presence != economic_outcome_presence::retired) ||
		    (effect.after_presence == economic_outcome_presence::present) !=
			    !effect.after.empty() ||
		    (i && !critical_entity_key_less(record.effects[i - 1].key, effect.key)) ||
		    std::none_of(record.command.keys.begin(), record.command.keys.end(),
				 [&](const critical_entity_key &key)
				 { return critical_entity_key_equal(key, effect.key); }))
			return false;
		for (const auto &owner : { effect.owner_before, effect.owner_after })
			if (owner.type == item_owner_type::unknown ?
				    owner.id || owner.context_id :
				    !item_owner_identity_valid(owner))
				return false;
	}
	return true;
}
} // namespace

bool economic_outcome_encode(const economic_outcome_record &record,
			     std::vector<uint8_t> *output) noexcept
try
{
	if (!output || !valid(record))
		return false;
	std::vector<uint8_t> command, plan;
	if (critical_command_encode(record.command, &command) !=
		    critical_command_codec_result::ok ||
	    economic_plan_encode(record.accounting, &plan) != economic_accounting_error::ok)
		return false;
	std::vector<uint8_t> result{ 'E', 'O', 'R', '1' };
	put<uint16_t>(result, record.version);
	put<uint16_t>(result, static_cast<uint16_t>(record.domain));
	put<uint64_t>(result, record.sequence);
	put<uint64_t>(result, record.logical_tick);
	put<uint16_t>(result, record.result_version);
	if (!append_blob(result, command) || !append_blob(result, plan) ||
	    !append_blob(result, record.result))
		return false;
	put<uint32_t>(result, static_cast<uint32_t>(record.effects.size()));
	for (const auto &effect : record.effects)
	{
		// Fixed fields need their own prospective bound before insertion.
		constexpr size_t fixed = 1 + 8 + 2 + 8 + 8 + 2 + 2 * (1 + 8 + 8);
		if (result.size() > outcome_limit || fixed > outcome_limit - result.size())
			return false;
		put<uint8_t>(result, static_cast<uint8_t>(effect.key.type));
		put<uint64_t>(result, effect.key.id);
		put<uint16_t>(result, effect.codec_version);
		put<uint64_t>(result, effect.before_revision);
		put<uint64_t>(result, effect.after_revision);
		put<uint8_t>(result, static_cast<uint8_t>(effect.before_presence));
		put<uint8_t>(result, static_cast<uint8_t>(effect.after_presence));
		put_owner(result, effect.owner_before);
		put_owner(result, effect.owner_after);
		if (!append_blob(result, effect.before) || !append_blob(result, effect.after))
			return false;
	}
	if (result.size() > outcome_limit)
		return false;
	output->swap(result);
	return true;
}
catch (...)
{
	return false;
}

bool economic_outcome_decode(std::span<const uint8_t> bytes,
			     economic_outcome_record *output) noexcept
try
{
	if (!output || bytes.size() > outcome_limit || bytes.size() < 4 ||
	    !std::equal(bytes.begin(), bytes.begin() + 4, "EOR1"))
		return false;
	reader in{ bytes, 4 };
	economic_outcome_record record;
	uint16_t domain = 0;
	std::vector<uint8_t> command, plan;
	uint32_t count = 0;
	if (!in.get(record.version) || !in.get(domain) || !in.get(record.sequence) ||
	    !in.get(record.logical_tick) || !in.get(record.result_version) || !in.blob(command) ||
	    !in.blob(plan) || !in.blob(record.result) || !in.get(count) ||
	    count > CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS ||
	    critical_command_decode(command.data(), command.size(), &record.command) !=
		    critical_command_codec_result::ok ||
	    economic_plan_decode(plan, &record.accounting) != economic_accounting_error::ok)
		return false;
	record.domain = static_cast<economic_outcome_domain>(domain);
	record.effects.reserve(count);
	for (uint32_t n = 0; n < count; ++n)
	{
		economic_outcome_effect effect;
		uint8_t type = 0, presence = 0, before_presence = 0;
		if (!in.get(type) || !in.get(effect.key.id) || !in.get(effect.codec_version) ||
		    !in.get(effect.before_revision) || !in.get(effect.after_revision) ||
		    !in.get(before_presence) || !in.get(presence) ||
		    !get_owner(in, effect.owner_before) || !get_owner(in, effect.owner_after) ||
		    !in.blob(effect.before) || !in.blob(effect.after))
			return false;
		effect.key.type = static_cast<critical_entity_type>(type);
		effect.after_presence = static_cast<economic_outcome_presence>(presence);
		effect.before_presence = static_cast<economic_outcome_presence>(before_presence);
		record.effects.push_back(std::move(effect));
	}
	if (in.at != bytes.size() || !valid(record))
		return false;
	std::vector<uint8_t> canonical;
	if (!economic_outcome_encode(record, &canonical) || canonical.size() != bytes.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = std::move(record);
	return true;
}
catch (...)
{
	return false;
}

namespace
{
bool outcome_storage_add(size_t *total, size_t bytes) noexcept
{
	if (bytes > SIZE_MAX - *total)
		return false;
	*total += bytes;
	return true;
}

template <typename T>
bool outcome_storage_vector(size_t *total, const std::vector<T> &values) noexcept
{
	return values.capacity() <= SIZE_MAX / sizeof(T) &&
	       outcome_storage_add(total, values.capacity() * sizeof(T));
}
} // namespace

bool economic_outcome_record_storage_bytes(const economic_outcome_record &record,
					   size_t *output) noexcept
{
	if (!output)
		return false;
	size_t total = sizeof(record);
	size_t command_heap = 0;
	if (!critical_command_current_heap_bytes(record.command, &command_heap) ||
	    !outcome_storage_add(&total, command_heap) ||
	    !outcome_storage_vector(&total, record.accounting.accounts) ||
	    !outcome_storage_vector(&total, record.accounting.postings) ||
	    !outcome_storage_vector(&total, record.accounting.children) ||
	    !outcome_storage_vector(&total, record.accounting.items_before) ||
	    !outcome_storage_vector(&total, record.accounting.items_after) ||
	    !outcome_storage_vector(&total, record.accounting.item_events) ||
	    !outcome_storage_vector(&total, record.result) ||
	    !outcome_storage_vector(&total, record.effects))
		return false;
	for (const auto &effect : record.effects)
	{
		if (!outcome_storage_vector(&total, effect.before) ||
		    !outcome_storage_vector(&total, effect.after))
			return false;
	}
	*output = total;
	return true;
}
