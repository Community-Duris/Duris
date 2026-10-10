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

#include <type_traits>

namespace
{
using birth_command_reserve_fn = bool (*)(size_t, void *) noexcept;

bool birth_command_add(size_t &bytes, size_t added) noexcept
{
	if (added > SIZE_MAX - bytes)
		return false;
	bytes += added;
	return true;
}

bool birth_command_admit(size_t base, size_t extra, birth_command_reserve_fn reserve,
			 void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}

bool birth_command_array(size_t count, size_t width, size_t &bytes) noexcept
{
	if (width && count > SIZE_MAX / width)
		return false;
	bytes = count * width;
	return true;
}

// Every buffer/old output and input span belongs to outer. Actual codec input
// spans are passed by value, separately from the caller's reference parameters.
error birth_command_recipe_encode_bounded(
	const std::vector<player_item_snapshot> &items,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	std::vector<uint8_t> *output, birth_command_reserve_fn reserve, void *context,
	size_t outer) noexcept
{
	if (!output || !reserve)
		return error::corrupt_evidence;
	constexpr size_t profile_object = sizeof(native_mobile_birth_recipe_allocation_profile);
	constexpr size_t item_span = sizeof(std::span<const player_item_snapshot>);
	constexpr size_t recipe_span = sizeof(std::span<const native_mobile_birth_item_recipe>);
	size_t scan = outer;
	if (!birth_command_add(scan, profile_object) ||
	    !birth_command_add(scan, native_mobile_birth_recipe_profile_inline_storage_bytes()) ||
	    !birth_command_add(scan, 2 * item_span + 2 * recipe_span) ||
	    !birth_command_admit(scan, 0, reserve, context))
		return error::capacity;
	native_mobile_birth_recipe_allocation_profile profile;
	const auto checked = native_mobile_birth_recipe_encode_profile(items, recipes, &profile);
	if (checked != error::ok)
		return checked;
	if (!profile.fresh_encode_storage_policy_supported)
		return error::unresolved;
	size_t validation = item_span + recipe_span, encoded = profile.encoder_inline_storage_bytes;
	if (!birth_command_add(validation, profile.validation_inline_storage_bytes) ||
	    !birth_command_add(encoded, profile.encoded_capacity_bytes))
		return error::capacity;
	size_t live = outer;
	if (!birth_command_add(live, profile_object) ||
	    !birth_command_add(live, item_span + recipe_span) ||
	    !birth_command_add(live, std::max(validation, encoded)) ||
	    !birth_command_admit(live, 0, reserve, context))
		return error::capacity;
	return native_mobile_birth_recipe_encode(items, recipes, output);
}

error birth_command_recipe_decode_bounded(const std::span<const uint8_t> &bytes,
					  const std::vector<player_item_snapshot> &items,
					  std::vector<native_mobile_birth_item_recipe> *output,
					  birth_command_reserve_fn reserve, void *context,
					  size_t outer, size_t *retained_heap) noexcept
{
	if (!output || !reserve)
		return error::corrupt_evidence;
	constexpr size_t profile_object = sizeof(native_mobile_birth_recipe_allocation_profile);
	constexpr size_t wire_span = sizeof(std::span<const uint8_t>);
	constexpr size_t item_span = sizeof(std::span<const player_item_snapshot>);
	size_t scan = outer;
	if (!birth_command_add(scan, profile_object) ||
	    !birth_command_add(scan, native_mobile_birth_recipe_profile_inline_storage_bytes()) ||
	    !birth_command_add(scan, 2 * wire_span + 2 * item_span) ||
	    !birth_command_admit(scan, 0, reserve, context))
		return error::capacity;
	native_mobile_birth_recipe_allocation_profile profile;
	const auto checked = native_mobile_birth_recipe_decode_profile(bytes, items, &profile);
	if (checked != error::ok)
		return checked;
	if (!profile.fresh_decode_storage_policy_supported)
		return error::unresolved;
	size_t preflight = wire_span + item_span, decoded = profile.decoder_inline_storage_bytes;
	if (!birth_command_add(preflight, profile.preflight_inline_storage_bytes) ||
	    !birth_command_add(decoded, profile.decoded_payload_bytes))
		return error::capacity;
	size_t live = outer;
	if (!birth_command_add(live, profile_object) ||
	    !birth_command_add(live, wire_span + item_span) ||
	    !birth_command_add(live, std::max(preflight, decoded)) ||
	    !birth_command_admit(live, 0, reserve, context))
		return error::capacity;
	const auto status = native_mobile_birth_recipe_decode(bytes, items, output);
	if (status == error::ok && retained_heap)
		*retained_heap = profile.decoded_payload_bytes;
	return status;
}

bool birth_command_heap(const critical_command &command, size_t &heap) noexcept
{
	heap = 0;
	size_t keys = 0, revisions = 0;
	return birth_command_array(command.keys.capacity(), sizeof(critical_entity_key), keys) &&
	       birth_command_array(command.expected_revisions.capacity(),
				   sizeof(critical_expected_revision), revisions) &&
	       birth_command_add(heap, keys) && birth_command_add(heap, revisions) &&
	       birth_command_add(heap, command.payload.capacity()) &&
	       birth_command_add(heap, command.accounting_intent.capacity());
}

bool birth_command_push_request(size_t size, size_t capacity, size_t width, size_t temporary,
				size_t &extra) noexcept
{
	extra = temporary;
	if (size != capacity)
		return true;
	const size_t growth = std::max(size, size_t{ 1 });
	if (growth > SIZE_MAX - size)
		return false;
	size_t fresh = 0;
	return birth_command_array(size + growth, width, fresh) && birth_command_add(extra, fresh);
}
} // namespace
namespace
{
struct birth_payload_workspace
{
	std::vector<uint8_t> constructor, image, recipe, candidate;
};
struct birth_payload_live
{
	birth_payload_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		return birth_command_add(out, work.constructor.capacity()) &&
		       birth_command_add(out, work.image.capacity()) &&
		       birth_command_add(out, work.recipe.capacity()) &&
		       birth_command_add(out, work.candidate.capacity());
	}
};

