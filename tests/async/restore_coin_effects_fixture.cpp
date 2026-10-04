#include "economy/economic_accounting_intent.h"

#include <algorithm>
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

static void decoder_case(const std::string &name, bool intent, const std::vector<uint8_t> &bytes)
{
	economic_frozen_intent frozen;
	economic_accounting_plan plan;
	const auto status = intent ? economic_intent_decode(bytes, &frozen) :
				     economic_plan_decode(bytes, &plan);
	std::cout << "{\"name\":\"" << name << "\",\"kind\":\"" << (intent ? "intent" : "plan")
		  << "\",\"bytes\":\"" << hex(bytes) << "\",\"accepted\":"
		  << (status == economic_accounting_error::ok ? "true" : "false") << "}\n";
}

static void decoder_corpus(const economic_accounting_plan &base,
			   const std::vector<std::pair<bool, std::vector<uint8_t>>> &goldens)
{
	for (size_t index = 0; index < goldens.size(); ++index)
	{
		const auto &[intent, bytes] = goldens[index];
		const auto name = "golden-" + std::to_string(index);
		decoder_case(name, intent, bytes);
		for (size_t offset = 0; offset < bytes.size(); ++offset)
		{
			auto changed = bytes;
			changed[offset] ^= 1;
			decoder_case(name + "-bit-" + std::to_string(offset), intent, changed);
		}
		auto changed = bytes;
		changed.pop_back();
		decoder_case(name + "-truncated", intent, changed);
		changed = bytes;
		changed.push_back(0);
		decoder_case(name + "-trailing", intent, changed);
		decoder_case(name + "-empty", intent, {});
	}
	for (uint16_t reason = 1; reason <= 46; ++reason)
	{
		economic_frozen_intent intent;
		intent.admission.metadata = base.metadata;
		auto &meta = intent.admission.metadata;
		meta.reason = static_cast<economic_reason>(reason);
		meta.actor_kind = reason >= 38 && reason <= 42 ?
					  economic_actor_kind::operator_action :
					  economic_actor_kind::domain;
		meta.original_operation_id = id(0x44);
		intent.command_binding[0] = 21;
		intent.domain_digest[0] = 22;
		std::vector<uint8_t> encoded_intent;
		for (uint16_t kind = 1; kind <= 23; ++kind)
		{
			meta.source_event = { static_cast<economic_source_kind>(kind), id(0x44),
					      meta.epoch, 7, reason == 18 ? 0U : 1U };
			if (economic_intent_encode(intent, &encoded_intent) ==
			    economic_accounting_error::ok)
				break;
		}
		assert(!encoded_intent.empty());
		const auto name = "reason-" + std::to_string(reason);
		decoder_case(name, true, encoded_intent);
		auto plan = base;
		static_cast<economic_operation_metadata &>(plan.metadata) = meta;
		assert(economic_intent_digest(intent, &plan.metadata.intent_digest) ==
		       economic_accounting_error::ok);
		plan.metadata.domain_digest = intent.domain_digest;
		// An independently assembled header exposes account-policy refusal even
		// when native encoding correctly refuses the proposed account shape.
		auto bytes = goldens[1].second;
		std::copy_n(encoded_intent.begin() + 4, 2, bytes.begin() + 4);
		std::copy_n(encoded_intent.begin() + 32, 64, bytes.begin() + 8);
		bytes[72] = encoded_intent[26];
		std::copy_n(encoded_intent.begin() + 96, 8, bytes.begin() + 76);
		std::copy_n(encoded_intent.begin() + 12, 14, bytes.begin() + 84);
		bytes[100] = encoded_intent[27];
		std::copy_n(encoded_intent.begin() + 112, 48, bytes.begin() + 104);
		std::copy(plan.metadata.intent_digest.begin(), plan.metadata.intent_digest.end(),
			  bytes.begin() + 152);
		std::copy(intent.domain_digest.begin(), intent.domain_digest.end(),
			  bytes.begin() + 184);
		decoder_case(name + "-opening", false, bytes);
		plan.accounts.clear();
		plan.postings.clear();
		if (reason == 18 || reason == 19 || reason == 45 || reason == 46)
		{
			economic_account_effect stake = {
				{ meta.lineage, economic_account_kind::gambling_stake, 8, 7 },
				{},
				{},
				0,
				1
			};
			economic_account_effect other = {
				{ meta.lineage, economic_account_kind::wallet, 7, 0 }, {}, {}, 0, 1
			};
			if (reason == 18)
			{
				stake.after[0] = 7;
				other.before[0] = 7;
				other.before_revision = 1;
				other.after_revision = 2;
			}
			else
			{
				stake.before[0] = 7;
				stake.before_revision = 1;
				stake.after_revision = 2;
				if (reason == 45)
				{
					other.key.kind = economic_account_kind::sink;
					other.after_revision = 0;
				}
				else
					other.after[0] = 7;
			}
			plan.accounts = { other, stake };
			const int64_t amount = reason == 18 ? -7 : 7;
			plan.postings = { { 0, 0, 0, { amount, 0, 0, 0 }, amount },
					  { 1, 1, 0, { -amount, 0, 0, 0 }, -amount } };
		}
		assert(economic_plan_encode(plan, &bytes) == economic_accounting_error::ok);
		decoder_case(name + "-valid-plan", false, bytes);
	}
	for (bool maximum : { false, true })
	{
		economic_frozen_intent intent;
		intent.admission.metadata = base.metadata;
		intent.command_binding[0] = 21;
		intent.domain_digest[0] = 22;
		if (maximum)
			intent.admission.facts.assign(8192 - 256, 0x5a);
		else
		{
			intent.admission.metadata.reason = economic_reason::item_move;
			intent.admission.metadata.actor_kind = economic_actor_kind::domain;
			intent.admission.metadata.source_event.reset();
		}
		std::vector<uint8_t> bytes;
		assert(economic_intent_encode(intent, &bytes) == economic_accounting_error::ok);
		decoder_case(maximum ? "intent-8192" : "no-source-intent", true, bytes);
		auto plan = base;
		static_cast<economic_operation_metadata &>(plan.metadata) =
			intent.admission.metadata;
		assert(economic_intent_digest(intent, &plan.metadata.intent_digest) ==
		       economic_accounting_error::ok);
		if (!maximum)
		{
			plan.accounts.clear();
			plan.postings.clear();
		}
		assert(economic_plan_encode(plan, &bytes) == economic_accounting_error::ok);
		decoder_case(maximum ? "plan-intent-8192" : "no-source-plan", false, bytes);
	}
}

