// Independent continuity checks over fully authenticated retained plans. This
// deliberately grants no native holding, origin or cross-epoch qualification.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_MONEY_HISTORY_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_MONEY_HISTORY_H

#include "qualify_flatfile_economic_records.h"

namespace restore_economic_money_history
{
using namespace restore_economic_records;
constexpr size_t maximum_edges = 32768, maximum_findings = 100;
struct edge_budget_refused : audit_budget_refused
{
};
using account_key = std::array<uint8_t, 40>;
struct edge
{
	identity epoch = {}, operation = {};
	account_key account = {};
	std::array<int64_t, 4> before = {}, after = {};
	uint64_t before_revision = 0, after_revision = 0;
	bool baseline = false;
};
struct finding
{
	identity epoch = {}, operation = {};
	account_key account = {};
	const char *code;
};
struct terminal
{
	identity epoch = {}, operation = {};
	account_key account = {};
	coins balance = {};
	uint64_t revision = 0;
	bool baseline_anchored = false, valid = false;
};
struct result
{
	size_t accepted_roots = 0, edges = 0, accounts = 0, baseline_anchored_accounts = 0;
	size_t unanchored_accounts = 0, invalid_accounts = 0;
	std::vector<finding> findings;
	bool valid() const { return invalid_accounts == 0; }
};
class checker
{
	std::vector<edge> edges;
	result output;

    public:
	void observe(const identity &epoch, const identity &operation,
		     std::span<const uint8_t> plan, std::span<const uint8_t> witness)
	{
		++output.accepted_roots;
		const auto count = number(plan, 216, 4);
		for (size_t i = 0; i < count; ++i)
		{
			audit_checkpoint();
			const auto row = plan.subspan(256 + i * 120, 120);
			if (!ordinary(number(row, 18, 2)))
				continue;
			if (edges.size() == maximum_edges)
				throw edge_budget_refused();
			edge value;
			value.epoch = epoch;
			value.operation = operation;
			std::copy_n(row.begin(), 40, value.account.begin());
			value.before = coin_vector(row, 40);
			value.after = coin_vector(row, 72);
			value.before_revision = number(row, 104, 8);
			value.after_revision = number(row, 112, 8);
			value.baseline = !witness.empty();
			if (value.baseline)
			{
				const auto holding = witness.subspan(192 + i * 112, 112);
				need(same(holding.first(40), row.first(40)));
				// The baseline posting's 0 -> 1 is synthetic initialization.
				// Ordinary effects use the retained holding's native clock.
				value.before = value.after;
				value.before_revision = value.after_revision =
					number(holding, 72, 8);
			}
			edges.push_back(value);
		}
	}
	result finish(const std::function<void(const terminal &)> &observe = {})
	{
		output.edges = edges.size();
		// Operation IDs, physical buckets and append order are not commit order.
		// Same-revision observations precede the edge leaving that revision.
		std::sort(edges.begin(), edges.end(),
			  [](const auto &a, const auto &b)
			  {
				  if (std::tie(a.epoch, a.account) != std::tie(b.epoch, b.account))
					  return std::tie(a.epoch, a.account) <
						 std::tie(b.epoch, b.account);
				  if (a.baseline != b.baseline)
					  return a.baseline;
				  return std::tie(a.before_revision, a.after_revision,
						  a.operation) <
					 std::tie(b.before_revision, b.after_revision, b.operation);
			  });
		for (size_t first = 0; first < edges.size();)
		{
			audit_checkpoint();
			size_t end = first + 1;
			while (end < edges.size() && edges[end].epoch == edges[first].epoch &&
			       edges[end].account == edges[first].account)
				++end;
			++output.accounts;
			if (edges[first].baseline)
				++output.baseline_anchored_accounts;
			else
				++output.unanchored_accounts;
			auto balance = edges[first].before;
			auto revision = edges[first].before_revision;
			bool valid = true;
			for (size_t i = first; i < end; ++i)
			{
				audit_checkpoint();
				const auto &row = edges[i];
				const char *code = nullptr;
				if (i != first && row.baseline)
					code = "repeated_baseline_origin";
				else if (row.before_revision < revision)
					code = "overlapping_revision";
				else if (row.before_revision > revision)
					code = "revision_gap";
				else if (row.before != balance)
					code = "balance_discontinuity";
				if (code)
				{
					valid = false;
					++output.invalid_accounts;
					if (output.findings.size() < maximum_findings)
						output.findings.push_back({ row.epoch,
									    row.operation,
									    row.account, code });
					break; // One bounded finding per account; never repair an edge.
				}
				balance = row.after;
				revision = row.after_revision;
			}
			if (observe)
				observe({ edges[first].epoch, edges[end - 1].operation,
					  edges[first].account, balance, revision,
					  edges[first].baseline, valid });
			first = end;
		}
		audit_checkpoint();
		return output;
	}
};
inline result audit(const std::filesystem::path &root,
		    restore_economic_authority::audit_budget &budget)
{
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	checker history;
	if (lock.locked())
		(void)restore_economic_records::checker(
			root, [&](const auto &epoch, const auto &operation, auto plan, auto witness)
			{ history.observe(epoch, operation, plan, witness); })
			.run();
	auto result = history.finish();
	lock.finish();
	audit_checkpoint();
	return result;
}
} // namespace restore_economic_money_history
#endif
