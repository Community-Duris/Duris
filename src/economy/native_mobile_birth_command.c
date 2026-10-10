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

namespace
{
using birth_source_source_query = bool (*)(size_t *) noexcept;
constexpr size_t birth_source_child_preflight_frames =
	// Six actual pointer parameters; query/outer and four size_t locals;
	// error result and the actual add/admit calls/boolean return carriers.
	6 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(error) + sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
error birth_source_child_preflight(birth_source_source_query full,
				   birth_source_source_query initial,
				   birth_source_source_query supplement, size_t query_frames,
				   size_t outer, bool (*reserve)(size_t, void *) noexcept,
				   void *context, size_t *child_outer) noexcept
{
	size_t source = 0, entry = 0, retained = 0, request = outer;
	if (!birth_command_add(request, birth_source_child_preflight_frames) ||
	    !birth_command_add(request, query_frames) ||
	    !birth_command_add(request, sizeof(size_t)) ||
	    !birth_command_admit(request, 0, reserve, context))
		return error::capacity;
	if (!full || !child_outer || !full(&source) || (initial && !initial(&entry)) ||
	    (supplement && !supplement(&retained)))
		return error::unresolved;
	request = outer;
	if (!birth_command_add(request, birth_source_child_preflight_frames) ||
	    !birth_command_add(request, source) || !birth_command_add(request, entry) ||
	    !birth_command_admit(request, 0, reserve, context))
		return error::capacity;
	request = outer;
	if (!birth_command_add(request, retained))
		return error::capacity;
	*child_outer = request;
	return error::ok;
}
error birth_source_image_encode(const quest_mobile_native_image &input,
				std::vector<uint8_t> *output,
				bool (*reserve)(size_t, void *) noexcept, void *context,
				size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		quest_mobile_native_image_encode_source_frame_bytes,
		quest_mobile_native_image_encode_initial_inline_bytes,
		quest_mobile_native_image_encode_source_supplement_frame_bytes,
		quest_mobile_native_image_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return image_error(
		quest_mobile_native_image_encode_bounded(input, output, reserve, context, child));
}
error birth_source_image_decode(const std::span<const uint8_t> &input,
				quest_mobile_native_image *output,
				bool (*reserve)(size_t, void *) noexcept, void *context,
				size_t outer, size_t *retained_heap) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.
	constexpr size_t own = 5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		quest_mobile_native_image_decode_source_frame_bytes,
		quest_mobile_native_image_decode_initial_inline_bytes,
		quest_mobile_native_image_decode_source_supplement_frame_bytes,
		quest_mobile_native_image_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return image_error(quest_mobile_native_image_decode_bounded(input, output, reserve, context,
								    child, retained_heap));
}
error birth_source_intent_decode(const std::span<const uint8_t> &input,
				 economic_frozen_intent *output,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		economic_intent_decode_source_frame_bytes,
		economic_intent_decode_initial_inline_bytes,
		economic_intent_decode_source_supplement_frame_bytes,
		economic_intent_decode_source_query_frame_bytes(), base, reserve, context, &child);
	if (checked != error::ok)
		return checked;
	return economic_intent_decode_bounded(input, output, reserve, context, child);
}
error birth_source_intent_freeze(const critical_command &command,
				 const economic_admission_facts &facts,
				 std::vector<uint8_t> *output,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.
	constexpr size_t own = 5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		economic_intent_freeze_fixed_source_frame_bytes,
		economic_intent_freeze_fixed_initial_inline_bytes,
		economic_intent_freeze_fixed_source_supplement_frame_bytes,
		economic_intent_freeze_fixed_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return economic_intent_freeze_fixed_bounded(command, facts, output, reserve, context,
						    child);
}
error birth_source_intent_verify(const critical_command &command,
				 const economic_frozen_intent &intent,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		economic_intent_verify_binding_fixed_source_frame_bytes,
		economic_intent_verify_binding_fixed_initial_inline_bytes, nullptr,
		economic_intent_verify_binding_fixed_source_query_frame_bytes(), base, reserve,
		context, &child);
	if (checked != error::ok)
		return checked;
	return economic_intent_verify_binding_fixed_bounded(command, intent, reserve, context,
							    child);
}
error birth_source_recipe_encode(const std::vector<player_item_snapshot> &items,
				 const std::span<const native_mobile_birth_item_recipe> &recipes,
				 std::vector<uint8_t> *output,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	constexpr size_t own = 5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		native_mobile_birth_recipe_encode_source_frame_bytes,
		native_mobile_birth_recipe_encode_initial_inline_bytes,
		native_mobile_birth_recipe_encode_source_supplement_frame_bytes,
		native_mobile_birth_recipe_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return birth_command_recipe_encode_bounded(items, recipes, output, reserve, context, child);
}
error birth_source_recipe_decode(const std::span<const uint8_t> &bytes,
				 const std::vector<player_item_snapshot> &items,
				 std::vector<native_mobile_birth_item_recipe> *output,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer, size_t *retained_heap) noexcept
{
	constexpr size_t own = 6 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto checked = birth_source_child_preflight(
		native_mobile_birth_recipe_decode_source_frame_bytes,
		native_mobile_birth_recipe_decode_initial_inline_bytes,
		native_mobile_birth_recipe_decode_source_supplement_frame_bytes,
		native_mobile_birth_recipe_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return birth_command_recipe_decode_bounded(bytes, items, output, reserve, context, child,
						   retained_heap);
}
} // namespace

