#ifndef DURIS_TELEMETRY_SCHEMA_PRIVATE_H
#define DURIS_TELEMETRY_SCHEMA_PRIVATE_H

#include <cstddef>
#include <cstdint>

/* Worker-only storage contract. It contains column identities and SQL metadata,
 * never handles, credentials, player names or record values. */
enum class telemetry_column_id : std::uint16_t
{
#define TELEMETRY_COLUMN(name, type, is_unsigned, width) name,
#define TELEMETRY_TABLE_COLUMN(table, name, nullable, auto_increment, default_kind)
#define TELEMETRY_INDEX(table, name, unique, columns)
#include "telemetry/telemetry_columns.inc"
#undef TELEMETRY_COLUMN
#undef TELEMETRY_TABLE_COLUMN
#undef TELEMETRY_INDEX

	count,
};

struct telemetry_column_descriptor
{
	const char *name;
	const char *sql_type;
	bool is_unsigned;
	std::uint32_t width;
};

inline constexpr telemetry_column_descriptor TELEMETRY_COLUMNS[] = {
#define TELEMETRY_COLUMN(name, type, is_unsigned, width) { #name, #type, is_unsigned, width },
#define TELEMETRY_TABLE_COLUMN(table, name, nullable, auto_increment, default_kind)
#define TELEMETRY_INDEX(table, name, unique, columns)
#include "telemetry/telemetry_columns.inc"
#undef TELEMETRY_COLUMN
#undef TELEMETRY_TABLE_COLUMN
#undef TELEMETRY_INDEX
};

constexpr const telemetry_column_descriptor &telemetry_column(telemetry_column_id id) noexcept
{
	return TELEMETRY_COLUMNS[static_cast<std::size_t>(id)];
}

enum class telemetry_column_default : std::uint8_t
{
	unconstrained, // Always supplied explicitly by the writer.
	null_value,
	zero,
	current_timestamp,
};

struct telemetry_table_column_descriptor
{
	const char *table;
	telemetry_column_id column;
	bool nullable;
	bool auto_increment;
	telemetry_column_default default_kind;
};

inline constexpr telemetry_table_column_descriptor TELEMETRY_TABLE_COLUMNS[] = {
#define TELEMETRY_COLUMN(name, type, is_unsigned, width)
#define TELEMETRY_TABLE_COLUMN(table, name, nullable, auto_increment, default_kind) \
	{ #table, telemetry_column_id::name, nullable, auto_increment,              \
	  telemetry_column_default::default_kind },
#define TELEMETRY_INDEX(table, name, unique, columns)
#include "telemetry/telemetry_columns.inc"
#undef TELEMETRY_COLUMN
#undef TELEMETRY_TABLE_COLUMN
#undef TELEMETRY_INDEX
};

struct telemetry_index_descriptor
{
	const char *table;
	const char *name;
	bool unique;
	const char *columns;
};

inline constexpr telemetry_index_descriptor TELEMETRY_INDEXES[] = {
#define TELEMETRY_COLUMN(name, type, is_unsigned, width)
#define TELEMETRY_TABLE_COLUMN(table, name, nullable, auto_increment, default_kind)
#define TELEMETRY_INDEX(table, name, unique, columns) { #table, #name, unique, columns },
#include "telemetry/telemetry_columns.inc"
#undef TELEMETRY_COLUMN
#undef TELEMETRY_TABLE_COLUMN
#undef TELEMETRY_INDEX
};

/* Header padding, reserved bytes and union arms absent for a record kind have
 * no fact column. Reserved fields are validated as zero; the union tag chooses
 * the mapped arm. ingested_utc_usec/ingest_id are sink generated; quarantine
 * payload/digest and recovery timestamps are explicit repository evidence. */

#endif
