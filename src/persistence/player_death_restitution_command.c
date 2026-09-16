#include "persistence/player_death_restitution_command.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <ctime>
#include <limits>
#include <new>
#include <openssl/sha.h>
#include <type_traits>
#include <utility>

namespace
{
constexpr uint32_t ITEM_STATE_MAGIC = 0x31545349; // IST1, little endian
constexpr uint16_t ITEM_STATE_VERSION = 1;
constexpr uint32_t PLAN_MAGIC = 0x31524450; // PDR1, little endian
constexpr uint32_t RESULT_MAGIC = 0x31535252; // RRS1, little endian
constexpr uint16_t RESULT_VERSION = 1;
constexpr size_t MAX_ACTOR_BYTES = 128;
constexpr size_t MAX_ITEM_AFFECTS = 128;
constexpr size_t MAX_ITEM_DESCRIPTIONS = 128;
constexpr size_t MAX_ITEM_TEXT_BYTES = 4096;
constexpr size_t MAX_PLAN_BYTES = 350 * 1024;
constexpr uint8_t ARTIFACT_EVIDENCE = 1;
constexpr uint8_t ARTIFACT_UID_APPROVAL = 2;
constexpr uint8_t ARTIFACT_DOMAIN_PRESENT = 4;
constexpr uint8_t ARTIFACT_DOMAIN_UID_PRESENT = 8;
constexpr uint8_t ARTIFACT_BASELINE_PRESENT = 16;
constexpr uint8_t ARTIFACT_BIND_PRESENT = 32;
constexpr uint8_t ARTIFACT_LEGACY_MASK = PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_GOD |
					 PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL;

bool append_bytes(std::vector<uint8_t> &output, const void *data, size_t size)
{
	if (!size)
		return true;
	if (!data)
		return false;
	const uint8_t *bytes = static_cast<const uint8_t *>(data);
	output.insert(output.end(), bytes, bytes + size);
	return true;
}

template <typename T> void append_le(std::vector<uint8_t> &output, T value)
{
	using unsigned_type = typename std::make_unsigned<T>::type;
	const unsigned_type encoded = static_cast<unsigned_type>(value);
	for (size_t index = 0; index < sizeof(T); ++index)
		output.push_back(static_cast<uint8_t>(encoded >> (index * 8)));
}

template <typename T> bool read_le(const uint8_t *input, size_t size, size_t *offset, T *value)
{
	if (!input || !offset || !value || *offset > size || sizeof(T) > size - *offset)
		return false;
	using unsigned_type = typename std::make_unsigned<T>::type;
	unsigned_type decoded = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		decoded |= static_cast<unsigned_type>(input[*offset + index]) << (index * 8);
	*offset += sizeof(T);
	*value = static_cast<T>(decoded);
	return true;
}

bool read_bytes(const uint8_t *input, size_t size, size_t *offset, size_t count,
		std::vector<uint8_t> *output)
{
	if (!input || !offset || !output || *offset > size || count > size - *offset)
		return false;
	try
	{
		output->assign(input + *offset, input + *offset + count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*offset += count;
	return true;
}

bool digest_nonzero(const std::array<uint8_t, 32> &digest)
{
	return std::any_of(digest.begin(), digest.end(), [](uint8_t value) { return value != 0; });
}

bool digest_matches(const std::array<uint8_t, 32> &expected, const std::vector<uint8_t> &payload)
{
	if (!digest_nonzero(expected))
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> actual = {};
	SHA256(payload.data(), payload.size(), actual.data());
	return actual == expected;
}

bool validate_state(const player_death_restitution_item_state &state)
{
	if (!state.item_uid || !state.vnum || !state.quantity ||
	    state.affects.size() > MAX_ITEM_AFFECTS ||
	    state.extra_descriptions.size() > MAX_ITEM_DESCRIPTIONS)
		return false;
	for (size_t index = 0; index < state.strings.size(); ++index)
		if (state.strings[index].size() > MAX_ITEM_TEXT_BYTES ||
		    (!state.string_present[index] && !state.strings[index].empty()))
			return false;
	for (const auto &description : state.extra_descriptions)
	{
		if (description.keyword.empty() || description.keyword.size() > 255 ||
		    description.description.size() > MAX_ITEM_TEXT_BYTES)
			return false;
	}
	return true;
}

bool encode_state_internal(const player_death_restitution_item_state &state,
			   std::vector<uint8_t> *encoded)
{
	if (!encoded || !validate_state(state))
		return false;
	try
	{
		encoded->clear();
		encoded->reserve(256);
		append_le<uint32_t>(*encoded, ITEM_STATE_MAGIC);
		append_le<uint16_t>(*encoded, ITEM_STATE_VERSION);
		uint16_t flags = 0;
		for (size_t index = 0; index < state.strings.size(); ++index)
			if (state.string_present[index])
				flags |= static_cast<uint16_t>(1U << index);
		for (size_t index = 0; index < state.bitvector_present.size(); ++index)
			if (state.bitvector_present[index])
				flags |= static_cast<uint16_t>(1U << (index + 4));
		append_le<uint16_t>(*encoded, flags);
		append_le<uint64_t>(*encoded, state.item_uid);
		append_le<uint32_t>(*encoded, state.vnum);
		append_le<int16_t>(*encoded, state.equip_slot);
		append_le<uint16_t>(*encoded, state.quantity);
		append_le<int32_t>(*encoded, state.weight);
		append_le<int32_t>(*encoded, state.cost);
		append_le<int32_t>(*encoded, state.timer);
		append_le<uint64_t>(*encoded, state.extra_flags);
		append_le<int32_t>(*encoded, state.wear_flags);
		append_le<int8_t>(*encoded, state.item_type);
		for (int32_t value : state.values)
			append_le<int32_t>(*encoded, value);
		append_le<int8_t>(*encoded, state.material);
		append_le<int16_t>(*encoded, state.condition);
		for (const auto &text : state.strings)
		{
			if (text.size() > UINT16_MAX)
				return false;
			append_le<uint16_t>(*encoded, static_cast<uint16_t>(text.size()));
			append_bytes(*encoded, text.data(), text.size());
		}
		for (uint64_t bitvector : state.bitvectors)
			append_le<uint64_t>(*encoded, bitvector);
		append_le<uint16_t>(*encoded, static_cast<uint16_t>(state.affects.size()));
		append_le<uint16_t>(*encoded,
				    static_cast<uint16_t>(state.extra_descriptions.size()));
		for (const auto &affect : state.affects)
		{
			append_le<int16_t>(*encoded, affect.location);
			append_le<int16_t>(*encoded, affect.modifier);
		}
		for (const auto &description : state.extra_descriptions)
		{
			append_le<uint16_t>(*encoded,
					    static_cast<uint16_t>(description.keyword.size()));
			append_le<uint32_t>(*encoded,
					    static_cast<uint32_t>(description.description.size()));
			append_bytes(*encoded, description.keyword.data(),
				     description.keyword.size());
			append_bytes(*encoded, description.description.data(),
				     description.description.size());
		}
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	return encoded->size() <= PLAYER_DEATH_RESTITUTION_MAX_ITEM_STATE_BYTES;
}

bool decode_state_internal(const uint8_t *encoded, size_t encoded_size,
			   player_death_restitution_item_state *state)
{
	if (!encoded || !state || encoded_size < 8 ||
	    encoded_size > PLAYER_DEATH_RESTITUTION_MAX_ITEM_STATE_BYTES)
		return false;
	size_t offset = 0;
	uint32_t magic = 0;
	uint16_t version = 0, flags = 0;
	if (!read_le(encoded, encoded_size, &offset, &magic) ||
	    !read_le(encoded, encoded_size, &offset, &version) ||
	    !read_le(encoded, encoded_size, &offset, &flags) || magic != ITEM_STATE_MAGIC ||
	    version != ITEM_STATE_VERSION || (flags & 0xfe00U))
		return false;
	player_death_restitution_item_state decoded = {};
	if (!read_le(encoded, encoded_size, &offset, &decoded.item_uid) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.vnum) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.equip_slot) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.quantity) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.weight) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.cost) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.timer) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.extra_flags) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.wear_flags) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.item_type))
		return false;
	for (int32_t &value : decoded.values)
		if (!read_le(encoded, encoded_size, &offset, &value))
			return false;
	if (!read_le(encoded, encoded_size, &offset, &decoded.material) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.condition))
		return false;
	for (size_t index = 0; index < decoded.strings.size(); ++index)
	{
		uint16_t length = 0;
		if (!read_le(encoded, encoded_size, &offset, &length) ||
		    length > MAX_ITEM_TEXT_BYTES ||
		    !read_bytes(encoded, encoded_size, &offset, length, &decoded.strings[index]))
			return false;
		decoded.string_present[index] = (flags & (1U << index)) != 0;
		if (!decoded.string_present[index] && length)
			return false;
	}
	for (size_t index = 0; index < decoded.bitvectors.size(); ++index)
	{
		if (!read_le(encoded, encoded_size, &offset, &decoded.bitvectors[index]))
			return false;
		decoded.bitvector_present[index] = (flags & (1U << (index + 4))) != 0;
	}
	uint16_t affect_count = 0, description_count = 0;
	if (!read_le(encoded, encoded_size, &offset, &affect_count) ||
	    !read_le(encoded, encoded_size, &offset, &description_count) ||
	    affect_count > MAX_ITEM_AFFECTS || description_count > MAX_ITEM_DESCRIPTIONS)
		return false;
	try
	{
		decoded.affects.reserve(affect_count);
		decoded.extra_descriptions.reserve(description_count);
		for (uint16_t index = 0; index < affect_count; ++index)
		{
			player_death_restitution_item_affect affect = {};
			if (!read_le(encoded, encoded_size, &offset, &affect.location) ||
			    !read_le(encoded, encoded_size, &offset, &affect.modifier))
				return false;
			decoded.affects.push_back(affect);
		}
		for (uint16_t index = 0; index < description_count; ++index)
		{
			uint16_t keyword_length = 0;
			uint32_t description_length = 0;
			player_death_restitution_item_extra_description description;
			if (!read_le(encoded, encoded_size, &offset, &keyword_length) ||
			    !read_le(encoded, encoded_size, &offset, &description_length) ||
			    !keyword_length || keyword_length > 255 ||
			    description_length > MAX_ITEM_TEXT_BYTES ||
			    !read_bytes(encoded, encoded_size, &offset, keyword_length,
					&description.keyword) ||
			    !read_bytes(encoded, encoded_size, &offset, description_length,
					&description.description))
				return false;
			decoded.extra_descriptions.push_back(std::move(description));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (offset != encoded_size || !validate_state(decoded))
		return false;
	*state = std::move(decoded);
	return true;
}

bool plan_item_valid(const player_death_restitution_plan &plan,
		     const player_death_restitution_item &item, const std::vector<uint64_t> &seen,
		     const std::vector<uint64_t> &prior_deliveries)
{
	if (!item.item_uid || item.classification.empty() ||
	    item.classification.size() > PLAYER_DEATH_RESTITUTION_MAX_CLASSIFICATION_BYTES ||
	    item.note.size() > PLAYER_DEATH_RESTITUTION_MAX_NOTE_BYTES ||
	    std::find(seen.begin(), seen.end(), item.item_uid) != seen.end())
		return false;
	if (item.disposition != player_death_restitution_disposition::deliver &&
	    item.disposition != player_death_restitution_disposition::unresolved &&
	    item.disposition != player_death_restitution_disposition::excluded)
		return false;
	if (item.disposition != player_death_restitution_disposition::deliver)
	{
		return (item.metadata_payload.empty() ||
			item.metadata_payload.size() <=
				PLAYER_DEATH_RESTITUTION_MAX_ITEM_STATE_BYTES) &&
		       (item.original_payload.empty() ||
			item.original_payload.size() <=
				PLAYER_DEATH_RESTITUTION_MAX_ORIGINAL_PAYLOAD_BYTES);
	}
	if (!item.source_root_item_uid || !item.delivered_root_item_uid ||
	    item.source_item_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    item.custody_item_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    item.expected_item_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    item.expected_owner_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    item.custody_owner_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    !item.expected_owner_state ||
	    item.expected_owner_state != PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE ||
	    !item.custody_state || item.vnum > static_cast<uint32_t>(INT32_MAX) ||
	    item.artifact_vnum > static_cast<uint32_t>(INT32_MAX) ||
	    item.custody_owner_type != PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE ||
	    item.custody_owner_id != plan.source_pid || item.custody_owner_context_id ||
	    item.source_item_revision != item.expected_item_revision ||
	    item.metadata_payload.empty() || item.original_payload.empty() ||
	    item.metadata_payload.size() > PLAYER_DEATH_RESTITUTION_MAX_ITEM_STATE_BYTES ||
	    item.original_payload.size() > PLAYER_DEATH_RESTITUTION_MAX_ORIGINAL_PAYLOAD_BYTES ||
	    !digest_matches(item.metadata_digest, item.metadata_payload))
		return false;
	player_death_restitution_item_state state = {};
	if (!decode_state_internal(item.metadata_payload.data(), item.metadata_payload.size(),
				   &state) ||
	    state.item_uid != item.item_uid || state.vnum != item.vnum)
		return false;
	const bool artifact = item.artifact_vnum != 0;
	if (artifact !=
	    (item.artifact_timing_evidence_present || item.artifact_timing_uid_approved))
		return false;
	if (!artifact)
	{
		if (item.artifact_approval_uid || item.artifact_loss_epoch ||
		    item.artifact_source_timer_epoch || item.artifact_usable_lifetime_seconds ||
		    item.artifact_source_location_type || item.artifact_source_location ||
		    item.artifact_type || item.artifact_domain_present ||
		    item.artifact_domain_item_uid_present || item.artifact_domain_item_uid ||
		    item.artifact_domain_item_revision || item.artifact_domain_revision ||
		    item.artifact_baseline_present || item.artifact_baseline_opening_timer_epoch ||
		    item.artifact_baseline_opening_bind_owner_pid ||
		    item.artifact_baseline_opening_bind_timer_epoch ||
		    item.artifact_baseline_opening_revision || item.artifact_bind_present ||
		    item.artifact_bind_owner_pid || item.artifact_bind_timer_epoch ||
		    item.artifact_legacy_projection_mask)
			return false;
	}
	else
	{
		if (item.artifact_usable_lifetime_seconds == 0 ||
		    item.artifact_usable_lifetime_seconds >
			    PLAYER_DEATH_RESTITUTION_MAX_TIMER_SECONDS ||
		    item.artifact_usable_lifetime_seconds > INT32_MAX ||
		    item.vnum != item.artifact_vnum || item.artifact_legacy_projection_mask == 0 ||
		    (item.artifact_legacy_projection_mask & ~ARTIFACT_LEGACY_MASK) ||
		    item.artifact_type < PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_MAJOR ||
		    item.artifact_type > PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_IOUN ||
		    (item.artifact_source_location_type !=
			     PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER &&
		     item.artifact_source_location_type !=
			     PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_CORPSE) ||
		    (item.artifact_source_location_type ==
			     PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER &&
		     item.artifact_source_location != static_cast<int32_t>(plan.source_pid) &&
		     item.artifact_source_location != -2) ||
		    (item.artifact_source_location_type ==
			     PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_CORPSE &&
		     item.artifact_source_location != static_cast<int32_t>(plan.source_pid)) ||
		    item.artifact_domain_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
		    item.artifact_domain_item_revision ==
			    PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
		    item.artifact_baseline_opening_timer_epoch >
			    static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) ||
		    item.artifact_baseline_opening_bind_timer_epoch < 0 ||
		    item.artifact_baseline_opening_bind_timer_epoch >
			    std::numeric_limits<int64_t>::max() ||
		    item.artifact_bind_timer_epoch < 0 ||
		    item.artifact_bind_timer_epoch > std::numeric_limits<int32_t>::max())
			return false;
		const bool historical = item.artifact_timing_evidence_present;
		const bool approved = item.artifact_timing_uid_approved;
		if (historical == approved ||
		    item.artifact_source_timer_epoch >
			    static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) ||
		    item.artifact_loss_epoch >
			    static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
			return false;
		if (historical)
		{
			if (item.artifact_source_timer_epoch <= item.artifact_loss_epoch ||
			    item.artifact_usable_lifetime_seconds !=
				    item.artifact_source_timer_epoch - item.artifact_loss_epoch ||
			    item.artifact_approval_uid)
				return false;
		}
		else if (item.artifact_approval_uid != item.item_uid)
			return false;
		if (item.artifact_bind_owner_pid != static_cast<int32_t>(plan.source_pid) &&
		    item.artifact_bind_owner_pid != 0 && item.artifact_bind_owner_pid != -1)
			return false;
		if (!item.artifact_domain_present &&
		    (item.artifact_domain_item_uid_present || item.artifact_domain_item_uid ||
		     item.artifact_domain_item_revision || item.artifact_domain_revision))
			return false;
		if (item.artifact_domain_present && !item.artifact_domain_item_uid_present &&
		    item.artifact_domain_item_uid)
			return false;
		if (item.artifact_domain_present && item.artifact_domain_item_uid_present &&
		    item.artifact_domain_item_uid != item.item_uid)
			return false;
		if (item.artifact_domain_present && item.artifact_domain_item_uid_present &&
		    item.artifact_domain_item_revision != item.expected_item_revision)
			return false;
		if (!item.artifact_domain_present && item.artifact_domain_item_revision)
			return false;
		if (!item.artifact_baseline_present &&
		    (item.artifact_baseline_opening_timer_epoch ||
		     item.artifact_baseline_opening_bind_owner_pid ||
		     item.artifact_baseline_opening_bind_timer_epoch ||
		     item.artifact_baseline_opening_revision))
			return false;
		if (item.artifact_baseline_present &&
		    (item.artifact_baseline_opening_timer_epoch !=
			     item.artifact_source_timer_epoch ||
		     item.artifact_baseline_opening_bind_owner_pid !=
			     item.artifact_bind_owner_pid ||
		     item.artifact_baseline_opening_bind_timer_epoch !=
			     item.artifact_bind_timer_epoch ||
		     (!item.artifact_domain_present && item.artifact_baseline_opening_revision)))
			return false;
		if (!item.artifact_bind_present &&
		    ((item.artifact_bind_owner_pid != 0 && item.artifact_bind_owner_pid != -1) ||
		     item.artifact_bind_timer_epoch != 0))
			return false;
	}
	if (item.source_parent_item_uid &&
	    (std::find(prior_deliveries.begin(), prior_deliveries.end(),
		       item.source_parent_item_uid) == prior_deliveries.end() ||
	     std::find(prior_deliveries.begin(), prior_deliveries.end(),
		       item.source_root_item_uid) == prior_deliveries.end()))
		return false;
	if (!item.source_parent_item_uid && item.source_root_item_uid != item.item_uid)
		return false;
	if (item.delivered_parent_item_uid &&
	    std::find(prior_deliveries.begin(), prior_deliveries.end(),
		      item.delivered_parent_item_uid) == prior_deliveries.end())
		return false;
	if (item.expected_owner_revision != plan.expected_source_owner_revision)
		return false;
	return true;
}

