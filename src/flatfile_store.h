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
bool flatfile_atomic_remove(const std::string &directory, const std::string &name, bool missing_ok,
			    std::string *error);
flatfile_read_result flatfile_read(const std::string &directory, const std::string &name,
				   size_t maximum_size, std::vector<uint8_t> *bytes,
				   std::string *error);
bool flatfile_lock_acquire(const std::string &directory, const std::string &name, int *lock_fd,
			   std::string *error);
void flatfile_lock_release(int lock_fd);

#endif
