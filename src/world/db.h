/* ***************************************************************************
 *  file: db.h , Database module.                            Part of Duris *
 *  Usage: Loading/Saving chars booting world.                               *
 *************************************************************************** */

#ifndef _SOJ_DB_H_
#define _SOJ_DB_H_

#include <span>
#include <memory>
#include <vector>
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "economy/native_mobile_birth_constructor_recipe.h"
#include "economy/economic_source_event.h"

#include <cstdint>
#include <cstddef>
#include <stdio.h>
#include <string>
using namespace std;

#define STRING(var) char(var)[MAX_STRING_LENGTH];

/* data files used by the game system */

#define SHOP_FILE "areas/world.shp"
#define WORLD_FILE "areas/world.wld" /* room definitions          */
#define MOB_FILE "areas/world.mob" /* monster prototypes        */
#define OBJ_FILE "areas/world.obj" /* object prototypes         */
#define ZONE_FILE "areas/world.zon" /* zone defs & command tables*/
#define JUSTICE_FILE "areas/world.justice" /* justice definition file   */

/* Read-only object-index boot used by offline static analysis tools. */
void boot_material_rarity_objects(int mini_mode);

#define MAIL_FILE "Accounts/mail" /* player pfiles moved under Accounts/; mail follows */
#define MAIL_FILE_LEGACY "Players/mail" /* pre-migration location */

#define CREDITS_FILE "lib/information/credits" /* for the 'credits' command */
#define MAP1_FILE "lib/information/map1"
#define MAP2_FILE "lib/information/map2"
#define MAP3_FILE "lib/information/map3"
#define MAP1A_FILE "lib/information/map1a"
#define MAP2A_FILE "lib/information/map2a"
#define MAP3A_FILE "lib/information/map3a"
#define GREETING_FILE "lib/information/greeting" /* this for ascii greetings */
#define GREETING1_FILE "lib/information/greeting.1"
#define GREETING2_FILE "lib/information/greeting.2"
#define GREETING3_FILE "lib/information/greeting.3"

#define GREETINGA_FILE "lib/information/greetinga" /* this for ANSI greetings */
#define GREETINGA1_FILE "lib/information/greetinga.1"
#define GREETINGA2_FILE "lib/information/greetinga.2"
#define GREETINGA3_FILE "lib/information/greetinga.3"
#define GREETINGA4_FILE "lib/information/greetinga.4"

#define HELP_PAGE_FILE "lib/information/help" /* for HELP <CR>             */
#define MOTD_FILE "lib/information/motd" /* messages of today         */

#define EMAIL_FILE "Players/emailreg" /* EMAIL Registration file */

#define GUILD_FRAG_FILE "lib/information/guild_frags" /* for the 'guild frag command' command    */
#define NEWS_FILE "lib/information/news" /* for the 'news' command    */
#define ARTIFACT_DIR "Players/Artifacts/"
#define ARTIFACT_MORT_DIR "Players/Artifacts/Mortal/"
#define ARTI_BIND_DIR "Players/Artifacts/Bind/"
#define MORTAL_ARTI_MAIN_FILE "Players/Artifacts/mortal_main" /* for the 'arti ' command    */
#define MORTAL_ARTI_IOUN_FILE "Players/Artifacts/mortal_ioun" /* for the 'arti ioun' command    */
#define MORTAL_ARTI_UNIQUE_FILE \
	"Players/Artifacts/mortal_unique" /* for the 'arti unique' command    */
#define PROJECTS_FILE "lib/information/projects" /* for the 'projects' command     */
#define FAQ_FILE "lib/information/faq" /* for the 'faq' command     */
#define RULES_FILE "lib/information/rules"
#define WIZLISTA_FILE "lib/information/wizlista"
#define WIZLIST_FILE "lib/information/wizlist" /* for WIZLIST               */
#define WIZMOTD_FILE "lib/information/wizmotd" /* wizard motd */

#define IDEA_FILE "lib/reports/ideas" /* for the 'idea'-command    */
#define TYPO_FILE "lib/reports/typos" /*         'typo'            */
#define BUG_FILE "lib/reports/bugs" /*         'bug'             */
#define CHEATERS_FILE "lib/reports/cheaters" /*         'bug'        */
#define OK_FILE "lib/reports/ok_file" /*         'bug'        */
#define BUG_CHEAT "lib/reports/cheats" /*         'cheat report'             */

