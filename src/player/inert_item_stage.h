#ifndef DURIS_INERT_ITEM_STAGE_H
#define DURIS_INERT_ITEM_STAGE_H

#include "core/structs.h"
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
	void reset() noexcept;
	P_obj object_ = nullptr;
	mm_ds *pool_ = nullptr;
	friend inert_item_stage_result prepare_inert_item_stage(const object_template &,
								const player_item_snapshot &,
								inert_item_stage &) noexcept;
};

// Accept an already prepared prototype and complete four-string SQL literal.
// No parser/template loading, normal instantiation, UID issuance or publication.
// Unsupported behavior refuses BEFORE allocation. Failure preserves output.
inert_item_stage_result prepare_inert_item_stage(const object_template &prototype,
						 const player_item_snapshot &literal,
						 inert_item_stage &output) noexcept;
#endif
