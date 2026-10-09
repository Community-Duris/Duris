#ifndef SMITH_NATIVE_COMMAND_H
#define SMITH_NATIVE_COMMAND_H

#include "item/smith_native_compound_codec.h"

#include <algorithm>
#include <new>
#include <utility>

// Pure original Smith17 envelope values. No identity, timestamp, source,
// factory, source claim, admission or execution capability is issued here.
namespace smith_native_command_detail
{
using error = economic_accounting_error;

inline error codec_error(player_snapshot_codec_result result) noexcept
{
	if (result == player_snapshot_codec_result::ok)
		return error::ok;
	if (result == player_snapshot_codec_result::allocation_failure ||
	    result == player_snapshot_codec_result::limit_exceeded)
		return error::capacity;
	return error::corrupt_evidence;
}

// Same original item-owner key convention as populate_command_entities.
// Native stock and player item-owner are the two owner fences. PC wallet,
// native cash/mobile revision and acknowledged save remain distinct facts.
inline error entities(const smith_native_compound_terms &terms,
		      std::vector<critical_entity_key> *keys,
		      std::vector<critical_expected_revision> *revisions)
{
	if (terms.selected_custody.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    terms.frozen_outputs.size() > ITEM_TRANSFER_MAX_ITEMS - terms.selected_custody.size())
		return error::capacity;
	const size_t count = 2 + terms.selected_custody.size() + terms.frozen_outputs.size();
	if (count > CRITICAL_COMMAND_MAX_KEYS)
		return error::capacity;
	keys->clear();
	revisions->clear();
	keys->reserve(count);
	revisions->reserve(count);
	const critical_entity_key player{ critical_entity_type::player, terms.player_pid };
	const critical_entity_key mobile{ critical_entity_type::native_mobile,
					  terms.native_before.reference.mobile_instance_id };
	keys->push_back(player);
	keys->push_back(mobile);
	revisions->push_back({ player, terms.expected_player_item_revision });
	revisions->push_back({ mobile, terms.native_before.reference.stock_revision });
	for (const auto &input : terms.selected_custody)
	{
		const critical_entity_key key{ critical_entity_type::item, input.item_uid };
		keys->push_back(key);
		revisions->push_back({ key, input.expected_item_revision });
	}
	for (const auto &output : terms.frozen_outputs)
	{
		const critical_entity_key key{ critical_entity_type::item, output.object_uid };
		keys->push_back(key);
		revisions->push_back({ key, ITEM_TRANSFER_ABSENT_REVISION });
	}
	std::sort(keys->begin(), keys->end(), critical_entity_key_less);
	if (std::adjacent_find(keys->begin(), keys->end(), critical_entity_key_equal) !=
	    keys->end())
		return error::invalid_identity;
	std::sort(revisions->begin(), revisions->end(), [](const auto &a, const auto &b)
		  { return critical_entity_key_less(a.key, b.key); });
	return error::ok;
}
} // namespace smith_native_command_detail

// Actual caller operation and time only. The schema1 preparation is structural;
// existing Smith17 legacy execution/support predicates remain closed.
inline economic_accounting_error
smith_native_command_build(const smith_native_compound_terms &terms,
			   const critical_operation_id &actual_operation,
			   uint64_t actual_accepted_at_usec, critical_command *output) noexcept
{
	using namespace smith_native_command_detail;
	if (!output)
		return error::corrupt_evidence;
	if (!actual_accepted_at_usec || critical_operation_id_is_zero(actual_operation))
		return error::invalid_identity;
	if (actual_operation.bytes != terms.operation_id.bytes)
		return error::payload_conflict;
	try
	{
		critical_command command{};
		command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		command.operation_id = actual_operation;
		command.type = critical_command_type::item_transfer;
		command.payload_version = ITEM_TRANSFER_SMITH_NATIVE_COMPOUND_PAYLOAD_VERSION;
		command.source_site = critical_source_site::command;
		command.deadline_class = critical_deadline_class::interactive;
		command.accepted_at_usec = actual_accepted_at_usec;
		auto status = codec_error(smith_native_compound_encode(terms, &command.payload));
		if (status != error::ok)
			return status;
		status = entities(terms, &command.keys, &command.expected_revisions);
		if (status != error::ok)
			return status;
		if (!critical_command_envelope_valid(command))
			return error::invalid_identity;
		*output = std::move(command);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// Decode and require the entire exact canonical original fence set. Extra,
// missing, duplicate or reordered keys/revisions refuse; no wallet/save clock
// substitution is permitted. Schema2 additionally requires publication held.
// Intent semantics and current root authority are authenticated elsewhere.
inline economic_accounting_error
smith_native_command_validate(const critical_command &command,
			      smith_native_compound_terms *output) noexcept
{
	using namespace smith_native_command_detail;
	if (!output)
		return error::corrupt_evidence;
	if ((command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	     command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload_version != ITEM_TRANSFER_SMITH_NATIVE_COMPOUND_PAYLOAD_VERSION)
		return error::invalid_version;
	if (command.source_site != critical_source_site::command ||
	    command.deadline_class != critical_deadline_class::interactive ||
	    !critical_command_envelope_valid(command) ||
	    (command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION ?
		     command.publication_required || !command.accounting_intent.empty() :
		     !command.publication_required || command.accounting_intent.empty()))
		return error::invalid_identity;
	try
	{
		smith_native_compound_terms terms;
		auto status = codec_error(smith_native_compound_decode(command.payload, &terms));
		if (status != error::ok)
			return status;
		if (terms.operation_id.bytes != command.operation_id.bytes)
			return error::payload_conflict;
		std::vector<critical_entity_key> keys;
		std::vector<critical_expected_revision> revisions;
		status = entities(terms, &keys, &revisions);
		if (status != error::ok)
			return status;
		if (keys.size() != command.keys.size() ||
		    revisions.size() != command.expected_revisions.size())
			return error::payload_conflict;
		for (size_t i = 0; i < keys.size(); ++i)
			if (!critical_entity_key_equal(keys[i], command.keys[i]) ||
			    !critical_entity_key_equal(revisions[i].key,
						       command.expected_revisions[i].key) ||
			    revisions[i].revision != command.expected_revisions[i].revision)
				return error::payload_conflict;
		*output = std::move(terms);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

#endif