#define MESS_FILE "lib/misc/messages" /* damage message            */
#define SOCMESS_FILE "lib/misc/actions" /* messgs for social acts    */
#define POSEMESS_FILE "lib/misc/poses" /* for 'pose'-command         */
#define MOB_LOOKUP "lib/misc/lookup.mob"
#define OBJ_LOOKUP "lib/misc/lookup.obj"
#define WLD_LOOKUP "lib/misc/lookup.wld"
#define ZON_LOOKUP "lib/misc/lookup.zon"
#define BAN_FILE "lib/misc/ban_sites"
#define WIZCONNECT_FILE "lib/misc/wizconnect_sites"

#define DISCLAIMER_FILE "lib/creation/disclaimer" /* Disclaimer files */
#define RACEWARS_FILE "lib/creation/racewars" /* for good/evil race explanation */
#define GENERALTABLE_FILE "lib/creation/generaltable" /* for race/class comparisons */
#define CLASSTABLE_FILE "lib/creation/classtable" /* for class table selection */
#define RACETABLE_FILE "lib/creation/racetable" /* for race table selection */
#define NAMECHART_FILE "lib/creation/namechart" /* Name selection page */
#define REROLL_FILE "lib/creation/reroll" /* for reroll explanation */
#define BONUS_FILE "lib/creation/bonus" /* for bonus explanation */
#define KEEPCHAR_FILE "lib/creation/keepchar" /* for accepting char explanation */
#define HOMETOWN_FILE "lib/creation/hometown"
#define ALIGNMENT_FILE "lib/creation/alignment"
#define SHUTDOWN_FILE "lib/creation/boom"

#define TERM_ARRAY_CHAR '\n'
#define NULL_FILE "\0"

#define QUEST_GOAL_ITEM 1
#define QUEST_GOAL_ITEM_TYPE 2
#define QUEST_GOAL_COINS 3
#define QUEST_GOAL_SKILL 4
#define QUEST_GOAL_EXP 5
#define QUEST_GOAL_UNKNOWN 10

#define QC_ACTION "qc_action"
#define QC_GREET "qc_nocorpse"
#define QC_UNBLOCK "qc_unblock"

struct ship_reg_node
{
	char *name;
	int vnum;
	struct ship_reg_node *next;
};

extern struct ship_reg_node *ship_reg_db;
void no_reset_zone_reset(int);

struct reboot_data
{
	char reboot_option[30];
	char reboot_file[255];
	FILE *whichfile;
};

#define RB_OP(data) (data).reboot_option
#define RB_FILE(data) (data).reboot_file
#define RB_FILE_PTR(data) (data).whichfile
#define REREAD_FILE(file) file_to_string_alloc(RB_FILE((file)), &RB_FILE_PTR((file)))

#define DO_ALL(str) ((is_abbrev((str), "all")) || *(str) == '*')
#define DO_LIST(str) ((is_abbrev((str), "list")) || *(str) == '?')

#define OK(ch) send_to_char("Okay.\r\n", (ch))

/* global variables */
extern unsigned long next_obj_uid;
extern string news;
extern char *projects;
extern string motd;
extern string wizmotd;
extern char *help;
extern char *rules;
extern char *wizlista;
extern char *greetinga;
extern char *disclaimer;
extern char *bugfile;
extern char *generaltable;
extern char *racewars;
extern char *classtable;
extern char *racetable;
extern char *namechart;
extern char *reroll;
extern char *bonus;
extern char *keepchar;
extern char *hometown_table;
extern char *alignment_table;

#define REAL 0
#define VIRTUAL 1

extern const char *MENU;
extern const char *GREETINGS;
extern const char *BACKGR_STORY;
#define WELC_MESSG \
	"\r\n\
        Welcome to New Duris\r\n\
\r\n\r\n"

#define ZONE_SILENT BIT_1
#define ZONE_SAFE BIT_2
#define ZONE_TOWN BIT_3
#define ZONE_NEW_RESET BIT_4
#define ZONE_MAP BIT_5
#define ZONE_CLOSED BIT_6
#define ZONE_PRIVATE BIT_7

// 1209600 = 14 days
// #define ARTIFACT_TIMER_SEC 1209600
#define ARTIFACT_BLOOD_DAYS 10

