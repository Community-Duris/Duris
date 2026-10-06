/*
   ***************************************************************************
   *  File: studioproclib.h                                   Part of Duris *
   *  Usage: the 'sayresponse' and 'transporter' object proclibs            *
   *                                                                        *
   *  Objects authored offline already carry _proclib_sayresponse /         *
   *  _proclib_transporter extra-descriptions.  With those names missing    *
   *  from object_proc_libs[], proclibObj_add() returns -1 and              *
   *  read_object() keeps the description as plain text (db.c:2940) - the   *
   *  objects load, nothing fires, nothing is logged.  This module makes    *
   *  the two names real.                                                   *
   *                                                                        *
   *  Kept in its own translation unit so that specs.library.c gains only   *
   *  an include, two registry rows and the prototype bridge.               *
   ***************************************************************************
 */

#ifndef _STUDIOPROCLIB_H_
#define _STUDIOPROCLIB_H_

#include "core/structs.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

char *proclibobj_parse_sayresponse(char *argument);
int proclibobj_sayresponse(P_obj obj, P_char ch, int cmd, char *argument);

char *proclibobj_parse_transporter(char *argument);
int proclibobj_transporter(P_obj obj, P_char ch, int cmd, char *argument);

/* Prototype-level bridge that lets special() reach INSTANCE proclibs.
   Installed by proclibObj_add() on the vnum of any object that gains a
   proclib, so 'enter <keyword>' reaches proclibobj_transporter without
   patching interp.c at all. */
int proclib_obj_cmd_bridge(P_obj obj, P_char ch, int cmd, char *argument);

/* Remember the object proc the bridge displaced on this vnum, so the
   bridge can call it first instead of the vnum having to choose between
   its existing proc and its instance proclibs. */
void proclib_chain_install(int rnum, int (*prev)(P_obj, P_char, int, char *));

// Value-only eligibility for original saved post-parse descriptions. No parser,
// special callback, description/flag mutation, binding or periodic scheduling.
bool proclib_saved_binding_eligible(P_obj, bool *eligible) noexcept;

class quest_mobile_native_item_stage;
enum class native_mobile_birth_library : uint8_t;
// Original parser/descriptor preparation only; native probes, events and
// bindings remain individually owned publication steps.
class quest_mobile_native_original_proclib
{
    private:
	friend class quest_mobile_native_item_stage;
	static int prepare(P_obj, char *name, char *arguments, size_t *library_index);
	static bool probe(P_obj, size_t library_index, bool *periodic) noexcept;
	static bool retained_library(size_t, native_mobile_birth_library *) noexcept;
	static bool retained_index(native_mobile_birth_library, size_t *) noexcept;
};

class shop_trade_original_procedure_binding_stage;
// Private allocation/chain bookkeeping capability, never SQL/native authority.
// Only the original cold binding owner may prepare/validate/commit its batch.
class proclib_recovery_chain_stage
{
    public:
	proclib_recovery_chain_stage() noexcept = default;
	~proclib_recovery_chain_stage() noexcept;
	proclib_recovery_chain_stage(proclib_recovery_chain_stage &&) noexcept;
	proclib_recovery_chain_stage &operator=(proclib_recovery_chain_stage &&) noexcept;
	proclib_recovery_chain_stage(const proclib_recovery_chain_stage &) = delete;
	proclib_recovery_chain_stage &operator=(const proclib_recovery_chain_stage &) = delete;

    private:
	friend class shop_trade_original_procedure_binding_stage;
	friend class quest_mobile_native_item_stage;
	struct request
	{
		int rnum;
		obj_proc_type previous;
	};
	static bool prepare(std::span<const request>, proclib_recovery_chain_stage &) noexcept;
	static bool predecessor_matches(int, obj_proc_type) noexcept;
	size_t retained_bytes() const noexcept;
	bool valid() const noexcept;
	void commit_unchecked() noexcept;
	void reset() noexcept;
	void *allocation_ = nullptr, *expected_ = nullptr;
	int expected_top_ = 0, expected_cap_ = 0, next_top_ = 0, next_cap_ = 0;
	bool prepared_ = false;
	std::vector<request> requests_;
};

// Probe one already-restored saved description through the existing registry.
// No parameter parser, description mutation, template binding or event schedule.
// Normal success preserves the individual library's CMD_SET_PERIODIC result;
// failure/exception preserves output. Caller owns original proof and must retain
// callback uncertainty/reobserve the complete world before another callback.
bool proclib_saved_periodic_probe(P_obj, size_t description_index, bool *periodic) noexcept;

/* Help text for the object_proc_libs[] registry rows in specs.library.c,
   hoisted here so each row stays one line. */
#define PROCLIB_SAYRESPONSE_HELP                                                     \
	"        Params: 'keywords' 'reply text'\n"                                  \
	"          keywords: quoted, space separated; replies when a player's say\n" \
	"                    contains any of them (object in room or held).\n"       \
	"          reply text: quoted; sent to the room as: <object> replies, '...'\n"

#define PROCLIB_TRANSPORTER_HELP                                                     \
	"        Params: keyword roomvnum\n"                                         \
	"          keyword: what the player must type after 'enter'.\n"              \
	"          roomvnum: destination room (validated when fired).  The object\n" \
	"                    must be on the ground in the actor's room.\n"

#endif /* _STUDIOPROCLIB_H_ */
