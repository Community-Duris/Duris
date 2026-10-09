#ifndef DURIS_FLATFILE_ACCOUNTING_SHOP_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_SHOP_TRANSACTION_H
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "flatfile/flatfile_item_repository.h"

// Every modern native pet contributes its actual namespace and owner clock,
// including empty forests and revision zero. Values grant no publication right.
struct flatfile_accounting_shop_pet_owner_revision
{
	item_owner_identity owner{ item_owner_type::pet, 0, 0 };
	uint64_t owner_revision = 0;
};
// CURRENT borrowed-lock values, independent of immutable original receipt proof.
// player is the complete reconciled literal snapshot; current cash comes from
// player_domains, not potentially older raw money status cells in player.
// No native hold, execution permission, producer checkpoint or publication ACK.
struct flatfile_accounting_shop_projection
{
	flatfile_player_domain_record player_domains;
	player_snapshot player;
	flatfile_shopkeeper_record keeper;
	uint64_t player_owner_revision = 0, keeper_owner_revision = 0;
	uint64_t primary_owner_revision = 0, counterparty_owner_revision = 0;
	std::vector<flatfile_item_ownership_record> player_custody, keeper_custody, pet_custody,
		selected_custody;
	std::vector<flatfile_accounting_shop_pet_owner_revision> pet_owner_revisions;
};
class flatfile_accounting_shop_transaction
{
    public:
	static critical_apply_result apply(const std::string &, const critical_command &);
	// Original command digest/native receipt/canonical root/source/item refs;
	// never re-derive historical cash or custody from progressed current state.
	static critical_apply_result verify_retained_locked(const std::string &,
							    const flatfile_authority_lock &,
							    const critical_command &) noexcept;
	// Caller holds original lock across receipt proof, CURRENT whole forests,
	// native publication and cleanup. No local/reacquired authority lock.
	// Refusal preserves output; missing/corrupt mandatory evidence never adopts
	// historical literal bodies or converts absence to an empty current forest.
	static unsigned int
	read_current_locked(const std::string &, const flatfile_authority_lock &,
			    const critical_command &, const critical_completion &,
			    flatfile_accounting_shop_projection *, std::string *) noexcept;

	// Genuine absent accounting/native trade receipts and full original v8
	// BEFORE values under this same borrowed root lock. This does not prove
	// coordinator refusal or that an earlier native source checkpoint never ran.
	// The original coordinator/pipeline separately owns refusal and hold release.
	// Strong output; no synthesized receipt, native permission or local lock.
	static unsigned int read_never_admitted_before_locked(const std::string &,
							      const flatfile_authority_lock &,
							      const critical_command &,
							      flatfile_accounting_shop_projection *,
							      std::string *) noexcept;

    private:
	static void verify_record_locked(const std::string &, const flatfile_authority_lock &,
					 const flatfile_accounting_record &, std::string *);
};
#endif
