#ifndef DURIS_COIN_PHYSICAL_RECOVERY_H
#define DURIS_COIN_PHYSICAL_RECOVERY_H

#include "persistence/critical_command_completion.h"
#include "player/inert_item_stage.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot.h"
#include <string>

class flatfile_authority_lock;
enum class flatfile_corpse_restore_result;

// Only the original shared boot function may prepare, publish, or release this
// holder. It owns an inert unpublished literal and, until the whole boot succeeds,
// any enrolled object. Destruction rolls that enrollment back; no independent
// per-UID publisher can escape the shared all-detached staging/cleanup boundary.
class flatfile_coin_boot_stage final
{
    public:
	flatfile_coin_boot_stage() noexcept = default;
	~flatfile_coin_boot_stage() noexcept;
	flatfile_coin_boot_stage(flatfile_coin_boot_stage &&) noexcept;
	flatfile_coin_boot_stage &operator=(flatfile_coin_boot_stage &&) noexcept;
	flatfile_coin_boot_stage(const flatfile_coin_boot_stage &) = delete;
	flatfile_coin_boot_stage &operator=(const flatfile_coin_boot_stage &) = delete;

    private:
	friend flatfile_corpse_restore_result flatfile_corpse_restore_catalog(
		const std::string &, std::string *);
	static bool prepare(const std::string &, const flatfile_authority_lock &,
		uint64_t uid, flatfile_coin_boot_stage &) noexcept;
	bool publish() noexcept;
	void finish() noexcept;
	void reset() noexcept;
	inert_item_stage prepared_;
	item_ownership_runtime_entry identity_ = {};
	player_item_snapshot literal_;
	std::string root_;
	const flatfile_authority_lock *cut_ = nullptr;
	P_obj published_ = nullptr;
};

// Classification only. Failure preserves supplied outputs; success grants no
// SQL authority, save reservation, physical publication or ACK permission.
bool coin_physical_recovery_identity(const critical_command &command, int *wallet_pid,
				     uint64_t *pile_uid) noexcept;

// Synchronous game-thread backend projection owner for the original single ordinary
// room pile drop/full pickup/partial pickup. Reacquires native current and retained
// authority on every call, including ACK retries. No actor is required. A true
// result requires confirmed SQL original-session cleanup or the original flat
// identity-then-authority lock through exact retained/current/native proof. This does NOT
// ACK or release a save hold; the caller retains its reservation and exact sealed
// receipt through guarded ACK. False retains the original operation unchanged.
// Native money rendering/disposal may be used; no domain completion callback,
// player command, template parser, normal object factory or UID issuance occurs.
// Flat central admission stays closed until shared hold/replay integration and qualification.
bool coin_physical_recovery_publish(const critical_command &command,
				    const critical_completion &sealed_completion) noexcept;

#ifndef __NO_MYSQL__
struct st_mysql;
// Game-thread SQL boot owner, after an ordinary coin operation's ACK. Reacquire
// authenticated retained proof/current custody inside the caller's original
// active transaction. Enroll only the exact current room pile and original UID;
// no wallet repair, command reconstruction, domain callback or ACK occurs.
// Caller must confirm original-session rollback/idle cleanup before claiming
// completion. Refusal retains durable proof and forbids legacy substitution.
bool coin_physical_recovery_restore_room(st_mysql *connection, uint64_t uid) noexcept;
#endif

// Complete original passive classification including full immutable intent,
// wallet/pile command, real room drop/pickup and literal shape checks.
// Strong output; no native construction, save hold, publication or ACK.
// Authentic inputs/prior outputs/all sibling owners remain full outer state.
bool coin_physical_recovery_identity_bounded(const critical_command &, int *, uint64_t *,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t outer_live) noexcept;

#endif
