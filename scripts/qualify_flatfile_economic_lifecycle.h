// Independent immutable lifecycle receipt validation. No native storage,
// mutation, recovery, lifecycle installation or codec calls are permitted here.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_LIFECYCLE_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_LIFECYCLE_H
#include "qualify_flatfile_economic_baseline.h"

namespace restore_economic_lifecycle
{
using namespace restore_economic_authority;
using account_key = std::array<uint8_t, 40>;
constexpr size_t receipt_limit = 1024 + 3071 * (156 + 2 * 50) + 512 * 1024 +
				 restore_economic_baseline::witness_limit + 4 * 1024 * 1024;
inline void append(bytes &out, std::span<const uint8_t> value)
{
	out.insert(out.end(), value.begin(), value.end());
}
inline bytes text(const char *value)
{
	return { value, value + std::strlen(value) };
}
inline bytes alias(reader &in)
{
	auto length = in.number(2);
	need(length <= 50);
	auto value = in.take(length);
	for (auto c : value)
		need((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-');
	return { value.begin(), value.end() };
}
struct mapping
{
	account_key account;
	uint64_t kind, authority, context, native;
	bytes name;
};
struct link
{
	digest command, plan;
	uint64_t revision;
	bool found = false;
};
class checker
{
	std::filesystem::path directory;
	std::map<identity, link> expected;

	template <typename Mapped>
	void receipt(const std::string &name, const identity &operation, const identity &lineage,
		     const epoch_catalog &catalog, std::span<const uint8_t> control, Mapped mapped)
	{
		auto body = frame(directory, name, "DURELR\0", receipt_limit);
		reader in{ body };
		need(in.fixed<16>() == operation && nonzero(operation) &&
		     in.fixed<16>() == lineage);
		auto epoch = in.fixed<16>();
		auto actor = in.number(8), accepted = in.number(8);
		auto requested_coverage = in.fixed<32>(), coverage = in.fixed<32>(),
		     boundary = in.fixed<32>();
		need(actor && accepted && nonzero(coverage) && nonzero(boundary) &&
		     (!nonzero(requested_coverage) || requested_coverage == coverage) &&
		     in.number(1) == 3);
		auto opening = in.fixed<40>();
		need(std::get<0>(restore_economic_baseline::account(opening, lineage)) == 9);
		auto baseline_operation = in.fixed<16>();
		auto revision = in.number(8);
		need(revision && same(in.take(16), control.subspan(16, 16)));
		auto selected_revision = in.number(8);
		need(selected_revision && selected_revision <= number(control, 80, 8));
		auto found = std::find_if(catalog.entries.begin(), catalog.entries.end(),
					  [&](const auto &entry) { return entry.epoch == epoch; });
		need(found != catalog.entries.end());
		const auto &marker = *found;
		need(marker.origin != initialization_origin::baseline_participant &&
		     marker.initialization == baseline_initialization::initialized &&
		     marker.creating_operation == operation &&
		     marker.initializing_operation == operation && marker.transition_kind == 1 &&
		     marker.transition_digest == coverage && marker.opening == opening);
		need(in.fixed<16>() == epoch && in.fixed<16>() == marker.predecessor &&
		     in.fixed<16>() == marker.creating_operation &&
		     in.number(8) == marker.ordinal && in.number(2) == marker.transition_kind &&
		     in.fixed<32>() == marker.transition_digest && in.number(1) == 2 &&
		     in.fixed<16>() == operation && in.fixed<40>() == opening);
		bytes derived(operation.begin(), operation.end());
		put(derived, 0x42415345, 4);
		put(derived, 0, 8);
		need(same(std::span<const uint8_t>(hash(derived)).first(16), baseline_operation));
		auto count = in.number(4);
		need(count <= 3071);
		std::vector<mapping> mappings;
		std::set<account_key> accounts;
		std::set<uint64_t> wallets;
		std::set<std::pair<bytes, uint64_t>> banks;
		for (size_t i = 0; i < count; ++i)
		{
			auto account = in.fixed<40>();
			auto [kind, authority, context] =
				restore_economic_baseline::account(account, lineage);
			need(authority <= 256 * 4096 && mapped(account) &&
			     accounts.insert(account).second);
			auto locator_kind = in.number(2), native = in.number(8);
			auto locator_name = alias(in);
			auto creating = in.fixed<16>(), retiring = in.fixed<16>(),
			     last = in.fixed<16>();
			auto mapping_revision = in.number(8);
			need(nonzero(creating) && nonzero(last) && !nonzero(retiring) &&
			     (mapping_revision || last == creating));
			if (kind == 1)
				need(context == 0 && locator_kind == 1 && native &&
				     native <= INT32_MAX && locator_name.empty() &&
				     wallets.insert(native).second);
			else
				need(kind == 2 && context <= 127 && locator_kind == 2 &&
				     native == authority && !locator_name.empty() &&
				     banks.insert({ locator_name, context }).second);
			mappings.push_back({ account, kind, authority, context, native,
					     std::move(locator_name) });
		}
		auto wallet_count = in.number(4), bank_count = in.number(4);
		need(wallet_count <= count && bank_count == count - wallet_count);
		struct source
		{
			std::array<uint64_t, 4> balance;
			uint64_t revision;
			digest fingerprint;
		};
		std::vector<source> sources;
		auto coverage_input = text("DURIS-FLATFILE-COVERAGE-V1");
		put(coverage_input, wallet_count, 8);
		put(coverage_input, bank_count, 8);
		for (size_t i = 0; i < count; ++i)
		{
			const bool wallet = i < wallet_count;
			auto pid = wallet ? in.number(4) : 0;
			auto name_bytes = alias(in);
			auto context = in.number(1), native_revision = in.number(8);
			const auto &mapping = mappings[i];
			need(!name_bytes.empty() && context <= 127 &&
			     mapping.kind == (wallet ? 1 : 2));
			if (wallet)
				need(pid && pid <= INT32_MAX && mapping.native == pid);
			else
				need(mapping.context == context && mapping.name == name_bytes);
			coverage_input.push_back(wallet ? 'W' : 'B');
			if (wallet)
				put(coverage_input, pid, 4);
			put(coverage_input, context, 1);
			put(coverage_input, name_bytes.size(), 4);
			append(coverage_input, name_bytes);
			put(coverage_input, native_revision, 8);
			auto fingerprint_input = text("DURIS-HOLDING-V1");
			put(fingerprint_input, wallet ? 1 : 2, 1);
			put(fingerprint_input, pid, 8);
			put(fingerprint_input, context, 1);
			put(fingerprint_input, name_bytes.size(), 4);
			append(fingerprint_input, name_bytes);
			put(fingerprint_input, native_revision, 8);
			source retained;
			retained.revision = native_revision;
			for (auto &amount : retained.balance)
			{
				amount = in.number(8);
				need(amount <= INT64_MAX);
				put(coverage_input, amount, 8);
				put(fingerprint_input, amount, 8);
			}
			retained.fingerprint = hash(fingerprint_input);
			sources.push_back(retained);
		}
		need(hash(coverage_input) == coverage);
		auto blob = [&](size_t limit)
		{
			auto size = in.number(4);
			need(size <= limit);
			return in.take(size);
		};
		auto command = blob(512 * 1024),
		     witness = blob(restore_economic_baseline::witness_limit),
		     plan = blob(4 * 1024 * 1024);
		in.done();
		(void)restore_economic_baseline::item_bytes(witness);
		need(witness.size() == 192 + count * 112 &&
		     same(witness.subspan(16, 16), lineage) &&
		     same(witness.subspan(32, 16), epoch) &&
		     same(witness.subspan(48, 16), operation) && number(witness, 64, 8) == actor &&
		     number(witness, 72, 8) == 0 && same(witness.subspan(80, 40), opening) &&
		     same(witness.subspan(120, 32), boundary) &&
		     same(witness.subspan(152, 32), coverage) && number(witness, 184, 4) == count &&
		     number(witness, 188, 4) == 0);
		std::map<account_key, std::span<const uint8_t>> holdings;
		for (size_t i = 0; i < count; ++i)
		{
			auto holding = witness.subspan(192 + i * 112, 112);
			reader key{ holding };
			need(holdings.emplace(key.fixed<40>(), holding).second);
		}
		for (size_t i = 0; i < count; ++i)
		{
			auto found_holding = holdings.find(mappings[i].account);
			need(found_holding != holdings.end());
			auto holding = found_holding->second;
			for (size_t part = 0; part < 4; ++part)
				need(number(holding, 40 + part * 8, 8) == sources[i].balance[part]);
			need(number(holding, 72, 8) == sources[i].revision &&
			     same(holding.subspan(80, 32), sources[i].fingerprint));
		}
		// Canonical lifecycle baseline command: one system fence, no native
		// expected revisions, fixed witness descriptor and fixed-size EAI1.
		need(command.size() == 376);
		reader cmd{ command };
		need(same(cmd.take(4), { reinterpret_cast<const uint8_t *>("CCM1"), 4 }) &&
		     cmd.number(4) == 2 && cmd.fixed<16>() == baseline_operation &&
		     cmd.number(2) == 20 && cmd.number(2) == 1 && cmd.number(2) == 6 &&
		     cmd.number(1) == 4 && cmd.number(1) == 0 && cmd.number(8) == accepted &&
		     cmd.number(4) == 1 && cmd.number(4) == 0 && cmd.number(4) == 48 &&
		     cmd.number(1) == 9 && cmd.number(7) == 0 &&
		     cmd.number(8) == 0x45434f4e42415345 &&
		     same(cmd.take(4), { reinterpret_cast<const uint8_t *>("EBC1"), 4 }) &&
		     cmd.number(2) == 1 && cmd.number(2) == 48 && cmd.number(4) == witness.size() &&
		     cmd.number(4) == 0 && cmd.fixed<32>() == hash(witness) &&
		     cmd.number(4) == 256);
		(void)cmd.take(256);
		cmd.done();
		need(plan.size() >= 256 &&
		     expected.emplace(baseline_operation,
				      link{ hash(command), hash(plan), revision })
			     .second);
	}

    public:
	explicit checker(const std::filesystem::path &root)
		: directory(root / "economic-evidence")
	{
	}
	template <typename Mapped> void load(const identity &lineage, const epoch_catalog &catalog,
					     std::span<const uint8_t> control, Mapped mapped)
	{
		std::set<identity> present;
		for (const auto &file : std::filesystem::directory_iterator(directory))
		{
			auto name = file.path().filename().string();
			if (!name.starts_with("lifecycle"))
				continue;
			need(name.size() == 46 && name.starts_with("lifecycle-") &&
			     name.ends_with(".elr") && expected.size() < 4096);
			auto operation = restore_economic_baseline::unhex(name.substr(10, 32));
			need(present.insert(operation).second);
			receipt(name, operation, lineage, catalog, control, mapped);
		}
		// The authority-bound origin supplies required-file discovery, including
		// old inactive epochs. An absent file cannot declare a generic origin.
		for (const auto &entry : catalog.entries)
			if (entry.origin == initialization_origin::lifecycle_owner)
				need(present.contains(entry.initializing_operation));
	}
	void record(const identity &operation, std::span<const uint8_t> command,
		    std::span<const uint8_t> plan, uint64_t revision, uint64_t result,
		    uint64_t result_bytes, uint64_t failure)
	{
		auto found = expected.find(operation);
		if (found == expected.end())
			return;
		auto &value = found->second;
		need(!value.found && !result && !result_bytes && !failure &&
		     value.revision == revision && value.command == hash(command) &&
		     value.plan == hash(plan));
		value.found = true;
	}
	size_t finish() const
	{
		for (const auto &[operation, value] : expected)
		{
			(void)operation;
			need(value.found);
		}
		return expected.size();
	}
};
} // namespace restore_economic_lifecycle
#endif
