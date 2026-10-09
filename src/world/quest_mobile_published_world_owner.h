#ifndef QUEST_MOBILE_PUBLISHED_WORLD_OWNER_H
#define QUEST_MOBILE_PUBLISHED_WORLD_OWNER_H

#include "world/db.h"
#include "world/quest_mobile_published_saved_forest.h"
#include "persistence/quest_mobile_native_origin_sql.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "item/item_ownership_runtime.h"
#include "world/quest_mobile_native_binding.h"
#include <memory>
#include <optional>

class sql_economic_runtime_boot_owner;
class economic_sql_lifecycle_guard;
class economic_sql_cutover_transaction_owner;
class economic_sql_accounting_lifecycle_transaction;
// Every capability, including empty-holder lifetime, belongs to the actual boot
// owner. Historical SQL values are never coordinator/publication authority.
class quest_mobile_published_world_owner final
{
    private:
	friend class sql_economic_runtime_boot_owner;
	friend class economic_sql_accounting_lifecycle_transaction;
	// Values only. The lifecycle owner must separately establish initialized
	// world state and retain same-session maintenance/coordinator exclusion.
	// No historical origin, custody or activation authority is granted here.
	enum maintenance_issue : uint32_t
	{
		invalid_identity = 1u << 0,
		duplicate_identity = 1u << 1,
		malformed_binding = 1u << 2,
		invalid_placement = 1u << 3,
		missing_link = 1u << 4,
		multiple_links = 1u << 5,
		foreign_link = 1u << 6,
		invalid_topology = 1u << 7,
		literal_refused = 1u << 8,
		missing_cache = 1u << 9,
		conflicting_cache = 1u << 10,
		unknown_cash_binding = 1u << 11,
		list_cycle = 1u << 12
	};
	enum class maintenance_identity : uint8_t
	{
		unbound,
		published,
		malformed
	};
	struct maintenance_character
	{
		uint64_t runtime = 0;
		bool npc = false, alive = false;
		int32_t room = -1;
		std::optional<int32_t> room_vnum, mobile_vnum;
		maintenance_identity identity = maintenance_identity::unbound;
		std::optional<quest_mobile_native_reference> reference;
		std::optional<quest_mobile_native_cash_reference> cash_binding;
		std::array<int64_t, 4> denominations{};
		std::optional<uint64_t> owner_revision;
		std::optional<item_owner_identity> observed_owner;
		uint32_t issues = 0;
		size_t room_links = 0;
		std::optional<player_snapshot_capture_result> native_items_result;
		std::vector<player_item_snapshot> native_items;
		bool original_provenance_unknown = true;
	};
	struct maintenance_object
	{
		uint64_t uid = 0;
		std::optional<int32_t> vnum, room_vnum;
		uint8_t location = 0;
		std::optional<size_t> character, parent, root, forest, cache_row;
		uint16_t equipment_slot = 0;
		size_t physical_links = 0;
		uint32_t issues = 0;
		bool original_provenance_unknown = true;
	};
	struct maintenance_forest
	{
		size_t root_object = 0;
		player_snapshot_capture_result result =
			player_snapshot_capture_result::invalid_identity;
		std::vector<player_item_snapshot> items;
	};
	struct maintenance_link
	{
		uint8_t location = 0;
		std::optional<size_t> character, parent, object;
		std::optional<int32_t> room;
		uint16_t equipment_slot = 0;
		uint32_t issues = 0;
	};
	struct maintenance_world_report
	{
		std::vector<maintenance_character> characters;
		std::vector<maintenance_object> objects;
		std::vector<maintenance_forest> forests;
		std::vector<maintenance_link> links;
		// Every active cache row, including orphan and foreign owner domains.
		std::vector<item_ownership_runtime_entry> active_cache;
		std::vector<std::vector<size_t>> cache_objects;
		std::vector<uint32_t> cache_issues;
		std::vector<std::optional<uint64_t>> cache_owner_revisions;
		bool character_list_complete = true, object_list_complete = true;
		bool physical_lists_complete = true;
		bool world_tables_present = false;
		uint32_t issues = 0;
		size_t retained_bytes = 0;
	};
	// Ordinary observation only, never recovery/construction or cache repair.
	// Semantic defects are retained in the report; operational errors preserve
	// output. Uses original row/forest and 64MiB retained-owner bounds. A report
	// with no rows is not proof that an initialized durable world is empty.
	static unsigned int observe_maintenance(const economic_sql_lifecycle_guard &,
						maintenance_world_report *) noexcept;
	// Fresh activation holds the transferred maintenance transaction owner.
	// Keep that genuine owner; an idle guard cannot stand in for it.
	static unsigned int observe_maintenance(economic_sql_cutover_transaction_owner &,
						maintenance_world_report *) noexcept;
	static unsigned int observe_maintenance_impl(const economic_sql_lifecycle_guard *,
						     economic_sql_cutover_transaction_owner *,
						     maintenance_world_report *) noexcept;
	struct locked_native
	{
		quest_mobile_native_published_origin origin;
		quest_mobile_native_image current;
		native_mobile_wallet_origin wallet;
		item_owner_identity owner{};
		uint64_t owner_revision = 0;
		std::vector<item_ownership_runtime_entry> custody;
	};
	// Root implements this member in its SQL participant. Complete selected set,
	// strong output on refusal; never silently omit malformed, missing or retired
	// lifetimes. Exclusions are real held references, compared in full.
	// Original reconnect-disabled IN_TRANS: all birth inboxes -> mappings ->
	// ascending natives/origins -> sorted owner revisions -> whole UID/foreign
	// root/parent/context cut. No transaction boundary, writes or world effects.
	static unsigned int
	read_locked(MYSQL *, const critical_operation_id &lineage,
		    std::span<const quest_mobile_native_reference> actual_held_exclusions,
		    std::vector<locked_native> *output) noexcept;

