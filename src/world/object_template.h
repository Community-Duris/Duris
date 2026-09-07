#ifndef DURIS_OBJECT_TEMPLATE_H
#define DURIS_OBJECT_TEMPLATE_H

#include "core/structs.h"
#include <string>
#include <vector>

// Owned prototype values only: no live objects, identities, links or events.
struct object_template_description
{
	std::string keyword;
	std::string description;
};
struct object_template
{
	decltype(obj_data::R_num) R_num{};
	decltype(obj_data::type) type{};
	decltype(obj_data::material) material{};
	decltype(obj_data::craftsmanship) craftsmanship{};
	decltype(obj_data::extra_flags) extra_flags{};
	decltype(obj_data::wear_flags) wear_flags{};
	decltype(obj_data::extra2_flags) extra2_flags{};
	decltype(obj_data::anti_flags) anti_flags{};
	decltype(obj_data::anti2_flags) anti2_flags{};
	decltype(obj_data::value) value{};
	decltype(obj_data::weight) weight{};
	decltype(obj_data::cost) cost{};
	decltype(obj_data::condition) condition{};
	decltype(obj_data::bitvector) bitvector{};
	decltype(obj_data::bitvector2) bitvector2{};
	decltype(obj_data::bitvector3) bitvector3{};
	decltype(obj_data::bitvector4) bitvector4{};
	decltype(obj_data::bitvector5) bitvector5{};
	decltype(obj_data::affected) affected{};
	decltype(obj_data::trap_eff) trap_eff{};
	decltype(obj_data::trap_dam) trap_dam{};
	decltype(obj_data::trap_charge) trap_charge{};
	decltype(obj_data::trap_level) trap_level{};
	std::string name, description, short_description, action_description;
	std::vector<object_template_description> descriptions;
};

// Boot-only cache population; misses at runtime never fall back to file parsing.
bool cache_object_template(int vnum);
const object_template *find_object_template(int vnum);
P_obj instantiate_object_template(const object_template &prototype);
#endif
