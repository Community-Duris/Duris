#ifndef DURIS_CONSUMABLE_DESCRIPTOR_LIFETIME_H
#define DURIS_CONSUMABLE_DESCRIPTOR_LIFETIME_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

struct descriptor_data;
struct mm_ds;
class item_consumable_publication_owner;
class consumable_prepared_output_owner;
class consumable_prepared_transport_owner;
class consumable_descriptor_lifetime_registry;
namespace duris_consumable_detail
{
class transport_descriptor_creator_access;
}

// Creator/close hooks only. No public raw-pointer registration or lease minting.
class consumable_descriptor_lifetime_hooks final
{
    private:
	friend int new_descriptor(int, int);
	friend void close_socket(descriptor_data *);
	friend int copyover_recover(int *, int *, int *);
	friend class duris_consumable_detail::transport_descriptor_creator_access;
	enum class origin
	{
		socket,
		transport_open,
		copyover
	};
	static void created(descriptor_data *, mm_ds *, origin) noexcept;
	static void invalidated(descriptor_data *) noexcept;
	static uint64_t recovery_begin() noexcept;
	static void recovery_failed(uint64_t) noexcept;
};

// Retained metadata only on async paths. All descriptor access is private to
// the actual game-thread, no-yield prepared writer scope below.
class consumable_descriptor_lifetime_loan final
{
    public:
	consumable_descriptor_lifetime_loan(const consumable_descriptor_lifetime_loan &) = delete;
	consumable_descriptor_lifetime_loan &
	operator=(const consumable_descriptor_lifetime_loan &) = delete;
	~consumable_descriptor_lifetime_loan() noexcept;

    private:
	friend class consumable_prepared_output_owner;
	friend class consumable_prepared_transport_owner;
	friend class consumable_descriptor_lifetime_registry;
	struct control;
	class scope final
	{
	    public:
		scope(const scope &) = delete;
		scope &operator=(const scope &) = delete;
		~scope() noexcept;

	    private:
		friend class consumable_descriptor_lifetime_loan;
		friend class consumable_prepared_output_owner;
		friend class consumable_prepared_transport_owner;
		explicit scope(control *, descriptor_data *) noexcept;
		descriptor_data *descriptor() const noexcept;
		control *control_ = nullptr;
		descriptor_data *descriptor_ = nullptr;
	};
	explicit consumable_descriptor_lifetime_loan(control *) noexcept;
	static std::unique_ptr<consumable_descriptor_lifetime_loan>
	borrow(descriptor_data *, bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	scope enter() const noexcept;
	static bool registry_storage_bytes(size_t *) noexcept;
	control *control_ = nullptr;
};

// Connected actual legacy plaintext queue participant. The caller supplies
// complete original rendered output; recipient rendering and economic/Save/SQL
// success are still owned by the real native publication owner. No public send.
class consumable_prepared_output_owner final
{
    public:
	consumable_prepared_output_owner(const consumable_prepared_output_owner &) = delete;
	consumable_prepared_output_owner &
	operator=(const consumable_prepared_output_owner &) = delete;
	~consumable_prepared_output_owner() noexcept;

    private:
	friend class item_consumable_publication_owner;
	struct state;
	explicit consumable_prepared_output_owner(std::unique_ptr<state>) noexcept;
	static std::unique_ptr<consumable_prepared_output_owner>
	prepare(descriptor_data *, std::span<const std::string_view>,
		bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	bool current() const noexcept;
	bool transfer() noexcept;
	bool current_storage_bytes(size_t *) const noexcept;
	std::unique_ptr<state> state_;
};

#endif
