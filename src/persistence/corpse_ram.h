#ifndef DURIS_CORPSE_RAM_H
#define DURIS_CORPSE_RAM_H

#include "persistence/corpse_lifecycle_command.h"
#include "persistence/critical_command_completion.h"
#include "item/item_transfer_command.h"

void corpse_ram_reset(uint64_t catalog_revision, bool flatfile);
bool corpse_ram_seed(const corpse_lifecycle_payload &metadata, uint64_t revision);
void corpse_ram_begin();
unsigned int corpse_ram_prepare_item(const item_transfer_payload &payload,
				     item_transfer_result *result);
critical_apply_result corpse_ram_prepare(const critical_command &command);
void corpse_ram_publish() noexcept;
void corpse_ram_note_world_change();

#endif