error birth_constructor_payload_encode_bounded(
	const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const quest_mobile_native_constructor_recipe &constructor, std::vector<uint8_t> *output,
	birth_command_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!output || !reserve)
		return error::corrupt_evidence;
	if (constructor.mobile_vnum != image.reference.mobile_vnum ||
	    ((constructor.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
	      constructor.wire_version ==
		      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) &&
	     constructor.reset_room_vnum != image.reference.birthplace_vnum) ||
	    !original_alchemist_grant_matches(image, constructor))
		return error::payload_conflict;
	if (!native_mobile_birth_constructor_recipe_valid(constructor))
		return error::corrupt_evidence;
	size_t base = outer;
	if (!birth_command_add(base, sizeof(birth_payload_workspace)) ||
	    !birth_command_add(base, sizeof(birth_payload_live)) ||
	    !birth_command_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_payload_workspace work;
		birth_payload_live live{ work, base };
		size_t current = base;
		if (!native_mobile_birth_constructor_recipe_encode_blob_bounded(
			    constructor, &work.constructor, reserve, context, current))
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		auto status = image_error(quest_mobile_native_image_encode_bounded(
			image, &work.image, reserve, context, current));
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_command_recipe_encode_bounded(image.items, recipes, &work.recipe,
							     reserve, context, current);
		if (status != error::ok)
			return status;
		const size_t fixed = CONSTRUCTOR_PAYLOAD_HEADER_BYTES + work.constructor.size();
		if (work.image.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed ||
		    work.recipe.size() >
			    CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed - work.image.size())
			return error::capacity;
		const size_t encoded = fixed + work.image.size() + work.recipe.size();
		if (!live.bytes(current) ||
		    !birth_command_admit(current, encoded, reserve, context))
			return error::capacity;
		work.candidate.reserve(encoded);
		work.candidate.insert(work.candidate.end(), CONSTRUCTOR_PAYLOAD_MAGIC.begin(),
				      CONSTRUCTOR_PAYLOAD_MAGIC.end());
		append_u32(work.candidate, static_cast<uint32_t>(work.image.size()));
		append_u32(work.candidate, static_cast<uint32_t>(work.recipe.size()));
		append_u32(work.candidate, static_cast<uint32_t>(work.constructor.size()));
		work.candidate.insert(work.candidate.end(), work.image.begin(), work.image.end());
		work.candidate.insert(work.candidate.end(), work.recipe.begin(), work.recipe.end());
		work.candidate.insert(work.candidate.end(), work.constructor.begin(),
				      work.constructor.end());
		*output = std::move(work.candidate);
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

struct birth_build_workspace
{
	critical_command candidate;
	economic_admission_facts facts;
};
struct birth_build_live
{
	birth_build_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t heap = 0;
		return birth_command_heap(work.candidate, heap) && birth_command_add(out, heap) &&
		       birth_command_add(out, work.facts.facts.capacity());
	}
};
} // namespace

