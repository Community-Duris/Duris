#include "persistence/economic_sql_source_snapshot.h"
#include "economy/economic_sql_source_normalize.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <string_view>
#include <type_traits>
#include <openssl/evp.h>
#include <span>

namespace
{
class equipment_test_digest
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{ EVP_MD_CTX_new(),
									 EVP_MD_CTX_free };

    public:
	equipment_test_digest()
	{
		assert(context && EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) == 1);
	}
	void bytes(std::span<const uint8_t> value)
	{
		assert(EVP_DigestUpdate(context.get(), value.data(), value.size()) == 1);
	}
	void number(uint64_t value)
	{
		std::array<uint8_t, 8> encoded = {};
		for (size_t i = 0; i < encoded.size(); ++i)
			encoded[i] = static_cast<uint8_t>(value >> (8 * i));
		bytes(encoded);
	}
	void text(std::string_view value)
	{
		number(value.size());
		bytes({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
	economic_sql_source_digest finish()
	{
		economic_sql_source_digest result = {};
		unsigned length = 0;
		assert(EVP_DigestFinal_ex(context.get(), result.data(), &length) == 1 &&
		       length == result.size());
		return result;
	}
};
// Test-only reframing makes each malformed numeric/correspondence case carry
// genuine matching hashes. Refusal must arise from the violated contract.
void reframe_equipment_test(economic_sql_source_snapshot &value)
{
	value.rows = value.cells = value.cell_bytes = 0;
	auto registry = [&](auto &tables, const char *tag)
	{
		equipment_test_digest manifest;
		manifest.text(tag);
		manifest.number(tables.size());
		for (auto &table : tables)
		{
			equipment_test_digest content;
			content.text("EST1");
			content.bytes(table.definition_digest);
			content.number(table.rows.size());
			value.rows += table.rows.size();
			for (auto &row : table.rows)
			{
				equipment_test_digest hash;
				hash.text("ESR1");
				hash.bytes(table.definition_digest);
				value.cells += row.cells.size();
				for (const auto &cell : row.cells)
				{
					hash.number(cell ? 1 : 0);
					if (cell)
					{
						hash.text(*cell);
						value.cell_bytes += cell->size();
					}
				}
				row.digest = hash.finish();
				content.bytes(row.digest);
			}
			table.content_digest = content.finish();
			manifest.bytes(table.content_digest);
		}
		return manifest.finish();
	};
	value.digest = registry(value.tables, "ESM1");
	value.item_sources_digest = registry(value.item_sources, "EIM1");
	if (value.version == 2)
	{
		value.item_equipment_sources_digest =
			registry(value.item_equipment_sources, "EIE2");
		equipment_test_digest custody;
		custody.text("ESC2");
		custody.bytes(value.digest);
		custody.bytes(value.item_sources_digest);
		custody.bytes(value.item_equipment_sources_digest);
		value.custody_digest = custody.finish();
	}
	else
	{
		value.item_equipment_sources_digest = {};
		value.custody_digest = {};
	}
}

}
// Bounded component DTO controls follow the original equipment test pattern.
// Matching framing is intentionally manufactured ONLY to test parser/report
// semantics. These cases do not establish SQL provenance, census or authority.
namespace
{
economic_sql_source_table component_table(const char *name, const char *columns, const char *order)
{
	economic_sql_source_table table;
	table.name = name;
	std::string_view names(columns);
	while (!names.empty())
	{
		const auto end = names.find(',');
		table.columns.emplace_back(names.substr(0, end));
		if (end == names.npos)
			break;
		names.remove_prefix(end + 1);
	}
	equipment_test_digest definition;
	definition.text("ESD1");
	definition.text(name);
	definition.text(order);
	definition.number(table.columns.size());
	for (const auto &column : table.columns)
		definition.text(column);
	table.definition_digest = definition.finish();
	return table;
}
void component_row(economic_sql_source_table &table,
		   std::initializer_list<std::optional<std::string>> cells)
{
	table.rows.push_back({ std::vector<std::optional<std::string>>(cells), {} });
}
void reframe_physical_component(economic_sql_physical_source_snapshot &value)
{
	reframe_equipment_test(value.source2);
	value.rows = value.source2.rows;
	value.cells = value.source2.cells;
	value.cell_bytes = value.source2.cell_bytes;
	equipment_test_digest binding;
	binding.text("EPH1");
	binding.bytes(value.source2.custody_digest);
	binding.number(value.physical_sources.size());
	for (auto &table : value.physical_sources)
	{
		equipment_test_digest content;
		content.text("EST1");
		content.bytes(table.definition_digest);
		content.number(table.rows.size());
		value.rows += table.rows.size();
		for (auto &row : table.rows)
		{
			equipment_test_digest hash;
			hash.text("ESR1");
			hash.bytes(table.definition_digest);
			value.cells += row.cells.size();
			for (const auto &cell : row.cells)
			{
				hash.number(cell ? 1 : 0);
				if (cell)
				{
					hash.text(*cell);
					value.cell_bytes += cell->size();
				}
			}
			row.digest = hash.finish();
			content.bytes(row.digest);
		}
		table.content_digest = content.finish();
		binding.bytes(table.content_digest);
	}
	value.digest = binding.finish();
}

}
