#include "flatfile/flatfile_shopkeeper_repository.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <new>

#ifndef EXPECT_FLAT_CHECKPOINT_SOURCE_SUPPORTED
#error Select the actual supported or refusal profile.
#endif
#ifdef NDEBUG
#error Checkpoint status controls require assertions.
#endif

namespace
{
using result = flatfile_shopkeeper_result;
bool fail_allocation = false;
size_t allocation_attempts = 0;
size_t failure_ordinal = 1;
struct budget
{
	size_t calls = 0, peak = 0;
	bool deny = false;
};
bool reserve(size_t bytes, void *opaque) noexcept
{
	auto &value = *static_cast<budget *>(opaque);
	++value.calls;
	value.peak = std::max(value.peak, bytes);
	return !value.deny && bytes <= 32 * 1024 * 1024;
}
flatfile_shopkeeper_record record(uint32_t shop)
{
	flatfile_shopkeeper_record value;
	value.shop_id = shop;
	value.mob_vnum = 11005;
	value.room_vnum = 22007;
	value.saved_at = 123;
	value.revision = 1;
	value.cash = 456;
	value.affects = { { 9, 20, -3, 4, { 1, 2, 4, 8, 16 } } };
	return value;
}
std::vector<uint8_t> encoded(const flatfile_shopkeeper_record &value)
{
	std::vector<uint8_t> bytes;
	assert(flatfile_shopkeeper_initial_checkpoint_encode(value, &bytes));
	return bytes;
}
size_t heap(const flatfile_shopkeeper_record &value)
{
	// These fixtures have no item rows or strings; prior affect storage remains outer.
	assert(value.items.empty());
	return value.affects.capacity() * sizeof(flatfile_shopkeeper_affect_record);
}
} // namespace

extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t bytes)
{
	if (fail_allocation)
	{
		if (++allocation_attempts == failure_ordinal)
			throw std::bad_alloc();
	}
	return __real__Znwm(bytes);
}

int main()
{
	// The additive result does not renumber any existing status.
	static_assert(static_cast<int>(result::ok) == 0);
	static_assert(static_cast<int>(result::not_found) == 1);
	static_assert(static_cast<int>(result::already_exists) == 2);
	static_assert(static_cast<int>(result::stale) == 3);
	static_assert(static_cast<int>(result::invalid) == 4);
	static_assert(static_cast<int>(result::io_error) == 5);
	static_assert(static_cast<int>(result::allocation_failure) == 6);
	constexpr bool supported = EXPECT_FLAT_CHECKPOINT_SOURCE_SUPPORTED != 0;
	size_t source = 937;
	assert(flatfile_shopkeeper_initial_checkpoint_decode_fixed_source_frame_bytes(&source) ==
	       supported);
	assert(supported ? source > 0 : source == 937);
	const auto original = record(7);
	const auto bytes = encoded(original);
	auto output = record(99);
	const auto prior = encoded(output);
	budget admitted;
	const size_t outer = sizeof(bytes) + bytes.capacity() + sizeof(output) + heap(output) +
			     sizeof(admitted) + sizeof(source);
	const auto decoded = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		bytes, &output, reserve, &admitted, outer);
	if (!supported)
	{
		assert(decoded == result::io_error && errno == ENOTSUP);
		assert(encoded(output) == prior);
		allocation_attempts = 0;
		fail_allocation = true;
		const auto refused = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
			bytes, &output, reserve, &admitted, outer);
		fail_allocation = false;
		assert(refused == result::io_error && errno == ENOTSUP && !allocation_attempts);
		assert(encoded(output) == prior);
		std::puts("FLAT_CHECKPOINT_ALLOCATION_STATUS supported=0 refusal_only=1");
		return 0;
	}
	assert(decoded == result::ok && encoded(output) == bytes && admitted.calls > 1);
	output = record(99);
	budget fault;
	allocation_attempts = 0;
	errno = EDOM;
	// The genuine fixed decoder's first C++ allocation is inside its bad_alloc catch.
	fail_allocation = true;
	const auto failed = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		bytes, &output, reserve, &fault, outer);
	fail_allocation = false;
	assert(failed == result::allocation_failure && errno == ENOMEM);
	assert(allocation_attempts == 1 && fault.calls > 1 && encoded(output) == prior);
	// Count the actual healthy path, then deny every allocation ordinal through
	// decode, original item validation and canonical re-encoding. A caught child
	// allocation failure must never become malformed input in its parent.
	failure_ordinal = SIZE_MAX;
	allocation_attempts = 0;
	budget complete;
	fail_allocation = true;
	const auto complete_result = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		bytes, &output, reserve, &complete, outer);
	fail_allocation = false;
	assert(complete_result == result::ok && encoded(output) == bytes);
	const size_t allocation_cuts = allocation_attempts;
	assert(allocation_cuts > 1);
	for (size_t ordinal = 1; ordinal <= allocation_cuts; ++ordinal)
	{
		output = record(99);
		budget cut;
		allocation_attempts = 0;
		failure_ordinal = ordinal;
		errno = EDOM;
		fail_allocation = true;
		const auto cut_result = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
			bytes, &output, reserve, &cut, outer);
		fail_allocation = false;
		assert(cut_result == result::allocation_failure && errno == ENOMEM);
		assert(allocation_attempts == ordinal && encoded(output) == prior);
	}
	failure_ordinal = 1;
	budget denied;
	denied.deny = true;
	allocation_attempts = 0;
	fail_allocation = true;
	const auto refused = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		bytes, &output, reserve, &denied, outer);
	fail_allocation = false;
	assert(refused == result::io_error && errno == ENOBUFS && !allocation_attempts);
	assert(denied.calls == 1 && encoded(output) == prior);
	for (const size_t offset : { size_t{ 0 }, size_t{ 8 } })
	{
		auto bad = bytes;
		bad[offset] ^= 1; // Existing magic/version refusal, before allocating a catalog.
		allocation_attempts = 0;
		fail_allocation = true;
		const auto malformed = flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
			bad, &output, reserve, &admitted, outer + bad.capacity());
		fail_allocation = false;
		assert(malformed == result::invalid && errno == EBADMSG && !allocation_attempts);
		assert(encoded(output) == prior);
	}
	assert(flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		       bytes, nullptr, reserve, &admitted, outer) == result::invalid);
	assert(errno == EINVAL);
	assert(flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		       bytes, &output, nullptr, &admitted, outer) == result::invalid);
	assert(errno == EINVAL && encoded(output) == prior);
	std::puts(
		"FLAT_CHECKPOINT_ALLOCATION_STATUS supported=1 allocation_failure=1 strong_output=1");
}