// Detached native preparation is available only to the original birth owner.
// This handle has explicit disposal: uncertain admitted work must retain it.
// It never issues a durable ID, validates admission or grants publication ACK.
struct char_data;
class quest_mobile_native_birth_owner;
struct native_mobile_birth_recovery_effect;
struct native_mobile_birth_recovery_choice;
struct native_mobile_birth_recovery_context;
struct quest_mobile_native_image;
struct quest_mobile_native_reference;
struct native_mobile_birth_cash_role_recipe;
struct flatfile_shopkeeper_record;
enum class player_snapshot_capture_result : uint8_t;
class quest_mobile_native_stage
{
    public:
	quest_mobile_native_stage() = default;
	quest_mobile_native_stage(const quest_mobile_native_stage &) = delete;
	quest_mobile_native_stage &operator=(const quest_mobile_native_stage &) = delete;
	quest_mobile_native_stage(quest_mobile_native_stage &&) = delete;
	quest_mobile_native_stage &operator=(quest_mobile_native_stage &&) = delete;

    private:
	struct char_data *character_ = nullptr;
	size_t publication_next_step_ = 0;
	bool publication_step_started_ = false, publication_consumed_ = false;
	uint64_t publication_runtime_id_ = 0;
	// Process-local restoration observations; never replace historical journal facts.
	// Only successful original cold-constructor replay may prepare these rows.
	bool shared_affect_constructor_restored_ = false;
	std::unique_ptr<const std::vector<uint8_t>> shared_affect_checkpoint_;
	std::unique_ptr<const std::vector<flatfile_shopkeeper_affect_record>> shared_affect_rows_;
	std::vector<struct affected_type *> shared_affect_installed_;
	struct char_data *shared_affect_actor_ = nullptr;
	uint64_t shared_affect_runtime_ = 0;
	int shared_affect_room_ = -1;
	size_t shared_affect_historical_prefix_ = 0, shared_affect_prefix_ = 0;
	bool shared_affect_step_started_ = false, shared_affect_complete_ = false;
	bool shared_affect_before_room_ready_ = false;
	struct nevent_data *shared_affect_balance_event_ = nullptr;
	unsigned long long shared_affect_balance_sequence_ = 0;
	bool shared_affect_balance_observed_ = false;
	bool shared_affect_balance_prepare_started_ = false;
	bool shared_affect_balance_prepare_returned_ = false;
	bool shared_affect_park_started_ = false, shared_affect_park_returned_ = false;
	bool shared_affect_park_succeeded_ = false;
	bool shared_affect_resume_started_ = false, shared_affect_resume_returned_ = false;
	struct nevent_data *shared_affect_resumed_event_ = nullptr;
	unsigned long long shared_affect_resumed_sequence_ = 0;
	// Private preparation binds the exact owned reconstructed body/full original
	// checkpoint. Saved AF rows are the sole intentionally missing source piece.
	bool prepare_shared_shopkeeper_affects(struct char_data *, uint64_t, int,
					       const quest_mobile_native_reference &,
					       const flatfile_shopkeeper_record &, size_t) noexcept;
	bool restore_shared_shopkeeper_affects_before_room(struct char_data *) noexcept;
	bool finish_shared_shopkeeper_affects_after_room(struct char_data *, int) noexcept;
	bool shared_shopkeeper_affect_charge(size_t *) const noexcept;
	bool shared_shopkeeper_affect_rows_current(struct char_data *) const noexcept;
	bool observe_shared_shopkeeper_balance(struct char_data *) noexcept;
	bool park_shared_shopkeeper_balance(struct char_data *) noexcept;
	bool apply_shared_shopkeeper_affects(struct char_data *, bool) noexcept;
	bool restoration_active_ = false;
	int restoration_room_ = -1;
	size_t restoration_prefix_ = 0, restoration_room_step_ = 0;
	std::array<uint8_t, 8> restoration_effects_{};
	std::array<uint8_t, 4> restoration_choices_{};
	std::array<int32_t, 4> restoration_delays_{};
	std::array<bool, 4> restoration_events_{};
	bool choose_publication_step(size_t, struct char_data *,
				     const native_mobile_birth_recovery_effect &,
				     native_mobile_birth_recovery_choice *) noexcept;
	bool publication_step(size_t, int, struct char_data *,
			      const native_mobile_birth_recovery_choice &,
			      native_mobile_birth_recovery_effect &, struct char_data **) noexcept;
	// Existing published body only, after the owner's genuine fresh SQL/world
	// cut and confirmed rollback. No callbacks, events or identity issuance.
	bool adopt_published(struct char_data *actual, uint64_t actual_runtime_id, int room_rnum,
			     const quest_mobile_native_image &original,
			     const native_mobile_birth_recovery_context &) noexcept;
	// Absent original body only, after genuine fresh SQL/native/custody/absence proof.
	// False may retain consumed runtime ownership; the same stage must be retried.
	bool restore_published(int room_rnum, std::span<const native_mobile_birth_recovery_effect>,
			       std::span<const native_mobile_birth_recovery_choice>,
			       struct char_data **live_after) noexcept;
	bool prepare(int nr, int type, bool apply_mob_gold);
	bool prepare_captured(int nr, int type, bool apply_mob_gold,
			      const quest_mobile_native_constructor_digest &current_build,
			      quest_mobile_native_constructor_recipe *) noexcept;
	// NBC2 verifies actual callback/selected reset-tail witnesses. The owner
	// applies the original zone modifier/shop binding after this constructor.
	bool prepare_captured(int nr, int type, bool apply_mob_gold,
			      const quest_mobile_native_constructor_digest &current_build,
			      int32_t reset_room_vnum, int configured_shop,
			      quest_mobile_native_constructor_recipe *) noexcept;
	bool restore_constructor_v2(
		const quest_mobile_native_constructor_recipe &,
		const quest_mobile_native_constructor_digest &current_build) noexcept;
	bool
	restore_constructor(const quest_mobile_native_constructor_recipe &,
			    const quest_mobile_native_constructor_digest &current_build) noexcept;
	struct char_data *character() const noexcept { return character_; }
	// Original detached shared keeper only. These passive observations neither
	// authenticate the caller's reset/source cut nor permit publication or ACK.
	// Revision1 and the caller's observed time are retained without a new clock.
	player_snapshot_capture_result capture_shopkeeper_checkpoint(
		struct char_data *actual, uint64_t original_runtime, int actual_pending_room,
		int configured_shop, const quest_mobile_native_reference &,
		const quest_mobile_native_constructor_recipe &original_constructor,
		const native_mobile_birth_cash_role_recipe &original_role,
		int64_t observed_saved_at, flatfile_shopkeeper_record *output) const noexcept;
	// Only an empty, unlinked, unscheduled preparation can be discarded.
	// The owner must first resolve/remove its own unadmitted staged stock.
	bool discard_empty() noexcept;
	// False retains the stage. True means consumed before room/special hooks,
	// even if a hook extracts the NPC or room insertion fails. Not an ACK.
	// An exception after consumption must not be treated as a fresh refusal.
	bool publish(int room_rnum, struct char_data **live_after_hooks);
	friend class quest_mobile_published_world_owner;
	friend class quest_mobile_native_birth_owner;
};

