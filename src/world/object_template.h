#ifndef DURIS_OBJECT_TEMPLATE_H
#define DURIS_OBJECT_TEMPLATE_H

#include "core/structs.h"
#include "mob/studioproclib.h"
#include <span>
#include <memory>
#include <string>
#include <vector>

// Owned prototype values only: no live objects, identities, links or events.
struct object_template_description
{
	std::string keyword;
	std::string description;
};
struct object_template
{
	decltype(obj_data::R_num) R_num{};
	decltype(obj_data::type) type{};
	decltype(obj_data::material) material{};
	decltype(obj_data::craftsmanship) craftsmanship{};
	decltype(obj_data::extra_flags) extra_flags{};
	decltype(obj_data::wear_flags) wear_flags{};
	decltype(obj_data::extra2_flags) extra2_flags{};
	decltype(obj_data::anti_flags) anti_flags{};
	decltype(obj_data::anti2_flags) anti2_flags{};
	decltype(obj_data::value) value{};
	decltype(obj_data::weight) weight{};
	decltype(obj_data::cost) cost{};
	decltype(obj_data::condition) condition{};
	decltype(obj_data::bitvector) bitvector{};
	decltype(obj_data::bitvector2) bitvector2{};
	decltype(obj_data::bitvector3) bitvector3{};
	decltype(obj_data::bitvector4) bitvector4{};
	decltype(obj_data::bitvector5) bitvector5{};
	decltype(obj_data::affected) affected{};
	decltype(obj_data::trap_eff) trap_eff{};
	decltype(obj_data::trap_dam) trap_dam{};
	decltype(obj_data::trap_charge) trap_charge{};
	decltype(obj_data::trap_level) trap_level{};
	std::string name, description, short_description, action_description;
	std::vector<object_template_description> descriptions;
};

// Boot-only cache population; misses at runtime never fall back to file parsing.
bool cache_object_template(int vnum);
const object_template *find_object_template(int vnum);
// Separate immutable, complete SQL boot catalog. Available only after every
// native indexed prototype was parsed successfully without construction effects.
// Readiness binds the sealed table/count/file; lookup also verifies exact target
// R_num/vnum/file-position/special-procedure identity. No allocation/parser/cache
// fallback occurs on lookup. Starter/runtime loaders keep their existing cache.
// Catalog failure or stale boot provenance returns false/nullptr, not partial
// coverage or authority to restore/ACK. Pointers last until world teardown/reboot.
bool recovery_object_templates_ready() noexcept;
// Private value lookup for the original flat money boot owner. Same complete
// immutable catalog/provenance checks; no native enrollment/command/ACK authority.
// Existing SQL readiness/lookup/finalization and their caller gates stay intact.
class flatfile_coin_boot_templates final
{
	friend class inert_item_stage;
	friend class flatfile_coin_boot_stage;
	friend class coin_physical_recovery_owner;
	friend class shop_trade_native_publication_owner;
	friend class shop_trade_original_item_stage;
	friend class shop_trade_original_procedure_binding_stage;
	friend bool finalize_flatfile_shop_recovery_object_template_bindings() noexcept;
	static bool ready() noexcept;
	static const object_template *find(int vnum) noexcept;
};
// One serialized SQL boot finalization after optional subsystem bindings and
// before worker/critical startup. Validates the complete already-parsed catalog,
// then snapshots existing function pointers without parsing/allocating/callbacks
// or rewriting native indices. Success preserves prototype values/addresses.
// Boot provenance failure closes the whole catalog; unavailable input stays
// unavailable. Runtime/foreign-thread/flatfile calls refuse without mutation.
// No UID, economic, source, native publication or ACK authority is granted.
// Later lazy instance binding/staleness remains a separate prerequisite.
bool finalize_recovery_object_template_bindings() noexcept;
// Same pre-worker complete catalog binding seal for the flat SHOP boot owner.
// Runtime/foreign-thread/non-flat calls refuse; this supplies no source or ACK authority.
bool finalize_flatfile_shop_recovery_object_template_bindings() noexcept;
const object_template *find_recovery_object_template(int vnum) noexcept;
struct player_item_snapshot;
class shop_trade_native_publication_owner;
class quest_mobile_native_item_binding;
class quest_mobile_native_flat_factory_scope;
class shop_trade_original_procedure_binding_stage
{
    public:
	shop_trade_original_procedure_binding_stage() noexcept = default;
	shop_trade_original_procedure_binding_stage(
		shop_trade_original_procedure_binding_stage &&) noexcept = default;
	shop_trade_original_procedure_binding_stage &
	operator=(shop_trade_original_procedure_binding_stage &&) noexcept = default;
	shop_trade_original_procedure_binding_stage(
		const shop_trade_original_procedure_binding_stage &) = delete;
	shop_trade_original_procedure_binding_stage &
	operator=(const shop_trade_original_procedure_binding_stage &) = delete;

