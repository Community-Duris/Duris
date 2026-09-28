#ifndef DURIS_ECONOMIC_SQL_SOURCE_SNAPSHOT_H
#define DURIS_ECONOMIC_SQL_SOURCE_SNAPSHOT_H
#include <mysql/mysql.h>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using economic_sql_source_digest = std::array<uint8_t, 32>;
struct economic_sql_source_row
{
	// Exact selected SQL bytes, including embedded NUL. SQL NULL is distinct
	// from an empty value. No numeric narrowing, coin normalization or decoding.
	std::vector<std::optional<std::string>> cells;
	economic_sql_source_digest digest = {};
	bool operator==(const economic_sql_source_row &) const = default;
};
struct economic_sql_source_table
{
	std::string name;
	std::vector<std::string> columns;
	// Definition binds this version's selected columns/expressions and order,
	// not the server's full DDL. Content binds every row, ordered by native PK.
	economic_sql_source_digest definition_digest = {}, content_digest = {};
	std::vector<economic_sql_source_row> rows;
	bool operator==(const economic_sql_source_table &) const = default;
};
struct economic_sql_source_snapshot
{
	uint32_t version = 1;
	std::vector<economic_sql_source_table> tables;
	economic_sql_source_digest digest = {};
	uint64_t rows = 0, cell_bytes = 0, cells = 0;
	bool operator==(const economic_sql_source_snapshot &) const = default;
};
struct economic_sql_source_limits
{
	uint64_t maximum_rows = 262144;
	uint64_t maximum_cells = 4 * 1024 * 1024;
	uint64_t maximum_cell_bytes = 64 * 1024 * 1024;
	uint64_t maximum_single_cell_bytes = 1024 * 1024;
};
// Own one RR consistent READ ONLY transaction on an otherwise idle autocommit
// connection. Automatic reconnect must be disabled. Never commit, modify source
// rows, provision, or recover. Retain metadata locks and require InnoDB sources.
// Caller supplies a dedicated connection with finite connect/read timeouts and
// discards it after any failure (session/rollback state can be uncertain). Query/scan latency is not a
// release budget. Temporary result streaming and retained DTO overhead are
// additional to counted cell bytes; row/cell limits bound that overhead.
// Selected native monetary/custody/receipt/quarantine columns only: no player
// secrets, complete physical projections, historical ledgers or source-complete
// coverage. Digests do NOT establish native lifetimes, global drain, a resumable
// cutover boundary or authority to activate. This owning evidence is private;
// names, amounts, UIDs and payloads must not be printed in public diagnostics.
// Hard ceilings equal the defaults above. Return 0 on success; errno/native SQL
// error code otherwise. No partial output on any failure, including rollback.
unsigned int economic_sql_capture_sources(MYSQL *, const economic_sql_source_limits &,
					  economic_sql_source_snapshot *) noexcept;
// Pure version/registry/bounds/framing verification for captured DTO consumers.
// Checks all retained cells and digests, not server provenance or complete DDL.
// A caller can manufacture matching hashes; this is NOT an authority capability.
unsigned int economic_sql_validate_sources(const economic_sql_source_snapshot &,
					   const economic_sql_source_limits & = {}) noexcept;
#endif