int main(int argc, char **argv)
{
	const bool corpus = argc == 2 && std::string(argv[1]) == "--decode-corpus";
	const bool history = argc == 2 && std::string(argv[1]) == "--restore-history";
	assert(argc == 1 || corpus || history);
	std::vector<std::pair<bool, std::vector<uint8_t>>> goldens;
	auto emit = [&](bool intent, const std::vector<uint8_t> &bytes)
	{
		goldens.emplace_back(intent, bytes);
		if (!corpus)
			output(bytes);
	};
	economic_accounting_plan base;
	auto &meta = base.metadata;
	meta.lineage = id(0x11);
	meta.epoch = id(0x22);
	meta.operation_id = id(0x33);
	meta.actor_kind = history ? economic_actor_kind::domain :
				    economic_actor_kind::operator_action;
	meta.actor_id = 1;
	meta.writer_id = 1;
	meta.reason = history ? economic_reason::coin_transfer : economic_reason::baseline;
	meta.source_event = { history ? economic_source_kind::lifecycle :
					economic_source_kind::baseline,
			      id(0x44), meta.epoch, 0, 0 };
	base.accounts = {
		{ { meta.lineage, economic_account_kind::wallet, 7, 0 }, {}, { 7, 0, 0, 0 }, 0, 1 },
		{ { meta.lineage, economic_account_kind::opening, 1, 0 }, {}, {}, 0, 0 }
	};
	base.postings = { { 0, 0, 0, { 7, 0, 0, 0 }, 7 }, { 1, 1, 0, { -7, 0, 0, 0 }, -7 } };
	if (history)
		base.accounts[1] = { { meta.lineage, economic_account_kind::bank, 8, 0 },
				     { 7, 0, 0, 0 },
				     {},
				     0,
				     1 };
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
	emit(true, frozen);
	emit(false, encoded);
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
	emit(false, encoded);
	compound.items_before[0].position.revision = UINT64_MAX - 1;
	compound.items_after[0].position.revision = UINT64_MAX;
	compound.item_events[0].before.revision = UINT64_MAX - 1;
	compound.item_events[0].after.revision = UINT64_MAX;
	assert(economic_plan_encode(compound, &encoded) == economic_accounting_error::ok);
	assert(economic_plan_decode(encoded, &decoded) == economic_accounting_error::ok);
	emit(false, encoded);
	intent.admission.metadata.operation_id = id(0x66);
	intent.admission.metadata.reason = economic_reason::gambling_stake;
	intent.admission.metadata.actor_kind = economic_actor_kind::domain;
	intent.admission.metadata.source_event = { economic_source_kind::gambling_round, id(0x44),
						   meta.epoch, 7, 0 };
	assert(economic_intent_encode(intent, &frozen) == economic_accounting_error::ok);
	emit(true, frozen);
	if (history)
	{
		intent.admission.metadata = meta;
		intent.admission.facts.assign(8192 - 256, 0x5a);
		assert(economic_intent_encode(intent, &frozen) == economic_accounting_error::ok);
		emit(true, frozen);
		return 0;
	}
	if (corpus)
	{
		decoder_corpus(base, goldens);
		return 0;
	}
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