    private:
	friend class shop_trade_native_publication_owner;
	friend class auction_native_publication_owner;
	friend class quest_mobile_native_birth_owner;
	friend class zone_reset_item_owner;
	friend class quest_mobile_published_saved_forest;
	friend int proclibObj_add(P_obj, char *, char *);
	friend P_obj instantiate_object_template(const object_template &);
	struct binding
	{
		size_t catalog_index;
		obj_proc_type before, after, predecessor;
		bool chain_needed = false;
	};
	static bool prepare(std::span<const P_obj>, std::span<const player_item_snapshot>,
			    shop_trade_original_procedure_binding_stage &) noexcept;
	static bool prepare_flat(std::span<const P_obj>, std::span<const player_item_snapshot>,
				 shop_trade_original_procedure_binding_stage &) noexcept;
	static bool prepare_native_birth(std::span<const quest_mobile_native_item_binding>,
					 shop_trade_original_procedure_binding_stage &) noexcept;
	// Genuine flat constructor tokens retain their actual parse outcome and
	// original binding predecessor. Saved-object eligibility is not consulted.
	static bool
	prepare_native_birth_flat(std::span<const quest_mobile_native_item_binding>,
				  shop_trade_original_procedure_binding_stage &) noexcept;
	// Prospective storage admission for the same private genuine factory proof.
	// Caller retains input span/tokens/scopes and prior output storage in outer,
	// and holds the maximum through return; success grants no new authority.
	// ENOBUFS refusal/overflow, ENOMEM allocation failure, ENOTSUP request ABI.
	static bool
	prepare_native_birth_flat_bounded(const std::span<const quest_mobile_native_item_binding> &,
					  shop_trade_original_procedure_binding_stage &,
					  bool (*reserve_scratch_peak)(size_t, void *) noexcept,
					  void *, size_t outer_live_scratch) noexcept;
	// Genuine cold restored ordinary native bindings: full sealed flat boot
	// catalog/current UID/native parse/predecessor proof, no live scope retag.
	// Caller independently proves frozen command/source/custody/receipt cut and
	// owns all input/prior output storage in outer; callback grants no authority.
	static bool prepare_native_birth_cold_flat_bounded(
		const std::span<const quest_mobile_native_item_binding> &,
		shop_trade_original_procedure_binding_stage &, bool (*)(size_t, void *) noexcept,
		void *, size_t outer_live_scratch) noexcept;
	size_t retained_bytes() const noexcept;
	bool valid() const noexcept;
	bool valid_flat() const noexcept;
	void commit_unchecked() noexcept;
	void commit_flat_unchecked() noexcept;
	static void observe_normal_binding(int, obj_proc_type, obj_proc_type) noexcept;
	// Same original notification after actual binding; only this sealed flat
	// catalog entry may advance, under the complete predecessor/index proof.
	static void observe_normal_binding_flat(int, obj_proc_type, obj_proc_type) noexcept;
	std::vector<binding> bindings_;
	proclib_recovery_chain_stage chain_;
	bool prepared_ = false;
	// Backend identity remains with the original retained stage, not caller values.
	bool flat_ = false;
	bool native_flat_ = false; // Native scope checks do not alter existing flat SHOP stages.
	// The binding batch owns immutable scope copies independently of factory
	// tokens. Every actual scope/root/control-block request is charged here,
	// including after the original factory releases its own metadata.
	std::vector<std::shared_ptr<const quest_mobile_native_flat_factory_scope>> flat_scopes_;
};

P_obj instantiate_object_template(const object_template &prototype);
#endif