bool encode_plan_internal(const player_death_restitution_plan &plan, std::vector<uint8_t> *encoded)
{
	if (!encoded || !player_death_restitution_plan_valid(plan))
		return false;
	try
	{
		encoded->clear();
		encoded->reserve(256 + plan.items.size() * 256);
		append_le<uint32_t>(*encoded, PLAN_MAGIC);
		append_le<uint16_t>(*encoded, PLAYER_DEATH_RESTITUTION_PAYLOAD_VERSION);
		append_le<uint16_t>(*encoded, 0);
		append_le<uint32_t>(*encoded, plan.source_pid);
		append_le<uint64_t>(*encoded, plan.death_revision);
		append_le<uint32_t>(*encoded, plan.recipient_pid);
		append_le<uint64_t>(*encoded, plan.expected_recipient_save_revision);
		append_le<uint64_t>(*encoded, plan.expected_source_owner_revision);
		append_le<uint64_t>(*encoded, plan.expected_recipient_owner_revision);
		append_le<uint64_t>(*encoded, plan.loss_epoch);
		append_bytes(*encoded, plan.restitution_id.bytes.data(),
			     plan.restitution_id.bytes.size());
		append_bytes(*encoded, plan.death_operation_id.bytes.data(),
			     plan.death_operation_id.bytes.size());
		append_bytes(*encoded, plan.evidence_digest.data(), plan.evidence_digest.size());
		append_bytes(*encoded, plan.plan_digest.data(), plan.plan_digest.size());
		append_le<uint16_t>(*encoded, static_cast<uint16_t>(plan.actor.size()));
		append_le<uint16_t>(*encoded, static_cast<uint16_t>(plan.reason.size()));
		append_le<uint16_t>(*encoded, static_cast<uint16_t>(plan.items.size()));
		append_le<uint16_t>(*encoded, 0);
		append_bytes(*encoded, plan.actor.data(), plan.actor.size());
		append_bytes(*encoded, plan.reason.data(), plan.reason.size());
		for (const auto &item : plan.items)
		{
			append_le<uint64_t>(*encoded, item.item_uid);
			append_le<uint64_t>(*encoded, item.source_root_item_uid);
			append_le<uint64_t>(*encoded, item.source_parent_item_uid);
			append_le<uint64_t>(*encoded, item.delivered_root_item_uid);
			append_le<uint64_t>(*encoded, item.delivered_parent_item_uid);
			append_le<uint64_t>(*encoded, item.source_item_revision);
			append_le<uint64_t>(*encoded, item.expected_item_revision);
			append_le<uint64_t>(*encoded, item.expected_owner_revision);
			append_le<uint8_t>(*encoded, item.expected_owner_state);
			append_le<uint64_t>(*encoded, item.custody_item_revision);
			append_le<uint8_t>(*encoded, item.custody_state);
			append_le<uint8_t>(*encoded, item.custody_owner_type);
			append_le<uint64_t>(*encoded, item.custody_owner_id);
			append_le<uint64_t>(*encoded, item.custody_owner_context_id);
			append_le<uint64_t>(*encoded, item.custody_owner_revision);
			append_le<uint32_t>(*encoded, item.vnum);
			append_le<uint32_t>(*encoded, item.artifact_vnum);
			append_le<uint8_t>(*encoded, static_cast<uint8_t>(item.disposition));
			uint8_t artifact_flags = 0;
			if (item.artifact_timing_evidence_present)
				artifact_flags |= ARTIFACT_EVIDENCE;
			if (item.artifact_timing_uid_approved)
				artifact_flags |= ARTIFACT_UID_APPROVAL;
			if (item.artifact_domain_present)
				artifact_flags |= ARTIFACT_DOMAIN_PRESENT;
			if (item.artifact_domain_item_uid_present)
				artifact_flags |= ARTIFACT_DOMAIN_UID_PRESENT;
			if (item.artifact_baseline_present)
				artifact_flags |= ARTIFACT_BASELINE_PRESENT;
			if (item.artifact_bind_present)
				artifact_flags |= ARTIFACT_BIND_PRESENT;
			append_le<uint8_t>(*encoded, artifact_flags);
			append_le<uint8_t>(*encoded, item.artifact_source_location_type);
			append_le<int32_t>(*encoded, item.artifact_source_location);
			append_le<uint8_t>(*encoded, item.artifact_type);
			append_le<uint8_t>(*encoded, item.artifact_legacy_projection_mask);
			append_le<uint16_t>(*encoded, 0);
			append_le<uint64_t>(*encoded, item.artifact_approval_uid);
			append_le<uint64_t>(*encoded, item.artifact_loss_epoch);
			append_le<uint64_t>(*encoded, item.artifact_source_timer_epoch);
			append_le<uint64_t>(*encoded, item.artifact_usable_lifetime_seconds);
			append_le<uint64_t>(*encoded, item.artifact_domain_item_uid);
			append_le<uint64_t>(*encoded, item.artifact_domain_item_revision);
			append_le<uint64_t>(*encoded, item.artifact_domain_revision);
			append_le<uint64_t>(*encoded, item.artifact_baseline_opening_timer_epoch);
			append_le<int32_t>(*encoded, item.artifact_baseline_opening_bind_owner_pid);
			append_le<int64_t>(*encoded,
					   item.artifact_baseline_opening_bind_timer_epoch);
			append_le<uint64_t>(*encoded, item.artifact_baseline_opening_revision);
			append_le<int32_t>(*encoded, item.artifact_bind_owner_pid);
			append_le<int64_t>(*encoded, item.artifact_bind_timer_epoch);
			append_le<uint16_t>(*encoded,
					    static_cast<uint16_t>(item.classification.size()));
			append_le<uint16_t>(*encoded, static_cast<uint16_t>(item.note.size()));
			append_le<uint32_t>(*encoded,
					    static_cast<uint32_t>(item.metadata_payload.size()));
			append_le<uint32_t>(*encoded,
					    static_cast<uint32_t>(item.original_payload.size()));
			append_bytes(*encoded, item.metadata_digest.data(),
				     item.metadata_digest.size());
			append_bytes(*encoded, item.classification.data(),
				     item.classification.size());
			append_bytes(*encoded, item.note.data(), item.note.size());
			append_bytes(*encoded, item.metadata_payload.data(),
				     item.metadata_payload.size());
			append_bytes(*encoded, item.original_payload.data(),
				     item.original_payload.size());
		}
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	return encoded->size() <= MAX_PLAN_BYTES &&
	       encoded->size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool decode_plan_internal(const uint8_t *encoded, size_t encoded_size,
			  player_death_restitution_plan *plan)
{
	if (!encoded || !plan || encoded_size < 8 || encoded_size > MAX_PLAN_BYTES)
		return false;
	size_t offset = 0;
	uint32_t magic = 0;
	uint16_t version = 0, reserved = 0, actor_length = 0, reason_length = 0, item_count = 0,
		 plan_reserved = 0;
	player_death_restitution_plan decoded = {};
	if (!read_le(encoded, encoded_size, &offset, &magic) ||
	    !read_le(encoded, encoded_size, &offset, &version) ||
	    !read_le(encoded, encoded_size, &offset, &reserved) || magic != PLAN_MAGIC ||
	    version != PLAYER_DEATH_RESTITUTION_PAYLOAD_VERSION || reserved != 0 ||
	    !read_le(encoded, encoded_size, &offset, &decoded.source_pid) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.death_revision) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.recipient_pid) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.expected_recipient_save_revision) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.expected_source_owner_revision) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.expected_recipient_owner_revision) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.loss_epoch))
		return false;
	if (offset > encoded_size || decoded.restitution_id.bytes.size() > encoded_size - offset)
		return false;
	memcpy(decoded.restitution_id.bytes.data(), encoded + offset,
	       decoded.restitution_id.bytes.size());
	offset += decoded.restitution_id.bytes.size();
	if (offset > encoded_size ||
	    decoded.death_operation_id.bytes.size() > encoded_size - offset)
		return false;
	memcpy(decoded.death_operation_id.bytes.data(), encoded + offset,
	       decoded.death_operation_id.bytes.size());
	offset += decoded.death_operation_id.bytes.size();
	if (offset > encoded_size || decoded.evidence_digest.size() > encoded_size - offset)
		return false;
	memcpy(decoded.evidence_digest.data(), encoded + offset, decoded.evidence_digest.size());
	offset += decoded.evidence_digest.size();
	if (offset > encoded_size || decoded.plan_digest.size() > encoded_size - offset)
		return false;
	memcpy(decoded.plan_digest.data(), encoded + offset, decoded.plan_digest.size());
	offset += decoded.plan_digest.size();
	if (!read_le(encoded, encoded_size, &offset, &actor_length) ||
	    !read_le(encoded, encoded_size, &offset, &reason_length) ||
	    !read_le(encoded, encoded_size, &offset, &item_count) ||
	    !read_le(encoded, encoded_size, &offset, &plan_reserved) || plan_reserved != 0 ||
	    actor_length > MAX_ACTOR_BYTES ||
	    reason_length > PLAYER_DEATH_RESTITUTION_MAX_NOTE_BYTES || item_count == 0 ||
	    item_count > PLAYER_DEATH_RESTITUTION_MAX_ITEMS ||
	    actor_length > encoded_size - offset ||
	    reason_length > encoded_size - offset - actor_length)
		return false;
	try
	{
		decoded.actor.assign(reinterpret_cast<const char *>(encoded + offset),
				     actor_length);
		offset += actor_length;
		decoded.reason.assign(reinterpret_cast<const char *>(encoded + offset),
				      reason_length);
		offset += reason_length;
		decoded.items.reserve(item_count);
		for (uint16_t index = 0; index < item_count; ++index)
		{
			player_death_restitution_item item = {};
			uint8_t disposition = 0, artifact_flags = 0;
			uint16_t item_reserved = 0, class_length = 0, note_length = 0;
			uint32_t metadata_length = 0, original_length = 0;
			if (!read_le(encoded, encoded_size, &offset, &item.item_uid) ||
			    !read_le(encoded, encoded_size, &offset, &item.source_root_item_uid) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.source_parent_item_uid) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.delivered_root_item_uid) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.delivered_parent_item_uid) ||
			    !read_le(encoded, encoded_size, &offset, &item.source_item_revision) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.expected_item_revision) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.expected_owner_revision) ||
			    !read_le(encoded, encoded_size, &offset, &item.expected_owner_state) ||
			    !read_le(encoded, encoded_size, &offset, &item.custody_item_revision) ||
			    !read_le(encoded, encoded_size, &offset, &item.custody_state) ||
			    !read_le(encoded, encoded_size, &offset, &item.custody_owner_type) ||
			    !read_le(encoded, encoded_size, &offset, &item.custody_owner_id) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.custody_owner_context_id) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.custody_owner_revision) ||
			    !read_le(encoded, encoded_size, &offset, &item.vnum) ||
			    !read_le(encoded, encoded_size, &offset, &item.artifact_vnum) ||
			    !read_le(encoded, encoded_size, &offset, &disposition) ||
			    !read_le(encoded, encoded_size, &offset, &artifact_flags) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_source_location_type) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_source_location) ||
			    !read_le(encoded, encoded_size, &offset, &item.artifact_type) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_legacy_projection_mask) ||
			    !read_le(encoded, encoded_size, &offset, &item_reserved) ||
			    !read_le(encoded, encoded_size, &offset, &item.artifact_approval_uid) ||
			    !read_le(encoded, encoded_size, &offset, &item.artifact_loss_epoch) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_source_timer_epoch) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_usable_lifetime_seconds) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_domain_item_uid) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_domain_item_revision) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_domain_revision) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_baseline_opening_timer_epoch) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_baseline_opening_bind_owner_pid) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_baseline_opening_bind_timer_epoch) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_baseline_opening_revision) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_bind_owner_pid) ||
			    !read_le(encoded, encoded_size, &offset,
				     &item.artifact_bind_timer_epoch) ||
			    !read_le(encoded, encoded_size, &offset, &class_length) ||
			    !read_le(encoded, encoded_size, &offset, &note_length) ||
			    !read_le(encoded, encoded_size, &offset, &metadata_length) ||
			    !read_le(encoded, encoded_size, &offset, &original_length) ||
			    item_reserved != 0 ||
			    (artifact_flags &
			     ~(ARTIFACT_EVIDENCE | ARTIFACT_UID_APPROVAL | ARTIFACT_DOMAIN_PRESENT |
			       ARTIFACT_DOMAIN_UID_PRESENT | ARTIFACT_BASELINE_PRESENT |
			       ARTIFACT_BIND_PRESENT)) ||
			    (item.artifact_legacy_projection_mask & ~ARTIFACT_LEGACY_MASK))
				return false;
			item.disposition =
				static_cast<player_death_restitution_disposition>(disposition);
			item.artifact_timing_evidence_present =
				(artifact_flags & ARTIFACT_EVIDENCE) != 0;
			item.artifact_timing_uid_approved =
				(artifact_flags & ARTIFACT_UID_APPROVAL) != 0;
			item.artifact_domain_present = (artifact_flags & ARTIFACT_DOMAIN_PRESENT) !=
						       0;
			item.artifact_domain_item_uid_present =
				(artifact_flags & ARTIFACT_DOMAIN_UID_PRESENT) != 0;
			item.artifact_baseline_present =
				(artifact_flags & ARTIFACT_BASELINE_PRESENT) != 0;
			item.artifact_bind_present = (artifact_flags & ARTIFACT_BIND_PRESENT) != 0;
			if (class_length > PLAYER_DEATH_RESTITUTION_MAX_CLASSIFICATION_BYTES ||
			    note_length > PLAYER_DEATH_RESTITUTION_MAX_NOTE_BYTES ||
			    metadata_length > PLAYER_DEATH_RESTITUTION_MAX_ITEM_STATE_BYTES ||
			    original_length > PLAYER_DEATH_RESTITUTION_MAX_ORIGINAL_PAYLOAD_BYTES ||
			    offset > encoded_size ||
			    item.metadata_digest.size() > encoded_size - offset)
				return false;
			memcpy(item.metadata_digest.data(), encoded + offset,
			       item.metadata_digest.size());
			offset += item.metadata_digest.size();
			if (class_length > encoded_size - offset)
				return false;
			item.classification.assign(reinterpret_cast<const char *>(encoded + offset),
						   class_length);
			offset += class_length;
			if (note_length > encoded_size - offset)
				return false;
			item.note.assign(reinterpret_cast<const char *>(encoded + offset),
					 note_length);
			offset += note_length;
			if (!read_bytes(encoded, encoded_size, &offset, metadata_length,
					&item.metadata_payload) ||
			    !read_bytes(encoded, encoded_size, &offset, original_length,
					&item.original_payload))
				return false;
			decoded.items.push_back(std::move(item));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (offset != encoded_size || !player_death_restitution_plan_valid(decoded))
		return false;
	*plan = std::move(decoded);
	return true;
}

} // namespace

