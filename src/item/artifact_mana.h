#ifndef DURIS_ARTIFACT_MANA_H
#define DURIS_ARTIFACT_MANA_H

#include "core/structs.h"
#include "item/artifact_mana_runtime.h"

// Main thread. Definition identities survive configuration toggles. Multiple
// forms may bind to the same profile ID; changing a UID's pool ID is rejected.
bool artifact_mana_publish(int object_vnum, const artifact_mana_profile &);
bool artifact_mana_can_publish(int object_vnum, const artifact_mana_profile &);
bool artifact_mana_inspect(P_obj, artifact_mana_record &);
bool artifact_mana_debit(P_obj, uint64_t cost, bool passive, uint64_t action_token);
void artifact_mana_pulse();
void artifact_mana_shutdown();
void do_itemmana(P_char, char *, int);

#endif
