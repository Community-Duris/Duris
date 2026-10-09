#ifndef ECONOMIC_SQL_INITIALIZED_ACTIVATION_VIEW_H
#define ECONOMIC_SQL_INITIALIZED_ACTIVATION_VIEW_H
#include "economy/economic_sql_source_normalize.h"
#include "world/economic_initialized_world_snapshot.h"
#include <span>

struct economic_sql_lifecycle_request;
struct economic_sql_activation_evidence;
struct economic_initialized_world_money_correspondence;
struct economic_sql_native_mobile_wallet_lifetime;
struct economic_initialized_world_native_money_correspondence;
// Synchronous borrowed evidence from the actual owner's same initialized-world
// and original-session RR cut. References expire when the verifier returns.
// Values/reports/digests grant no authority. Independent Plan5 verification must
// inspect raw providers and perform the full live/persisted money/forest join;
// neither primary's report nor a complete-looking route count substitutes for it.
// Keep original source2 framing within physical; native raw catalog is retained
// in persisted.native_mobile. Unloaded holdings/history remain explicit.
struct economic_sql_initialized_activation_view
{
	const economic_initialized_world_snapshot &world;
	const economic_sql_physical_source_snapshot &physical;
	const sql_room_item_source_snapshot &room;
	const sql_room_creation_source_snapshot &creation;
	const economic_sql_persisted_correspondence &persisted;
	// Primary comparison only; independent verifier still consumes raw evidence.
	const economic_initialized_world_money_correspondence &pc_money;
	// Same-session authenticated CURRENT lifetimes with historical birth namespace.
	// Borrowed metadata/report grant no authority and expire with the raw cut.
	std::span<const economic_sql_native_mobile_wallet_lifetime> native_wallets;
	const economic_initialized_world_native_money_correspondence &npc_money;
	const economic_sql_source_limits &original_limits;
};
using economic_sql_initialized_activation_verifier = unsigned int (*)(
	MYSQL *, const economic_sql_lifecycle_request &, const economic_sql_activation_evidence &,
	const economic_sql_initialized_activation_view &) noexcept;
#endif
