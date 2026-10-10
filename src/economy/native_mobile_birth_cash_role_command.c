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

namespace
{
using cash_source_source_query = bool (*)(size_t *) noexcept;
constexpr size_t cash_source_child_preflight_frames =
	// Six actual pointer parameters; query/outer and four size_t locals;
	// error result and the actual add/admit calls/boolean return carriers.
	6 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(error) + sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
error cash_source_child_preflight(cash_source_source_query full, cash_source_source_query initial,
				  cash_source_source_query supplement, size_t query_frames,
				  size_t outer, bool (*reserve)(size_t, void *) noexcept,
				  void *context, size_t *child_outer) noexcept
{
	size_t source = 0, entry = 0, retained = 0, request = outer;
	if (!cash_role_add(request, cash_source_child_preflight_frames) ||
	    !cash_role_add(request, query_frames) || !cash_role_add(request, sizeof(size_t)) ||
	    !cash_role_admit(request, 0, reserve, context))
		return error::capacity;
	if (!full || !child_outer || !full(&source) || (initial && !initial(&entry)) ||
	    (supplement && !supplement(&retained)))
		return error::unresolved;
	request = outer;
	if (!cash_role_add(request, cash_source_child_preflight_frames) ||
	    !cash_role_add(request, source) || !cash_role_add(request, entry) ||
	    !cash_role_admit(request, 0, reserve, context))
		return error::capacity;
	request = outer;
	if (!cash_role_add(request, retained))
		return error::capacity;
	*child_outer = request;
	return error::ok;
}
error cash_source_intent_decode(const std::span<const uint8_t> &input,
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
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
		economic_intent_decode_source_frame_bytes,
		economic_intent_decode_initial_inline_bytes,
		economic_intent_decode_source_supplement_frame_bytes,
		economic_intent_decode_source_query_frame_bytes(), base, reserve, context, &child);
	if (checked != error::ok)
		return checked;
	return economic_intent_decode_bounded(input, output, reserve, context, child);
}
error cash_source_intent_freeze(const critical_command &command,
				const economic_admission_facts &facts, std::vector<uint8_t> *output,
				bool (*reserve)(size_t, void *) noexcept, void *context,
				size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.
	constexpr size_t own = 5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
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
error cash_source_intent_verify(const critical_command &command,
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
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
		economic_intent_verify_binding_fixed_source_frame_bytes,
		economic_intent_verify_binding_fixed_initial_inline_bytes, nullptr,
		economic_intent_verify_binding_fixed_source_query_frame_bytes(), base, reserve,
		context, &child);
	if (checked != error::ok)
		return checked;
	return economic_intent_verify_binding_fixed_bounded(command, intent, reserve, context,
							    child);
}
} // namespace

