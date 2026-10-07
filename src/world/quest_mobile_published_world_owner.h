#ifndef QUEST_MOBILE_PUBLISHED_WORLD_OWNER_H
#define QUEST_MOBILE_PUBLISHED_WORLD_OWNER_H

#include "world/db.h"
#include "world/quest_mobile_published_saved_forest.h"
#include "persistence/quest_mobile_native_origin_sql.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "item/item_ownership_runtime.h"
#include <memory>

class sql_economic_runtime_boot_owner;
// Every capability, including empty-holder lifetime, belongs to the actual boot
// owner. Historical SQL values are never coordinator/publication authority.
class quest_mobile_published_world_owner final
{
    private:
	friend class sql_economic_runtime_boot_owner;
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
