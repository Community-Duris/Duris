#ifndef QUEST_MOBILE_PUBLISHED_SAVED_FOREST_H
#define QUEST_MOBILE_PUBLISHED_SAVED_FOREST_H

#include "player/inert_item_stage.h"
#include "world/object_template.h"
#include "world/quest_mobile_native.h"
#include <array>
#include <span>
#include <vector>

// Empty holder lifecycle is public; every staging/effect capability belongs only
// to the exact published-world owner. No value grants SQL/custody/world authority.
class quest_mobile_published_saved_forest final
{
    public:
	quest_mobile_published_saved_forest() noexcept = default;
	~quest_mobile_published_saved_forest() noexcept = default;
	quest_mobile_published_saved_forest(const quest_mobile_published_saved_forest &) = delete;
	quest_mobile_published_saved_forest &
	operator=(const quest_mobile_published_saved_forest &) = delete;
	// Consumed/uncertain holders remain at one stable owner address. Only
	// prepare's private transfer into an empty output can move detached state.
	quest_mobile_published_saved_forest(quest_mobile_published_saved_forest &&) = delete;
	quest_mobile_published_saved_forest &
	operator=(quest_mobile_published_saved_forest &&) = delete;

    private:
	friend class quest_mobile_published_world_owner;
	struct reload_state
	{
		std::array<shop_trade_original_reload_effect, 4> steps{};
		std::vector<shop_trade_original_reload_effect> probes;
	};
	// Caller authenticates complete CURRENT SQL lifetime/origin/cash/forest and
	// custody/global absence BEFORE allocation. This helper performs no SQL.
	// Retains unchanged full source, including reference/cash/equipment, and
	// separate detached zero-slot views. Output is unchanged on every refusal.
	static bool prepare(const quest_mobile_native_image &,
			    quest_mobile_published_saved_forest &) noexcept;
	const quest_mobile_native_image &source() const noexcept { return source_; }
	std::span<const P_obj> objects() const noexcept { return objects_; }
	bool consumed() const noexcept { return consumed_; }
	// After consumption, fresh is mandatory and must come from root's actual
	// complete re-census with proven live-body lifetimes. Stored pointers then
	// carry no ownership; never dereference them without that fresh observation.
	bool valid(std::span<const P_obj> fresh = {}) const noexcept;
	// Root supplies fresh full SQL/custody/absence cut and preallocated empty
	// output. Validate before original no-fail binding commit/all-row release.
	// No links, registration, callbacks or allocation occur after the commit.
	// Root owns every released object and installs/validates original topology,
	// equipment and enrollment before invoking any reload capability.
	bool consume(std::span<P_obj> output) noexcept;
	// Root freshly re-censuses full SQL/world identity before AND after each
	// call. No object pointer may survive a callback without a new census.
	// Actual original saved-policy effects only; unreturned/failed steps refuse
	// permanently within this holder. Completed steps are observed, never rerun.
	bool reload_step(size_t row, unsigned int step, P_obj fresh) noexcept;
	bool proclib_probe(size_t row, size_t description, P_obj fresh) noexcept;
	const reload_state *effects(size_t row) const noexcept;
	bool reload_complete() const noexcept;
	size_t retained_bytes() const noexcept;
	bool row_valid(size_t row, P_obj fresh) const noexcept;
	void transfer_prepared(quest_mobile_published_saved_forest &&) noexcept;
	quest_mobile_native_image source_;
	std::vector<player_item_snapshot> detached_;
	std::vector<shop_trade_original_item_stage> staged_;
	std::vector<const object_template *> templates_;
	std::vector<P_obj> objects_;
	std::vector<reload_state> reload_;
	shop_trade_original_procedure_binding_stage bindings_;
	bool prepared_ = false, consumed_ = false;
};
#endif