namespace
{
// These pure local initial getters price the actual original encoder vector,
// not any encoded-length or baseline allowance. Query owns output P and B.
bool birth_source_canonical_initial(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = sizeof(std::vector<uint8_t>);
	return true;
}
error birth_source_retain_value_source(birth_source_source_query source, size_t query_frames,
				       bool (*reserve)(size_t, void *) noexcept, void *context,
				       size_t &outer) noexcept
{
	// Actual source/reserve/context/outer refs, query/base/child and returned
	// accessor N, status/result, plus checked-add source. Helper is transient.
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return error::capacity;
	const auto status = birth_source_child_preflight(source, nullptr, source, query_frames,
							 base, reserve, context, &child);
	if (status != error::ok)
		return status;
	// Exact just-added helper source is released on return, independently of
	// the genuine returned SOURCE supplement. No storage is observed here.
	outer = child - own;
	return error::ok;
}
critical_command_codec_result
birth_source_canonical_encode(const critical_command &command, std::vector<uint8_t> *output,
			      bool (*reserve)(size_t, void *) noexcept, void *context,
			      size_t outer) noexcept
{
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
			       sizeof(critical_command_codec_result) + sizeof(void *) +
			       sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return critical_command_codec_result::overflow;
	const auto checked = birth_source_child_preflight(
		critical_command_startup_codec_source_frame_bytes, birth_source_canonical_initial,
		critical_command_startup_codec_source_frame_bytes,
		critical_command_startup_codec_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return critical_command_codec_result::overflow;
	return critical_command_encode_bounded(command, output, reserve, context, child);
}
bool birth_source_constructor_encode(const quest_mobile_native_constructor_recipe &input,
				     std::vector<uint8_t> *output,
				     bool (*reserve)(size_t, void *) noexcept, void *context,
				     size_t outer) noexcept
{
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
			       2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!birth_command_add(base, own))
		return false;
	const auto checked = birth_source_child_preflight(
		native_mobile_birth_constructor_recipe_own_source_frame_bytes,
		native_mobile_birth_constructor_recipe_initial_inline_bytes, nullptr,
		native_mobile_birth_constructor_recipe_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return false;
	return native_mobile_birth_constructor_recipe_encode_blob_fixed_bounded(
		input, output, reserve, context, child);
}
} // namespace

namespace
{
// Additive v1 function's actual arguments, base/current/own query locals, error
// status/result and query pointer/boolean. Workspace/live are separate INLINE.
constexpr size_t birth_v1_decode_named_frames = 5 * sizeof(void *) + 4 * sizeof(size_t) +
						2 * sizeof(error) + sizeof(size_t *) + sizeof(bool);
// Original-reconstruction arguments, checked/status, base/current/extra,
// candidate alias, range-for range/begin/end/item, comparison closure/arguments.
constexpr size_t birth_v1_build_named_frames =
	5 * sizeof(void *) + sizeof(critical_source_site) + sizeof(uint64_t) + 4 * sizeof(size_t) +
	2 * sizeof(error) + 5 * sizeof(void *) + sizeof(bool);
constexpr size_t birth_v3_build_named_frames =
	7 * sizeof(void *) + sizeof(critical_source_site) + sizeof(uint64_t) + 4 * sizeof(size_t) +
	2 * sizeof(error) + 5 * sizeof(void *) + sizeof(bool);
// Exact local arithmetic/heap/push/live scalar graph, retained conservatively
// across sequential calls. No STL, callback descendant or native stack claim.
constexpr size_t birth_v1_helper_named_frames =
	(sizeof(size_t *) + sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(size_t) + sizeof(size_t *) + sizeof(bool)) +
	(2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(6 * sizeof(size_t) + sizeof(size_t *) + sizeof(bool)) +
	(2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(bool)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(sizeof(player_snapshot_codec_result) + sizeof(error));
}

bool native_mobile_birth_command_v1_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	// Both decode and nested rebuild inline objects coexist. This upper bound
	// includes rebuild before its actual construction; inputs stay with caller.
	*output = sizeof(birth_decode_workspace) + sizeof(birth_decode_live) +
		  sizeof(birth_build_workspace) + sizeof(birth_build_live) +
		  sizeof(std::span<const uint8_t>);
	return true;
}

bool native_mobile_birth_command_v1_decode_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = birth_v1_decode_named_frames + birth_v1_build_named_frames +
		  birth_v1_helper_named_frames;
	return true;
}

bool native_mobile_birth_command_constructor_build_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = birth_v3_build_named_frames;
	return true;
}

bool native_mobile_birth_command_constructor_build_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = sizeof(birth_build_workspace) + sizeof(birth_build_live) +
		  sizeof(birth_payload_workspace) + sizeof(birth_payload_live) +
		  3 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>);
	return true;
}

namespace
{
// Actual H100 dispatch arguments/base/result; actual v3 decode arguments,
// bytes/image/constructor aliases, framing sizes/current/base/status/result.
// This supplements named entries ONLY. Rebuild/recipe/constructor STL and lower
// image/intent graphs must be joined before a complete source claim.
constexpr size_t birth_historical_dispatch_entry_frames =
	7 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_v2_decode_entry_frames =
	8 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(error) + sizeof(bool);
constexpr size_t birth_v3_decode_entry_frames =
	11 * sizeof(void *) + 7 * sizeof(size_t) + 2 * sizeof(error) + sizeof(bool);
constexpr size_t birth_historical_entry_frames =
	birth_historical_dispatch_entry_frames +
	(birth_v2_decode_entry_frames > birth_v3_decode_entry_frames ?
		 birth_v2_decode_entry_frames :
		 birth_v3_decode_entry_frames);
constexpr size_t birth_general_entry_frames = 5 * sizeof(void *) + 5 * sizeof(size_t) +
					      2 * sizeof(error) + sizeof(size_t *) + sizeof(bool);
}

namespace
{
// Private owning SOURCE inventory. Original bodies are not replaced.
// Same genuine GNU13 vector/allocator/iterator families from the current
// critical owner, with the actual birth byte/key/revision types. The selected
// pointer-iterator ordinary LP64 policy must hold. Value objects and requests
// already owned by workspace/preflight are excluded here.
namespace
{
[[maybe_unused]] constexpr size_t birth_own_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
[[maybe_unused]] constexpr size_t birth_own_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
[[maybe_unused]] constexpr size_t birth_own_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
[[maybe_unused]] constexpr size_t birth_own_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
[[maybe_unused]] constexpr size_t birth_own_vector_frames =
	birth_own_allocator_frames + birth_own_copy_frames + birth_own_relocate_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
template <typename T> [[maybe_unused]] constexpr size_t birth_own_move_frame_bytes() noexcept
{
	using A = std::allocator<T>;
	// operator=(vector&&): this/x/returned-reference, move-storage bool;
	// its genuine std::move reference parameter and returned reference.
	return 3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	       // _M_move_assign(true): this/x/tag; the actual __tmp vector.
	       2 * sizeof(void *) + sizeof(std::true_type) + sizeof(std::vector<T>) +
	       // Selected get_allocator: vector/base this, allocator receivers,
	       // returned references and allocator const-copy/base-copy this/source.
	       7 * sizeof(void *) + sizeof(A) +
	       // vector(const A&) -> _Vector_base(const A&) -> _Vector_impl(const A&)
	       // -> allocator const-copy -> base const-copy -> impl-data default.
	       11 * sizeof(void *) +
	       // Both actual swaps: this/x, impl-data temporary's three pointers,
	       // default ctor receiver, and three _M_copy_data this/source scopes.
	       2 * (2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(void *) +
		    3 * (2 * sizeof(void *))) +
	       // Both actual _M_get_Tp_allocator this and returned references.
	       2 * (2 * sizeof(void *)) +
	       // Direct C++20 __alloc_on_move references, std::move references,
	       // generated allocator assignment and generated base assignment.
	       2 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 3 * sizeof(void *) +
	       // Genuine temporary vector destructor and existing allocator/Destroy
	       // source, with the allocation/native operator bodies still OPEN.
	       sizeof(void *) + birth_own_allocator_frames;
}
[[maybe_unused]] constexpr size_t birth_own_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + birth_own_vector_frames;
template <typename T, typename Comparator>
[[maybe_unused]] constexpr size_t birth_own_sort_leaf_frames()
{
	// Same real GCC13 sort/partition/insertion/heap/copy/adjacent call scopes
	// as UID sorting. Values and comparator carriers use their genuine types.
	// Original key less/equal this-free argument/result scopes and revision
	// lambda this/left/right/result plus its nested key less call.
	return 3 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	       18 * sizeof(void *) + 7 * sizeof(Comparator) + sizeof(T) + 16 * sizeof(void *) +
	       6 * sizeof(Comparator) + 2 * sizeof(T) + 23 * sizeof(void *) +
	       11 * sizeof(std::ptrdiff_t) + 7 * sizeof(Comparator) + 4 * sizeof(T) +
	       8 * sizeof(void *) + 5 * sizeof(Comparator) + 4 * sizeof(bool) +
	       5 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) +
	       2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::ptrdiff_t) + 9 * sizeof(void *) +
	       2 * sizeof(Comparator) + sizeof(bool);
}

