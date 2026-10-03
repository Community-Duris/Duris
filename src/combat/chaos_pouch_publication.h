#ifndef CHAOS_POUCH_PUBLICATION_H
#define CHAOS_POUCH_PUBLICATION_H

struct obj_data;
struct craft_pouch_mutation;

// Allocate a complete counter replacement before changing the live object.
// A replay whose counters already match the after-image is a successful no-op.
bool chaos_pouch_publish_committed(obj_data *pouch, const craft_pouch_mutation &mutation);

#endif