bool player_death_restitution_item_state_encode(const player_death_restitution_item_state &state,
						std::vector<uint8_t> *encoded)
{
	return encode_state_internal(state, encoded);
}

bool player_death_restitution_item_state_decode(const uint8_t *encoded, size_t encoded_size,
						player_death_restitution_item_state *state)
{
	return decode_state_internal(encoded, encoded_size, state);
}

bool player_death_restitution_item_state_valid(const player_death_restitution_item_state &state)
{
	return validate_state(state);
}

bool player_death_restitution_plan_valid(const player_death_restitution_plan &plan)
{
	if (!plan.source_pid || plan.source_pid > static_cast<uint32_t>(INT32_MAX) ||
	    !plan.death_revision || !plan.recipient_pid ||
	    plan.recipient_pid > static_cast<uint32_t>(INT32_MAX) ||
	    critical_operation_id_is_zero(plan.restitution_id) ||
	    critical_operation_id_is_zero(plan.death_operation_id) ||
	    !digest_nonzero(plan.evidence_digest) || !digest_nonzero(plan.plan_digest) ||
	    plan.expected_recipient_save_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    plan.expected_source_owner_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    plan.expected_recipient_owner_revision == PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD ||
	    (plan.source_pid == plan.recipient_pid &&
	     plan.expected_recipient_owner_revision != plan.expected_source_owner_revision) ||
	    !plan.loss_epoch ||
	    plan.loss_epoch > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) ||
	    plan.actor.empty() || plan.actor.size() > MAX_ACTOR_BYTES || plan.reason.empty() ||
	    plan.reason.size() > PLAYER_DEATH_RESTITUTION_MAX_NOTE_BYTES || plan.items.empty() ||
	    plan.items.size() > PLAYER_DEATH_RESTITUTION_MAX_ITEMS)
		return false;
	std::vector<uint64_t> seen;
	std::vector<uint64_t> prior_deliveries;
	std::vector<uint32_t> seen_artifact_vnums;
	try
	{
		seen.reserve(plan.items.size());
		prior_deliveries.reserve(plan.items.size());
		seen_artifact_vnums.reserve(plan.items.size());
		for (const auto &item : plan.items)
		{
			if (!plan_item_valid(plan, item, seen, prior_deliveries))
				return false;
			if (item.disposition == player_death_restitution_disposition::deliver &&
			    item.artifact_vnum != 0)
			{
				if (std::find(seen_artifact_vnums.begin(),
					      seen_artifact_vnums.end(),
					      item.artifact_vnum) != seen_artifact_vnums.end())
					return false;
				seen_artifact_vnums.push_back(item.artifact_vnum);
			}
			seen.push_back(item.item_uid);
			if (item.disposition == player_death_restitution_disposition::deliver)
				prior_deliveries.push_back(item.item_uid);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return std::any_of(
		plan.items.begin(), plan.items.end(), [](const auto &item)
		{ return item.disposition == player_death_restitution_disposition::deliver; });
}

bool player_death_restitution_artifact_delivery_timer(const player_death_restitution_item &item,
						      uint64_t delivery_epoch,
						      uint64_t *delivered_timer_epoch)
{
	if (!delivered_timer_epoch || !item.artifact_vnum ||
	    !item.artifact_usable_lifetime_seconds ||
	    item.artifact_usable_lifetime_seconds > PLAYER_DEATH_RESTITUTION_MAX_TIMER_SECONDS ||
	    item.artifact_usable_lifetime_seconds > INT32_MAX ||
	    item.artifact_timing_evidence_present == item.artifact_timing_uid_approved ||
	    (item.artifact_timing_evidence_present &&
	     (item.artifact_loss_epoch == 0 ||
	      item.artifact_source_timer_epoch <= item.artifact_loss_epoch ||
	      item.artifact_usable_lifetime_seconds !=
		      item.artifact_source_timer_epoch - item.artifact_loss_epoch)) ||
	    (item.artifact_timing_evidence_present && item.artifact_approval_uid) ||
	    (item.artifact_timing_uid_approved && item.artifact_approval_uid != item.item_uid) ||
	    (!item.artifact_timing_evidence_present &&
	     (!item.artifact_timing_uid_approved || item.artifact_approval_uid != item.item_uid)) ||
	    delivery_epoch >
		    std::numeric_limits<uint64_t>::max() - item.artifact_usable_lifetime_seconds)
		return false;
	*delivered_timer_epoch = delivery_epoch + item.artifact_usable_lifetime_seconds;
	return true;
}

bool player_death_restitution_command_build(const player_death_restitution_plan &plan,
					    critical_command *command)
{
	if (!command)
		return false;
	player_death_restitution_plan normalized = plan;
	if (!normalized.accepted_at_usec)
	{
		struct timespec now = {};
		if (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0)
			return false;
		normalized.accepted_at_usec = static_cast<uint64_t>(now.tv_sec) * 1000000ULL +
					      static_cast<uint64_t>(now.tv_nsec) / 1000ULL;
	}
	if (!player_death_restitution_plan_valid(normalized))
		return false;
	std::vector<uint8_t> payload;
	if (!encode_plan_internal(normalized, &payload))
		return false;
	critical_command built = {};
	built.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	built.operation_id = normalized.restitution_id;
	built.type = critical_command_type::player_death_restitution;
	built.payload_version = PLAYER_DEATH_RESTITUTION_PAYLOAD_VERSION;
	built.source_site = critical_source_site::recovery;
	built.deadline_class = critical_deadline_class::recovery;
	built.accepted_at_usec = normalized.accepted_at_usec;
	built.keys.push_back({ critical_entity_type::player, normalized.recipient_pid });
	built.expected_revisions.push_back(
		{ { critical_entity_type::player, normalized.recipient_pid },
		  normalized.expected_recipient_save_revision });
	built.payload = std::move(payload);
	if (!critical_command_normalize(&built))
		return false;
	*command = std::move(built);
	return true;
}

bool player_death_restitution_command_decode_payload(const critical_command &command,
						     player_death_restitution_plan *plan)
{
	if (!plan || command.type != critical_command_type::player_death_restitution ||
	    command.payload_version != PLAYER_DEATH_RESTITUTION_PAYLOAD_VERSION ||
	    !command.accepted_at_usec || command.payload.empty())
		return false;
	player_death_restitution_plan decoded = {};
	if (!decode_plan_internal(command.payload.data(), command.payload.size(), &decoded) ||
	    !critical_operation_id_equal(decoded.restitution_id, command.operation_id))
		return false;
	decoded.accepted_at_usec = command.accepted_at_usec;
	if (!player_death_restitution_plan_valid(decoded) || command.keys.size() != 1 ||
	    command.keys[0].type != critical_entity_type::player ||
	    command.keys[0].id != decoded.recipient_pid || command.expected_revisions.size() != 1 ||
	    command.expected_revisions[0].key.id != decoded.recipient_pid ||
	    command.expected_revisions[0].revision != decoded.expected_recipient_save_revision)
		return false;
	*plan = std::move(decoded);
	return true;
}

bool player_death_restitution_command_encode_result(
	const player_death_restitution_result &result,
	std::array<uint8_t, PLAYER_DEATH_RESTITUTION_RESULT_BYTES> *encoded)
{
	if (!encoded || critical_operation_id_is_zero(result.restitution_id) ||
	    !result.source_pid || !result.recipient_pid || result.candidate_count == 0 ||
	    result.delivered_count > result.candidate_count ||
	    result.unresolved_count > result.candidate_count ||
	    result.delivered_count + result.unresolved_count > result.candidate_count)
		return false;
	encoded->fill(0);
	size_t offset = 0;
	auto put = [&](auto value)
	{
		using value_type = decltype(value);
		using unsigned_type = typename std::make_unsigned<value_type>::type;
		const unsigned_type encoded_value = static_cast<unsigned_type>(value);
		for (size_t index = 0; index < sizeof(value); ++index)
			(*encoded)[offset++] = static_cast<uint8_t>(encoded_value >> (index * 8));
	};
	put(static_cast<uint32_t>(RESULT_MAGIC));
	put(static_cast<uint16_t>(RESULT_VERSION));
	put(static_cast<uint16_t>(result.mutation_applied ? 1 : 0));
	std::copy(result.restitution_id.bytes.begin(), result.restitution_id.bytes.end(),
		  encoded->begin() + offset);
	offset += result.restitution_id.bytes.size();
	put(result.source_pid);
	put(result.recipient_pid);
	put(result.delivery_epoch);
	put(result.durable_revision);
	put(result.candidate_count);
	put(result.delivered_count);
	put(result.unresolved_count);
	return offset <= encoded->size();
}

bool player_death_restitution_command_decode_result(const uint8_t *encoded, size_t encoded_size,
						    player_death_restitution_result *result)
{
	if (!encoded || !result || encoded_size != PLAYER_DEATH_RESTITUTION_RESULT_BYTES)
		return false;
	size_t offset = 0;
	uint32_t magic = 0;
	uint16_t version = 0, flags = 0;
	player_death_restitution_result decoded = {};
	if (!read_le(encoded, encoded_size, &offset, &magic) ||
	    !read_le(encoded, encoded_size, &offset, &version) ||
	    !read_le(encoded, encoded_size, &offset, &flags) || magic != RESULT_MAGIC ||
	    version != RESULT_VERSION || (flags & ~1U) ||
	    offset + decoded.restitution_id.bytes.size() > encoded_size)
		return false;
	decoded.mutation_applied = (flags & 1U) != 0;
	memcpy(decoded.restitution_id.bytes.data(), encoded + offset,
	       decoded.restitution_id.bytes.size());
	offset += decoded.restitution_id.bytes.size();
	if (!read_le(encoded, encoded_size, &offset, &decoded.source_pid) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.recipient_pid) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.delivery_epoch) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.durable_revision) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.candidate_count) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.delivered_count) ||
	    !read_le(encoded, encoded_size, &offset, &decoded.unresolved_count) ||
	    std::any_of(encoded + offset, encoded + encoded_size,
			[](uint8_t value) { return value != 0; }))
		return false;
	if (critical_operation_id_is_zero(decoded.restitution_id) || !decoded.source_pid ||
	    !decoded.recipient_pid || decoded.candidate_count == 0 ||
	    decoded.delivered_count > decoded.candidate_count ||
	    decoded.unresolved_count > decoded.candidate_count ||
	    decoded.delivered_count + decoded.unresolved_count > decoded.candidate_count)
		return false;
	*result = decoded;
	return true;
}
