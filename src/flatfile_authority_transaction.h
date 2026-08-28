#ifndef DURIS_FLATFILE_AUTHORITY_TRANSACTION_H
#define DURIS_FLATFILE_AUTHORITY_TRANSACTION_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct flatfile_authority_after_image
{
	std::string filename;
	std::vector<uint8_t> bytes;
};

enum class flatfile_authority_transaction_result
{
	ok,
	not_found,
	invalid,
	io_error
};

class flatfile_authority_lock
{
    public:
	flatfile_authority_lock() noexcept;
	~flatfile_authority_lock();
	flatfile_authority_lock(const flatfile_authority_lock &) = delete;
	flatfile_authority_lock &operator=(const flatfile_authority_lock &) = delete;

	bool acquire(const std::string &root, std::string *error);

    private:
	struct state;
	std::unique_ptr<state> state_;
	bool owns(const std::string &root) const;
	friend flatfile_authority_transaction_result
	flatfile_authority_transaction_recover(const std::string &, const flatfile_authority_lock &,
					       std::string *);
	friend flatfile_authority_transaction_result
	flatfile_authority_transaction_commit(const std::string &, const flatfile_authority_lock &,
					      const std::vector<flatfile_authority_after_image> &,
					      std::string *);
};

flatfile_authority_transaction_result
flatfile_authority_transaction_recover(const std::string &root, const flatfile_authority_lock &lock,
				       std::string *error);
flatfile_authority_transaction_result
flatfile_authority_transaction_commit(const std::string &root, const flatfile_authority_lock &lock,
				      const std::vector<flatfile_authority_after_image> &images,
				      std::string *error);

#endif