// Actual constructor stage: explicit ownership, no automatic native disposal.
// Only original birth owner may consume it after its genuine admitted SQL proof.
struct obj_data;
class shop_trade_original_procedure_binding_stage;
class quest_mobile_native_item_stage;
class quest_mobile_native_container_shell;
struct native_mobile_birth_item_recipe;
struct player_item_snapshot;
struct object_template;
// Retained selected-root/source provenance supplied only by the genuine ROOM
// source owner. It supplies no source, UID, admission or publication permission.
class quest_mobile_native_flat_factory_scope final
{
    public:
	~quest_mobile_native_flat_factory_scope() = default;

    private:
	friend class zone_reset_item_owner;
	friend class quest_mobile_native_item_stage;
	friend class shop_trade_original_procedure_binding_stage;
	quest_mobile_native_flat_factory_scope(const std::string &selected_root,
					       const economic_source_event &source)
		: root_(selected_root)
		, source_(source)
	{
	}
	quest_mobile_native_flat_factory_scope(const quest_mobile_native_flat_factory_scope &) =
		default;
	bool current() const noexcept;
	size_t retained_heap_bytes() const noexcept;
	std::string root_;
	economic_source_event source_{};
};
// Read-only original constructor facts for the existing checked binding batch.
// No public construction, source/SQL/publication/ACK permission.
class quest_mobile_native_item_binding
{
    public:
	quest_mobile_native_item_binding(const quest_mobile_native_item_binding &) = default;

