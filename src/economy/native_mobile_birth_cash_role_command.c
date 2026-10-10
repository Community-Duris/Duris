#include "economy/native_mobile_birth_cash_role_command.h"

#include "item/item_transfer_command.h"

#include <algorithm>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
using error = economic_accounting_error;
constexpr std::array<uint8_t, 4> magic{ 'N', 'M', 'B', '4' };
constexpr size_t header_bytes = 12;
constexpr size_t fixed_bytes = header_bytes + NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES;
static_assert(fixed_bytes < CRITICAL_COMMAND_MAX_PAYLOAD_BYTES);

void append_u32(std::vector<uint8_t> &bytes, uint32_t value)
{
	for (unsigned shift = 0; shift < 32; shift += 8)
		bytes.push_back(static_cast<uint8_t>(value >> shift));
}
uint32_t read_u32(std::span<const uint8_t> bytes, size_t offset) noexcept
{
	uint32_t value = 0;
	for (size_t i = 0; i < 4; ++i)
		value |= static_cast<uint32_t>(bytes[offset + i]) << (8 * i);
	return value;
}

// Rebinding is structural validation of the supplied complete body, never an
// admission step. Keep original metadata and every original expected revision.
error bind_intent(critical_command &candidate, const economic_admission_facts &facts)
{
	candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	candidate.accounting_intent.clear();
	candidate.publication_required = false;
	auto status = economic_intent_freeze(candidate, facts, &candidate.accounting_intent);
	if (status != error::ok)
		return status;
	candidate.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	candidate.publication_required = true;
	return critical_command_envelope_valid(candidate) ? error::ok : error::corrupt_evidence;
}

critical_entity_key selected_shop(const native_mobile_birth_cash_role_recipe &role) noexcept
{
	// Called only after accepted NBC4 shape proves one nonnegative actual slot.
	return { critical_entity_type::shopkeeper,
		 item_shopkeeper_owner_id(static_cast<uint32_t>(role.original.reset_shop_index)) };
}
}

