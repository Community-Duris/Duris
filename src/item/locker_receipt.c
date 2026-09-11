#include "item/locker_receipt.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <openssl/crypto.h>
#include <openssl/sha.h>

namespace
{
constexpr size_t header_bytes = 16, command_max = 1024;
constexpr size_t receipt_max =
	header_bytes + command_max + LOCKER_RECEIPT_TEXT_MAX + SHA256_DIGEST_LENGTH;

void put32(std::vector<uint8_t> &bytes, size_t offset, uint32_t value)
{
	for (size_t n = 0; n < 4; ++n)
		bytes[offset + n] = static_cast<uint8_t>(value >> (n * 8));
}
uint32_t get32(const std::vector<uint8_t> &bytes, size_t offset)
{
	uint32_t value = 0;
	for (size_t n = 0; n < 4; ++n)
		value |= static_cast<uint32_t>(bytes[offset + n]) << (n * 8);
	return value;
}
bool valid(const locker_receipt &receipt, currency_command_payload *payment)
{
	if (receipt.text.empty() || receipt.text.size() > LOCKER_RECEIPT_TEXT_MAX ||
	    receipt.text.find('\0') != std::string::npos ||
	    static_cast<unsigned>(receipt.state) >
		    static_cast<unsigned>(locker_receipt_state::failed) ||
	    !critical_command_valid(receipt.payment) ||
	    !currency_command_decode_payload(receipt.payment, payment) ||
	    (payment->reason != currency_reason_type::bank_payment &&
	     payment->reason != currency_reason_type::wallet_spend))
		return false;
	int64_t total = 0, denomination = 1;
	for (size_t n = 0; n < CURRENCY_DENOMINATION_COUNT; ++n)
	{
		for (const auto *delta : { &payment->wallet_delta, &payment->bank_delta })
		{
			if (delta->amount[n] < -INT_MAX || delta->amount[n] > INT_MAX)
				return false;
			total += delta->amount[n] * denomination;
		}
		denomination *= 10;
	}
	return total < 0;
}
}

bool locker_receipt_encode(const locker_receipt &receipt, std::vector<uint8_t> *bytes)
{
	currency_command_payload payment = {};
	std::vector<uint8_t> command;
	if (!bytes || !valid(receipt, &payment) ||
	    critical_command_encode(receipt.payment, &command) !=
		    critical_command_codec_result::ok ||
	    command.size() > command_max)
		return false;
	bytes->assign(header_bytes, 0);
	memcpy(bytes->data(), "LIDR", 4);
	(*bytes)[4] = 1;
	(*bytes)[5] = static_cast<uint8_t>(receipt.state);
	put32(*bytes, 8, static_cast<uint32_t>(command.size()));
	put32(*bytes, 12, static_cast<uint32_t>(receipt.text.size()));
	bytes->insert(bytes->end(), command.begin(), command.end());
	bytes->insert(bytes->end(), receipt.text.begin(), receipt.text.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes->data(), bytes->size(), digest.data());
	bytes->insert(bytes->end(), digest.begin(), digest.end());
	return true;
}

bool locker_receipt_decode(const std::vector<uint8_t> &bytes, uint32_t pid, locker_receipt *receipt)
{
	if (!receipt || bytes.size() < header_bytes + SHA256_DIGEST_LENGTH ||
	    bytes.size() > receipt_max || memcmp(bytes.data(), "LIDR", 4) || bytes[4] != 1 ||
	    bytes[6] || bytes[7])
		return false;
	const size_t command_size = get32(bytes, 8), text_size = get32(bytes, 12);
	if (command_size > command_max || text_size > LOCKER_RECEIPT_TEXT_MAX ||
	    header_bytes + command_size + text_size + SHA256_DIGEST_LENGTH != bytes.size())
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size() - digest.size(), digest.data());
	if (CRYPTO_memcmp(digest.data(), bytes.data() + bytes.size() - digest.size(),
			  digest.size()))
		return false;
	locker_receipt candidate;
	if (critical_command_decode(bytes.data() + header_bytes, command_size,
				    &candidate.payment) != critical_command_codec_result::ok)
		return false;
	candidate.state = static_cast<locker_receipt_state>(bytes[5]);
	candidate.text.assign(
		reinterpret_cast<const char *>(bytes.data() + header_bytes + command_size),
		text_size);
	currency_command_payload payment = {};
	if (!valid(candidate, &payment) || payment.pid != pid)
		return false;
	*receipt = std::move(candidate);
	return true;
}

flatfile_read_result locker_receipt_read(const std::string &directory, uint32_t pid,
					 locker_receipt *receipt)
{
	std::vector<uint8_t> bytes;
	std::string error;
	const auto result = flatfile_read(directory, std::to_string(pid) + ".receipt", receipt_max,
					  &bytes, &error);
	if (result != flatfile_read_result::ok)
		return result;
	return locker_receipt_decode(bytes, pid, receipt) ? flatfile_read_result::ok :
							    flatfile_read_result::invalid;
}

bool locker_receipt_write(const std::string &directory, const locker_receipt &receipt)
{
	std::vector<uint8_t> bytes;
	currency_command_payload payment = {};
	std::string error;
	return valid(receipt, &payment) && locker_receipt_encode(receipt, &bytes) &&
	       flatfile_atomic_write(directory, std::to_string(payment.pid) + ".receipt", bytes,
				     &error);
}