using birth_own_revision_comparator =
	decltype([](const critical_expected_revision &a, const critical_expected_revision &b)
		 { return critical_entity_key_less(a.key, b.key); });
[[maybe_unused]] constexpr size_t birth_own_sort_depth(size_t count) noexcept
{
	size_t depth = 0;
	while (count > 1)
	{
		count >>= 1;
		++depth;
	}
	return 2 * depth + 1;
}
template <typename T, typename C>
[[maybe_unused]] constexpr size_t birth_own_sort_complete() noexcept
{
	// The actual sort is reached only after both key vectors pass MAX_KEYS.
	// __introsort_loop preserves iterator/length/comparator at every nested
	// level; heap/insertion/partition source is the identical typed leaf map.
	return birth_own_sort_leaf_frames<T, C>() +
	       birth_own_sort_depth(CRITICAL_COMMAND_MAX_KEYS) *
		       (3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(C));
}
[[maybe_unused]] constexpr size_t birth_own_sorts =
	birth_own_sort_complete<critical_entity_key, decltype(&critical_entity_key_less)>() +
	birth_own_sort_complete<critical_expected_revision, birth_own_revision_comparator>();
[[maybe_unused]] constexpr size_t birth_own_byte_equal =
	// vector== and array== receiver/source/result, and their begin/end/size
	// wrappers. std::equal -> equal_aux -> aux1 -> equal<true>; real simple
	// constexpr bool and length, niter bases and __memcmp runtime boundary.
	2 * (2 * sizeof(void *) + sizeof(bool)) + 4 * (sizeof(void *) + sizeof(size_t)) +
	8 * (2 * sizeof(void *)) + 4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) +
	sizeof(size_t) + 3 * (sizeof(void *) + sizeof(void *)) +
	// __memcmp first1/first2/num and returned int. Builtin/native body OPEN.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
[[maybe_unused]] constexpr size_t birth_own_span_methods =
	// Actual dynamic span range ctor: this/range-reference, ranges::data
	// receiver/range/returned pointer, vector::data this/result; ranges::size
	// receiver/range/returned size and vector::size this/result; extent ctor.
	2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
	// subspan(this,offset,count), returned span constructor this/ptr/n,
	// extent receiver/n. Returned inline span is admitted by the old helper.
	sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
	sizeof(size_t) +
	// begin/end: this+actual iterator, iterator constructor this/ref,
	// data this/result, size this/result -> extent this/result, operator[]
	// this/index/reference-return. CPO receivers refer to static objects;
	// those objects are not fictitious automatic closure storage.
	2 * (sizeof(void *) + sizeof(void *)) + 2 * (2 * sizeof(void *)) + 2 * sizeof(void *) +
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + sizeof(size_t);
[[maybe_unused]] constexpr size_t birth_own_vector_default_cleanup(size_t vectors) noexcept
{
	// Actual byte/key/revision vectors use six-this default construction.
	// Decode owns7, build owns5, payload owns4. Callers supply the exact
	// reached family count; nested child's source is independently self-owned.
	// Image/items and recipe nested-value lifetime are separate lower exports.
	return vectors * 6 * sizeof(void *) +
	       vectors * (4 * sizeof(void *) + birth_own_allocator_frames);
}
[[maybe_unused]] constexpr size_t birth_own_byte_key_revision_lifetime(size_t vectors) noexcept
{
	// Actual command generated move (this/source) plus four vector moves;
	// facts/intent generated default/destructor/assignment receiver carriers.
	return 2 * sizeof(void *) + birth_own_move_frame_bytes<critical_entity_key>() +
	       birth_own_move_frame_bytes<critical_expected_revision>() +
	       2 * birth_own_move_frame_bytes<uint8_t>() + 3 * (2 * sizeof(void *)) +
	       4 * sizeof(void *) +
	       // std::move's argument+reference return; fixed member copy this/source.
	       2 * sizeof(void *) + 2 * sizeof(void *) + birth_own_vector_default_cleanup(vectors);
}
[[maybe_unused]] constexpr size_t birth_own_control =
	// add(ref,amount)->bool; admit(base,extra,reserve,context)->bool;
	// array(count,width,out)->bool; heap(command,out,keys,revisions)->bool;
	// push_request(size,capacity,width,temporary,out,growth,fresh)->bool.
	(sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool)) +
	// Actual payload/build/decode live.bytes this/out, heap scalar(s), bool.
	3 * (2 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(size_t) +
	// sticky bool relay reserve(bytes,opaque), state-reference and result.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// image_error original parameter and result; std::max<size_t> refs/result.
	sizeof(player_snapshot_codec_result) + sizeof(error) + 3 * sizeof(void *);
[[maybe_unused]] constexpr size_t birth_own_recipe_encode_wrapper =
	// Recipe encode: five pointer parameters; outer/scan/validation/encoded/
	// live, checked/return. Decode adds retained pointer and third status.
	5 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(error);
[[maybe_unused]] constexpr size_t birth_own_recipe_decode_wrapper =
	6 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(error);
[[maybe_unused]] constexpr size_t birth_own_payload_recipe_encoder =
	// Actual payload encode helpers. Constructor has constructor ref in
	// addition to recipe route, same outer/base/current/fixed/encoded scalars.
	5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error);
[[maybe_unused]] constexpr size_t birth_own_payload_constructor_encoder =
	6 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(error);
[[maybe_unused]] constexpr size_t birth_own_append_u32 =
	// append_u32(bytes,value,shift) and read_u32(span-by-value,offset,value,
	// index,return). read's input span is an actual formal not a workspace DTO.
	sizeof(void *) + sizeof(uint32_t) + sizeof(unsigned);