    private:
	friend class quest_mobile_native_item_stage;
	friend class shop_trade_original_procedure_binding_stage;
	using procedure = int (*)(struct obj_data *, struct char_data *, int, char *);
	struct obj_data *object_;
	uint64_t uid_;
	int rnum_, vnum_;
	long position_;
	procedure before_;
	bool parsed_proclib_;
	// Only the actual cold factory can prove a retained bridge over a bare
	// original predecessor. This is not a parser result or transport field.
	bool restored_bridge_request_ = false;
	// Pending flat provenance cannot fall back to the original SQL token path.
	bool flat_factory_ = false;
	std::shared_ptr<const quest_mobile_native_flat_factory_scope> flat_scope_;
	quest_mobile_native_item_binding(struct obj_data *object, uint64_t uid, int rnum, int vnum,
					 long position, procedure before,
					 bool parsed_proclib) noexcept
		: object_(object)
		, uid_(uid)
		, rnum_(rnum)
		, vnum_(vnum)
		, position_(position)
		, before_(before)
		, parsed_proclib_(parsed_proclib)
	{
	}
};
// Original carrier values only; they do not authorize effects or publication.
struct quest_mobile_native_item_progress
{
	uint32_t next_step = 0;
	bool current_step_started = false, admitted = false, published = false;
};
struct quest_mobile_native_item_effect
{
	bool started = false, returned = false, succeeded = false, periodic = false;
};
struct item_ownership_runtime_entry;
class zone_reset_room_publication_owner;
enum class zone_reset_room_nest_result;
class quest_mobile_native_item_stage
{
    public:
	quest_mobile_native_item_stage() noexcept = default;
	quest_mobile_native_item_stage(const quest_mobile_native_item_stage &) = delete;
	quest_mobile_native_item_stage &operator=(const quest_mobile_native_item_stage &) = delete;

