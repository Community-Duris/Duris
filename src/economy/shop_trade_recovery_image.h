#ifndef DURIS_SHOP_TRADE_RECOVERY_IMAGE_H
#define DURIS_SHOP_TRADE_RECOVERY_IMAGE_H

#include "economy/shop_trade_recovery_manifest.h"
#include "persistence/shop_item_runtime_payload.h"

#ifndef __NO_MYSQL__
// Pure reconstruction from an already complete, locked current image. The
// caller owns authority, lifetime, custody and physical census verification.
// Retained order and digest authenticate values, not SQL or publication authority.
// Missing/extra rows and unavailable literal payloads refuse; no repair/adoption,
// historical row-ID assertion, world mutation, runtime-token fabrication or ACK.
// Strong output on failure. Current row IDs serve only to resolve parent edges.
bool shop_trade_recovery_image_reconstruct(const shop_trade_recovery_forest_binding &,
					   shop_trade_recovery_forest_role,
					   const shop_item_runtime_image &,
					   std::vector<player_item_snapshot> *) noexcept;
#endif

#endif