[[maybe_unused]] constexpr size_t birth_own_read_u32 =
	sizeof(std::span<const uint8_t>) + sizeof(size_t) + 3 * sizeof(uint32_t);
[[maybe_unused]] constexpr size_t birth_own_metadata_wrappers =
	// original_metadata(meta,image,result/return); original_birth(image,
	// reference-alias,bool); source_event_encode(event,out,kind,index,result).
	// Its three real source arrays are already admitted before original call.
	2 * sizeof(void *) + 2 * sizeof(error) + 2 * sizeof(void *) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(uint16_t) + 2 * sizeof(size_t) + 2 * sizeof(error) +
	// Source encoding std::copy and fixed-array access use the real same
	// pointer copy family below. Optional(source-event) const deref/engaged.
	3 * (sizeof(void *) + sizeof(bool)) + 4 * (2 * sizeof(void *));
[[maybe_unused]] constexpr size_t birth_own_alchemist_match =
	// original_alchemist_grant_matches image/constructor, matches, actual
	// range/begin/end/current-ref and bool; no per-item/depth multiplier.
	6 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
[[maybe_unused]] constexpr size_t birth_own_cash_null_validation =
	// Only actual cash_transition(nullptr,after) is reached. before/after refs
	// and result; cash_valid(image)->bool. optional bool/deref and underlying
	// payload _M_is_engaged/_M_get references/results. No BEFORE branch.
	3 * sizeof(void *) + 2 * sizeof(bool) + 3 * (sizeof(void *) + sizeof(bool)) +
	4 * (2 * sizeof(void *)) +
	// all_of/find_if_not/__find_if_not/__find_if RA and actual captured-image
	// predicate+negation adapters; each closure holds the image reference.
	4 * (3 * sizeof(void *) + sizeof(void *) + sizeof(bool)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 6 * sizeof(void *) + 4 * sizeof(void *) +
	3 * sizeof(bool) + sizeof(int64_t) + 4 * (2 * sizeof(void *));
[[maybe_unused]] constexpr size_t birth_own_metadata_assignment =
	// Real metadata's source optional has trivially copyable/destructible
	// economic_source_event and selected GNU13 uses defaulted trivial optional
	// copy assignment. Generated metadata this/source and source-event fields;
	// no _M_construct allocation or nontrivial optional assignment branch.
	2 * sizeof(void *) + 2 * sizeof(void *);
// Each entry is a conservative SUM of its genuine reached source families.
// Named original scalar frames are separately added at the actual entry.
// Full lower codec/source and owned image/recipe lifetime are not hidden here.
[[maybe_unused]] constexpr size_t birth_own_build_common =
	birth_own_control + birth_own_metadata_wrappers + birth_own_cash_null_validation +
	birth_own_metadata_assignment + birth_own_vector_frames + birth_own_sorts +
	birth_own_byte_equal;
[[maybe_unused]] constexpr size_t birth_own_v1_build =
	birth_own_build_common + birth_own_byte_key_revision_lifetime(5);
[[maybe_unused]] constexpr size_t birth_own_recipe_build =
	birth_own_build_common + birth_own_byte_key_revision_lifetime(9) +
	birth_own_recipe_encode_wrapper + birth_own_payload_recipe_encoder + birth_own_append_u32 +
	birth_own_span_methods;
[[maybe_unused]] constexpr size_t birth_own_constructor_build =
	birth_own_build_common + birth_own_byte_key_revision_lifetime(9) +
	birth_own_recipe_encode_wrapper + birth_own_payload_constructor_encoder +
	birth_own_append_u32 + birth_own_span_methods + birth_own_alchemist_match;
[[maybe_unused]] constexpr size_t birth_own_decode = birth_own_control + birth_own_vector_frames +
						     birth_own_byte_key_revision_lifetime(7) +
						     birth_own_byte_equal + birth_own_span_methods;
[[maybe_unused]] constexpr size_t birth_own_recipe_decode =
	birth_own_decode + birth_own_recipe_decode_wrapper + birth_own_read_u32;
[[maybe_unused]] constexpr size_t birth_own_constructor_decode =
	birth_own_recipe_decode +
	// Actual constructor/image aliases and original alchemist scanner source.
	birth_own_alchemist_match;
[[maybe_unused]] constexpr size_t birth_own_cash_selector =
	// Original selected_shop(role)->key and real item_shopkeeper_owner_id
	// formal uint32 and return uint64. Shop temporary is admission-owned.
	sizeof(void *) + sizeof(critical_entity_key) + sizeof(uint32_t) + sizeof(uint64_t);
[[maybe_unused]] constexpr size_t birth_own_cash_control =
	// Actual cash add(ref,amount), admit(base,extra,reserve,context), and
	// heap(command,bytes,copied,keys,revisions). No birth-only array/push.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) +
	// Genuine growth max references/result in build.
	3 * sizeof(void *);
[[maybe_unused]] constexpr size_t birth_own_cash_build_live =
	// build_live::bytes this/out/heap/returned bool.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
[[maybe_unused]] constexpr size_t birth_own_cash_decode_live =
	// decode_live::bytes this/out/original_heap/expected_heap/returned bool.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
[[maybe_unused]] constexpr size_t birth_own_cash_clear =
	// Both bind paths call vector::clear(this) -> _M_erase_at_end(this,pos,n).
	// Its genuine _Destroy/deallocation source is in vector_frames.
	3 * sizeof(void *) + sizeof(size_t);
using birth_own_cash_key_comparator = decltype(&critical_entity_key_less);
using birth_own_cash_iter_value_comparator =
	__gnu_cxx::__ops::_Iter_comp_val<birth_own_cash_key_comparator>;
[[maybe_unused]] constexpr size_t birth_own_cash_lower_bound =
	// Public lower_bound(first,last,val,comparator,result), actual pointer
	// comparator; __lower_bound first/last/val/adapter,len,half,middle,result.
	4 * sizeof(void *) + sizeof(birth_own_cash_key_comparator) + 5 * sizeof(void *) +
	sizeof(birth_own_cash_iter_value_comparator) + 2 * sizeof(std::ptrdiff_t) +
	// __iter_comp_val by-value comparator+returned adapter, real adapter ctor
	// this/comparator and its two reached std::move reference chains.
	sizeof(birth_own_cash_key_comparator) + sizeof(birth_own_cash_iter_value_comparator) +
	sizeof(void *) + sizeof(birth_own_cash_key_comparator) + 2 * (2 * sizeof(void *)) +
	// Adapter operator(this,it,val,bool) and iterator dereference(this,result).
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	// The exact lower_bound random-access distance (two iterators/result),
	// category(ref/tag), __distance(two iterators/tag/result), iterator
	// subtraction(two refs/result), and its two base(this/ref-result) calls.
	2 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) +
	sizeof(std::ptrdiff_t) + 2 * (2 * sizeof(void *)) +
	// advance(ref,n,locald), category, __advance(ref,n,tag), actual +=, ++
	// and -- receiver/result branches. Runtime ordinary C++20 signed n.
	sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	2 * (2 * sizeof(void *));
