#include "economy/economic_accounting_intent.h"

#include <cassert>
#include <iostream>
#include <sstream>

static critical_operation_id id(uint8_t value)
{
	critical_operation_id result;
	result.bytes.fill(value);
	return result;
}

static void output(const std::vector<uint8_t> &bytes)
{
	for (size_t i = 0; i < 4; ++i)
		std::cout.put(static_cast<char>(bytes.size() >> (i * 8)));
	std::cout.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

static std::string hex(std::span<const uint8_t> bytes)
{
	const char *digits = "0123456789abcdef";
	std::string result;
	for (uint8_t byte : bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	return result;
}

static void coins(std::ostringstream &out, const economic_coin_vector &value)
{
	out << '[';
	for (size_t i = 0; i < value.size(); ++i)
		out << (i ? "," : "") << value[i];
	out << ']';
}

static void oracle(const char *name, const economic_accounting_plan &plan,
		   size_t corrupt_key_offset = 40)
{
	bool valid = economic_coin_effects_validate(plan.accounts, plan.postings, 0) ==
		     economic_accounting_error::ok;
	std::ostringstream out;
	out << "{\"name\":\"" << name << "\",\"effects\":[";
	for (size_t i = 0; i < plan.accounts.size(); ++i)
	{
		const auto &effect = plan.accounts[i];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key;
		assert(economic_account_key_encode(effect.key, &key) ==
		       economic_accounting_error::ok);
		if (corrupt_key_offset < key.size())
		{
			key[corrupt_key_offset] = 2;
			economic_account_key decoded;
			valid = valid && economic_account_key_decode(key, &decoded) ==
						 economic_accounting_error::ok;
		}
		out << (i ? "," : "") << "{\"account_key\":\"" << hex(key) << "\",\"before\":";
		coins(out, effect.before);
		out << ",\"after\":";
		coins(out, effect.after);
		out << ",\"before_revision\":" << effect.before_revision
		    << ",\"after_revision\":" << effect.after_revision << '}';
	}
	out << "],\"postings\":[";
	for (size_t i = 0; i < plan.postings.size(); ++i)
	{
		const auto &posting = plan.postings[i];
		out << (i ? "," : "") << "{\"event_index\":" << posting.event_index
		    << ",\"account_index\":" << posting.account_index
		    << ",\"child_index\":" << posting.child_index << ",\"delta\":";
		coins(out, posting.delta);
		out << ",\"copper\":" << posting.copper << '}';
	}
	out << "],\"accepted\":" << (valid ? "true" : "false") << '}';
	const auto text = out.str();
	output({ text.begin(), text.end() });
}

int main()
{
	economic_accounting_plan base;
	auto &meta = base.metadata;
	meta.lineage = id(0x11);
	meta.epoch = id(0x22);
	meta.operation_id = id(0x33);
	meta.actor_kind = economic_actor_kind::operator_action;
	meta.actor_id = 1;
	meta.writer_id = 1;
	meta.reason = economic_reason::baseline;
	meta.source_event = { economic_source_kind::baseline, id(0x44), meta.epoch, 0, 0 };
	base.accounts = {
		{ { meta.lineage, economic_account_kind::wallet, 7, 0 }, {}, { 7, 0, 0, 0 }, 0, 1 },
		{ { meta.lineage, economic_account_kind::opening, 1, 0 }, {}, {}, 0, 0 }
	};
	base.postings = { { 0, 0, 0, { 7, 0, 0, 0 }, 7 }, { 1, 1, 0, { -7, 0, 0, 0 }, -7 } };
	economic_frozen_intent intent;
	intent.admission.metadata = meta;
	intent.command_binding[0] = 21;
	intent.domain_digest[0] = 22;
	std::vector<uint8_t> frozen, encoded;
	assert(economic_intent_encode(intent, &frozen) == economic_accounting_error::ok);
	assert(economic_intent_digest(intent, &meta.intent_digest) ==
	       economic_accounting_error::ok);
	meta.domain_digest = intent.domain_digest;
	assert(economic_plan_encode(base, &encoded) == economic_accounting_error::ok);
	economic_accounting_plan decoded;
	assert(economic_plan_decode(encoded, &decoded) == economic_accounting_error::ok);
	output(frozen);
	output(encoded);
	auto compound = base;
	critical_operation_id child;
	assert(critical_operation_id_derive(meta.operation_id, 1, 1, &child));
	compound.children = { { child, 1, 1, 0, 1 } };
	for (auto &posting : compound.postings)
		posting.child_index = 1;
	economic_item_position before, after;
	before.owner = { item_owner_type::player, 42, 0 };
	before.state = item_custody_state::active;
	before.root_uid = 99;
	before.revision = 1;
	after = before;
	after.owner = { item_owner_type::room, 4, 0 };
	after.revision = 2;
	compound.items_before = { { 99, before } };
	compound.items_after = { { 99, after } };
	compound.item_events = { { 0, 1, 99, before, after } };
	assert(economic_plan_encode(compound, &encoded) == economic_accounting_error::ok);
	assert(economic_plan_decode(encoded, &decoded) == economic_accounting_error::ok);
	output(encoded);
	compound.items_before[0].position.revision = UINT64_MAX - 1;
	compound.items_after[0].position.revision = UINT64_MAX;
	compound.item_events[0].before.revision = UINT64_MAX - 1;
	compound.item_events[0].after.revision = UINT64_MAX;
	assert(economic_plan_encode(compound, &encoded) == economic_accounting_error::ok);
	assert(economic_plan_decode(encoded, &decoded) == economic_accounting_error::ok);
	output(encoded);
	intent.admission.metadata.operation_id = id(0x66);
	intent.admission.metadata.reason = economic_reason::gambling_stake;
	intent.admission.metadata.actor_kind = economic_actor_kind::domain;
	intent.admission.metadata.source_event = { economic_source_kind::gambling_round, id(0x44),
						   meta.epoch, 7, 0 };
	assert(economic_intent_encode(intent, &frozen) == economic_accounting_error::ok);
	output(frozen);
	oracle("opening", base);
	for (uint16_t kind = 1; kind <= 11; ++kind)
	{
		auto candidate = base;
		candidate.accounts[0].before = { 7, 0, 0, 0 };
		candidate.accounts[0].after = {};
		candidate.accounts[0].before_revision = 1;
		candidate.accounts[0].after_revision = 2;
		candidate.accounts[1].key.kind = static_cast<economic_account_kind>(kind);
		candidate.accounts[1].key.authority_id = 8;
		if (economic_account_is_ordinary(candidate.accounts[1].key.kind))
		{
			candidate.accounts[1].after = { 7, 0, 0, 0 };
			candidate.accounts[1].after_revision = 1;
		}
		candidate.postings[0].delta = { -7, 0, 0, 0 };
		candidate.postings[0].copper = -7;
		candidate.postings[1].delta = { 7, 0, 0, 0 };
		candidate.postings[1].copper = 7;
		const auto name = "kind-" + std::to_string(kind);
		oracle(name.c_str(), candidate);
	}
	for (size_t variant = 0; variant < 20; ++variant)
	{
		auto candidate = base;
		const char *name;
		switch (variant)
		{
		case 0:
			name = "system-before";
			candidate.accounts[1].before[0] = -7;
			break;
		case 1:
			name = "system-after";
			candidate.accounts[1].after[0] = -7;
			break;
		case 2:
			name = "system-before-revision";
			candidate.accounts[1].before_revision = 1;
			break;
		case 3:
			name = "system-after-revision";
			candidate.accounts[1].after_revision = 1;
			break;
		case 4:
			name = "ordinary-stale";
			candidate.accounts[0].after_revision = 0;
			break;
		case 5:
			name = "ordinary-backwards";
			candidate.accounts[0].before_revision = 2;
			break;
		case 6:
			name = "ordinary-negative-before";
			candidate.accounts[0].before[0] = -1;
			break;
		case 7:
			name = "ordinary-negative-after";
			candidate.accounts[0].after[0] = -1;
			break;
		case 8:
			name = "ordinary-overflow";
			candidate.accounts[0].after[3] = INT64_MAX;
			break;
		case 9:
			name = "revision-only";
			candidate.accounts.resize(1);
			candidate.postings.clear();
			candidate.accounts[0].after = {};
			break;
		case 10:
			name = "unposted-stale";
			candidate.accounts.resize(1);
			candidate.postings.clear();
			candidate.accounts[0].after = {};
			candidate.accounts[0].after_revision = 0;
			break;
		case 11:
			name = "unposted-system";
			candidate.accounts.erase(candidate.accounts.begin());
			candidate.postings.clear();
			break;
		case 12:
			name = "zero-postings";
			for (auto &post : candidate.postings)
			{
				post.delta = {};
				post.copper = 0;
			}
			break;
		case 13:
			name = "sparse-events";
			candidate.postings[1].event_index = UINT32_MAX;
			break;
		case 14:
			name = "duplicate-events";
			candidate.postings[1].event_index = 0;
			break;
		case 15:
			name = "key-order";
			candidate.accounts[0].key.authority_id = 256;
			candidate.accounts[1].key = candidate.accounts[0].key;
			candidate.accounts[1].key.authority_id = 255;
			break;
		case 16:
			name = "invalid-key-version";
			oracle(name, candidate, 16);
			continue;
		case 17:
			name = "invalid-key-padding";
			oracle(name, candidate, 36);
			continue;
		default:
			name = variant == 18 ? "numeric-key-order" : "uint64-key-order";
			candidate.accounts[0].key.authority_id = variant == 18 ? 255 :
										 UINT64_MAX - 1;
			candidate.accounts[0].key.context_id = variant == 18 ? 0 : UINT64_MAX;
			candidate.accounts[1].key.kind = economic_account_kind::wallet;
			candidate.accounts[1].key.authority_id = variant == 18 ? 256 : UINT64_MAX;
			candidate.accounts[1].before = { 7, 0, 0, 0 };
			candidate.accounts[1].before_revision = 1;
			candidate.accounts[1].after_revision = 2;
			break;
		}
		oracle(name, candidate);
	}
}