	// Read-only union correlation after the authentic journal has drained. Values
	// select no authority: the real boot owner retains admission/session/writer
	// exclusion and supplies the complete unexcluded read_locked cut each time.
	struct held_native
	{
		quest_mobile_native_reference reference;
		uint64_t runtime;
	};
	static bool select_held(const std::vector<locked_native> &full,
				std::vector<held_native> *output) noexcept;
	static bool split_held(const std::vector<locked_native> &full,
			       const std::vector<held_native> &held,
			       std::vector<locked_native> *unheld) noexcept;
	static bool observe_union(std::vector<held_native> *) noexcept;
	static bool verify_held(const locked_native &, const held_native &) noexcept;

	class idle_token final
	{
		friend class sql_economic_runtime_boot_owner;
		friend class quest_mobile_published_world_owner;
		// Root issues only after genuine same-original-session read-only rollback
		// and idle confirmation. Retain real lifecycle/admission/currency-writer
		// exclusion across proof, construction and each re-census/callback.
		idle_token(MYSQL *connection, unsigned long original_session,
			   const critical_operation_id &lineage) noexcept
			: connection_(connection)
			, session_(original_session)
			, lineage_(lineage)
		{
		}
		MYSQL *connection_;
		unsigned long session_;
		critical_operation_id lineage_;
	};
	enum class progress : uint8_t
	{
		refused,
		pending,
		complete
	};
	struct entry;
	quest_mobile_published_world_owner() noexcept;
	~quest_mobile_published_world_owner() noexcept;
	quest_mobile_published_world_owner(const quest_mobile_published_world_owner &) = delete;
	quest_mobile_published_world_owner &
	operator=(const quest_mobile_published_world_owner &) = delete;
	// Called only with the actual read_locked output and root-issued idle token.
	// Preallocates retained correlation/effect/output state before consumption.
	bool prepare(std::vector<locked_native> &&, const idle_token &) noexcept;
	// A fresh complete read_locked/rollback cut is required before EVERY call.
	// Rechecks exact frozen set; at most one original reload/enrollment callback
	// per call. Started/unreturned or returned failure never becomes retryable.
	progress advance(const std::vector<locked_native> &fresh, const idle_token &) noexcept;
	bool verify(const std::vector<locked_native> &fresh, const idle_token &) noexcept;
	bool token_valid(const idle_token &) const noexcept;
	bool same_cut(const std::vector<locked_native> &) const noexcept;
	bool census(entry &, bool require_registered) noexcept;
	bool construct(entry &) noexcept;
	bool materialize(entry &) noexcept;
	bool restore_mobile(entry &) noexcept;
	bool charge() const noexcept;
	std::vector<std::unique_ptr<entry>> entries_;
	critical_operation_id lineage_{};
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	bool prepared_ = false;
};
#endif
