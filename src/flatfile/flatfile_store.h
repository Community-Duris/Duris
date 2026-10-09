#ifndef DURIS_FLATFILE_STORE_H
#define DURIS_FLATFILE_STORE_H

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

enum class flatfile_read_result
{
	ok,
	not_found,
	invalid,
	io_error
};

bool flatfile_atomic_write(const std::string &directory, const std::string &name,
			   const std::vector<uint8_t> &bytes, std::string *error);
// Reports rename separately: a false return after publication is uncertain.
bool flatfile_atomic_write_with_publication(const std::string &directory, const std::string &name,
					    const std::vector<uint8_t> &bytes, std::string *error,
					    bool *published);
bool flatfile_atomic_remove(const std::string &directory, const std::string &name, bool missing_ok,
			    std::string *error);
flatfile_read_result flatfile_read(const std::string &directory, const std::string &name,
				   size_t maximum_size, std::vector<uint8_t> *bytes,
				   std::string *error);
bool flatfile_lock_acquire(const std::string &directory, const std::string &name, int *lock_fd,
			   std::string *error);
void flatfile_lock_release(int lock_fd);

// Passive prospective memory admission only: no storage/source/ACK authority.
// The callback is nonallocating and admits absolute simultaneously live scratch.
// Caller includes its existing buffers/paths in outer_live_scratch, holds the
// admitted peak until these temporaries die, and restores prior aggregate on exit.
using flatfile_scratch_reserve_fn = bool (*)(size_t, void *) noexcept;
// Secure SAME-FD metadata/size check precedes first fresh vector allocation.
// Strong output on every refusal; no allocation-backed diagnostics. ENOBUFS is
// budget refusal, ENOMEM allocation failure, never not_found/absence authority.
// Deliberate capacity policy is pinned to libstdc++13 C++11 ABI; otherwise ENOTSUP.
flatfile_read_result flatfile_read_bounded(const std::string &directory, const std::string &name,
					   size_t maximum_size, std::vector<uint8_t> *bytes,
					   flatfile_scratch_reserve_fn reserve_scratch_peak,
					   void *context, size_t outer_live_scratch) noexcept;

#endif
