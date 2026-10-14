#ifndef CORPSE_MONEY_LOAD_ORIGIN_H
#define CORPSE_MONEY_LOAD_ORIGIN_H

#include <cstdint>
#include <type_traits>

struct obj_data;

// Process-local observation of one original corpse-loader row with no stored
// item UID. The genuine loader records the freshly constructed object's address,
// UID and corpse identity. It does not create custody, a durable UID or authority.
// No constructor: obj_data is allocated and reused by zeroing native pools.
struct corpse_money_load_origin
{
    private:
	const obj_data *object_;
	uint64_t item_uid_;
	uint64_t corpse_owner_id_;
	friend bool sql_load_all_corpses(void);

    public:
	bool present() const noexcept { return object_ || item_uid_ || corpse_owner_id_; }
	bool matches(const obj_data *object, uint64_t item_uid,
		     uint64_t corpse_owner_id) const noexcept
	{
		return object && item_uid && corpse_owner_id && object_ == object &&
		       item_uid_ == item_uid && corpse_owner_id_ == corpse_owner_id;
	}
};

static_assert(std::is_trivial_v<corpse_money_load_origin>);
static_assert(std::is_trivially_copyable_v<corpse_money_load_origin>);
static_assert(std::is_standard_layout_v<corpse_money_load_origin>);

#endif
