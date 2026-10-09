#include "persistence/zone_reset_creation_budget.h"
#include "persistence/sql_room_creation_source.h"
#include "persistence/sql_room_item_payload.h"
#include "persistence/quest_mobile_native_sql.h"
#include <array>
#include <cerrno>
#include <new>
#include <type_traits>
namespace
{
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code ? code : EIO };
}
void add(uint64_t &used, uint64_t extra, uint64_t maximum)
{
	require(used <= maximum && extra <= maximum - used, E2BIG);
	used += extra;
}
economic_sql_source_limits adjusted_limits(const economic_sql_physical_source_snapshot &base,
					   const sql_room_item_source_snapshot &room,
					   const sql_room_creation_source_snapshot &creation,
					   const economic_sql_source_limits &original)
{
	const auto status =
		sql_room_creation_source_validate_sources(base, room, creation, original);
	require(!status, status);
	// Existing pure/native validators require positive maxima. All subtraction
	// stays guarded, including exact exhaustion; never wrap a depleted budget.
	require(creation.additional_rows < original.maximum_rows &&
			creation.additional_cells < original.maximum_cells &&
			creation.additional_cell_bytes < original.maximum_cell_bytes,
		E2BIG);
	auto adjusted = original;
	adjusted.maximum_rows -= creation.additional_rows;
	adjusted.maximum_cells -= creation.additional_cells;
	adjusted.maximum_cell_bytes -= creation.additional_cell_bytes;
	require(room.rows <= adjusted.maximum_rows && room.cells <= adjusted.maximum_cells &&
			room.cell_bytes <= adjusted.maximum_cell_bytes,
		E2BIG);
	return adjusted;
}
}
unsigned int zone_reset_creation_budget_prepare(const economic_sql_physical_source_snapshot &base,
						const sql_room_item_source_snapshot &room,
						const sql_room_creation_source_snapshot &creation,
						const economic_sql_source_limits &original,
						economic_sql_source_limits *output) noexcept
{
	try
	{
		require(output, EINVAL);
		const auto adjusted = adjusted_limits(base, room, creation, original);
		static_assert(std::is_nothrow_copy_assignable_v<economic_sql_source_limits>);
		*output = adjusted;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
unsigned int
zone_reset_creation_budget_validate_native(const economic_sql_physical_source_snapshot &base,
					   const sql_room_item_source_snapshot &room,
					   const sql_room_creation_source_snapshot &creation,
					   const economic_sql_source_limits &original,
					   const quest_mobile_native_sql_catalog &native,
					   zone_reset_creation_budget_totals *output) noexcept
{
	try
	{
		require(output, EINVAL);
		const auto adjusted = adjusted_limits(base, room, creation, original);
		require(native.physical_digest == base.digest &&
			room.tables.size() == native.validated_room_table_digests.size());
		for (size_t i = 0; i < room.tables.size(); ++i)
			require(native.validated_room_table_digests[i] ==
				room.tables[i].content_digest);
		zone_reset_creation_budget_totals cumulative{ room.rows, room.cells,
							      room.cell_bytes };
		add(cumulative.rows, native.catalog.size(), adjusted.maximum_rows);
		require(native.catalog.size() <= (adjusted.maximum_cells - cumulative.cells) / 6,
			E2BIG);
		add(cumulative.cells, static_cast<uint64_t>(native.catalog.size()) * 6,
		    adjusted.maximum_cells);
		constexpr std::array<uint64_t, 5> scalar_bounds{ 20, 20, 20, 3, 20 };
		for (const auto &row : native.catalog)
			for (size_t field = 0; field < row.cells.size(); ++field)
				if (row.cells[field])
				{
					const auto length = row.cells[field]->size();
					if (field < scalar_bounds.size())
						require(length <= scalar_bounds[field] &&
								length <=
									original.maximum_single_cell_bytes,
							E2BIG);
					else
						// This is the EXISTING raw6 native-body rule, already owned by the
						// catalog capture. It neither changes generic cells nor caps/policy.
						require(length <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
					add(cumulative.cell_bytes, length,
					    adjusted.maximum_cell_bytes);
				}
		require(native.rows == cumulative.rows && native.cells == cumulative.cells &&
			native.cell_bytes == cumulative.cell_bytes);
		add(cumulative.rows, creation.additional_rows, original.maximum_rows);
		add(cumulative.cells, creation.additional_cells, original.maximum_cells);
		add(cumulative.cell_bytes, creation.additional_cell_bytes,
		    original.maximum_cell_bytes);
		static_assert(std::is_nothrow_copy_assignable_v<zone_reset_creation_budget_totals>);
		*output = cumulative;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