error native_mobile_birth_command_build_bounded(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const quest_mobile_native_constructor_recipe &constructor, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!output || !accepted_at_usec || site < critical_source_site::command ||
	    site > critical_source_site::operator_repair)
		return error::invalid_identity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)metadata;
	(void)image;
	(void)recipes;
	(void)constructor;
	(void)reserve;
	(void)context;
	(void)outer;
	return error::unresolved;
#else
	// Original metadata owns two source arrays; the source encoder owns its
	// distinct result array. All checks precede candidate allocations as before.
	if (!birth_command_admit(outer,
				 3 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
				 reserve, context))
		return error::capacity;
	const auto checked = original_metadata(metadata, image);
	if (checked != error::ok)
		return checked;
	size_t base = outer;
	if (!birth_command_add(base, sizeof(birth_build_workspace)) ||
	    !birth_command_add(base, sizeof(birth_build_live)) ||
	    !birth_command_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_build_workspace work;
		birth_build_live live{ work, base };
		auto &candidate = work.candidate;
		candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		candidate.operation_id = image.reference.birth_operation;
		candidate.type = critical_command_type::native_mobile_birth;
		candidate.payload_version = NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION;
		candidate.source_site = site;
		candidate.deadline_class = critical_deadline_class::background;
		candidate.accepted_at_usec = accepted_at_usec;
		auto status = birth_constructor_payload_encode_bounded(
			image, recipes, constructor, &candidate.payload, reserve, context, base);
		if (status != error::ok)
			return status;
		if (candidate.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return error::capacity;
		size_t current = 0, extra = 0;
		if (!live.bytes(current) ||
		    !birth_command_push_request(candidate.keys.size(), candidate.keys.capacity(),
						sizeof(critical_entity_key),
						sizeof(critical_entity_key), extra) ||
		    !birth_command_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.keys.push_back({ critical_entity_type::native_mobile,
					   image.reference.mobile_instance_id });
		if (!live.bytes(current) ||
		    !birth_command_push_request(candidate.expected_revisions.size(),
						candidate.expected_revisions.capacity(),
						sizeof(critical_expected_revision),
						sizeof(critical_expected_revision), extra) ||
		    !birth_command_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		if (image.reference.provenance == quest_mobile_birth_provenance::reset)
		{
			if (!live.bytes(current) ||
			    !birth_command_push_request(candidate.keys.size(),
							candidate.keys.capacity(),
							sizeof(critical_entity_key),
							sizeof(critical_entity_key), extra) ||
			    !birth_command_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.keys.push_back(
				{ critical_entity_type::zone,
				  static_cast<uint64_t>(image.reference.reset_zone_vnum) + 1 });
		}
		for (const auto &item : image.items)
		{
			if (!live.bytes(current) ||
			    !birth_command_push_request(candidate.keys.size(),
							candidate.keys.capacity(),
							sizeof(critical_entity_key),
							sizeof(critical_entity_key), extra) ||
			    !birth_command_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.keys.push_back({ critical_entity_type::item, item.object_uid });
			if (!live.bytes(current) ||
			    !birth_command_push_request(candidate.expected_revisions.size(),
							candidate.expected_revisions.capacity(),
							sizeof(critical_expected_revision),
							sizeof(critical_expected_revision),
							extra) ||
			    !birth_command_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		}
		if (candidate.keys.size() > CRITICAL_COMMAND_MAX_KEYS ||
		    candidate.expected_revisions.size() > CRITICAL_COMMAND_MAX_KEYS)
			return error::capacity;
		std::sort(candidate.keys.begin(), candidate.keys.end(), critical_entity_key_less);
		std::sort(candidate.expected_revisions.begin(), candidate.expected_revisions.end(),
			  [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		work.facts.metadata = metadata;
		if (!live.bytes(current))
			return error::capacity;
		status = economic_intent_freeze_bounded(candidate, work.facts,
							&candidate.accounting_intent, reserve,
							context, current);
		if (status != error::ok)
			return status;
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
#endif
}
namespace
{
struct birth_bool_codec_reservation
{
	birth_command_reserve_fn callback;
	void *context;
	bool refused = false;
	static bool reserve(size_t bytes, void *opaque) noexcept
	{
		auto &state = *static_cast<birth_bool_codec_reservation *>(opaque);
		if (!state.callback || !state.callback(bytes, state.context))
		{
			state.refused = true;
			return false;
		}
		return true;
	}
};

struct birth_decode_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	quest_mobile_native_constructor_recipe constructor;
	economic_frozen_intent intent;
	critical_command expected;
	std::vector<uint8_t> actual_bytes, expected_bytes;
	std::span<const uint8_t> payload, image_wire, recipe_wire, constructor_wire, intent_wire;
	std::span<const native_mobile_birth_item_recipe> recipe_values;
	size_t image_heap = 0, recipe_heap = 0;
};
struct birth_decode_live
{
	birth_decode_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t expected_heap = 0;
		return birth_command_heap(work.expected, expected_heap) &&
		       birth_command_add(out, expected_heap) &&
		       birth_command_add(out, work.image_heap) &&
		       birth_command_add(out, work.recipe_heap) &&
		       birth_command_add(out, work.intent.admission.facts.capacity()) &&
		       birth_command_add(out, work.actual_bytes.capacity()) &&
		       birth_command_add(out, work.expected_bytes.capacity());
	}
};
} // namespace

error native_mobile_birth_command_decode_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	quest_mobile_native_constructor_recipe *constructor_output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *retained_image_heap, size_t *retained_recipe_heap) noexcept
{
	if (!output || !recipe_output || !constructor_output ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    !command.publication_required || !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer;
	(void)retained_image_heap;
	(void)retained_recipe_heap;
	return error::unresolved;
#else
	size_t base = outer;
	if (!birth_command_add(base, sizeof(birth_decode_workspace)) ||
	    !birth_command_add(base, sizeof(birth_decode_live)) ||
	    !birth_command_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	try
	{
		birth_decode_workspace work;
		birth_decode_live live{ work, base };
		work.payload = std::span<const uint8_t>(command.payload);
		const auto &bytes = work.payload;
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
		work.image_wire = bytes.subspan(CONSTRUCTOR_PAYLOAD_HEADER_BYTES, image_size);
		work.recipe_wire =
			bytes.subspan(CONSTRUCTOR_PAYLOAD_HEADER_BYTES + image_size, recipe_size);
		work.constructor_wire =
			bytes.subspan(CONSTRUCTOR_PAYLOAD_HEADER_BYTES + image_size + recipe_size,
				      constructor_size);
		size_t current = base;
		auto status = image_error(quest_mobile_native_image_decode_bounded(
			work.image_wire, &work.image, reserve, context, current, &work.image_heap));
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_command_recipe_decode_bounded(work.recipe_wire, work.image.items,
							     &work.recipes, reserve, context,
							     current, &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		{
			if (!birth_command_add(current, sizeof(birth_bool_codec_reservation)) ||
			    !birth_command_admit(current, 0, reserve, context))
				return error::capacity;
			birth_bool_codec_reservation reservation{ reserve, context };
			if (!native_mobile_birth_constructor_recipe_decode_bounded(
				    work.constructor_wire, &work.constructor,
				    birth_bool_codec_reservation::reserve, &reservation, current))
				return reservation.refused ? error::capacity :
							     error::corrupt_evidence;
		}
		const auto &constructor = work.constructor;
		const auto &image = work.image;
		if (constructor.mobile_vnum != image.reference.mobile_vnum ||
		    ((constructor.wire_version ==
			      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
		      constructor.wire_version ==
			      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) &&
		     constructor.reset_room_vnum != image.reference.birthplace_vnum) ||
		    !original_alchemist_grant_matches(image, constructor))
			return error::payload_conflict;
		if (!live.bytes(current) ||
		    !birth_command_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve,
							context, current);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!live.bytes(current))
			return error::capacity;
		status = economic_intent_verify_binding_bounded(command, work.intent, reserve,
								context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current,
					 sizeof(std::span<const native_mobile_birth_item_recipe>),
					 reserve, context))
			return error::capacity;
		work.recipe_values = std::span<const native_mobile_birth_item_recipe>(work.recipes);
		status = native_mobile_birth_command_build_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values,
			work.constructor, command.source_site, command.accepted_at_usec,
			&work.expected, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (critical_command_encode_bounded(command, &work.actual_bytes, reserve, context,
						    current) != critical_command_codec_result::ok)
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		if (critical_command_encode_bounded(work.expected, &work.expected_bytes, reserve,
						    context,
						    current) != critical_command_codec_result::ok)
			return error::capacity;
		if (work.actual_bytes != work.expected_bytes)
			return error::payload_conflict;
		static_assert(
			std::is_nothrow_copy_assignable_v<quest_mobile_native_constructor_recipe>);
		static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
		static_assert(std::is_nothrow_move_assignable_v<
			      std::vector<native_mobile_birth_item_recipe>>);
		*constructor_output = work.constructor;
		*recipe_output = std::move(work.recipes);
		*output = std::move(work.image);
		if (retained_image_heap)
			*retained_image_heap = work.image_heap;
		if (retained_recipe_heap)
			*retained_recipe_heap = work.recipe_heap;
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
#endif
}
namespace
{
// Full NMB2 companions preserve historical recipe framing and canonical proof.
error birth_recipe_payload_encode_bounded(
	const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	std::vector<uint8_t> *output, birth_command_reserve_fn reserve, void *context,
	size_t outer) noexcept
{
	if (!output || !reserve)
		return error::corrupt_evidence;
	size_t base = outer;
	if (!birth_command_add(base, sizeof(birth_payload_workspace)) ||
	    !birth_command_add(base, sizeof(birth_payload_live)) ||
	    !birth_command_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_payload_workspace work;
		birth_payload_live live{ work, base };
		size_t current = base;
		auto status = image_error(quest_mobile_native_image_encode_bounded(
			image, &work.image, reserve, context, current));
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_command_recipe_encode_bounded(image.items, recipes, &work.recipe,
							     reserve, context, current);
		if (status != error::ok)
			return status;
		const size_t fixed = RECIPE_PAYLOAD_HEADER_BYTES;
		if (work.image.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed ||
		    work.recipe.size() >
			    CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed - work.image.size())
			return error::capacity;
		const size_t encoded = fixed + work.image.size() + work.recipe.size();
		if (!live.bytes(current) ||
		    !birth_command_admit(current, encoded, reserve, context))
			return error::capacity;
		work.candidate.reserve(encoded);
		work.candidate.insert(work.candidate.end(), RECIPE_PAYLOAD_MAGIC.begin(),
				      RECIPE_PAYLOAD_MAGIC.end());
		append_u32(work.candidate, static_cast<uint32_t>(work.image.size()));
		append_u32(work.candidate, static_cast<uint32_t>(work.recipe.size()));
		work.candidate.insert(work.candidate.end(), work.image.begin(), work.image.end());
		work.candidate.insert(work.candidate.end(), work.recipe.begin(), work.recipe.end());
		*output = std::move(work.candidate);
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

error birth_recipe_command_build_bounded(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!output || !accepted_at_usec || site < critical_source_site::command ||
	    site > critical_source_site::operator_repair)
		return error::invalid_identity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)metadata;
	(void)image;
	(void)recipes;
	(void)reserve;
	(void)context;
	(void)outer;
	return error::unresolved;
#else
	// Original metadata owns two source arrays; the source encoder owns its
	// distinct result array. All checks precede candidate allocations as before.
	if (!birth_command_admit(outer,
				 3 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
				 reserve, context))
		return error::capacity;
	const auto checked = original_metadata(metadata, image);
	if (checked != error::ok)
		return checked;
	size_t base = outer;
	if (!birth_command_add(base, sizeof(birth_build_workspace)) ||
	    !birth_command_add(base, sizeof(birth_build_live)) ||
	    !birth_command_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_build_workspace work;
		birth_build_live live{ work, base };
		auto &candidate = work.candidate;
		candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		candidate.operation_id = image.reference.birth_operation;
		candidate.type = critical_command_type::native_mobile_birth;
		candidate.payload_version = NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION;
		candidate.source_site = site;
		candidate.deadline_class = critical_deadline_class::background;
		candidate.accepted_at_usec = accepted_at_usec;
		auto status = birth_recipe_payload_encode_bounded(
			image, recipes, &candidate.payload, reserve, context, base);
		if (status != error::ok)
			return status;
		if (candidate.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return error::capacity;
		size_t current = 0, extra = 0;
		if (!live.bytes(current) ||
		    !birth_command_push_request(candidate.keys.size(), candidate.keys.capacity(),
						sizeof(critical_entity_key),
						sizeof(critical_entity_key), extra) ||
		    !birth_command_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.keys.push_back({ critical_entity_type::native_mobile,
					   image.reference.mobile_instance_id });
		if (!live.bytes(current) ||
		    !birth_command_push_request(candidate.expected_revisions.size(),
						candidate.expected_revisions.capacity(),
						sizeof(critical_expected_revision),
						sizeof(critical_expected_revision), extra) ||
		    !birth_command_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		if (image.reference.provenance == quest_mobile_birth_provenance::reset)
		{
			if (!live.bytes(current) ||
			    !birth_command_push_request(candidate.keys.size(),
							candidate.keys.capacity(),
							sizeof(critical_entity_key),
							sizeof(critical_entity_key), extra) ||
			    !birth_command_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.keys.push_back(
				{ critical_entity_type::zone,
				  static_cast<uint64_t>(image.reference.reset_zone_vnum) + 1 });
		}
		for (const auto &item : image.items)
		{
			if (!live.bytes(current) ||
			    !birth_command_push_request(candidate.keys.size(),
							candidate.keys.capacity(),
							sizeof(critical_entity_key),
							sizeof(critical_entity_key), extra) ||
			    !birth_command_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.keys.push_back({ critical_entity_type::item, item.object_uid });
			if (!live.bytes(current) ||
			    !birth_command_push_request(candidate.expected_revisions.size(),
							candidate.expected_revisions.capacity(),
							sizeof(critical_expected_revision),
							sizeof(critical_expected_revision),
							extra) ||
			    !birth_command_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		}
		if (candidate.keys.size() > CRITICAL_COMMAND_MAX_KEYS ||
		    candidate.expected_revisions.size() > CRITICAL_COMMAND_MAX_KEYS)
			return error::capacity;
		std::sort(candidate.keys.begin(), candidate.keys.end(), critical_entity_key_less);
		std::sort(candidate.expected_revisions.begin(), candidate.expected_revisions.end(),
			  [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		work.facts.metadata = metadata;
		if (!live.bytes(current))
			return error::capacity;
		status = economic_intent_freeze_bounded(candidate, work.facts,
							&candidate.accounting_intent, reserve,
							context, current);
		if (status != error::ok)
			return status;
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
#endif
}

error birth_recipe_command_decode_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *retained_image_heap, size_t *retained_recipe_heap) noexcept
{
	if (!output || !recipe_output ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    !command.publication_required || !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer;
	(void)retained_image_heap;
	(void)retained_recipe_heap;
	return error::unresolved;
#else
	size_t base = outer;
	if (!birth_command_add(base, sizeof(birth_decode_workspace)) ||
	    !birth_command_add(base, sizeof(birth_decode_live)) ||
	    !birth_command_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	try
	{
		birth_decode_workspace work;
		birth_decode_live live{ work, base };
		work.payload = std::span<const uint8_t>(command.payload);
		const auto &bytes = work.payload;
		if (bytes.size() < RECIPE_PAYLOAD_HEADER_BYTES ||
		    !std::equal(RECIPE_PAYLOAD_MAGIC.begin(), RECIPE_PAYLOAD_MAGIC.end(),
				bytes.begin()))
			return error::corrupt_evidence;
		const size_t image_size = read_u32(bytes, 4);
		const size_t recipe_size = read_u32(bytes, 8);
		const size_t body_size = bytes.size() - RECIPE_PAYLOAD_HEADER_BYTES;
		if (!image_size || !recipe_size || image_size > body_size ||
		    recipe_size != body_size - image_size)
			return error::corrupt_evidence;
		work.image_wire = bytes.subspan(RECIPE_PAYLOAD_HEADER_BYTES, image_size);
		work.recipe_wire =
			bytes.subspan(RECIPE_PAYLOAD_HEADER_BYTES + image_size, recipe_size);
		size_t current = base;
		auto status = image_error(quest_mobile_native_image_decode_bounded(
			work.image_wire, &work.image, reserve, context, current, &work.image_heap));
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_command_recipe_decode_bounded(work.recipe_wire, work.image.items,
							     &work.recipes, reserve, context,
							     current, &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve,
							context, current);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!live.bytes(current))
			return error::capacity;
		status = economic_intent_verify_binding_bounded(command, work.intent, reserve,
								context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current,
					 sizeof(std::span<const native_mobile_birth_item_recipe>),
					 reserve, context))
			return error::capacity;
		work.recipe_values = std::span<const native_mobile_birth_item_recipe>(work.recipes);
		status = birth_recipe_command_build_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values,
			command.source_site, command.accepted_at_usec, &work.expected, reserve,
			context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (critical_command_encode_bounded(command, &work.actual_bytes, reserve, context,
						    current) != critical_command_codec_result::ok)
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		if (critical_command_encode_bounded(work.expected, &work.expected_bytes, reserve,
						    context,
						    current) != critical_command_codec_result::ok)
			return error::capacity;
		if (work.actual_bytes != work.expected_bytes)
			return error::payload_conflict;
		static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
		static_assert(std::is_nothrow_move_assignable_v<
			      std::vector<native_mobile_birth_item_recipe>>);
		*recipe_output = std::move(work.recipes);
		*output = std::move(work.image);
		if (retained_image_heap)
			*retained_image_heap = work.image_heap;
		if (retained_recipe_heap)
			*retained_recipe_heap = work.recipe_heap;
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
#endif
}
} // namespace

economic_accounting_error native_mobile_birth_command_decode_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipes,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_image_heap_bytes, size_t *retained_recipe_heap_bytes) noexcept
{
	if (command.payload_version == NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION)
		return birth_recipe_command_decode_bounded(command, output, recipes, reserve,
							   context, outer_live,
							   retained_image_heap_bytes,
							   retained_recipe_heap_bytes);
	if (command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
		return error::corrupt_evidence;
	// The real constructor output remains live throughout the genuine v3 decoder.
	size_t base = outer_live;
	if (!birth_command_add(base, sizeof(quest_mobile_native_constructor_recipe)) ||
	    !birth_command_admit(base, 0, reserve, context))
		return error::capacity;
	quest_mobile_native_constructor_recipe constructor;
	return native_mobile_birth_command_decode_bounded(command, output, recipes, &constructor,
							  reserve, context, base,
							  retained_image_heap_bytes,
							  retained_recipe_heap_bytes);
}