namespace
{
// These pure local initial getters price the actual original encoder vector,
// not any encoded-length or baseline allowance. Query owns output P and B.
bool cash_source_canonical_initial(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = sizeof(std::vector<uint8_t>);
	return true;
}
error cash_source_retain_value_source(cash_source_source_query source, size_t query_frames,
				      bool (*reserve)(size_t, void *) noexcept, void *context,
				      size_t &outer) noexcept
{
	// Actual source/reserve/context/outer refs, query/base/child and returned
	// accessor N, status/result, plus checked-add source. Helper is transient.
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto status = cash_source_child_preflight(source, nullptr, source, query_frames, base,
							reserve, context, &child);
	if (status != error::ok)
		return status;
	// Exact just-added helper source is released on return, independently of
	// the genuine returned SOURCE supplement. No storage is observed here.
	outer = child - own;
	return error::ok;
}
critical_command_codec_result cash_source_canonical_encode(const critical_command &command,
							   std::vector<uint8_t> *output,
							   bool (*reserve)(size_t, void *) noexcept,
							   void *context, size_t outer) noexcept
{
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
			       sizeof(critical_command_codec_result) + sizeof(void *) +
			       sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return critical_command_codec_result::overflow;
	const auto checked = cash_source_child_preflight(
		critical_command_startup_codec_source_frame_bytes, cash_source_canonical_initial,
		critical_command_startup_codec_source_frame_bytes,
		critical_command_startup_codec_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return critical_command_codec_result::overflow;
	return critical_command_encode_bounded(command, output, reserve, context, child);
}
bool cash_source_constructor_encode(const quest_mobile_native_constructor_recipe &input,
				    std::vector<uint8_t> *output,
				    bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer) noexcept
{
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
			       2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return false;
	const auto checked = cash_source_child_preflight(
		native_mobile_birth_constructor_recipe_own_source_frame_bytes,
		native_mobile_birth_constructor_recipe_initial_inline_bytes, nullptr,
		native_mobile_birth_constructor_recipe_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return false;
	return native_mobile_birth_constructor_recipe_encode_blob_fixed_bounded(
		input, output, reserve, context, child);
}
error cash_source_role_encode(const native_mobile_birth_cash_role_recipe &input,
			      native_mobile_birth_cash_role_recipe_bytes *output,
			      bool (*reserve)(size_t, void *) noexcept, void *context,
			      size_t outer) noexcept
{
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
		native_mobile_birth_cash_role_recipe_source_frame_bytes,
		native_mobile_birth_cash_role_recipe_initial_inline_bytes, nullptr,
		native_mobile_birth_cash_role_recipe_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return native_mobile_birth_cash_role_recipe_encode_fixed_bounded(input, output, reserve,
									 context, child);
}
error cash_source_role_decode(const std::span<const uint8_t> &input,
			      native_mobile_birth_cash_role_recipe *output,
			      bool (*reserve)(size_t, void *) noexcept, void *context,
			      size_t outer) noexcept
{
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
		native_mobile_birth_cash_role_recipe_source_frame_bytes,
		native_mobile_birth_cash_role_recipe_initial_inline_bytes, nullptr,
		native_mobile_birth_cash_role_recipe_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return native_mobile_birth_cash_role_recipe_decode_fixed_bounded(input, output, reserve,
									 context, child);
}
} // namespace

namespace
{
// Decode: actual8pointer parameters, outer/base/current/copied/original/role
// sizes; bytes/original aliases; lower_bound result; status/return/query locals.
constexpr size_t cash_role_fixed_decode_entry_frames =
	11 * sizeof(void *) + 7 * sizeof(size_t) + 2 * sizeof(error) + sizeof(critical_entity_key) +
	sizeof(size_t *) + sizeof(bool);
// Build/bind actual arguments and named scalar aliases. All lower library,
// iterator, recipe, copy/move/cleanup and hash chains are separate SOURCE gates.
constexpr size_t cash_role_fixed_build_entry_frames =
	9 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(uint64_t) + sizeof(critical_source_site) +
	2 * sizeof(error) + sizeof(bool);
constexpr size_t cash_role_fixed_bind_entry_frames =
	4 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(error) + sizeof(bool);
}

bool native_mobile_birth_cash_role_command_decode_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = cash_role_fixed_decode_entry_frames + cash_role_fixed_bind_entry_frames;
	return true;
}

bool native_mobile_birth_cash_role_command_build_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = cash_role_fixed_build_entry_frames + cash_role_fixed_bind_entry_frames;
	return true;
}

bool native_mobile_birth_cash_role_command_build_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	*output = sizeof(cash_role_build_workspace) + sizeof(cash_role_build_live);
	return true;
}

bool native_mobile_birth_cash_role_command_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	// Real owned decode and nested cash-role rebuild inline values only. Further
	// genuine nested v3/image/intent initial inline is queried separately by ROOT.
	*output = sizeof(cash_role_decode_workspace) + sizeof(cash_role_decode_live) +
		  sizeof(cash_role_build_workspace) + sizeof(cash_role_build_live) +
		  sizeof(std::span<const uint8_t>);
	return true;
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
constexpr size_t cash_source_build_owned =
	birth_own_cash_build +
	// Actual params and candidate alias, outer/base/current/payload/extra/growth.
	8 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(uint64_t) + sizeof(critical_source_site) +
	2 * sizeof(error) + sizeof(bool) + cash_role_fixed_bind_entry_frames +
	// The actual role validation/query/error temporary at the owning entry.
	sizeof(size_t) + sizeof(error) + sizeof(bool);
constexpr size_t cash_source_decode_owned =
	birth_own_cash_decode +
	// Actual8args+bytes/original/found aliases; six scalar locals, status/result.
	// The actual shop key is already admitted by the original body before use.
	11 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(error) + sizeof(bool) +
	cash_role_fixed_bind_entry_frames + sizeof(size_t) + sizeof(error) + sizeof(bool);
