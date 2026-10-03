#ifndef DURIS_SPELL_WARD_IDENTITY_PRIVATE_H
#define DURIS_SPELL_WARD_IDENTITY_PRIVATE_H

// Shared original ward values and classification only. Mutable ward behavior
// stays in spell_wards.c; this private provider is also linked by PFILE.
namespace spell_ward_identity
{
enum ward_kind
{
	WARD_KIND_MINOR = 0,
	WARD_KIND_SPIRIT,
	WARD_KIND_GREATER_SPIRIT,
	WARD_KIND_GLOBE,
	WARD_KIND_COUNT
};

extern const int ward_spells[WARD_KIND_COUNT];
bool is_ward_spell(int spell);
}

#endif