// Genuine key-less invocation is retained through the actual critical
// validation SOURCE export (the same original critical_entity_key_less).
[[maybe_unused]] constexpr size_t birth_own_cash_projection =
	birth_own_cash_selector + birth_own_cash_lower_bound +
	// Generated critical_command copy assignment(this,source) and four
	// vector copy operator= scopes(this,source,xlen,tmp,returned-reference).
	// Same underlying allocate/copy/assign/Destroy graph above; propagation
	// false for actual std::allocator so replacement branch is inactive.
	2 * sizeof(void *) + 4 * (4 * sizeof(void *) + sizeof(size_t)) +
	// C++20 erase(const_iterator): this/position/result plus begin/cbegin,
	// arithmetic and retained converted iterator. _M_erase this/pos/result;
	// move3 genuine copy_m and traits::destroy/Destroy_at trivial receiver.
	3 * sizeof(void *) + 4 * (2 * sizeof(void *)) + 2 * sizeof(std::ptrdiff_t) +
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	// Original role_valid wrapper is one ref/bool, real constructor validator
	// comes from the lower validator-only export, not a whole role codec.
	sizeof(void *) + sizeof(bool);
[[maybe_unused]] constexpr size_t birth_own_cash_build =
	birth_own_cash_control + birth_own_vector_frames + birth_own_cash_selector +
	birth_own_cash_build_live + birth_own_cash_clear + birth_own_byte_key_revision_lifetime(6) +
	birth_own_sort_complete<critical_entity_key, decltype(&critical_entity_key_less)>() +
	birth_own_metadata_assignment + birth_own_append_u32 + sizeof(void *) + sizeof(bool);
[[maybe_unused]] constexpr size_t birth_own_cash_decode =
	birth_own_cash_control + birth_own_vector_frames + birth_own_cash_projection +
	birth_own_cash_decode_live + birth_own_cash_clear +
	birth_own_byte_key_revision_lifetime(12) + birth_own_byte_equal + birth_own_span_methods +
	birth_own_read_u32;
} // namespace

namespace
{
error birth_source_admit_owned(size_t own, bool (*reserve)(size_t, void *) noexcept, void *context,
			       size_t &outer) noexcept
{
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&   \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                                \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	!defined(__SANITIZE_UNDEFINED__) &&                                                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8)
		return error::unresolved;
	// Real own/outer parameters, three pointer parameters, request/valid/query
	// locals and returned/error/bool carriers. The helper itself is transient.
	constexpr size_t helper = 3 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(error) +
				  2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) +
				  sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
				  sizeof(bool);
	size_t request = outer, valid = 0;
	if (!birth_command_add(request, helper) || !birth_command_add(request, own) ||
	    !birth_command_add(request, sizeof(size_t)) ||
	    !birth_command_admit(request, 0, reserve, context))
		return error::capacity;
	// The actual entry uses the original envelope/ID/key predicates. Their
	// exported genuine validation graph dominates those direct calls; it is
	// retained, independently of owned workspace objects and capacities.
	valid = critical_command_valid_frame_bytes();
	request = outer;
	if (!birth_command_add(request, own) || !birth_command_add(request, valid))
		return error::capacity;
	size_t temporary = request;
	if (!birth_command_add(temporary, helper) ||
	    !birth_command_admit(temporary, 0, reserve, context))
		return error::capacity;
	outer = request;
	return error::ok;
#else
	(void)own;
	(void)reserve;
	(void)context;
	(void)outer;
	return error::unresolved;
#endif
}
} // namespace
namespace
{
enum class birth_source_route : uint8_t
{
	v1_build,
	recipe_build,
	constructor_build,
	v1_decode,
	recipe_decode,
	constructor_decode,
	historical_decode,
	general_decode
};
constexpr size_t birth_source_v1_build_owned =
	birth_own_v1_build + 10 * sizeof(void *) + 4 * sizeof(size_t) +
	sizeof(critical_source_site) + sizeof(uint64_t) + 3 * sizeof(error) + sizeof(bool) +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_source_recipe_build_owned =
	birth_own_recipe_build + 11 * sizeof(void *) + 4 * sizeof(size_t) +
	sizeof(critical_source_site) + sizeof(uint64_t) + 3 * sizeof(error) + sizeof(bool) +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_source_constructor_build_owned =
	birth_own_constructor_build + 12 * sizeof(void *) + 4 * sizeof(size_t) +
	sizeof(critical_source_site) + sizeof(uint64_t) + 3 * sizeof(error) + sizeof(bool) +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_source_v1_decode_owned =
	birth_own_decode + 5 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(error) +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_source_recipe_decode_owned =
	birth_own_recipe_decode + birth_v2_decode_entry_frames +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_source_constructor_decode_owned =
	birth_own_constructor_decode + birth_v3_decode_entry_frames +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool) + 2 * sizeof(size_t) + sizeof(error);
constexpr size_t birth_source_historical_decode_owned =
	0 + birth_historical_dispatch_entry_frames +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t birth_source_general_decode_owned =
	sizeof(uint16_t) + sizeof(bool) + 5 * sizeof(void *) + 4 * sizeof(size_t) +
	2 * sizeof(error) + sizeof(bool) +
	// Added direct lifetime/validator query returned N and error comparison.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
} // namespace