error cash_source_admit_owned(size_t own, bool (*reserve)(size_t, void *) noexcept, void *context,
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
	if (!cash_role_add(request, helper) || !cash_role_add(request, own) ||
	    !cash_role_add(request, sizeof(size_t)) ||
	    !cash_role_admit(request, 0, reserve, context))
		return error::capacity;
	// The actual entry uses the original envelope/ID/key predicates. Their
	// exported genuine validation graph dominates those direct calls; it is
	// retained, independently of owned workspace objects and capacities.
	valid = critical_command_valid_frame_bytes();
	request = outer;
	if (!cash_role_add(request, own) || !cash_role_add(request, valid))
		return error::capacity;
	size_t temporary = request;
	if (!cash_role_add(temporary, helper) || !cash_role_admit(temporary, 0, reserve, context))
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
error cash_source_original_build(const economic_operation_metadata &metadata,
				 const quest_mobile_native_image &image,
				 const std::span<const native_mobile_birth_item_recipe> &recipes,
				 const quest_mobile_native_constructor_recipe &constructor,
				 critical_source_site site, uint64_t at, critical_command *output,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	constexpr size_t own = 7 * sizeof(void *) + 4 * sizeof(size_t) +
			       sizeof(critical_source_site) + sizeof(uint64_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
		native_mobile_birth_command_constructor_build_source_frame_bytes,
		native_mobile_birth_command_constructor_build_initial_inline_bytes, nullptr,
		native_mobile_birth_command_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return native_mobile_birth_command_build_fixed_bounded(
		metadata, image, recipes, constructor, site, at, output, reserve, context, child);
}
error cash_source_original_decode(const critical_command &command,
				  quest_mobile_native_image *output,
				  std::vector<native_mobile_birth_item_recipe> *recipes,
				  quest_mobile_native_constructor_recipe *constructor,
				  bool (*reserve)(size_t, void *) noexcept, void *context,
				  size_t outer, size_t *image_heap, size_t *recipe_heap) noexcept
{
	constexpr size_t own = 8 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer, child = 0;
	if (!cash_role_add(base, own))
		return error::capacity;
	const auto checked = cash_source_child_preflight(
		native_mobile_birth_command_constructor_decode_source_frame_bytes,
		native_mobile_birth_command_historical_decode_initial_inline_bytes, nullptr,
		native_mobile_birth_command_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return native_mobile_birth_command_decode_constructor_fixed_bounded(
		command, output, recipes, constructor, reserve, context, child, image_heap,
		recipe_heap);
}
} // namespace
error cash_role_bind_intent_fixed_bounded(critical_command &candidate,
					  const economic_admission_facts &facts,
					  cash_role_reserve_fn reserve, void *context,
					  size_t outer) noexcept
{
	candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	candidate.accounting_intent.clear();
	candidate.publication_required = false;
	const auto status = cash_source_intent_freeze(
		candidate, facts, &candidate.accounting_intent, reserve, context, outer);
	if (status != error::ok)
		return status;
	candidate.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	candidate.publication_required = true;
	return critical_command_envelope_valid(candidate) ? error::ok : error::corrupt_evidence;
}

} // namespace

error native_mobile_birth_cash_role_command_build_fixed_bounded(
	const economic_operation_metadata &metadata, const quest_mobile_native_image &image,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const native_mobile_birth_cash_role_recipe &role, critical_source_site site,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	const auto own_source_status =
		cash_source_admit_owned(cash_source_build_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

	if (const auto value_source_status = cash_source_retain_value_source(
		    native_mobile_birth_constructor_recipe_valid_source_frame_bytes,
		    native_mobile_birth_constructor_recipe_query_frame_bytes(), reserve, context,
		    outer);
	    value_source_status != error::ok)
		return value_source_status;
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
		auto status =
			cash_source_role_encode(role, &work.role_bytes, reserve, context, base);
		if (status != error::ok)
			return status;
		status = cash_source_original_build(metadata, image, recipes, role.original, site,
						    accepted_at_usec, &candidate, reserve, context,
						    base);
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
		status = cash_role_bind_intent_fixed_bounded(candidate, work.facts, reserve,
							     context, current);
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

economic_accounting_error native_mobile_birth_cash_role_command_decode_fixed_bounded(
	const critical_command &command, quest_mobile_native_image *output,
	std::vector<native_mobile_birth_item_recipe> *recipe_output,
	native_mobile_birth_cash_role_recipe *role_output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer, size_t *retained_image_heap,
	size_t *retained_recipe_heap) noexcept
{
	const auto own_source_status =
		cash_source_admit_owned(cash_source_decode_owned, reserve, context, outer);
	if (own_source_status != error::ok)
		return own_source_status;

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
	if (const auto value_source_status = cash_source_retain_value_source(
		    quest_mobile_native_image_lifetime_source_frame_bytes,
		    quest_mobile_native_image_lifetime_source_query_frame_bytes(), reserve, context,
		    outer);
	    value_source_status != error::ok)
		return value_source_status;
	if (const auto value_source_status = cash_source_retain_value_source(
		    native_mobile_birth_recipe_value_lifecycle_source_frame_bytes,
		    native_mobile_birth_recipe_source_query_frame_bytes(), reserve, context, outer);
	    value_source_status != error::ok)
		return value_source_status;
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
		auto status = cash_source_role_decode(work.embedded_role, &work.role, reserve,
						      context, base);
		if (status != error::ok)
			return status;
		size_t current = 0;
		if (!live.bytes(current) ||
		    !cash_role_admit(current, sizeof(std::span<const uint8_t>), reserve, context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = cash_source_intent_decode(work.intent_wire, &work.intent, reserve, context,
						   current);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!live.bytes(current))
			return error::capacity;
		status = cash_source_intent_verify(command, work.intent, reserve, context, current);
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
		status = cash_role_bind_intent_fixed_bounded(original, work.intent.admission,
							     reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = cash_source_original_decode(original, &work.image, &work.recipes,
						     &work.constructor, reserve, context, current,
						     &work.image_heap, &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (!cash_source_constructor_encode(work.constructor, &work.constructor_bytes,
						    reserve, context, current))
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
		status = native_mobile_birth_cash_role_command_build_fixed_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values, work.role,
			command.source_site, command.accepted_at_usec, &work.expected, reserve,
			context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (cash_source_canonical_encode(command, &work.actual_bytes, reserve, context,
						 current) != critical_command_codec_result::ok)
			return error::capacity;
		if (!live.bytes(current))
			return error::capacity;
		if (cash_source_canonical_encode(work.expected, &work.expected_bytes, reserve,
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
namespace
{
bool cash_source_complete_profile(bool decode, size_t *output) noexcept
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
	size_t total = decode ? cash_source_decode_owned : cash_source_build_owned;
	size_t child = 0;
	if (!native_mobile_birth_cash_role_recipe_source_frame_bytes(&child) ||
	    !cash_role_add(total, child))
		return false;
	if (!native_mobile_birth_constructor_recipe_valid_source_frame_bytes(&child) ||
	    !cash_role_add(total, child))
		return false;
	if (!economic_intent_freeze_fixed_source_frame_bytes(&child) ||
	    !cash_role_add(total, child))
		return false;
	if (!native_mobile_birth_command_constructor_build_source_frame_bytes(&child) ||
	    !cash_role_add(total, child))
		return false;
	if (decode)
	{
		if (!native_mobile_birth_command_constructor_decode_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		if (!quest_mobile_native_image_lifetime_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		if (!native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		if (!native_mobile_birth_constructor_recipe_own_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		if (!economic_intent_decode_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		if (!economic_intent_verify_binding_fixed_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		if (!critical_command_startup_codec_source_frame_bytes(&child) ||
		    !cash_role_add(total, child))
			return false;
		// Actual original-decode wrapper, constructor encode, canonical
		// encode and intent decode/proof wrapper declarations (sequential sum).
		if (!cash_role_add(total,
				   8 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
					   4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(error) +
					   2 * sizeof(bool) + 4 * sizeof(void *) +
					   4 * sizeof(size_t) + sizeof(error) +
					   sizeof(critical_command_codec_result) +
					   2 * (4 * sizeof(void *) + 4 * sizeof(size_t) +
						2 * sizeof(error)) +
					   5 * (sizeof(void *) + sizeof(size_t) + sizeof(bool))))
			return false;
	}
	// Actual original-build, role encode/decode and freeze wrapper fields;
	// each self-owned wrapper source is prospective only in this pure getter.
	if (!cash_role_add(total,
			   7 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(critical_source_site) +
				   sizeof(uint64_t) + 2 * sizeof(error) + 4 * sizeof(void *) +
				   4 * sizeof(size_t) + 2 * sizeof(error) + 5 * sizeof(void *) +
				   4 * sizeof(size_t) + 2 * sizeof(error) +
				   3 * (sizeof(void *) + sizeof(size_t) + sizeof(bool))))
		return false;
	if (!cash_role_add(total, critical_command_valid_frame_bytes()) ||
	    !cash_role_add(
		    total,
		    cash_source_child_preflight_frames +
			    // Genuine value-source and owning-admission helper scalar/query frames.
			    4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(error) +
			    sizeof(void *) + sizeof(size_t) + sizeof(bool) + 3 * sizeof(void *) +
			    5 * sizeof(size_t) + sizeof(error) + 2 * sizeof(bool) + sizeof(void *) +
			    sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) +
			    2 * sizeof(size_t) + sizeof(bool)))
		return false;
	*output = total;
	return true;
#else
	(void)decode;
	return false;
#endif
}
} // namespace
bool native_mobile_birth_cash_role_command_build_source_frame_bytes(size_t *output) noexcept
{
	return cash_source_complete_profile(false, output);
}
bool native_mobile_birth_cash_role_command_decode_source_frame_bytes(size_t *output) noexcept
{
	return cash_source_complete_profile(true, output);
}
