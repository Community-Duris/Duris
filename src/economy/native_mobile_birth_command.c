#include "economy/native_mobile_birth_command.h"

#include <algorithm>
#include <new>
#include <utility>

namespace
{
using error = economic_accounting_error;

error image_error(player_snapshot_codec_result result) noexcept
{
	if (result == player_snapshot_codec_result::ok)
		return error::ok;
	if (result == player_snapshot_codec_result::allocation_failure ||
	    result == player_snapshot_codec_result::limit_exceeded)
		return error::capacity;
	return error::corrupt_evidence;
}

bool original_birth(const quest_mobile_native_image &image) noexcept
{
	const auto &reference = image.reference;
	return quest_mobile_native_reference_valid(reference) &&
	       reference.birth_source.kind == economic_source_kind::npc_generation &&
	       reference.birth_source.generation.bytes != reference.birth_operation.bytes &&
	       reference.mobile_revision == 1 && reference.stock_revision == 1 &&
	       image.last_transition_operation.bytes == reference.birth_operation.bytes &&
	       quest_mobile_native_cash_transition_valid(nullptr, image);
}

error original_metadata(const economic_operation_metadata &metadata,
			const quest_mobile_native_image &image) noexcept
{
	auto result = economic_operation_metadata_validate(metadata);
	if (result != error::ok)
		return result;
	if (!original_birth(image) ||
	    metadata.operation_id.bytes != image.reference.birth_operation.bytes ||
	    !critical_operation_id_is_zero(metadata.original_operation_id) ||
	    metadata.actor_kind != economic_actor_kind::domain ||
	    metadata.actor_id != image.reference.mobile_instance_id ||
	    metadata.writer_id != ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH ||
	    metadata.reason != economic_reason::npc_reward || metadata.policy_version != 1 ||
	    metadata.compiler_version != 1 || !metadata.source_event)
		return error::unauthorized;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> actual{}, expected{};
	if (economic_source_event_encode(*metadata.source_event, &actual) != error::ok ||
	    economic_source_event_encode(image.reference.birth_source, &expected) != error::ok ||
	    actual != expected)
		return error::payload_conflict;
	return error::ok;
}

constexpr std::array<uint8_t, 4> RECIPE_PAYLOAD_MAGIC{ 'N', 'M', 'B', '2' };
constexpr size_t RECIPE_PAYLOAD_HEADER_BYTES = 12;

void append_u32(std::vector<uint8_t> &bytes, uint32_t value)
{
	for (unsigned shift = 0; shift < 32; shift += 8)
		bytes.push_back(static_cast<uint8_t>(value >> shift));
}
uint32_t read_u32(std::span<const uint8_t> bytes, size_t offset) noexcept
{
	uint32_t value = 0;
	for (unsigned index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8);
	return value;
}

error payload_encode(const quest_mobile_native_image &image,
		     std::span<const native_mobile_birth_item_recipe> recipes, bool with_recipe,
		     std::vector<uint8_t> *output)
{
	if (!with_recipe)
		return image_error(quest_mobile_native_image_encode(image, output));
	std::vector<uint8_t> image_bytes, recipe_bytes;
	auto result = image_error(quest_mobile_native_image_encode(image, &image_bytes));
	if (result != error::ok)
		return result;
	result = native_mobile_birth_recipe_encode(image.items, recipes, &recipe_bytes);
	if (result != error::ok)
		return result;
	if (image_bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - RECIPE_PAYLOAD_HEADER_BYTES ||
	    recipe_bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - RECIPE_PAYLOAD_HEADER_BYTES -
					  image_bytes.size())
		return error::capacity;
	std::vector<uint8_t> candidate;
	candidate.reserve(RECIPE_PAYLOAD_HEADER_BYTES + image_bytes.size() + recipe_bytes.size());
	candidate.insert(candidate.end(), RECIPE_PAYLOAD_MAGIC.begin(), RECIPE_PAYLOAD_MAGIC.end());
	append_u32(candidate, static_cast<uint32_t>(image_bytes.size()));
	append_u32(candidate, static_cast<uint32_t>(recipe_bytes.size()));
	candidate.insert(candidate.end(), image_bytes.begin(), image_bytes.end());
	candidate.insert(candidate.end(), recipe_bytes.begin(), recipe_bytes.end());
	*output = std::move(candidate);
	return error::ok;
}

constexpr std::array<uint8_t, 4> CONSTRUCTOR_PAYLOAD_MAGIC{ 'N', 'M', 'B', '3' };
constexpr size_t CONSTRUCTOR_PAYLOAD_HEADER_BYTES = 16;

// The original alchemist grant is the poison vial (VNUM102). Its final placement
// may change through later original G/E/P commands; only exact identity is bound.
bool original_alchemist_grant_matches(
	const quest_mobile_native_image &image,
	const quest_mobile_native_constructor_recipe &constructor) noexcept
{
	if (constructor.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION ||
	    !constructor.alchemist_grant_uid)
		return true;
	size_t matches = 0;
	for (const auto &item : image.items)
		if (item.object_uid == constructor.alchemist_grant_uid)
		{
			if (item.vnum != 102)
				return false;
			++matches;
		}
	return matches == 1;
}

error constructor_payload_encode(const quest_mobile_native_image &image,
				 std::span<const native_mobile_birth_item_recipe> recipes,
				 const quest_mobile_native_constructor_recipe &constructor,
				 std::vector<uint8_t> *output)
{
	std::vector<uint8_t> constructor_bytes;
	if (constructor.mobile_vnum != image.reference.mobile_vnum ||
	    ((constructor.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
	      constructor.wire_version ==
		      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) &&
	     constructor.reset_room_vnum != image.reference.birthplace_vnum) ||
	    !original_alchemist_grant_matches(image, constructor))
		return error::payload_conflict;
	if (!native_mobile_birth_constructor_recipe_valid(constructor))
		return error::corrupt_evidence;
	if (!native_mobile_birth_constructor_recipe_encode_blob(constructor, &constructor_bytes))
		return error::capacity;
	std::vector<uint8_t> image_bytes, recipe_bytes;
	auto result = image_error(quest_mobile_native_image_encode(image, &image_bytes));
	if (result != error::ok)
		return result;
	result = native_mobile_birth_recipe_encode(image.items, recipes, &recipe_bytes);
	if (result != error::ok)
		return result;
	static_assert(CONSTRUCTOR_PAYLOAD_HEADER_BYTES +
			      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_MAX_BYTES <
		      CRITICAL_COMMAND_MAX_PAYLOAD_BYTES);
	const size_t fixed_bytes = CONSTRUCTOR_PAYLOAD_HEADER_BYTES + constructor_bytes.size();
	if (image_bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed_bytes ||
	    recipe_bytes.size() >
		    CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed_bytes - image_bytes.size())
		return error::capacity;
	std::vector<uint8_t> candidate;
	candidate.reserve(fixed_bytes + image_bytes.size() + recipe_bytes.size());
	candidate.insert(candidate.end(), CONSTRUCTOR_PAYLOAD_MAGIC.begin(),
			 CONSTRUCTOR_PAYLOAD_MAGIC.end());
	append_u32(candidate, static_cast<uint32_t>(image_bytes.size()));
	append_u32(candidate, static_cast<uint32_t>(recipe_bytes.size()));
	append_u32(candidate, static_cast<uint32_t>(constructor_bytes.size()));
	candidate.insert(candidate.end(), image_bytes.begin(), image_bytes.end());
	candidate.insert(candidate.end(), recipe_bytes.begin(), recipe_bytes.end());
	candidate.insert(candidate.end(), constructor_bytes.begin(), constructor_bytes.end());
	*output = std::move(candidate);
	return error::ok;
}

error constructor_payload_decode(const critical_command &command, quest_mobile_native_image *image,
				 std::vector<native_mobile_birth_item_recipe> *recipes,
				 quest_mobile_native_constructor_recipe *constructor)
{
	const std::span<const uint8_t> bytes(command.payload);
	if (bytes.size() < CONSTRUCTOR_PAYLOAD_HEADER_BYTES ||
	    !std::equal(CONSTRUCTOR_PAYLOAD_MAGIC.begin(), CONSTRUCTOR_PAYLOAD_MAGIC.end(),
			bytes.begin()))
		return error::corrupt_evidence;
	const size_t image_size = read_u32(bytes, 4);
	const size_t recipe_size = read_u32(bytes, 8);
	const size_t constructor_size = read_u32(bytes, 12);
	const size_t body_size = bytes.size() - CONSTRUCTOR_PAYLOAD_HEADER_BYTES;
	if (!image_size || !recipe_size ||
	    (constructor_size != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES &&
	     constructor_size != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES &&
	     constructor_size != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES) ||
	    image_size > body_size || recipe_size > body_size - image_size ||
	    constructor_size != body_size - image_size - recipe_size)
		return error::corrupt_evidence;
	auto result = image_error(quest_mobile_native_image_decode(
		bytes.subspan(CONSTRUCTOR_PAYLOAD_HEADER_BYTES, image_size), image));
	if (result != error::ok)
		return result;
	result = native_mobile_birth_recipe_decode(
		bytes.subspan(CONSTRUCTOR_PAYLOAD_HEADER_BYTES + image_size, recipe_size),
		image->items, recipes);
	if (result != error::ok)
		return result;
	if (!native_mobile_birth_constructor_recipe_decode(
		    bytes.subspan(CONSTRUCTOR_PAYLOAD_HEADER_BYTES + image_size + recipe_size,
				  constructor_size),
		    constructor))
		return error::corrupt_evidence;
	if (constructor->mobile_vnum != image->reference.mobile_vnum ||
	    ((constructor->wire_version ==
		      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
	      constructor->wire_version ==
		      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) &&
	     constructor->reset_room_vnum != image->reference.birthplace_vnum) ||
	    !original_alchemist_grant_matches(*image, *constructor))
		return error::payload_conflict;
	return error::ok;
}

error payload_decode(const critical_command &command, quest_mobile_native_image *image,
		     std::vector<native_mobile_birth_item_recipe> *recipes,
		     quest_mobile_native_constructor_recipe *constructor)
{
	if (command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
		return constructor_payload_decode(command, image, recipes, constructor);
	if (command.payload_version == NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION)
		return image_error(quest_mobile_native_image_decode(command.payload, image));
	const std::span<const uint8_t> bytes(command.payload);
	if (bytes.size() < RECIPE_PAYLOAD_HEADER_BYTES ||
	    !std::equal(RECIPE_PAYLOAD_MAGIC.begin(), RECIPE_PAYLOAD_MAGIC.end(), bytes.begin()))
		return error::corrupt_evidence;
	const size_t image_size = read_u32(bytes, 4);
	const size_t recipe_size = read_u32(bytes, 8);
	if (!image_size || !recipe_size ||
	    image_size > bytes.size() - RECIPE_PAYLOAD_HEADER_BYTES ||
	    recipe_size != bytes.size() - RECIPE_PAYLOAD_HEADER_BYTES - image_size)
		return error::corrupt_evidence;
	auto result = image_error(quest_mobile_native_image_decode(
		bytes.subspan(RECIPE_PAYLOAD_HEADER_BYTES, image_size), image));
	if (result != error::ok)
		return result;
	return native_mobile_birth_recipe_decode(
		bytes.subspan(RECIPE_PAYLOAD_HEADER_BYTES + image_size, recipe_size), image->items,
		recipes);
}
}

static economic_accounting_error native_mobile_birth_command_build_impl(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	std::span<const native_mobile_birth_item_recipe> recipes, bool with_recipe,
	const quest_mobile_native_constructor_recipe *constructor,
	critical_source_site original_source_site, uint64_t accepted_at_usec,
	critical_command *output) noexcept
{
	if (!output || !accepted_at_usec || original_source_site < critical_source_site::command ||
	    original_source_site > critical_source_site::operator_repair)
		return error::invalid_identity;
	const auto status = original_metadata(metadata, image);
	if (status != error::ok)
		return status;
	try
	{
		critical_command candidate{};
		candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		candidate.operation_id = image.reference.birth_operation;
		candidate.type = critical_command_type::native_mobile_birth;
		candidate.payload_version =
			constructor ? NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION :
			with_recipe ? NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION :
				      NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION;
		candidate.source_site = original_source_site;
		candidate.deadline_class = critical_deadline_class::background;
		candidate.accepted_at_usec = accepted_at_usec;
		auto result = constructor ? constructor_payload_encode(image, recipes, *constructor,
								       &candidate.payload) :
					    payload_encode(image, recipes, with_recipe,
							   &candidate.payload);
		if (result != error::ok)
			return result;
		if (candidate.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return error::capacity;
		candidate.keys.push_back({ critical_entity_type::native_mobile,
					   image.reference.mobile_instance_id });
		candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		if (image.reference.provenance == quest_mobile_birth_provenance::reset)
			candidate.keys.push_back(
				{ critical_entity_type::zone,
				  static_cast<uint64_t>(image.reference.reset_zone_vnum) + 1 });
		for (const auto &item : image.items)
		{
			candidate.keys.push_back({ critical_entity_type::item, item.object_uid });
			candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		}
		if (candidate.keys.size() > CRITICAL_COMMAND_MAX_KEYS ||
		    candidate.expected_revisions.size() > CRITICAL_COMMAND_MAX_KEYS)
			return error::capacity;
		std::sort(candidate.keys.begin(), candidate.keys.end(), critical_entity_key_less);
		std::sort(candidate.expected_revisions.begin(), candidate.expected_revisions.end(),
			  [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		economic_admission_facts facts;
		facts.metadata = metadata;
		result = economic_intent_freeze(candidate, facts, &candidate.accounting_intent);
		if (result != error::ok)
			return result;
		candidate.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		candidate.publication_required = true;
		if (!critical_command_envelope_valid(candidate))
			return error::corrupt_evidence;
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

static economic_accounting_error native_mobile_birth_command_decode_impl(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	quest_mobile_native_constructor_recipe *constructor_output) noexcept
{
	if (!output || command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    !native_mobile_birth_payload_version_supported(command.payload_version) ||
	    !command.publication_required || !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
	try
	{
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		quest_mobile_native_constructor_recipe constructor;
		auto result = payload_decode(command, &image, &recipes, &constructor);
		if (result != error::ok)
			return result;
		economic_frozen_intent intent;
		result = economic_intent_decode(command.accounting_intent, &intent);
		if (result != error::ok)
			return result;
		if (intent.admission.facts_version != 1 || !intent.admission.facts.empty())
			return error::payload_conflict;
		result = economic_intent_verify_binding(command, intent);
		if (result != error::ok)
			return result;
		critical_command expected;
		result = native_mobile_birth_command_build_impl(
			intent.admission.metadata, image, recipes,
			command.payload_version != NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION,
			command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ?
				&constructor :
				nullptr,
			command.source_site, command.accepted_at_usec, &expected);
		if (result != error::ok)
			return result;
		std::vector<uint8_t> actual_bytes, expected_bytes;
		if (critical_command_encode(command, &actual_bytes) !=
			    critical_command_codec_result::ok ||
		    critical_command_encode(expected, &expected_bytes) !=
			    critical_command_codec_result::ok)
			return error::capacity;
		if (actual_bytes != expected_bytes)
			return error::payload_conflict;
		if (constructor_output)
			*constructor_output = constructor;
		if (recipe_output)
			*recipe_output = std::move(recipes);
		*output = std::move(image);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error
native_mobile_birth_command_build(const economic_operation_metadata &metadata,
				  const quest_mobile_native_image &image, critical_source_site site,
				  uint64_t accepted_at_usec, critical_command *output) noexcept
{
	return native_mobile_birth_command_build_impl(metadata, image, {}, false, nullptr, site,
						      accepted_at_usec, output);
}
economic_accounting_error native_mobile_birth_command_build(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	std::span<const native_mobile_birth_item_recipe> recipes, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	return native_mobile_birth_command_build_impl(metadata, image, recipes, true, nullptr, site,
						      accepted_at_usec, output);
}
economic_accounting_error
native_mobile_birth_command_decode(const critical_command &command,
				   quest_mobile_native_image *output) noexcept
{
	return native_mobile_birth_command_decode_impl(command, output, nullptr, nullptr);
}
economic_accounting_error
native_mobile_birth_command_decode(const critical_command &command,
				   quest_mobile_native_image *output,
				   std::vector<native_mobile_birth_item_recipe> *recipes) noexcept
{
	if (!recipes)
		return error::corrupt_evidence;
	return native_mobile_birth_command_decode_impl(command, output, recipes, nullptr);
}

economic_accounting_error native_mobile_birth_command_build(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	std::span<const native_mobile_birth_item_recipe> recipes,
	const quest_mobile_native_constructor_recipe &constructor, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	return native_mobile_birth_command_build_impl(metadata, image, recipes, true, &constructor,
						      site, accepted_at_usec, output);
}

economic_accounting_error
native_mobile_birth_command_decode(const critical_command &command,
				   quest_mobile_native_image *output,
				   std::vector<native_mobile_birth_item_recipe> *recipes,
				   quest_mobile_native_constructor_recipe *constructor) noexcept
{
	if (!recipes || !constructor ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
		return error::corrupt_evidence;
	return native_mobile_birth_command_decode_impl(command, output, recipes, constructor);
}
