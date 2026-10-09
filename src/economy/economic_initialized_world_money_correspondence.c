#include "economy/economic_initialized_world_money_correspondence.h"
#include "economy/currency_command.h"
#include <climits>
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <new>
#include <tuple>
#include <utility>

namespace
{
using issue = economic_world_money_issue;
struct failure
{
	economic_accounting_error code;
};
void require(bool value,
	     economic_accounting_error code = economic_accounting_error::corrupt_evidence)
{
	if (!value)
		throw failure{ code };
}
template <class T> T integer(const std::optional<std::string> &cell)
{
	require(cell && !cell->empty());
	T value = 0;
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	require(parsed.ec == std::errc{} && parsed.ptr == cell->data() + cell->size() &&
		std::to_string(value) == *cell);
	return value;
}
using canonical_account_name = std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1>;
std::optional<canonical_account_name> canonical_account(const std::optional<std::string> &input)
{
	if (!input || input->empty() || input->size() > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return {};
	canonical_account_name result{};
	for (size_t n = 0; n < input->size(); ++n)
	{
		unsigned char character = static_cast<unsigned char>((*input)[n]);
		if (character >= 'A' && character <= 'Z')
			character += 'a' - 'A';
		if (!((character >= 'a' && character <= 'z') ||
		      (character >= '0' && character <= '9') || character == '_' ||
		      character == '-'))
			return {};
		result[n] = static_cast<char>(character);
	}
	return result;
}
using bank_key = std::tuple<canonical_account_name, uint8_t, size_t>;
struct comparer
{
	const economic_initialized_world_snapshot &world;
	const economic_sql_source_snapshot &raw;
	const economic_sql_source_limits &limits;
	size_t detail_limit;
	economic_initialized_world_money_correspondence result;
	std::vector<size_t> body_descriptor, body_projection, holding_sql;
	std::vector<std::pair<uint64_t, size_t>> player_pids, sql_wallets;
	std::vector<bank_key> projections, sql_banks;
	size_t bytes = 0;
	template <class T> void charge(const std::vector<T> &values)
	{
		require(bytes <= limits.maximum_cell_bytes &&
				values.capacity() <=
					(limits.maximum_cell_bytes - bytes) / sizeof(T),
			economic_accounting_error::capacity);
		bytes += values.capacity() * sizeof(T);
	}
	template <class T> void reserve(std::vector<T> &values, size_t count)
	{
		if (count <= values.capacity())
			return;
		const size_t delta = count - values.capacity();
		require(bytes <= limits.maximum_cell_bytes &&
				delta <= (limits.maximum_cell_bytes - bytes) / sizeof(T),
			economic_accounting_error::capacity);
		bytes += delta * sizeof(T);
		values.reserve(count);
	}
	template <class T> void push(std::vector<T> &values, const T &value)
	{
		if (values.size() == values.capacity())
			reserve(values, values.capacity() ? values.capacity() * 2 : 1);
		values.push_back(value);
	}
	void observe(uint32_t &mask, issue code, size_t body = SIZE_MAX,
		     size_t projection = SIZE_MAX, size_t holding = SIZE_MAX)
	{
		const auto bit = uint32_t{ 1 } << uint8_t(code);
		if (mask & bit)
			return;
		mask |= bit;
		++result.issue_counts[size_t(code)];
		++result.diagnostic_count;
		if (result.diagnostics.size() < detail_limit)
			push(result.diagnostics,
			     economic_world_money_diagnostic{ code, body, projection, holding });
	}
	void balance(const std::optional<economic_coin_vector> &value, uint32_t &mask, size_t body,
		     size_t projection, size_t holding)
	{
		if (!value)
		{
			observe(mask, issue::unknown_balance, body, projection, holding);
			return;
		}
		if (std::any_of(value->begin(), value->end(), [](int64_t n) { return n < 0; }))
			observe(mask, issue::negative_balance, body, projection, holding);
		int64_t total = 0;
		if (economic_coin_value(*value, &total) != economic_accounting_error::ok)
			observe(mask, issue::accounting_overflow, body, projection, holding);
	}
	std::optional<canonical_account_name> account(size_t body) const
	{
		const size_t descriptor = body_descriptor[body];
		if (descriptor == SIZE_MAX ||
		    !canonical_account(world.descriptors[descriptor].account_name))
			return {};
		return canonical_account(world.descriptors[descriptor].account_name);
	}
	void prepare_world()
	{
		reserve(body_descriptor, world.bodies.size());
		body_descriptor.assign(world.bodies.size(), SIZE_MAX);
		reserve(body_projection, world.bodies.size());
		body_projection.assign(world.bodies.size(), SIZE_MAX);
		reserve(player_pids, world.bodies.size());
		reserve(result.wallets, world.bodies.size());
		reserve(result.projections, world.bank_projections.size());
		result.projections.resize(world.bank_projections.size());
		reserve(projections, world.bank_projections.size());
		for (size_t d = 0; d < world.descriptors.size(); ++d)
		{
			const auto &descriptor = world.descriptors[d];
			if (descriptor.character_index != ECONOMIC_WORLD_NO_INDEX)
				require(descriptor.character_index < world.bodies.size() &&
					world.bodies[descriptor.character_index].descriptor_index ==
						d);
			require(descriptor.original_index == ECONOMIC_WORLD_NO_INDEX ||
				(descriptor.character_index != ECONOMIC_WORLD_NO_INDEX &&
				 descriptor.original_index != descriptor.character_index));
			for (uint32_t b : { descriptor.character_index, descriptor.original_index })
			{
				if (b == ECONOMIC_WORLD_NO_INDEX)
					continue;
				require(b < world.bodies.size() &&
					(body_descriptor[b] == SIZE_MAX ||
					 body_descriptor[b] == d));
				body_descriptor[b] = d;
			}
		}
		for (size_t b = 0; b < world.bodies.size(); ++b)
		{
			const auto &body = world.bodies[b];
			require(body.descriptor_index == ECONOMIC_WORLD_NO_INDEX ||
				(body.descriptor_index < world.descriptors.size() &&
				 body_descriptor[b] == body.descriptor_index));
			require(body.player_body_index == ECONOMIC_WORLD_NO_INDEX ||
				body.player_body_index < world.bodies.size());
			require(body.original_body_index == ECONOMIC_WORLD_NO_INDEX ||
				body.original_body_index < world.bodies.size());
			if (body.npc)
				continue;
			economic_world_wallet_correspondence row;
			row.body_index = b;
			if (body.player_pid <= 0)
				observe(row.issues, issue::invalid_identity, b);
			else
				player_pids.emplace_back(uint64_t(body.player_pid),
							 result.wallets.size());
			if (!account(b))
				observe(row.issues, issue::unobserved_account, b);
			if (body.wallet_revision == UINT64_MAX)
				observe(row.issues, issue::unavailable_revision, b);
			balance(body.cash, row.issues, b, SIZE_MAX, SIZE_MAX);
			result.wallets.push_back(row);
		}
		std::sort(player_pids.begin(), player_pids.end());
		for (size_t first = 0; first < player_pids.size();)
		{
			size_t last = first + 1;
			while (last < player_pids.size() &&
			       player_pids[last].first == player_pids[first].first)
				++last;
			if (last - first > 1)
				for (size_t n = first; n < last; ++n)
					observe(result.wallets[player_pids[n].second].issues,
						issue::duplicate_player_pid,
						result.wallets[player_pids[n].second].body_index);
			first = last;
		}
		for (size_t p = 0; p < world.bank_projections.size(); ++p)
		{
			const auto &bank = world.bank_projections[p];
			require(bank.body_index < world.bodies.size() &&
				!world.bodies[bank.body_index].npc);
			auto &row = result.projections[p];
			row.projection_index = p;
			if (body_projection[bank.body_index] != SIZE_MAX)
			{
				observe(row.issues, issue::duplicate_bank_projection,
					bank.body_index, p);
				const auto previous = body_projection[bank.body_index];
				observe(result.projections[previous].issues,
					issue::duplicate_bank_projection, bank.body_index,
					previous);
			}
			else
				body_projection[bank.body_index] = p;
			const auto observed = account(bank.body_index);
			if (bank.account_name &&
			    (!canonical_account(bank.account_name) || !observed ||
			     *canonical_account(bank.account_name) != *observed))
				observe(row.issues, issue::invalid_descriptor_correlation,
					bank.body_index, p);
			if (!observed)
				observe(row.issues, issue::unobserved_account, bank.body_index, p);
			if (bank.racewar > INT8_MAX)
				observe(row.issues, issue::invalid_identity, bank.body_index, p);
			if (bank.revision == UINT64_MAX)
				observe(row.issues, issue::unavailable_revision, bank.body_index,
					p);
			balance(bank.denominations, row.issues, bank.body_index, p, SIZE_MAX);
			if (observed && bank.racewar <= INT8_MAX)
				projections.emplace_back(*observed, bank.racewar, p);
		}
		for (auto &wallet : result.wallets)
			if (body_projection[wallet.body_index] == SIZE_MAX)
				observe(wallet.issues, issue::missing_bank_projection,
					wallet.body_index);
		std::sort(projections.begin(), projections.end());
	}
	void prepare_sql()
	{
		const auto &holdings = result.normalized_sources.holdings;
		reserve(holding_sql, holdings.size());
		holding_sql.assign(holdings.size(), SIZE_MAX);
		reserve(result.sql_money, holdings.size());
		reserve(sql_wallets, holdings.size());
		reserve(sql_banks, holdings.size());
		for (size_t h = 0; h < holdings.size(); ++h)
		{
			const auto &holding = holdings[h];
			if (holding.kind != economic_sql_holding_kind::wallet &&
			    holding.kind != economic_sql_holding_kind::bank)
				continue;
			require(holding.disposition == economic_sql_holding_disposition::current &&
				holding.source.table < raw.tables.size());
			const auto &table = raw.tables[holding.source.table];
			require(table.name == (holding.kind == economic_sql_holding_kind::wallet ?
						       "player_data" :
						       "account_banks") &&
				holding.source.row < table.rows.size());
			const auto &cells = table.rows[holding.source.row].cells;
			require(cells.size() == (holding.kind == economic_sql_holding_kind::wallet ?
							 9 :
							 8) &&
				holding.source.digest == table.rows[holding.source.row].digest &&
				integer<uint64_t>(cells[0]) == holding.native_id);
			economic_world_sql_money_correspondence row;
			row.holding_index = h;
			if (!holding.native_id || holding.native_id == UINT64_MAX)
				observe(row.issues, issue::invalid_identity, SIZE_MAX, SIZE_MAX, h);
			if (holding.kind == economic_sql_holding_kind::bank &&
			    holding.native_id > UINT32_MAX)
				observe(row.issues, issue::invalid_identity, SIZE_MAX, SIZE_MAX, h);
			if (holding.kind == economic_sql_holding_kind::wallet &&
			    holding.native_id > INT32_MAX)
				observe(row.issues, issue::invalid_identity, SIZE_MAX, SIZE_MAX, h);
			if (!canonical_account(cells[1]))
				observe(row.issues, issue::unobserved_account, SIZE_MAX, SIZE_MAX,
					h);
			const auto racewar = integer<int8_t>(cells[2]);
			if (racewar < 0)
				observe(row.issues, issue::invalid_identity, SIZE_MAX, SIZE_MAX, h);
			if (!holding.native_revision || *holding.native_revision == UINT64_MAX)
				observe(row.issues, issue::unavailable_revision, SIZE_MAX, SIZE_MAX,
					h);
			balance(holding.balance, row.issues, SIZE_MAX, SIZE_MAX, h);
			holding_sql[h] = result.sql_money.size();
			result.sql_money.push_back(row);
			if (holding.kind == economic_sql_holding_kind::wallet)
				sql_wallets.emplace_back(holding.native_id, h);
			else if (canonical_account(cells[1]) && racewar >= 0)
			{
				require(holding.native_context == racewar);
				sql_banks.emplace_back(*canonical_account(cells[1]),
						       uint8_t(racewar), h);
			}
		}
		std::sort(sql_wallets.begin(), sql_wallets.end());
		std::sort(sql_banks.begin(), sql_banks.end());
		for (size_t first = 0; first < sql_banks.size();)
		{
			size_t last = first + 1;
			while (last < sql_banks.size() &&
			       std::get<0>(sql_banks[last]) == std::get<0>(sql_banks[first]) &&
			       std::get<1>(sql_banks[last]) == std::get<1>(sql_banks[first]))
				++last;
			if (last - first > 1)
				for (size_t n = first; n < last; ++n)
				{
					const size_t h = std::get<2>(sql_banks[n]);
					observe(result.sql_money[holding_sql[h]].issues,
						issue::duplicate_sql_bank_identity, SIZE_MAX,
						SIZE_MAX, h);
				}
			first = last;
		}
	}
	void compare_wallets()
	{
		for (auto &row : result.wallets)
		{
			const auto &body = world.bodies[row.body_index];
			if (body.player_pid <= 0)
				continue;
			const auto first = std::lower_bound(
				sql_wallets.begin(), sql_wallets.end(),
				std::pair<uint64_t, size_t>{ uint64_t(body.player_pid), 0 });
			const auto last = std::upper_bound(
				sql_wallets.begin(), sql_wallets.end(),
				std::pair<uint64_t, size_t>{ uint64_t(body.player_pid), SIZE_MAX });
			if (first == last)
			{
				observe(row.issues, issue::missing_sql_counterpart, row.body_index);
				continue;
			}
			require(last - first == 1);
			const size_t h = first->second;
			row.holding_index = h;
			auto &sql = result.sql_money[holding_sql[h]];
			++sql.observed_counterpart_count;
			const auto &holding = result.normalized_sources.holdings[h];
			const auto &cells =
				raw.tables[holding.source.table].rows[holding.source.row].cells;
			const auto observed = account(row.body_index);
			if (observed && (!canonical_account(cells[1]) ||
					 *observed != *canonical_account(cells[1])))
				observe(row.issues, issue::account_mismatch, row.body_index,
					SIZE_MAX, h);
			const size_t projection = body_projection[row.body_index];
			if (projection != SIZE_MAX &&
			    integer<int8_t>(cells[2]) != world.bank_projections[projection].racewar)
				observe(row.issues, issue::racewar_mismatch, row.body_index,
					projection, h);
			if (!holding.native_revision || *holding.native_revision == UINT64_MAX)
				observe(row.issues, issue::unavailable_revision, row.body_index,
					SIZE_MAX, h);
			else if (body.wallet_revision != *holding.native_revision)
				observe(row.issues, issue::revision_mismatch, row.body_index,
					SIZE_MAX, h);
			if (!holding.balance)
				observe(row.issues, issue::unknown_balance, row.body_index,
					SIZE_MAX, h);
			else if (body.cash != *holding.balance)
				observe(row.issues, issue::balance_mismatch, row.body_index,
					SIZE_MAX, h);
			row.exact_current_correspondence = !row.issues && !sql.issues;
		}
	}
	void compare_banks()
	{
		reserve(result.banks, projections.size());
		for (size_t first = 0; first < projections.size();)
		{
			size_t last = first + 1;
			while (last < projections.size() &&
			       std::get<0>(projections[last]) == std::get<0>(projections[first]) &&
			       std::get<1>(projections[last]) == std::get<1>(projections[first]))
				++last;
			const size_t p = std::get<2>(projections[first]);
			const auto &bank = world.bank_projections[p];
			economic_world_bank_comparison row;
			row.first_projection_index = p;
			row.projection_count = last - first;
			for (size_t n = first; n < last; ++n)
			{
				const size_t next = std::get<2>(projections[n]);
				result.projections[next].comparison_index = result.banks.size();
				// Propagate pre-existing projection defects without losing their original counts.
				row.issues |= result.projections[next].issues;
				if (world.bank_projections[next].denominations !=
					    bank.denominations ||
				    world.bank_projections[next].revision != bank.revision)
					observe(row.issues, issue::conflicting_shared_projection,
						bank.body_index, p);
			}
			const bank_key low{ std::get<0>(projections[first]), bank.racewar, 0 },
				high{ std::get<0>(projections[first]), bank.racewar, SIZE_MAX };
			const auto begin =
				std::lower_bound(sql_banks.begin(), sql_banks.end(), low);
			const auto end = std::upper_bound(sql_banks.begin(), sql_banks.end(), high);
			if (begin == end)
				observe(row.issues, issue::missing_sql_counterpart, bank.body_index,
					p);
			else if (end - begin != 1)
				observe(row.issues, issue::duplicate_sql_bank_identity,
					bank.body_index, p);
			else
			{
				const size_t h = std::get<2>(*begin);
				row.holding_index = h;
				auto &sql = result.sql_money[holding_sql[h]];
				// One unique loaded account comparison, not one balance per PC projection.
				++sql.observed_counterpart_count;
				const auto &holding = result.normalized_sources.holdings[h];
				if (!holding.native_revision ||
				    *holding.native_revision == UINT64_MAX)
					observe(row.issues, issue::unavailable_revision,
						bank.body_index, p, h);
				else if (bank.revision != *holding.native_revision)
					observe(row.issues, issue::revision_mismatch,
						bank.body_index, p, h);
				if (!holding.balance)
					observe(row.issues, issue::unknown_balance, bank.body_index,
						p, h);
				else if (bank.denominations != *holding.balance)
					observe(row.issues, issue::balance_mismatch,
						bank.body_index, p, h);
				row.exact_current_correspondence = !row.issues && !sql.issues;
			}
			result.banks.push_back(row);
			first = last;
		}
	}
};
}
economic_accounting_error economic_initialized_world_compare_pc_money(
	const economic_initialized_world_snapshot &world, const economic_sql_source_snapshot &raw,
	const economic_sql_source_limits &limits, size_t diagnostic_limit,
	economic_initialized_world_money_correspondence *output) noexcept
{
	try
	{
		const economic_sql_source_limits hard;
		require(output && diagnostic_limit && diagnostic_limit <= 512 &&
			world.version == 1);
		require(limits.maximum_rows && limits.maximum_rows <= hard.maximum_rows &&
			limits.maximum_cells && limits.maximum_cells <= hard.maximum_cells &&
			limits.maximum_cell_bytes &&
			limits.maximum_cell_bytes <= hard.maximum_cell_bytes &&
			limits.maximum_single_cell_bytes &&
			limits.maximum_single_cell_bytes <= hard.maximum_single_cell_bytes);
		require(world.rows <= limits.maximum_rows &&
				raw.rows <= limits.maximum_rows - world.rows &&
				world.cells <= limits.maximum_cells &&
				raw.cells <= limits.maximum_cells - world.cells &&
				world.cell_bytes <= limits.maximum_cell_bytes &&
				raw.cell_bytes <= limits.maximum_cell_bytes - world.cell_bytes,
			economic_accounting_error::capacity);
		size_t rows = 0;
		for (size_t count :
		     { world.rooms.size(), world.bodies.size(), world.descriptors.size(),
		       world.items.size(), world.bank_projections.size() })
		{
			require(count <= limits.maximum_rows - rows,
				economic_accounting_error::capacity);
			rows += count;
		}
		require(rows <= world.rows);
		uint64_t private_bytes = 0;
		for (const auto &descriptor : world.descriptors)
			if (descriptor.account_name)
			{
				require(descriptor.account_name->size() <=
							limits.maximum_single_cell_bytes &&
						descriptor.account_name->size() <=
							world.cell_bytes - private_bytes,
					economic_accounting_error::capacity);
				private_bytes += descriptor.account_name->size();
			}
		for (const auto &bank : world.bank_projections)
			if (bank.account_name)
			{
				require(bank.account_name->size() <=
							limits.maximum_single_cell_bytes &&
						bank.account_name->size() <=
							world.cell_bytes - private_bytes,
					economic_accounting_error::capacity);
				private_bytes += bank.account_name->size();
			}
		const auto framing = economic_sql_validate_sources(raw, limits);
		if (framing)
			return framing == E2BIG || framing == ENOMEM ?
				       economic_accounting_error::capacity :
				       economic_accounting_error::corrupt_evidence;
		comparer value{ world, raw, limits, diagnostic_limit, {}, {}, {}, {}, {}, {},
				{},    {},  0 };
		const auto normalized = economic_sql_normalize_sources(
			raw, diagnostic_limit, &value.result.normalized_sources);
		if (normalized != economic_accounting_error::ok)
			return normalized;
		value.charge(value.result.normalized_sources.holdings);
		value.charge(value.result.normalized_sources.items);
		value.charge(value.result.normalized_sources.owners);
		value.charge(value.result.normalized_sources.diagnostics);
		value.prepare_world();
		value.prepare_sql();
		value.compare_wallets();
		value.compare_banks();
		value.result.diagnostics_truncated = value.result.diagnostic_count >
						     value.result.diagnostics.size();
		*output = std::move(value.result);
		return economic_accounting_error::ok;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	catch (...)
	{
		return economic_accounting_error::corrupt_evidence;
	}
}

namespace
{
using native_issue = economic_world_native_money_issue;
using native_reference_bytes = std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>;
using native_index = std::pair<uint64_t, size_t>;
template <class T> std::optional<T> native_number(const std::optional<std::string> &cell)
{
	if (!cell || cell->empty())
		return {};
	T value = 0;
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	if (parsed.ec != std::errc{} || parsed.ptr != cell->data() + cell->size() ||
	    std::to_string(value) != *cell)
		return {};
	return value;
}
bool native_identifier(uint64_t value)
{
	return value && value != UINT64_MAX;
}
struct native_money_value
{
	std::optional<uint64_t> id;
	std::optional<native_reference_bytes> reference;
	std::optional<quest_mobile_native_cash> cash;
	quest_mobile_lifetime_state state = {};
	critical_operation_id birth_operation{};
};
struct native_money_comparer
{
	const economic_initialized_world_snapshot &world;
	const quest_mobile_native_sql_catalog &raw;
	std::span<const economic_sql_native_mobile_wallet_lifetime> lifetimes;
	const economic_sql_source_limits &limits;
	size_t detail_limit;
	economic_initialized_world_native_money_correspondence result;
	std::vector<native_money_value> parsed;
	std::vector<native_index> catalog_ids, body_ids, lifetime_ids, mapping_ids;
	size_t bytes = 0;
	template <class T> void reserve(std::vector<T> &values, size_t count)
	{
		if (count <= values.capacity())
			return;
		const size_t delta = count - values.capacity();
		require(bytes <= limits.maximum_cell_bytes &&
				delta <= (limits.maximum_cell_bytes - bytes) / sizeof(T),
			economic_accounting_error::capacity);
		bytes += delta * sizeof(T);
		values.reserve(count);
	}
	void observe(uint32_t &mask, native_issue code, size_t body = SIZE_MAX,
		     size_t catalog = SIZE_MAX, size_t lifetime = SIZE_MAX)
	{
		const uint32_t bit = uint32_t{ 1 } << uint8_t(code);
		if (mask & bit)
			return;
		mask |= bit;
		++result.issue_counts[size_t(code)];
		++result.diagnostic_count;
		if (result.diagnostics.size() < detail_limit)
		{
			if (result.diagnostics.size() == result.diagnostics.capacity())
				reserve(result.diagnostics,
					std::min(detail_limit,
						 result.diagnostics.capacity() ?
							 result.diagnostics.capacity() * 2 :
							 size_t{ 1 }));
			result.diagnostics.push_back({ code, body, catalog, lifetime });
		}
	}
	void cash(const economic_coin_vector &value, uint64_t revision, uint32_t &mask, size_t body,
		  size_t catalog, size_t lifetime)
	{
		if (!revision || revision == UINT64_MAX)
			observe(mask, native_issue::unavailable_cash_revision, body, catalog,
				lifetime);
		if (std::any_of(value.begin(), value.end(), [](int64_t n) { return n < 0; }))
			observe(mask, native_issue::negative_balance, body, catalog, lifetime);
		int64_t total = 0;
		if (economic_coin_value(value, &total) != economic_accounting_error::ok)
			observe(mask, native_issue::accounting_overflow, body, catalog, lifetime);
	}
	void image_budget(const quest_mobile_native_image &image)
	{
		size_t temporary = 0;
		auto charge = [&](size_t count, size_t width)
		{
			require(bytes <= limits.maximum_cell_bytes &&
					temporary <= limits.maximum_cell_bytes - bytes &&
					count <= (limits.maximum_cell_bytes - bytes - temporary) /
							 width,
				economic_accounting_error::capacity);
			temporary += count * width;
		};
		charge(image.items.capacity(), sizeof(player_item_snapshot));
		for (const auto &item : image.items)
		{
			for (const std::string *text :
			     { &item.name, &item.short_description, &item.description,
			       &item.action_description })
				charge(text->capacity(), 1);
			charge(item.dynamic_affects.capacity(),
			       sizeof(player_item_dynamic_affect_snapshot));
			charge(item.extra_descriptions.capacity(),
			       sizeof(player_item_extra_description_snapshot));
			for (const auto &extra : item.extra_descriptions)
			{
				charge(extra.keyword.capacity(), 1);
				charge(extra.description.capacity(), 1);
				charge(extra.spell_ids.capacity(), sizeof(int32_t));
			}
		}
	}
	void parse_catalog()
	{
		reserve(result.catalog, raw.catalog.size());
		result.catalog.resize(raw.catalog.size());
		reserve(parsed, raw.catalog.size());
		parsed.resize(raw.catalog.size());
		reserve(catalog_ids, raw.catalog.size());
		for (size_t c = 0; c < raw.catalog.size(); ++c)
		{
			auto &row = result.catalog[c];
			row.catalog_index = c;
			const auto &cells = raw.catalog[c].cells;
			auto &value = parsed[c];
			value.id = native_number<uint64_t>(cells[0]);
			const auto mobile = native_number<uint64_t>(cells[1]),
				   stock = native_number<uint64_t>(cells[2]),
				   length = native_number<uint64_t>(cells[4]);
			const auto state = native_number<uint8_t>(cells[3]);
			if (value.id && native_identifier(*value.id))
				catalog_ids.emplace_back(*value.id, c);
			if (!value.id || !native_identifier(*value.id) || !mobile || !*mobile ||
			    !stock || !*stock || !state || (*state != 1 && *state != 2) ||
			    !length || !cells[5] || *length != cells[5]->size())
				observe(row.issues, native_issue::invalid_raw_row, SIZE_MAX, c);
			if (!cells[5])
			{
				observe(row.issues, native_issue::invalid_current_image, SIZE_MAX,
					c);
				continue;
			}
			quest_mobile_native_image image;
			const auto decoded = quest_mobile_native_image_decode(
				{ reinterpret_cast<const uint8_t *>(cells[5]->data()),
				  cells[5]->size() },
				&image);
			if (decoded == player_snapshot_codec_result::allocation_failure ||
			    decoded == player_snapshot_codec_result::limit_exceeded)
				throw failure{ economic_accounting_error::capacity };
			if (decoded != player_snapshot_codec_result::ok)
			{
				observe(row.issues, native_issue::invalid_current_image, SIZE_MAX,
					c);
				continue;
			}
			image_budget(image);
			native_reference_bytes reference{};
			require(quest_mobile_native_reference_encode(image.reference, &reference) ==
				player_snapshot_codec_result::ok);
			value.reference = reference;
			value.cash = image.cash;
			value.state = image.state;
			value.birth_operation = image.reference.birth_operation;
			row.retired_history = image.state == quest_mobile_lifetime_state::retired;
			if (value.id !=
				    std::optional<uint64_t>(image.reference.mobile_instance_id) ||
			    mobile != std::optional<uint64_t>(image.reference.mobile_revision) ||
			    stock != std::optional<uint64_t>(image.reference.stock_revision) ||
			    state != std::optional<uint8_t>(uint8_t(image.state)))
				observe(row.issues, native_issue::raw_image_mismatch, SIZE_MAX, c);
			if (!row.retired_history)
			{
				if (!value.cash)
					observe(row.issues, native_issue::unknown_current_cash,
						SIZE_MAX, c);
				else
					cash(value.cash->denominations.amount, value.cash->revision,
					     row.issues, SIZE_MAX, c, SIZE_MAX);
			}
		}
		std::sort(catalog_ids.begin(), catalog_ids.end());
	}
	template <class F> void duplicates(const std::vector<native_index> &index, F finding)
	{
		for (size_t first = 0; first < index.size();)
		{
			size_t last = first + 1;
			while (last < index.size() && index[last].first == index[first].first)
				++last;
			if (last - first > 1)
				for (size_t n = first; n < last; ++n)
					finding(index[n].second);
			first = last;
		}
	}
	std::optional<size_t> unique(const std::vector<native_index> &index, uint64_t id) const
	{
		const auto first =
			std::lower_bound(index.begin(), index.end(), native_index{ id, 0 });
		const auto last =
			std::upper_bound(index.begin(), index.end(), native_index{ id, SIZE_MAX });
		return last - first == 1 ? std::optional<size_t>(first->second) : std::nullopt;
	}
	bool present(const std::vector<native_index> &index, uint64_t id) const
	{
		const auto first =
			std::lower_bound(index.begin(), index.end(), native_index{ id, 0 });
		return first != index.end() && first->first == id;
	}
	void prepare_lifetimes()
	{
		reserve(result.lifetimes, lifetimes.size());
		result.lifetimes.resize(lifetimes.size());
		reserve(lifetime_ids, lifetimes.size());
		reserve(mapping_ids, lifetimes.size());
		for (size_t l = 0; l < lifetimes.size(); ++l)
		{
			const auto &value = lifetimes[l];
			auto &row = result.lifetimes[l];
			row.lifetime_index = l;
			if (!economic_account_key_valid(value.account) ||
			    value.account.kind != economic_account_kind::wallet ||
			    value.account.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT ||
			    !native_identifier(value.account.authority_id) ||
			    !native_identifier(value.native_id) ||
			    value.active_native_id != value.native_id ||
			    value.native_state != quest_mobile_lifetime_state::live ||
			    value.mapping_revision ||
			    !critical_operation_id_is_zero(value.retiring_operation_id) ||
			    critical_operation_id_is_zero(value.creating_operation_id) ||
			    critical_operation_id_is_zero(value.birth_epoch))
				observe(row.issues, native_issue::invalid_lifetime, SIZE_MAX,
					SIZE_MAX, l);
			if (!lifetimes.empty() &&
			    value.account.lineage.bytes != lifetimes[0].account.lineage.bytes)
				observe(row.issues, native_issue::mixed_lifetime_lineage, SIZE_MAX,
					SIZE_MAX, l);
			cash(value.balance, value.native_revision, row.issues, SIZE_MAX, SIZE_MAX,
			     l);
			if (native_identifier(value.native_id))
				lifetime_ids.emplace_back(value.native_id, l);
			if (native_identifier(value.account.authority_id))
				mapping_ids.emplace_back(value.account.authority_id, l);
		}
		std::sort(lifetime_ids.begin(), lifetime_ids.end());
		std::sort(mapping_ids.begin(), mapping_ids.end());
		duplicates(lifetime_ids,
			   [&](size_t l)
			   {
				   observe(result.lifetimes[l].issues,
					   native_issue::duplicate_lifetime_instance, SIZE_MAX,
					   SIZE_MAX, l);
			   });
		duplicates(mapping_ids,
			   [&](size_t l)
			   {
				   observe(result.lifetimes[l].issues,
					   native_issue::duplicate_mapping_identity, SIZE_MAX,
					   SIZE_MAX, l);
			   });
		duplicates(catalog_ids,
			   [&](size_t c) {
				   observe(result.catalog[c].issues,
					   native_issue::duplicate_catalog_instance, SIZE_MAX, c);
			   });
	}
	void compare_catalog_lifetimes()
	{
		for (size_t c = 0; c < parsed.size(); ++c)
		{
			const auto &value = parsed[c];
			auto &row = result.catalog[c];
			if (!value.id || !native_identifier(*value.id))
				continue;
			const auto l = unique(lifetime_ids, *value.id);
			if (!l)
			{
				if (!row.retired_history)
					observe(row.issues,
						present(lifetime_ids, *value.id) ?
							native_issue::duplicate_lifetime_instance :
							native_issue::missing_authenticated_lifetime,
						SIZE_MAX, c);
				continue;
			}
			row.lifetime_index = *l;
			const auto &current = lifetimes[*l];
			if (row.retired_history)
				observe(row.issues, native_issue::retired_current_counterpart,
					SIZE_MAX, c, *l);
			if (value.reference &&
			    value.birth_operation.bytes != current.creating_operation_id.bytes)
				observe(row.issues, native_issue::birth_operation_mismatch,
					SIZE_MAX, c, *l);
			if (value.cash)
			{
				if (value.cash->revision != current.native_revision)
					observe(row.issues, native_issue::current_revision_mismatch,
						SIZE_MAX, c, *l);
				if (value.cash->denominations.amount != current.balance)
					observe(row.issues, native_issue::current_balance_mismatch,
						SIZE_MAX, c, *l);
			}
			row.exact_current_lifetime_correspondence =
				!row.issues && !result.lifetimes[*l].issues &&
				unique(catalog_ids, *value.id).has_value();
		}
		for (size_t l = 0; l < lifetimes.size(); ++l)
		{
			auto &row = result.lifetimes[l];
			const auto c = unique(catalog_ids, lifetimes[l].native_id);
			if (!c)
			{
				observe(row.issues,
					present(catalog_ids, lifetimes[l].native_id) ?
						native_issue::duplicate_catalog_instance :
						native_issue::missing_catalog_counterpart,
					SIZE_MAX, SIZE_MAX, l);
				continue;
			}
			row.catalog_index = *c;
			if (result.catalog[*c].retired_history)
				observe(row.issues, native_issue::retired_current_counterpart,
					SIZE_MAX, *c, l);
			row.exact_current_catalog_correspondence =
				!row.issues &&
				result.catalog[*c].exact_current_lifetime_correspondence &&
				result.catalog[*c].lifetime_index == std::optional<size_t>(l);
		}
	}
	void prepare_bodies()
	{
		reserve(result.bodies, world.bodies.size());
		reserve(body_ids, world.bodies.size());
		for (size_t b = 0; b < world.bodies.size(); ++b)
		{
			const auto &value = world.bodies[b];
			if (!value.npc)
				continue;
			economic_world_native_money_body row;
			row.body_index = b;
			int64_t total = 0;
			if (std::any_of(value.cash.begin(), value.cash.end(),
					[](int64_t n) { return n < 0; }))
				observe(row.issues, native_issue::negative_balance, b);
			if (economic_coin_value(value.cash, &total) !=
			    economic_accounting_error::ok)
				observe(row.issues, native_issue::accounting_overflow, b);
			if (!value.native_reference && !value.native_cash_reference)
				observe(row.issues, native_issue::unresolved_first_opening, b);
			else
			{
				if (!value.native_reference)
					observe(row.issues, native_issue::invalid_native_reference,
						b);
				if (!value.native_cash_reference)
					observe(row.issues, native_issue::missing_cash_reference,
						b);
				if (value.native_reference &&
				    native_identifier(value.native_reference->mobile_instance_id))
					body_ids.emplace_back(
						value.native_reference->mobile_instance_id,
						result.bodies.size());
				else if (value.native_cash_reference &&
					 native_identifier(value.native_cash_reference->reference
								   .mobile_instance_id))
					body_ids.emplace_back(value.native_cash_reference->reference
								      .mobile_instance_id,
							      result.bodies.size());
			}
			result.bodies.push_back(row);
		}
		std::sort(body_ids.begin(), body_ids.end());
		duplicates(body_ids,
			   [&](size_t b)
			   {
				   observe(result.bodies[b].issues,
					   native_issue::duplicate_observed_instance,
					   result.bodies[b].body_index);
			   });
	}
	void compare_bodies()
	{
		for (auto &row : result.bodies)
		{
			const size_t b = row.body_index;
			const auto &body = world.bodies[b];
			if (!body.native_reference && !body.native_cash_reference)
				continue;
			const uint64_t id =
				body.native_reference ?
					body.native_reference->mobile_instance_id :
					body.native_cash_reference->reference.mobile_instance_id;
			const auto c = unique(catalog_ids, id), l = unique(lifetime_ids, id);
			row.catalog_index = c;
			row.lifetime_index = l;
			if (!c)
				observe(row.issues,
					present(catalog_ids, id) ?
						native_issue::duplicate_catalog_instance :
						native_issue::missing_catalog_counterpart,
					b);
			else
				++result.catalog[*c].observed_body_count;
			if (!l)
				observe(row.issues,
					present(lifetime_ids, id) ?
						native_issue::duplicate_lifetime_instance :
						native_issue::missing_authenticated_lifetime,
					b);
			else
				++result.lifetimes[*l].observed_body_count;
			native_reference_bytes reference{}, cash_reference{};
			const bool valid_reference = body.native_reference &&
						     quest_mobile_native_reference_encode(
							     *body.native_reference, &reference) ==
							     player_snapshot_codec_result::ok;
			if (!valid_reference)
				observe(row.issues, native_issue::invalid_native_reference, b,
					c.value_or(SIZE_MAX), l.value_or(SIZE_MAX));
			if (body.native_reference &&
			    (!body.mobile_vnum ||
			     *body.mobile_vnum != body.native_reference->mobile_vnum))
				observe(row.issues,
					native_issue::unobserved_or_mismatched_prototype, b,
					c.value_or(SIZE_MAX), l.value_or(SIZE_MAX));
			if (body.native_cash_reference)
			{
				const auto &money = *body.native_cash_reference;
				const bool valid_cash_reference =
					quest_mobile_native_reference_encode(money.reference,
									     &cash_reference) ==
					player_snapshot_codec_result::ok;
				if (!valid_cash_reference)
					observe(row.issues, native_issue::invalid_native_reference,
						b, c.value_or(SIZE_MAX), l.value_or(SIZE_MAX));
				if (!valid_reference || !valid_cash_reference ||
				    reference != cash_reference)
					observe(row.issues, native_issue::mixed_native_reference, b,
						c.value_or(SIZE_MAX), l.value_or(SIZE_MAX));
				cash(money.denominations, money.cash_revision, row.issues, b,
				     c.value_or(SIZE_MAX), l.value_or(SIZE_MAX));
				if (body.cash != money.denominations)
					observe(row.issues, native_issue::current_balance_mismatch,
						b, c.value_or(SIZE_MAX), l.value_or(SIZE_MAX));
				if (l)
				{
					const auto &lifetime = lifetimes[*l];
					if (money.wallet_mapping_id !=
						    lifetime.account.authority_id ||
					    money.lineage.bytes != lifetime.account.lineage.bytes ||
					    money.birth_epoch.bytes != lifetime.birth_epoch.bytes)
						observe(row.issues,
							native_issue::namespace_mismatch, b,
							c.value_or(SIZE_MAX), *l);
					if (money.reference.birth_operation.bytes !=
					    lifetime.creating_operation_id.bytes)
						observe(row.issues,
							native_issue::birth_operation_mismatch, b,
							c.value_or(SIZE_MAX), *l);
					if (money.cash_revision != lifetime.native_revision)
						observe(row.issues,
							native_issue::current_revision_mismatch, b,
							c.value_or(SIZE_MAX), *l);
					if (money.denominations != lifetime.balance ||
					    body.cash != lifetime.balance)
						observe(row.issues,
							native_issue::current_balance_mismatch, b,
							c.value_or(SIZE_MAX), *l);
				}
			}
			if (c)
			{
				const auto &value = parsed[*c];
				if (result.catalog[*c].retired_history)
					observe(row.issues,
						native_issue::retired_current_counterpart, b, *c,
						l.value_or(SIZE_MAX));
				if (!value.reference || !valid_reference ||
				    reference != *value.reference)
					observe(row.issues, native_issue::reference_mismatch, b, *c,
						l.value_or(SIZE_MAX));
				if (!value.cash)
					observe(row.issues, native_issue::unknown_current_cash, b,
						*c, l.value_or(SIZE_MAX));
				else
				{
					if (body.cash != value.cash->denominations.amount ||
					    (body.native_cash_reference &&
					     body.native_cash_reference->denominations !=
						     value.cash->denominations.amount))
						observe(row.issues,
							native_issue::current_balance_mismatch, b,
							*c, l.value_or(SIZE_MAX));
					if (body.native_cash_reference &&
					    body.native_cash_reference->cash_revision !=
						    value.cash->revision)
						observe(row.issues,
							native_issue::current_revision_mismatch, b,
							*c, l.value_or(SIZE_MAX));
				}
			}
			row.exact_current_correspondence =
				!row.issues && c && l &&
				result.catalog[*c].exact_current_lifetime_correspondence &&
				result.lifetimes[*l].exact_current_catalog_correspondence;
		}
	}
};
}
economic_accounting_error economic_initialized_world_compare_addressed_npc_money(
	const economic_initialized_world_snapshot &world,
	const quest_mobile_native_sql_catalog &catalog,
	std::span<const economic_sql_native_mobile_wallet_lifetime> lifetimes,
	const economic_sql_source_limits &limits, size_t diagnostic_limit,
	economic_initialized_world_native_money_correspondence *output) noexcept
{
	try
	{
		const economic_sql_source_limits hard;
		require(output && diagnostic_limit && diagnostic_limit <= 512 &&
			world.version == 1);
		require(limits.maximum_rows && limits.maximum_rows <= hard.maximum_rows &&
			limits.maximum_cells && limits.maximum_cells <= hard.maximum_cells &&
			limits.maximum_cell_bytes &&
			limits.maximum_cell_bytes <= hard.maximum_cell_bytes &&
			limits.maximum_single_cell_bytes &&
			limits.maximum_single_cell_bytes <= hard.maximum_single_cell_bytes);
		require(world.rows <= limits.maximum_rows &&
				catalog.rows <= limits.maximum_rows - world.rows &&
				world.cells <= limits.maximum_cells &&
				catalog.cells <= limits.maximum_cells - world.cells &&
				world.cell_bytes <= limits.maximum_cell_bytes &&
				catalog.cell_bytes <= limits.maximum_cell_bytes - world.cell_bytes,
			economic_accounting_error::capacity);
		require(world.bodies.size() <= world.rows &&
				catalog.catalog.size() <= catalog.rows &&
				catalog.catalog.size() <= catalog.cells / 6 &&
				lifetimes.size() <= limits.maximum_rows,
			economic_accounting_error::capacity);
		uint64_t raw_bytes = 0;
		constexpr std::array<uint64_t, 5> scalar_bounds{ 20, 20, 20, 3, 20 };
		for (const auto &row : catalog.catalog)
			for (size_t field = 0; field < row.cells.size(); ++field)
				if (row.cells[field])
				{
					const auto length = row.cells[field]->size();
					if (field < scalar_bounds.size())
						require(length <= scalar_bounds[field] &&
								length <=
									limits.maximum_single_cell_bytes,
							economic_accounting_error::capacity);
					else
						require(length <= PLAYER_SNAPSHOT_MAX_BYTES,
							economic_accounting_error::capacity);
					require(length <= catalog.cell_bytes - raw_bytes,
						economic_accounting_error::capacity);
					raw_bytes += length;
				}
		native_money_comparer value{ world, catalog, lifetimes, limits, diagnostic_limit,
					     {},    {},	     {},	{},	{},
					     {},    0 };
		value.parse_catalog();
		value.prepare_lifetimes();
		value.compare_catalog_lifetimes();
		value.prepare_bodies();
		value.compare_bodies();
		value.result.diagnostics_truncated = value.result.diagnostic_count >
						     value.result.diagnostics.size();
		*output = std::move(value.result);
		return economic_accounting_error::ok;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	catch (...)
	{
		return economic_accounting_error::corrupt_evidence;
	}
}
