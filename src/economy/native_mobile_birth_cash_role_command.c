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
