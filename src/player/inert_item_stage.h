#ifndef DURIS_INERT_ITEM_STAGE_H
#define DURIS_INERT_ITEM_STAGE_H

#include "core/structs.h"
#include <array>
struct object_template;
struct player_item_snapshot;
struct mm_ds;

enum class inert_item_stage_result
{
	ok,
	invalid,
	unsupported,
	allocation_unavailable
};

// Main-thread, unpublished literal allocation. No public ownership release:
// final native proof/graph enrollment needs a separate integrated owner.
// Drain every stage before world pools or debug memory logging are torn down.
class inert_item_stage
{
    public:
	inert_item_stage() noexcept = default;
	~inert_item_stage() noexcept;
	inert_item_stage(inert_item_stage &&other) noexcept;
	inert_item_stage &operator=(inert_item_stage &&other) noexcept;
	inert_item_stage(const inert_item_stage &) = delete;
	inert_item_stage &operator=(const inert_item_stage &) = delete;
	const obj_data *get() const noexcept { return object_; }

    private:
	friend class ordinary_drop_enrollment_owner;
	friend class coin_physical_recovery_owner;
	friend class shop_trade_native_publication_owner;
	friend class shop_trade_original_item_stage;
	friend class native_mobile_birth_literal_stage;
	void reset() noexcept;
	P_obj object_ = nullptr;
	mm_ds *pool_ = nullptr;
	static inert_item_stage_result allocate_literal(const object_template &,
							const player_item_snapshot &,
							inert_item_stage &) noexcept;
	friend inert_item_stage_result prepare_inert_item_stage(const object_template &,
								const player_item_snapshot &,
								inert_item_stage &) noexcept;
	friend inert_item_stage_result prepare_inert_money_stage(const player_item_snapshot &,
								 uint64_t,
								 const std::array<int32_t, 4> &,
								 inert_item_stage &) noexcept;
};

// Distinct private birth literal allocator. No construction/probe/conversion,
// UID issuance, native enrollment or publication authority; private stage only.
class native_mobile_birth_literal_stage final
{
    private:
	friend class quest_mobile_native_item_stage;
	native_mobile_birth_literal_stage() noexcept = default;
	~native_mobile_birth_literal_stage() noexcept;
	native_mobile_birth_literal_stage(const native_mobile_birth_literal_stage &) = delete;
	native_mobile_birth_literal_stage &
	operator=(const native_mobile_birth_literal_stage &) = delete;
	static bool prepare(const object_template &, const player_item_snapshot &,
			    native_mobile_birth_literal_stage &) noexcept;
	void reset() noexcept;
	P_obj object_ = nullptr;
	mm_ds *pool_ = nullptr;
	mm_ds *affect_pool_ = nullptr;
};

// Value state only. These flags cannot grant SQL/native publication authority.
struct shop_trade_original_reload_effect
{
	bool started = false, returned = false, succeeded = false, periodic = false;
};

// Separate private original SHOP staging capability. Existing generic inert
// eligibility and legacy loaders retain their own behavior. This holder owns
// only unpublished complete persisted literals; consuming it requires the exact
// original SHOP SQL/receipt/global-absence cut in the native publication owner.
class shop_trade_original_item_stage
{
    public:
	shop_trade_original_item_stage() noexcept = default;
	~shop_trade_original_item_stage() noexcept;
	shop_trade_original_item_stage(shop_trade_original_item_stage &&) noexcept;
	shop_trade_original_item_stage &operator=(shop_trade_original_item_stage &&) noexcept;
	shop_trade_original_item_stage(const shop_trade_original_item_stage &) = delete;
	shop_trade_original_item_stage &operator=(const shop_trade_original_item_stage &) = delete;

    private:
	friend class shop_trade_native_publication_owner;
	static bool prepare(const object_template &, const player_item_snapshot &,
			    shop_trade_original_item_stage &) noexcept;
	static bool reload_step(P_obj, const object_template &, unsigned int,
				shop_trade_original_reload_effect &) noexcept;
	static bool proclib_probe(P_obj, const object_template &, size_t,
				  shop_trade_original_reload_effect &) noexcept;
	void reset() noexcept;
	P_obj object_ = nullptr;
	mm_ds *pool_ = nullptr;
	mm_ds *affect_pool_ = nullptr;
};

// Pure bounded eligibility, with no allocation, pool access or output mutation.
// This is a classification only; it grants no native enrollment capability.
inert_item_stage_result inert_item_stage_eligibility(const object_template &prototype,
						     const player_item_snapshot &literal) noexcept;

// Accept an already prepared prototype and complete four-string SQL literal.
// No parser/template loading, normal instantiation, UID issuance or publication.
// Unsupported behavior refuses BEFORE allocation. Failure preserves output.
inert_item_stage_result prepare_inert_item_stage(const object_template &prototype,
						 const player_item_snapshot &literal,
						 inert_item_stage &output) noexcept;

// Detached constructor for the existing single-root, full-literal room money
// representation. Bind the original UID and values to denominations supplied by
// the native owner; these arguments alone do not establish durable authority.
// Uses only the sealed boot catalog and private pool/text allocation. No renderer,
// UID issuance, global enrollment, custody update or publication ACK occurs.
// Caller must prove the decoded graph has exactly one item and no children.
// Failure preserves output. Generic item eligibility still refuses ITEM_MONEY.
inert_item_stage_result
prepare_inert_money_stage(const player_item_snapshot &literal, uint64_t original_uid,
			  const std::array<int32_t, 4> &verified_denominations,
			  inert_item_stage &output) noexcept;
#endif
