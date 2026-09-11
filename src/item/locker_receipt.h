#pragma once

#include "economy/currency_command.h"
#include "flatfile/flatfile_store.h"
#include <string>

constexpr size_t LOCKER_RECEIPT_TEXT_MAX = 64 * 1024;
enum class locker_receipt_state : uint8_t
{
	prepared,
	paid,
	failed
};
struct locker_receipt
{
	critical_command payment{};
	std::string text;
	locker_receipt_state state = locker_receipt_state::prepared;
};

bool locker_receipt_encode(const locker_receipt &receipt, std::vector<uint8_t> *bytes);
bool locker_receipt_decode(const std::vector<uint8_t> &bytes, uint32_t pid,
			   locker_receipt *receipt);
flatfile_read_result locker_receipt_read(const std::string &directory, uint32_t pid,
					 locker_receipt *receipt);
bool locker_receipt_write(const std::string &directory, const locker_receipt &receipt);
