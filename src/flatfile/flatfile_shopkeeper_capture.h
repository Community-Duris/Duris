#ifndef DURIS_FLATFILE_SHOPKEEPER_CAPTURE_H
#define DURIS_FLATFILE_SHOPKEEPER_CAPTURE_H

#include "flatfile/flatfile_shopkeeper_repository.h"
#include "player_snapshot.h"

struct char_data;
typedef struct char_data *P_char;

player_snapshot_capture_result flatfile_shopkeeper_capture(P_char shopkeeper, uint32_t shop_id,
							   uint64_t revision, int64_t saved_at,
							   flatfile_shopkeeper_record *record_out);

#endif