economic_accounting_error native_mobile_birth_cash_role_command_build(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	std::span<const native_mobile_birth_item_recipe> recipes,
	const native_mobile_birth_cash_role_recipe &role, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(role))
		return error::corrupt_evidence;
	try
	{
		native_mobile_birth_cash_role_recipe_bytes role_bytes;
		if (!native_mobile_birth_cash_role_recipe_encode(role, &role_bytes))
			return error::capacity;
		critical_command candidate;
		auto status = native_mobile_birth_command_build(metadata, image, recipes,
								role.original, site,
								accepted_at_usec, &candidate);
		if (status != error::ok)
			return status;
		// Preserve the complete original canonical NMB3 payload; NBC4 repeats
		// its exact NBC3 capsule rather than dropping any original evidence.
		if (candidate.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed_bytes)
			return error::capacity;
		std::vector<uint8_t> payload;
		payload.reserve(fixed_bytes + candidate.payload.size());
		payload.insert(payload.end(), magic.begin(), magic.end());
		append_u32(payload, static_cast<uint32_t>(candidate.payload.size()));
		append_u32(payload, static_cast<uint32_t>(role_bytes.size()));
		payload.insert(payload.end(), candidate.payload.begin(), candidate.payload.end());
		payload.insert(payload.end(), role_bytes.begin(), role_bytes.end());
		candidate.payload_version = NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION;
		candidate.payload = std::move(payload);
		if (role.role == native_mobile_birth_cash_role::shared_shopkeeper)
		{
			if (candidate.keys.size() >= CRITICAL_COMMAND_MAX_KEYS)
				return error::capacity;
			candidate.keys.push_back(selected_shop(role));
			std::sort(candidate.keys.begin(), candidate.keys.end(),
				  critical_entity_key_less);
			// NBC4 has no current shop-row revision. Do not add expected zero
			// or infer absence: the genuine atomic participant supplies its CAS.
		}
		economic_admission_facts facts;
		facts.metadata = metadata;
		status = bind_intent(candidate, facts);
		if (status != error::ok)
			return status;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
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

economic_accounting_error native_mobile_birth_cash_role_command_decode(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	native_mobile_birth_cash_role_recipe *role_output) noexcept
{
	if (!output || !recipe_output || !role_output ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
	    !command.publication_required || !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
	try
	{
		const std::span<const uint8_t> bytes(command.payload);
		if (bytes.size() < fixed_bytes ||
		    !std::equal(magic.begin(), magic.end(), bytes.begin()))
			return error::corrupt_evidence;
		const size_t original_size = read_u32(bytes, 4);
		const size_t role_size = read_u32(bytes, 8);
		if (!original_size || role_size != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES ||
		    original_size != bytes.size() - fixed_bytes)
			return error::corrupt_evidence;
		native_mobile_birth_cash_role_recipe role;
		const auto embedded_role = bytes.subspan(header_bytes + original_size, role_size);
		if (!native_mobile_birth_cash_role_recipe_decode(embedded_role, &role))
			return error::corrupt_evidence;
		economic_frozen_intent intent;
		auto status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		if (intent.admission.facts_version != 1 || !intent.admission.facts.empty())
			return error::payload_conflict;
		status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;

		// Only after full outer binding proof, project original payload/keys
		// and bind its metadata separately for unchanged v3 structural checks.
		critical_command original = command;
		original.payload_version = NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION;
		original.payload.assign(bytes.begin() + header_bytes,
					bytes.begin() + header_bytes + original_size);
		if (role.role == native_mobile_birth_cash_role::shared_shopkeeper)
		{
			const auto shop = selected_shop(role);
			const auto found = std::lower_bound(original.keys.begin(),
							    original.keys.end(), shop,
							    critical_entity_key_less);
			if (found == original.keys.end() ||
			    !critical_entity_key_equal(*found, shop))
				return error::payload_conflict;
			original.keys.erase(found);
		}
		status = bind_intent(original, intent.admission);
		if (status != error::ok)
			return status;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		quest_mobile_native_constructor_recipe constructor;
		status = native_mobile_birth_command_decode(original, &image, &recipes,
							    &constructor);
		if (status != error::ok)
			return status;
		std::vector<uint8_t> constructor_bytes;
		if (!native_mobile_birth_constructor_recipe_encode_blob(constructor,
									&constructor_bytes))
			return error::capacity;
		if (constructor_bytes.size() !=
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES ||
		    !std::equal(constructor_bytes.begin(), constructor_bytes.end(),
				embedded_role.begin() + 8))
			return error::payload_conflict;

		critical_command expected;
		status = native_mobile_birth_cash_role_command_build(
			intent.admission.metadata, image, recipes, role, command.source_site,
			command.accepted_at_usec, &expected);
		if (status != error::ok)
			return status;
		std::vector<uint8_t> actual_bytes, expected_bytes;
		if (critical_command_encode(command, &actual_bytes) !=
			    critical_command_codec_result::ok ||
		    critical_command_encode(expected, &expected_bytes) !=
			    critical_command_codec_result::ok)
			return error::capacity;
		if (actual_bytes != expected_bytes)
			return error::payload_conflict;
		static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
		static_assert(std::is_nothrow_move_assignable_v<
			      std::vector<native_mobile_birth_item_recipe>>);
		static_assert(
			std::is_nothrow_move_assignable_v<native_mobile_birth_cash_role_recipe>);
		*role_output = std::move(role);
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

#include <type_traits>

namespace
{
using cash_role_reserve_fn = bool (*)(size_t, void *) noexcept;
bool cash_role_add(size_t &bytes, size_t added) noexcept
{
	if (added > SIZE_MAX - bytes)
		return false;
	bytes += added;
	return true;
}
bool cash_role_admit(size_t base, size_t extra, cash_role_reserve_fn reserve,
		     void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}
bool cash_role_heap(const critical_command &command, size_t &bytes, bool copied = false) noexcept
{
	bytes = 0;
	const size_t keys = copied ? command.keys.size() : command.keys.capacity();
	const size_t revisions = copied ? command.expected_revisions.size() :
					  command.expected_revisions.capacity();
	if (keys > SIZE_MAX / sizeof(critical_entity_key) ||
	    revisions > SIZE_MAX / sizeof(critical_expected_revision))
		return false;
	return cash_role_add(bytes, keys * sizeof(critical_entity_key)) &&
	       cash_role_add(bytes, revisions * sizeof(critical_expected_revision)) &&
	       cash_role_add(bytes, copied ? command.payload.size() : command.payload.capacity()) &&
	       cash_role_add(bytes, copied ? command.accounting_intent.size() :
					     command.accounting_intent.capacity());
}
struct cash_role_bool_reservation
{
	cash_role_reserve_fn callback;
	void *context;
	bool refused = false;
	static bool reserve(size_t bytes, void *opaque) noexcept
	{
		auto &state = *static_cast<cash_role_bool_reservation *>(opaque);
		if (!state.callback || !state.callback(bytes, state.context))
		{
			state.refused = true;
			return false;
		}
		return true;
	}
};

error cash_role_recipe_encode_bounded(const native_mobile_birth_cash_role_recipe &role,
				      native_mobile_birth_cash_role_recipe_bytes *output,
				      cash_role_reserve_fn reserve, void *context,
				      size_t outer) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(role))
		return error::corrupt_evidence;
	struct workspace
	{
		std::vector<uint8_t> original;
		native_mobile_birth_cash_role_recipe_bytes candidate{};
	};
	size_t base = outer;
	if (!cash_role_add(base, sizeof(workspace)) || !cash_role_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		workspace work;
		if (!native_mobile_birth_constructor_recipe_encode_blob_bounded(
			    role.original, &work.original, reserve, context, base) ||
		    work.original.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)
			return error::capacity;
		work.candidate[0] = 'N';
		work.candidate[1] = 'B';
		work.candidate[2] = 'C';
		work.candidate[3] = '4';
		work.candidate[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION;
		std::copy(work.original.begin(), work.original.end(), work.candidate.begin() + 8);
		constexpr size_t role_offset =
			8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
		work.candidate[role_offset] = static_cast<uint8_t>(role.role);
		for (size_t i = 0; i < 4; ++i)
			work.candidate[role_offset + 1 + i] =
				static_cast<uint8_t>(role.configured_shop_matches >> (8 * i));
		*output = work.candidate;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

error cash_role_recipe_decode_bounded(const std::span<const uint8_t> &bytes,
				      native_mobile_birth_cash_role_recipe *output,
				      cash_role_reserve_fn reserve, void *context,
				      size_t outer) noexcept
{
	if (!output || bytes.size() != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES ||
	    bytes[0] != 'N' || bytes[1] != 'B' || bytes[2] != 'C' || bytes[3] != '4' ||
	    bytes[4] != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION || bytes[5] || bytes[6] ||
	    bytes[7])
		return error::corrupt_evidence;
	struct workspace
	{
		native_mobile_birth_cash_role_recipe candidate;
		std::span<const uint8_t> original;
		cash_role_bool_reservation reservation;
	};
	size_t base = outer;
	if (!cash_role_add(base, sizeof(workspace)) ||
	    !cash_role_admit(base, sizeof(std::span<const uint8_t>) + sizeof(error), reserve,
			     context))
		return error::capacity;
	workspace work{ {}, {}, { reserve, context } };
	work.original = bytes.subspan(8, NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES);
	const auto decoded = native_mobile_birth_constructor_recipe_decode_status_bounded(
		work.original, &work.candidate.original, cash_role_bool_reservation::reserve,
		&work.reservation, base);
	if (decoded != error::ok)
		return decoded;
	constexpr size_t role_offset = 8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
	work.candidate.role = static_cast<native_mobile_birth_cash_role>(bytes[role_offset]);
	work.candidate.configured_shop_matches = 0;
	for (size_t i = 0; i < 4; ++i)
		work.candidate.configured_shop_matches |= uint32_t(bytes[role_offset + 1 + i])
							  << (8 * i);
	if (!native_mobile_birth_cash_role_recipe_valid(work.candidate))
		return error::corrupt_evidence;
	*output = work.candidate;
	return error::ok;
}

error cash_role_bind_intent_bounded(critical_command &candidate,
				    const economic_admission_facts &facts,
				    cash_role_reserve_fn reserve, void *context,
				    size_t outer) noexcept
{
	candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	candidate.accounting_intent.clear();
	candidate.publication_required = false;
	const auto status = economic_intent_freeze_bounded(
		candidate, facts, &candidate.accounting_intent, reserve, context, outer);
	if (status != error::ok)
		return status;
	candidate.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	candidate.publication_required = true;
	return critical_command_envelope_valid(candidate) ? error::ok : error::corrupt_evidence;
}

struct cash_role_build_workspace
{
	native_mobile_birth_cash_role_recipe_bytes role_bytes;
	critical_command candidate;
	std::vector<uint8_t> payload;
	economic_admission_facts facts;
};
struct cash_role_build_live
{
	cash_role_build_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t heap = 0;
		return cash_role_heap(work.candidate, heap) && cash_role_add(out, heap) &&
		       cash_role_add(out, work.payload.capacity()) &&
		       cash_role_add(out, work.facts.facts.capacity());
	}
};
} // namespace

error native_mobile_birth_cash_role_command_build_bounded(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const native_mobile_birth_cash_role_recipe &role, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(role))
		return error::corrupt_evidence;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)metadata;
	(void)image;
	(void)recipes;
	(void)site;
	(void)accepted_at_usec;
	(void)reserve;
	(void)context;
	(void)outer;
	return error::unresolved;
#else
	size_t base = outer;
	if (!cash_role_add(base, sizeof(cash_role_build_workspace)) ||
	    !cash_role_add(base, sizeof(cash_role_build_live)) ||
	    !cash_role_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		cash_role_build_workspace work;
		cash_role_build_live live{ work, base };
		auto &candidate = work.candidate;
		auto status = cash_role_recipe_encode_bounded(role, &work.role_bytes, reserve,
							      context, base);
		if (status != error::ok)
			return status;
		status = native_mobile_birth_command_build_bounded(metadata, image, recipes,
								   role.original, site,
								   accepted_at_usec, &candidate,
								   reserve, context, base);
		if (status != error::ok)
			return status;
		if (candidate.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed_bytes)
			return error::capacity;
		size_t current = 0;
		const size_t payload_size = fixed_bytes + candidate.payload.size();
		if (!live.bytes(current) ||
		    !cash_role_admit(current, payload_size, reserve, context))
			return error::capacity;
		work.payload.reserve(payload_size);
		work.payload.insert(work.payload.end(), magic.begin(), magic.end());
		append_u32(work.payload, static_cast<uint32_t>(candidate.payload.size()));
		append_u32(work.payload, static_cast<uint32_t>(work.role_bytes.size()));
		work.payload.insert(work.payload.end(), candidate.payload.begin(),
				    candidate.payload.end());
		work.payload.insert(work.payload.end(), work.role_bytes.begin(),
				    work.role_bytes.end());
		candidate.payload_version = NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION;
		candidate.payload = std::move(work.payload);
		if (role.role == native_mobile_birth_cash_role::shared_shopkeeper)
		{
			if (candidate.keys.size() >= CRITICAL_COMMAND_MAX_KEYS)
				return error::capacity;
			size_t extra = sizeof(critical_entity_key);
			if (candidate.keys.size() == candidate.keys.capacity())
			{
				const size_t growth = std::max(candidate.keys.size(), size_t{ 1 });
				if (growth > SIZE_MAX - candidate.keys.size() ||
				    candidate.keys.size() + growth >
					    (SIZE_MAX - extra) / sizeof(critical_entity_key))
					return error::capacity;
				extra += (candidate.keys.size() + growth) *
					 sizeof(critical_entity_key);
			}
			if (!live.bytes(current) ||
			    !cash_role_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.keys.push_back(selected_shop(role));
			std::sort(candidate.keys.begin(), candidate.keys.end(),
				  critical_entity_key_less);
		}
		work.facts.metadata = metadata;
		if (!live.bytes(current))
			return error::capacity;
		status = cash_role_bind_intent_bounded(candidate, work.facts, reserve, context,
						       current);
		if (status != error::ok)
			return status;
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
struct cash_role_decode_workspace
{
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	critical_command original, expected;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	quest_mobile_native_constructor_recipe constructor;
	std::vector<uint8_t> constructor_bytes, actual_bytes, expected_bytes;
	std::span<const uint8_t> bytes, embedded_role, intent_wire;
	std::span<const native_mobile_birth_item_recipe> recipe_values;
	size_t image_heap = 0, recipe_heap = 0;
};
struct cash_role_decode_live
{
	cash_role_decode_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t original_heap = 0, expected_heap = 0;
		return cash_role_heap(work.original, original_heap) &&
		       cash_role_heap(work.expected, expected_heap) &&
		       cash_role_add(out, original_heap) && cash_role_add(out, expected_heap) &&
		       cash_role_add(out, work.image_heap) &&
		       cash_role_add(out, work.recipe_heap) &&
		       cash_role_add(out, work.intent.admission.facts.capacity()) &&
		       cash_role_add(out, work.constructor_bytes.capacity()) &&
		       cash_role_add(out, work.actual_bytes.capacity()) &&
		       cash_role_add(out, work.expected_bytes.capacity());
	}
};
}

economic_accounting_error native_mobile_birth_cash_role_command_decode_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	native_mobile_birth_cash_role_recipe *role_output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer, size_t *retained_image_heap,
	size_t *retained_recipe_heap) noexcept
{
	if (!output || !recipe_output || !role_output ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::native_mobile_birth ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
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
	if (!cash_role_add(base, sizeof(cash_role_decode_workspace)) ||
	    !cash_role_add(base, sizeof(cash_role_decode_live)) ||
	    !cash_role_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	try
	{
		cash_role_decode_workspace work;
		cash_role_decode_live live{ work, base };
		work.bytes = std::span<const uint8_t>(command.payload);
		const auto &bytes = work.bytes;
		if (bytes.size() < fixed_bytes ||
		    !std::equal(magic.begin(), magic.end(), bytes.begin()))
			return error::corrupt_evidence;
		const size_t original_size = read_u32(bytes, 4);
		const size_t role_size = read_u32(bytes, 8);
		if (!original_size || role_size != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES ||
		    original_size != bytes.size() - fixed_bytes)
			return error::corrupt_evidence;
		work.embedded_role = bytes.subspan(header_bytes + original_size, role_size);
		auto status = cash_role_recipe_decode_bounded(work.embedded_role, &work.role,
							      reserve, context, base);
		if (status != error::ok)
			return status;
		size_t current = 0;
		if (!live.bytes(current) ||
		    !cash_role_admit(current, sizeof(std::span<const uint8_t>), reserve, context))
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
		size_t copied_heap = 0;
		if (!cash_role_heap(command, copied_heap, true) || !live.bytes(current) ||
		    !cash_role_admit(current, copied_heap, reserve, context))
			return error::capacity;
		work.original = command;
		auto &original = work.original;
		original.payload_version = NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION;
		original.payload.assign(bytes.begin() + header_bytes,
					bytes.begin() + header_bytes + original_size);
		if (work.role.role == native_mobile_birth_cash_role::shared_shopkeeper)
		{
			if (!live.bytes(current) ||
			    !cash_role_admit(current, sizeof(critical_entity_key), reserve,
					     context))
				return error::capacity;
			const auto shop = selected_shop(work.role);
			const auto found = std::lower_bound(original.keys.begin(),
							    original.keys.end(), shop,
							    critical_entity_key_less);
			if (found == original.keys.end() ||
			    !critical_entity_key_equal(*found, shop))
				return error::payload_conflict;
			original.keys.erase(found);
		}
		if (!live.bytes(current))
			return error::capacity;
		status = cash_role_bind_intent_bounded(original, work.intent.admission, reserve,
						       context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = native_mobile_birth_command_decode_bounded(
			original, &work.image, &work.recipes, &work.constructor, reserve, context,
			current, &work.image_heap, &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (!native_mobile_birth_constructor_recipe_encode_blob_bounded(
			    work.constructor, &work.constructor_bytes, reserve, context, current))
			return error::capacity;
		if (work.constructor_bytes.size() !=
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES ||
		    !std::equal(work.constructor_bytes.begin(), work.constructor_bytes.end(),
				work.embedded_role.begin() + 8))
			return error::payload_conflict;
		if (!live.bytes(current) ||
		    !cash_role_admit(current,
				     sizeof(std::span<const native_mobile_birth_item_recipe>),
				     reserve, context))
			return error::capacity;
		work.recipe_values = std::span<const native_mobile_birth_item_recipe>(work.recipes);
		status = native_mobile_birth_cash_role_command_build_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values, work.role,
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
		static_assert(
			std::is_nothrow_move_assignable_v<native_mobile_birth_cash_role_recipe>);
		*role_output = std::move(work.role);
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