error birth_constructor_payload_encode_fixed_bounded(
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
	if (const auto value_source_status = birth_source_retain_value_source(
		    native_mobile_birth_constructor_recipe_valid_source_frame_bytes,
		    native_mobile_birth_constructor_recipe_query_frame_bytes(), reserve, context,
		    outer);
	    value_source_status != error::ok)
		return value_source_status;
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
		if (!birth_source_constructor_encode(constructor, &work.constructor, reserve,
						     context, current))
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		auto status =
			birth_source_image_encode(image, &work.image, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_source_recipe_encode(image.items, recipes, &work.recipe, reserve,
						    context, current);
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

error birth_recipe_payload_encode_fixed_bounded(
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
		auto status =
			birth_source_image_encode(image, &work.image, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_source_recipe_encode(image.items, recipes, &work.recipe, reserve,
						    context, current);
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

error birth_v1_command_build_bounded(const economic_operation_metadata &metadata,
				     const quest_mobile_native_image &image,
				     critical_source_site site, uint64_t accepted_at_usec,
				     critical_command *output,
				     bool (*reserve)(size_t, void *) noexcept, void *context,
				     size_t outer) noexcept
{
	const auto own_source_status =
		birth_source_admit_owned(birth_source_v1_build_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

	if (!output || !accepted_at_usec || site < critical_source_site::command ||
	    site > critical_source_site::operator_repair)
		return error::invalid_identity;
#if !(__cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) &&                     \
      defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
      _GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
      !defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                          \
      !defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__))
	(void)metadata;
	(void)image;
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
	if (const auto value_source_status = birth_source_retain_value_source(
		    economic_operation_metadata_validate_source_frame_bytes,
		    economic_operation_metadata_validate_source_query_frame_bytes(), reserve,
		    context, outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = birth_source_retain_value_source(
		    quest_mobile_native_reference_valid_source_frame_bytes,
		    quest_mobile_native_reference_valid_source_query_frame_bytes(), reserve,
		    context, outer);
	    value_source_status != error::ok)
		return value_source_status;

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
		candidate.payload_version = NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION;
		candidate.source_site = site;
		candidate.deadline_class = critical_deadline_class::background;
		candidate.accepted_at_usec = accepted_at_usec;
		auto status = birth_source_image_encode(image, &candidate.payload, reserve, context,
							base);
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
		status = birth_source_intent_freeze(candidate, work.facts,
						    &candidate.accounting_intent, reserve, context,
						    current);
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

error birth_recipe_command_build_fixed_bounded(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	const auto own_source_status =
		birth_source_admit_owned(birth_source_recipe_build_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

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
	if (const auto value_source_status = birth_source_retain_value_source(
		    economic_operation_metadata_validate_source_frame_bytes,
		    economic_operation_metadata_validate_source_query_frame_bytes(), reserve,
		    context, outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = birth_source_retain_value_source(
		    quest_mobile_native_reference_valid_source_frame_bytes,
		    quest_mobile_native_reference_valid_source_query_frame_bytes(), reserve,
		    context, outer);
	    value_source_status != error::ok)
		return value_source_status;

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
		auto status = birth_recipe_payload_encode_fixed_bounded(
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
		status = birth_source_intent_freeze(candidate, work.facts,
						    &candidate.accounting_intent, reserve, context,
						    current);
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

error birth_recipe_command_decode_fixed_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *retained_image_heap, size_t *retained_recipe_heap) noexcept
{
	const auto own_source_status =
		birth_source_admit_owned(birth_source_recipe_decode_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

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
	if (const auto value_source_status = birth_source_retain_value_source(
		    quest_mobile_native_image_lifetime_source_frame_bytes,
		    quest_mobile_native_image_lifetime_source_query_frame_bytes(), reserve, context,
		    outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = birth_source_retain_value_source(
		    native_mobile_birth_recipe_value_lifecycle_source_frame_bytes,
		    native_mobile_birth_recipe_source_query_frame_bytes(), reserve, context, outer);
	    value_source_status != error::ok)
		return value_source_status;
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
		auto status = birth_source_image_decode(work.image_wire, &work.image, reserve,
							context, current, &work.image_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_source_recipe_decode(work.recipe_wire, work.image.items,
						    &work.recipes, reserve, context, current,
						    &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = birth_source_intent_decode(work.intent_wire, &work.intent, reserve,
						    context, current);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!live.bytes(current))
			return error::capacity;
		status =
			birth_source_intent_verify(command, work.intent, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current,
					 sizeof(std::span<const native_mobile_birth_item_recipe>),
					 reserve, context))
			return error::capacity;
		work.recipe_values = std::span<const native_mobile_birth_item_recipe>(work.recipes);
		status = birth_recipe_command_build_fixed_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values,
			command.source_site, command.accepted_at_usec, &work.expected, reserve,
			context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (birth_source_canonical_encode(command, &work.actual_bytes, reserve, context,
						  current) != critical_command_codec_result::ok)
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		if (birth_source_canonical_encode(work.expected, &work.expected_bytes, reserve,
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

error native_mobile_birth_command_build_fixed_bounded(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const quest_mobile_native_constructor_recipe &constructor, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	const auto own_source_status = birth_source_admit_owned(
		birth_source_constructor_build_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

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
	if (const auto value_source_status = birth_source_retain_value_source(
		    economic_operation_metadata_validate_source_frame_bytes,
		    economic_operation_metadata_validate_source_query_frame_bytes(), reserve,
		    context, outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = birth_source_retain_value_source(
		    quest_mobile_native_reference_valid_source_frame_bytes,
		    quest_mobile_native_reference_valid_source_query_frame_bytes(), reserve,
		    context, outer);
	    value_source_status != error::ok)
		return value_source_status;

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
		auto status = birth_constructor_payload_encode_fixed_bounded(
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
		status = birth_source_intent_freeze(candidate, work.facts,
						    &candidate.accounting_intent, reserve, context,
						    current);
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

error native_mobile_birth_command_decode_v1_bounded(const critical_command &command,
						    quest_mobile_native_image *output,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer,
						    size_t *retained_image_heap) noexcept
{
	const auto own_source_status =
		birth_source_admit_owned(birth_source_v1_decode_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

	if (!output || command.payload_version != NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    !command.publication_required || !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
#if !(__cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) &&                     \
      defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
      _GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
      !defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                          \
      !defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__))
	(void)reserve;
	(void)context;
	(void)outer;
	(void)retained_image_heap;
	return error::unresolved;
#else
	if (const auto value_source_status = birth_source_retain_value_source(
		    quest_mobile_native_image_lifetime_source_frame_bytes,
		    quest_mobile_native_image_lifetime_source_query_frame_bytes(), reserve, context,
		    outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = birth_source_retain_value_source(
		    native_mobile_birth_recipe_value_lifecycle_source_frame_bytes,
		    native_mobile_birth_recipe_source_query_frame_bytes(), reserve, context, outer);
	    value_source_status != error::ok)
		return value_source_status;
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
		work.image_wire = work.payload;
		size_t current = base;
		auto status = birth_source_image_decode(work.image_wire, &work.image, reserve,
							context, current, &work.image_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = birth_source_intent_decode(work.intent_wire, &work.intent, reserve,
						    context, current);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!live.bytes(current))
			return error::capacity;
		status =
			birth_source_intent_verify(command, work.intent, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_v1_command_build_bounded(work.intent.admission.metadata, work.image,
							command.source_site,
							command.accepted_at_usec, &work.expected,
							reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (birth_source_canonical_encode(command, &work.actual_bytes, reserve, context,
						  current) != critical_command_codec_result::ok)
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		if (birth_source_canonical_encode(work.expected, &work.expected_bytes, reserve,
						  context,
						  current) != critical_command_codec_result::ok)
			return error::capacity;
		if (work.actual_bytes != work.expected_bytes)
			return error::payload_conflict;
		static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
		*output = std::move(work.image);
		if (retained_image_heap)
			*retained_image_heap = work.image_heap;
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
error native_mobile_birth_command_decode_constructor_fixed_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	quest_mobile_native_constructor_recipe *constructor_output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *retained_image_heap, size_t *retained_recipe_heap) noexcept
{
	const auto own_source_status = birth_source_admit_owned(
		birth_source_constructor_decode_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

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
	if (const auto value_source_status = birth_source_retain_value_source(
		    quest_mobile_native_image_lifetime_source_frame_bytes,
		    quest_mobile_native_image_lifetime_source_query_frame_bytes(), reserve, context,
		    outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = birth_source_retain_value_source(
		    native_mobile_birth_recipe_value_lifecycle_source_frame_bytes,
		    native_mobile_birth_recipe_source_query_frame_bytes(), reserve, context, outer);
	    value_source_status != error::ok)
		return value_source_status;
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
		auto status = birth_source_image_decode(work.image_wire, &work.image, reserve,
							context, current, &work.image_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = birth_source_recipe_decode(work.recipe_wire, work.image.items,
						    &work.recipes, reserve, context, current,
						    &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		{
			if (!birth_command_add(current, sizeof(birth_bool_codec_reservation)) ||
			    !birth_command_admit(current, 0, reserve, context))
				return error::capacity;
			size_t constructor_outer = 0;
			const auto constructor_preflight = birth_source_child_preflight(
				native_mobile_birth_constructor_recipe_own_source_frame_bytes,
				native_mobile_birth_constructor_recipe_initial_inline_bytes,
				nullptr, native_mobile_birth_constructor_recipe_query_frame_bytes(),
				current, reserve, context, &constructor_outer);
			if (constructor_preflight != error::ok)
				return constructor_preflight;
			birth_bool_codec_reservation reservation{ reserve, context };
			if (!native_mobile_birth_constructor_recipe_decode_fixed_bounded(
				    work.constructor_wire, &work.constructor,
				    birth_bool_codec_reservation::reserve, &reservation,
				    constructor_outer))
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
		status = birth_source_intent_decode(work.intent_wire, &work.intent, reserve,
						    context, current);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!live.bytes(current))
			return error::capacity;
		status =
			birth_source_intent_verify(command, work.intent, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current) ||
		    !birth_command_admit(current,
					 sizeof(std::span<const native_mobile_birth_item_recipe>),
					 reserve, context))
			return error::capacity;
		work.recipe_values = std::span<const native_mobile_birth_item_recipe>(work.recipes);
		status = native_mobile_birth_command_build_fixed_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values,
			work.constructor, command.source_site, command.accepted_at_usec,
			&work.expected, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (birth_source_canonical_encode(command, &work.actual_bytes, reserve, context,
						  current) != critical_command_codec_result::ok)
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		if (birth_source_canonical_encode(work.expected, &work.expected_bytes, reserve,
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

economic_accounting_error native_mobile_birth_command_historical_decode_fixed_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipes,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_image_heap_bytes, size_t *retained_recipe_heap_bytes) noexcept
{
	const auto own_source_status = birth_source_admit_owned(
		birth_source_historical_decode_owned, reserve, context, outer_live);
	if (own_source_status != error::ok)
		return own_source_status;

	if (command.payload_version != NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION &&
	    command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
		return error::corrupt_evidence;
	if (command.payload_version == NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION)
		return birth_recipe_command_decode_fixed_bounded(command, output, recipes, reserve,
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
	return native_mobile_birth_command_decode_constructor_fixed_bounded(
		command, output, recipes, &constructor, reserve, context, base,
		retained_image_heap_bytes, retained_recipe_heap_bytes);
}

bool native_mobile_birth_command_historical_decode_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = birth_historical_entry_frames;
	return true;
}

bool native_mobile_birth_command_historical_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	// Full actual prospective decode/rebuild/encode DTOs. These inline values
	// are admitted by each real child before construction; preflight is transient.
	*output = sizeof(quest_mobile_native_constructor_recipe) + sizeof(birth_decode_workspace) +
		  sizeof(birth_decode_live) + sizeof(birth_build_workspace) +
		  sizeof(birth_build_live) + sizeof(birth_payload_workspace) +
		  sizeof(birth_payload_live) + sizeof(birth_bool_codec_reservation) +
		  sizeof(std::span<const uint8_t>);
	return true;
}

bool native_mobile_birth_command_general_decode_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	// Pure prospective local call graph, not an additional retained outer term.
	constexpr size_t v1 = birth_v1_decode_named_frames + birth_v1_build_named_frames +
			      birth_v1_helper_named_frames;
	*output = birth_general_entry_frames +
		  (v1 > birth_historical_entry_frames ? v1 : birth_historical_entry_frames);
	return true;
}

bool native_mobile_birth_command_general_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	// Historical actual inline dominates v1: same decode/rebuild workspace,
	// plus discarded recipe vector and actual H100 constructor/output framing.
	*output = sizeof(std::vector<native_mobile_birth_item_recipe>) +
		  sizeof(quest_mobile_native_constructor_recipe) + sizeof(birth_decode_workspace) +
		  sizeof(birth_decode_live) + sizeof(birth_build_workspace) +
		  sizeof(birth_build_live) + sizeof(birth_payload_workspace) +
		  sizeof(birth_payload_live) + sizeof(birth_bool_codec_reservation) +
		  sizeof(std::span<const uint8_t>);
	return true;
}

economic_accounting_error
native_mobile_birth_command_decode_bounded(const critical_command &command,
					   quest_mobile_native_image *output,
					   bool (*reserve)(size_t, void *) noexcept, void *context,
					   size_t outer, size_t *retained_image_heap_bytes) noexcept
{
	const auto own_source_status = birth_source_admit_owned(birth_source_general_decode_owned,
								reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

	if (!output || command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    !native_mobile_birth_payload_version_supported(command.payload_version) ||
	    !command.publication_required || !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
	// Actual discarded recipe output lives to return, matching original general
	// decode_impl. Original constructor local is retained by the v3 dispatcher.
	size_t base = outer;
	if (command.payload_version == NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION)
		return native_mobile_birth_command_decode_v1_bounded(
			command, output, reserve, context, base, retained_image_heap_bytes);
	if (const auto value_source_status = birth_source_retain_value_source(
		    native_mobile_birth_recipe_value_lifecycle_source_frame_bytes,
		    native_mobile_birth_recipe_source_query_frame_bytes(), reserve, context, base);
	    value_source_status != error::ok)
		return value_source_status;
	if (!birth_command_add(base, sizeof(std::vector<native_mobile_birth_item_recipe>)) ||
	    !birth_command_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		std::vector<native_mobile_birth_item_recipe> recipes;
		size_t image_heap = 0;
		size_t recipe_heap = 0;
		const auto status = native_mobile_birth_command_historical_decode_fixed_bounded(
			command, output, &recipes, reserve, context, base, &image_heap,
			&recipe_heap);
		if (status != error::ok)
			return status;
		if (retained_image_heap_bytes)
			*retained_image_heap_bytes = image_heap;
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

namespace
{
bool birth_source_complete_profile(birth_source_route route, size_t *output) noexcept
{
	if (!output)
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&   \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                                \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	!defined(__SANITIZE_UNDEFINED__) &&                                                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8)
		return false;
	size_t total = 0, child = 0;
	switch (route)
	{
	case birth_source_route::v1_build:
		total = birth_source_v1_build_owned + 0;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::recipe_build:
		total = birth_source_recipe_build_owned + 0;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipeenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::constructor_build:
		total = birth_source_constructor_build_owned + 0;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_own_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      sizeof(error) + 2 * sizeof(bool) +
						      sizeof(void *) + sizeof(size_t) +
						      sizeof(bool)))
			return false; // actual ctor wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipeenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::v1_decode:
		total = birth_source_v1_decode_owned + birth_source_v1_build_owned;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_lifetime_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_verify_binding_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!critical_command_startup_codec_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imagedec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual intentdec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual verify wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total,
				       4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
					       sizeof(critical_command_codec_result) +
					       sizeof(void *) + sizeof(size_t) + sizeof(bool)))
			return false; // actual canonical wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::recipe_decode:
		total = birth_source_recipe_decode_owned + birth_source_recipe_build_owned;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_lifetime_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_verify_binding_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!critical_command_startup_codec_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imagedec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipeenc wrapper
		if (!birth_command_add(total, 6 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipedec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual intentdec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual verify wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total,
				       4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
					       sizeof(critical_command_codec_result) +
					       sizeof(void *) + sizeof(size_t) + sizeof(bool)))
			return false; // actual canonical wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::constructor_decode:
		total = birth_source_constructor_decode_owned +
			birth_source_constructor_build_owned;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_own_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_lifetime_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_verify_binding_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!critical_command_startup_codec_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      sizeof(error) + 2 * sizeof(bool) +
						      sizeof(void *) + sizeof(size_t) +
						      sizeof(bool)))
			return false; // actual ctor wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imagedec wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipeenc wrapper
		if (!birth_command_add(total, 6 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipedec wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual intentdec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual verify wrapper
		if (!birth_command_add(total,
				       4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
					       sizeof(critical_command_codec_result) +
					       sizeof(void *) + sizeof(size_t) + sizeof(bool)))
			return false; // actual canonical wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::historical_decode:
		total = birth_source_historical_decode_owned +
			birth_source_constructor_decode_owned +
			birth_source_constructor_build_owned;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_own_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_lifetime_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_verify_binding_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!critical_command_startup_codec_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      sizeof(error) + 2 * sizeof(bool) +
						      sizeof(void *) + sizeof(size_t) +
						      sizeof(bool)))
			return false; // actual ctor wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imagedec wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipeenc wrapper
		if (!birth_command_add(total, 6 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipedec wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual intentdec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual verify wrapper
		if (!birth_command_add(total,
				       4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
					       sizeof(critical_command_codec_result) +
					       sizeof(void *) + sizeof(size_t) + sizeof(bool)))
			return false; // actual canonical wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	case birth_source_route::general_decode:
		total = birth_source_general_decode_owned + birth_source_historical_decode_owned +
			birth_source_constructor_decode_owned +
			birth_source_constructor_build_owned + birth_source_v1_decode_owned +
			birth_source_v1_build_owned;
		if (!economic_operation_metadata_validate_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_reference_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_valid_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_own_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!quest_mobile_native_image_lifetime_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_encode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_decode_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!economic_intent_verify_binding_fixed_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!critical_command_startup_codec_source_frame_bytes(&child) ||
		    !birth_command_add(total, child))
			return false;
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      sizeof(error) + 2 * sizeof(bool) +
						      sizeof(void *) + sizeof(size_t) +
						      sizeof(bool)))
			return false; // actual ctor wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imageenc wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual imagedec wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipeenc wrapper
		if (!birth_command_add(total, 6 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual recipedec wrapper
		if (!birth_command_add(total, 5 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual freeze wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual intentdec wrapper
		if (!birth_command_add(total, 4 * sizeof(void *) + 4 * sizeof(size_t) +
						      2 * sizeof(error) + sizeof(void *) +
						      sizeof(size_t) + sizeof(bool)))
			return false; // actual verify wrapper
		if (!birth_command_add(total,
				       4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
					       sizeof(critical_command_codec_result) +
					       sizeof(void *) + sizeof(size_t) + sizeof(bool)))
			return false; // actual canonical wrapper
		if (!birth_command_add(total, critical_command_valid_frame_bytes()))
			return false;
		break;
	}
	// Every directly selected wrapper owns its genuine lexical source. Full
	// getters include those reached wrappers and their preflight helper here;
	// retained parent SOURCE excludes these wrapper-owned terms.
	// Exact generic child preflight, value-source relay and owning admission
	// helper declarations. Sequential scopes conservatively sum; no private
	// candidate or native/emitted stack allowance is fabricated.
	if (!birth_command_add(total, birth_source_child_preflight_frames + 4 * sizeof(void *) +
					      4 * sizeof(size_t) + 2 * sizeof(error) +
					      sizeof(void *) + sizeof(size_t) + sizeof(bool) +
					      3 * sizeof(void *) + 5 * sizeof(size_t) +
					      sizeof(error) + 2 * sizeof(bool) + sizeof(void *) +
					      sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) +
					      2 * sizeof(size_t) + sizeof(bool)))
		return false;
	*output = total;
	return true;
#else
	(void)route;
	return false;
#endif
}
} // namespace
bool native_mobile_birth_command_v1_decode_source_frame_bytes(size_t *output) noexcept
{
	return birth_source_complete_profile(birth_source_route::v1_decode, output);
}
bool native_mobile_birth_command_constructor_build_source_frame_bytes(size_t *output) noexcept
{
	return birth_source_complete_profile(birth_source_route::constructor_build, output);
}
bool native_mobile_birth_command_constructor_decode_source_frame_bytes(size_t *output) noexcept
{
	return birth_source_complete_profile(birth_source_route::constructor_decode, output);
}
bool native_mobile_birth_command_historical_decode_source_frame_bytes(size_t *output) noexcept
{
	return birth_source_complete_profile(birth_source_route::historical_decode, output);
}
bool native_mobile_birth_command_general_decode_source_frame_bytes(size_t *output) noexcept
{
	return birth_source_complete_profile(birth_source_route::general_decode, output);
}