    private:
	friend class quest_mobile_native_birth_owner;
	friend class zone_reset_item_owner;
	friend class zone_reset_room_publication_owner;
	// Original Smith owner retains the actual constructor stage and its private
	// producer edits only that stage; friendship grants no source or admission.
	friend class smith_native_compound_owner;
	friend class smith_native_producer;
	struct implementation;
	implementation *state_ = nullptr;
	static bool prepare(int nr, int type, uint64_t supplied_reserved_uid,
			    quest_mobile_native_item_stage *) noexcept;
	// False may retain the authentic unresolved factory candidate in output.
	// This private seam grants no source/admission/publication authority.
	static bool prepare_retaining(int nr, int type, uint64_t supplied_reserved_uid,
				      quest_mobile_native_item_stage *) noexcept;
	static bool prepare_impl(int nr, int type, uint64_t supplied_reserved_uid,
				 quest_mobile_native_item_stage *,
				 bool retain_failed_candidate) noexcept;
	// Only the genuine retained flat source owner can construct this scope.
	// A failed scope copy still leaves the actual attempted factory retained.
	static bool prepare_retaining_flat(int nr, int type, uint64_t reserved_uid,
					   const quest_mobile_native_flat_factory_scope &,
					   quest_mobile_native_item_stage *) noexcept;
	bool is_flat_factory() const noexcept;
	bool flat_factory_matches(const std::string &selected_root,
				  const economic_source_event &original_source) const noexcept;
	bool empty() const noexcept;
	// Pure original constructor-owned graph changes, never a P/source permit.
	// The span is the complete selected room tree plus the new detached child.
	static zone_reset_room_nest_result
	nest_room(std::span<quest_mobile_native_item_stage *>, quest_mobile_native_item_stage &root,
		  quest_mobile_native_item_stage &child,
		  quest_mobile_native_item_stage &target) noexcept;
	static bool detach_room(std::span<quest_mobile_native_item_stage *>,
				quest_mobile_native_item_stage &root,
				quest_mobile_native_item_stage &child,
				quest_mobile_native_item_stage &target) noexcept;
	// Genuine complete private-tree proof plus original scalar detach. Outer owns
	// all input/stage/global retention; retain admitted phase high-water through
	// return. No effect on budget/proof refusal; no callbacks after unlink.
	// Pinned GCC13 libstdc++ policy; grants no source/publication authority.
	static bool detach_room_bounded(const std::span<quest_mobile_native_item_stage *> &,
					quest_mobile_native_item_stage &,
					quest_mobile_native_item_stage &,
					quest_mobile_native_item_stage &,
					bool (*)(size_t, void *) noexcept, void *,
					size_t outer_live) noexcept;
	static bool room_graph_ready_bounded(const std::span<quest_mobile_native_item_stage *> &,
					     quest_mobile_native_item_stage &,
					     quest_mobile_native_item_stage &,
					     quest_mobile_native_item_stage &, bool detaching,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t outer_live) noexcept;
	static bool room_graph_ready(std::span<quest_mobile_native_item_stage *>,
				     quest_mobile_native_item_stage &root,
				     quest_mobile_native_item_stage &child,
				     quest_mobile_native_item_stage &target,
				     bool detaching) noexcept;
	// Shared original prepend chronology across live and genuine pending factories.
	// False is an observation gap; true/null is an authentic no-match result.
	// The actual producer must separately authenticate source and target custody.
	static bool original_reset_target(int rnum, struct obj_data **selected,
					  bool *pending) noexcept;
	bool owns_pending_original_target(struct obj_data *) const noexcept;
	struct obj_data *object() const noexcept;
	bool capture_container_shell(quest_mobile_native_container_shell *) noexcept;
	quest_mobile_native_item_binding binding_input() const noexcept;
	size_t retained_bytes() const noexcept;
	// Genuine frozen flat cold restoration only: no live factory retag/RNG.
	// Outer includes CURRENT object/affect pools and Zombie registry ONCE, all
	// existing inputs/output stages. Refresh globals + stage on EVERY return.
	static bool restore_bounded(const player_item_snapshot &,
				    const native_mobile_birth_item_recipe &,
				    quest_mobile_native_item_stage *,
				    bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	static bool restore_bound_bounded(const player_item_snapshot &,
					  const native_mobile_birth_item_recipe &,
					  quest_mobile_native_item_stage *,
					  bool (*)(size_t, void *) noexcept, void *,
					  size_t) noexcept;
	static bool restore_rebind_bounded(const player_item_snapshot &,
					   const native_mobile_birth_item_recipe &,
					   quest_mobile_native_item_stage *,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t) noexcept;
	// Actual genuine flat warm/cold private metadata + raw heap, includes inline stage;
	// excludes pooled slots/pages and reserved pointer already in registry capacity.
	// Body leaves private allowance only at original full successful publication.
	// Strong scalar output; original retained_bytes observation remains unchanged.
	bool retained_bytes_excluding_literal_pools(size_t *) const noexcept;

	bool capture_recipe(const player_item_snapshot &,
			    native_mobile_birth_item_recipe *) const noexcept;
	static bool restore(const player_item_snapshot &, const native_mobile_birth_item_recipe &,
			    quest_mobile_native_item_stage *) noexcept;
	// Original committed binding must match the real bridge/predecessor or
	// switch installation. This shape/body helper grants no root authority.
	// Member scope retains the original private proclib predecessor capability.
	static const object_template *
	find_bound_recovery_template(const player_item_snapshot &,
				     const native_mobile_birth_item_recipe &) noexcept;
	static bool restore_bound(const player_item_snapshot &,
				  const native_mobile_birth_item_recipe &,
				  quest_mobile_native_item_stage *) noexcept;
	// Cold captured bridge with exact sealed/current bare predecessor only.
	// Retains its explicit original bridge request for the checked whole batch.
	static bool restore_rebind(const player_item_snapshot &,
				   const native_mobile_birth_item_recipe &,
				   quest_mobile_native_item_stage *) noexcept;
	// Releases only metadata from successful actual borrowed-world adoption.
	// Factory-owned stages cannot use this path; live objects/events stay intact.
	bool abandon_adoption() noexcept;
	void retain_admitted() noexcept;
	// Retained admission refuses disposal; original owner must resolve it first.
	bool discard_unadmitted() noexcept;
	// Root consumes every staged object before NPC/room hooks can extract stock.
	// Actual callbacks are separate once-only steps; returned is not success/ACK.
	struct obj_data *publish() noexcept;
	// Same real constructor consumption for a complete room forest. The
	// genuine context owner must retain admission/progress before calling.
	// Preflight and cache projection complete before nonallocating global
	// links; every refusal leaves stages, world, cache and output unchanged.
	static bool publish_many(std::span<quest_mobile_native_item_stage *>,
				 std::span<struct obj_data *>,
				 std::span<const item_ownership_runtime_entry>) noexcept;
	// Complete private ROOM forest publication with prospective scratch requests.
	// Three input spans are borrowed by reference; actual copied spans/temporary
	// lookups/bit-vector/selection/full cached-link capture and atomic hydration
	// are admitted before requests. Complete original predicates remain required.
	// Caller outer includes initial runtime cache observer EXACTLY ONCE and all
	// already-live inputs/stages/old outputs. Retain admitted peak through return
	// and refresh CURRENT cache allowance on EVERY return (even false after a
	// global reserve). No allocations/budget callbacks after first consumption.
	// Pinned GCC13 libstdc++ C++11 ABI; unsupported policy refuses. No new source,
	// admission, physical proof or ACK authority; original method unchanged.
	static bool publish_many_bounded(const std::span<quest_mobile_native_item_stage *> &,
					 const std::span<struct obj_data *> &,
					 const std::span<const item_ownership_runtime_entry> &,
					 bool (*)(size_t, void *) noexcept, void *,
					 size_t outer_live) noexcept;

	// Complete genuine original item service step with actual pool/output/Zombie
	// providers. Caller outer owns their initial global observers and private
	// stage/input retention ONCE; refresh CURRENT globals + stage EVERY return.
	// Actual returned/succeeded markers precede fallible scheduling diagnostics;
	// false after an effected action leaves started held and next_step unchanged.
	// No re-probe/RNG, lifecycle bypass, source or publication authority.
	bool publication_step_bounded(size_t, struct obj_data *expected,
				      quest_mobile_native_item_effect &,
				      bool (*)(size_t, void *) noexcept, void *,
				      size_t outer_live) noexcept;
	size_t publication_step_count() const noexcept;
	bool publication_step(size_t, struct obj_data *expected,
			      quest_mobile_native_item_effect &) noexcept;
	// Rebuild only historically returned-success runtime services on the exact
	// actually published restored body. Pending original effects stay pending.
	bool rebuild_enrollment(struct obj_data *expected, const native_mobile_birth_item_recipe &,
				const quest_mobile_native_item_progress &,
				std::span<const quest_mobile_native_item_effect>) noexcept;

	// Complete frozen flat present-object adoption; full recorded prefix/current
	// event/proc/UID/literal proof. Metadata only, no native reenrollment or RNG.
	static bool
	adopt_published_bounded(const player_item_snapshot &,
				const native_mobile_birth_item_recipe &, struct obj_data *,
				const quest_mobile_native_item_progress &,
				const std::span<const quest_mobile_native_item_effect> &,
				quest_mobile_native_item_stage *, bool (*)(size_t, void *) noexcept,
				void *, size_t) noexcept;
	// Genuine completed enrollment-prefix reconstruction. Outer includes initial
	// own stage + CURRENT object/affect/Zombie/event/output storage ONCE. Refresh
	// all current storage on EVERY return. Actual successful scheduling is retained
	// before later diagnostics can refuse, and must never be replayed.
	bool rebuild_enrollment_bounded(struct obj_data *, const native_mobile_birth_item_recipe &,
					const quest_mobile_native_item_progress &,
					const std::span<const quest_mobile_native_item_effect> &,
					bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	bool release_published() noexcept;
	// Exact original metadata-release guard, for nonfailing whole-batch cleanup.
	bool can_release_published() const noexcept;
	bool read_progress(quest_mobile_native_item_progress *) const noexcept;
	static bool adopt_published(const player_item_snapshot &,
				    const native_mobile_birth_item_recipe &, struct obj_data *,
				    const quest_mobile_native_item_progress &,
				    std::span<const quest_mobile_native_item_effect>,
				    quest_mobile_native_item_stage *) noexcept;
};

// Pure observation preflight for original cold/native owners. Neither result
// authorizes custody, bindings, consumption, enrollment or a source claim.
// An observed hot body must stay with its real constructor/publication owner.
bool quest_mobile_native_item_cold_prepend_body_ready(struct obj_data *) noexcept;
bool quest_mobile_native_item_cold_prepend_cut_ready(size_t count) noexcept;
void quest_mobile_native_item_observe_native_prepend(struct obj_data *) noexcept;
// Nonallocating observation at the original extraction cut; grants no authority.
void quest_mobile_native_item_observe_extraction(struct obj_data *) noexcept;
void free_world();

#endif /* #ifndef _SOJ_DB_H_ */
