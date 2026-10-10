#include "economy/currency_command.h"

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace
{
constexpr size_t PID_OFFSET = 0;
constexpr size_t RACEWAR_OFFSET = 4;
constexpr size_t REASON_OFFSET = 6;
constexpr size_t REASON_ID_OFFSET = 8;
constexpr size_t NAME_LENGTH_OFFSET = 16;
constexpr size_t NAME_OFFSET = 17;
constexpr size_t WALLET_OFFSET = 68;
constexpr size_t BANK_OFFSET = 100;

void put_u16(uint8_t *output, uint16_t value)
{
	output[0] = static_cast<uint8_t>(value);
	output[1] = static_cast<uint8_t>(value >> 8);
}

void put_u32(uint8_t *output, uint32_t value)
{
	for (unsigned int byte = 0; byte < 4; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

void put_u64(uint8_t *output, uint64_t value)
{
	for (unsigned int byte = 0; byte < 8; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint16_t get_u16(const uint8_t *input)
{
	return static_cast<uint16_t>(input[0]) |
	       static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8);
}

uint32_t get_u32(const uint8_t *input)
{
	uint32_t value = 0;
	for (unsigned int byte = 0; byte < 4; ++byte)
		value |= static_cast<uint32_t>(input[byte]) << (byte * 8);
	return value;
}

uint64_t get_u64(const uint8_t *input)
{
	uint64_t value = 0;
	for (unsigned int byte = 0; byte < 8; ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return value;
}

bool valid_reason(currency_reason_type reason)
{
	return reason > currency_reason_type::unknown &&
	       reason <= currency_reason_type::corpse_lifecycle;
}

bool valid_name(const char *name, size_t *length)
{
	if (!name || !length)
		return false;
	*length = strnlen(name, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1);
	if (!*length || *length > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	for (size_t index = 0; index < *length; ++index)
		if (static_cast<unsigned char>(name[index]) < 0x20)
			return false;
	return true;
}

bool vector_valid(const currency_vector &vector)
{
	return std::all_of(vector.amount.begin(), vector.amount.end(), [](int64_t amount)
			   { return amount != std::numeric_limits<int64_t>::min(); });
}

bool any_delta(const currency_command_payload &payload)
{
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		if (payload.wallet_delta.amount[index] || payload.bank_delta.amount[index])
			return true;
	return false;
}

void encode_vector(uint8_t *output, const currency_vector &vector)
{
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		put_u64(output + index * 8, static_cast<uint64_t>(vector.amount[index]));
}

currency_vector decode_vector(const uint8_t *input)
{
	currency_vector vector = {};
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		vector.amount[index] = static_cast<int64_t>(get_u64(input + index * 8));
	return vector;
}
} // namespace

bool currency_account_key(const char *account_name, uint8_t racewar, critical_entity_key *key)
{
	size_t length = 0;
	if (!key || !valid_name(account_name, &length))
		return false;
	std::string canonical(account_name, length);
	std::transform(canonical.begin(), canonical.end(), canonical.begin(),
		       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	canonical.push_back('\0');
	canonical.push_back(static_cast<char>(racewar));
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(reinterpret_cast<const unsigned char *>(canonical.data()), canonical.size(),
	       digest.data());
	uint64_t identity = get_u64(digest.data());
	if (!identity)
		identity = 1;
	*key = { critical_entity_type::account, identity };
	return true;
}

bool currency_command_is_rebasable_wallet_reward(const currency_command_payload &payload)
{
	if (payload.reason != currency_reason_type::wallet_reward)
		return false;
	bool positive = false;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		if (payload.wallet_delta.amount[index] < 0 || payload.bank_delta.amount[index])
			return false;
		positive = positive || payload.wallet_delta.amount[index] > 0;
	}
	return positive;
}

bool currency_command_is_rebasable_bank_reward(const currency_command_payload &payload)
{
	if (payload.reason != currency_reason_type::chaos_starter_reward)
		return false;
	bool positive_bank = false;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		if (payload.wallet_delta.amount[index] || payload.bank_delta.amount[index] < 0)
			return false;
		positive_bank = positive_bank || payload.bank_delta.amount[index] > 0;
	}
	return positive_bank;
}

bool currency_command_is_rebasable_reward(const currency_command_payload &payload)
{
	return currency_command_is_rebasable_wallet_reward(payload) ||
	       currency_command_is_rebasable_bank_reward(payload);
}

bool currency_command_encode_payload(const currency_command_payload &payload,
				     std::vector<uint8_t> *encoded)
{
	size_t name_length = 0;
	if (!encoded || !payload.pid || !valid_reason(payload.reason) ||
	    !valid_name(payload.account_name.data(), &name_length) ||
	    !vector_valid(payload.wallet_delta) || !vector_valid(payload.bank_delta) ||
	    (!any_delta(payload) && payload.reason != currency_reason_type::corpse_lifecycle))
		return false;
	encoded->assign(CURRENCY_COMMAND_PAYLOAD_BYTES, 0);
	put_u32(encoded->data() + PID_OFFSET, payload.pid);
	(*encoded)[RACEWAR_OFFSET] = payload.racewar;
	put_u16(encoded->data() + REASON_OFFSET, static_cast<uint16_t>(payload.reason));
	put_u64(encoded->data() + REASON_ID_OFFSET, static_cast<uint64_t>(payload.reason_id));
	(*encoded)[NAME_LENGTH_OFFSET] = static_cast<uint8_t>(name_length);
	memcpy(encoded->data() + NAME_OFFSET, payload.account_name.data(), name_length);
	encode_vector(encoded->data() + WALLET_OFFSET, payload.wallet_delta);
	encode_vector(encoded->data() + BANK_OFFSET, payload.bank_delta);
	return true;
}

bool currency_command_decode_payload(const critical_command &command,
				     currency_command_payload *payload)
{
	if (!payload || command.type != critical_command_type::account_bank ||
	    command.payload_version != CURRENCY_COMMAND_PAYLOAD_VERSION ||
	    command.payload.size() != CURRENCY_COMMAND_PAYLOAD_BYTES)
		return false;
	const size_t name_length = command.payload[NAME_LENGTH_OFFSET];
	if (!name_length || name_length > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	for (size_t index = NAME_OFFSET + name_length; index < WALLET_OFFSET; ++index)
		if (command.payload[index])
			return false;
	for (size_t index = BANK_OFFSET + 32; index < command.payload.size(); ++index)
		if (command.payload[index])
			return false;
	*payload = {};
	payload->pid = get_u32(command.payload.data() + PID_OFFSET);
	payload->racewar = command.payload[RACEWAR_OFFSET];
	payload->reason =
		static_cast<currency_reason_type>(get_u16(command.payload.data() + REASON_OFFSET));
	payload->reason_id =
		static_cast<int64_t>(get_u64(command.payload.data() + REASON_ID_OFFSET));
	memcpy(payload->account_name.data(), command.payload.data() + NAME_OFFSET, name_length);
	payload->wallet_delta = decode_vector(command.payload.data() + WALLET_OFFSET);
	payload->bank_delta = decode_vector(command.payload.data() + BANK_OFFSET);
	size_t checked_length = 0;
	critical_entity_key account_key = {};
	const critical_entity_key player_key = { critical_entity_type::player, payload->pid };
	return payload->pid && valid_reason(payload->reason) &&
	       valid_name(payload->account_name.data(), &checked_length) &&
	       checked_length == name_length && vector_valid(payload->wallet_delta) &&
	       vector_valid(payload->bank_delta) &&
	       (any_delta(*payload) || payload->reason == currency_reason_type::corpse_lifecycle) &&
	       currency_account_key(payload->account_name.data(), payload->racewar, &account_key) &&
	       command.keys.size() == 2 && command.expected_revisions.size() == 2 &&
	       critical_entity_key_equal(command.keys[0], player_key) &&
	       critical_entity_key_equal(command.keys[1], account_key) &&
	       critical_entity_key_equal(command.expected_revisions[0].key, player_key) &&
	       critical_entity_key_equal(command.expected_revisions[1].key, account_key);
}

bool currency_command_encode_result(const currency_command_result &result,
				    std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> *encoded)
{
	if (!encoded || !vector_valid(result.wallet) || !vector_valid(result.bank))
		return false;
	encode_vector(encoded->data(), result.wallet);
	encode_vector(encoded->data() + 32, result.bank);
	put_u64(encoded->data() + 64, result.wallet_revision);
	put_u64(encoded->data() + 72, result.bank_revision);
	return true;
}

bool currency_command_decode_result(const uint8_t *encoded, size_t size,
				    currency_command_result *result)
{
	if (!encoded || size != CURRENCY_RESULT_PAYLOAD_BYTES || !result)
		return false;
	*result = { .wallet = decode_vector(encoded),
		    .bank = decode_vector(encoded + 32),
		    .wallet_revision = get_u64(encoded + 64),
		    .bank_revision = get_u64(encoded + 72) };
	return vector_valid(result->wallet) && vector_valid(result->bank);
}

bool currency_command_build(critical_command *command, critical_operation_id operation_id,
			    const currency_command_payload &payload,
			    uint64_t expected_wallet_revision, uint64_t expected_bank_revision,
			    critical_source_site source_site,
			    critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	std::vector<uint8_t> encoded;
	critical_entity_key account_key = {};
	if (!currency_command_encode_payload(payload, &encoded) ||
	    !currency_account_key(payload.account_name.data(), payload.racewar, &account_key))
		return false;
	const critical_entity_key player_key = { critical_entity_type::player, payload.pid };
	*command = {
		.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		.operation_id = operation_id,
		.type = critical_command_type::account_bank,
		.payload_version = CURRENCY_COMMAND_PAYLOAD_VERSION,
		.source_site = source_site,
		.deadline_class = deadline_class,
		.accepted_at_usec = 0,
		.keys = { player_key, account_key },
		.expected_revisions = { { player_key, expected_wallet_revision },
					{ account_key, expected_bank_revision } },
		.payload = std::move(encoded),
	};
	return true;
}

unsigned int currency_prepare_mutation(const currency_command_payload &payload,
				       const currency_command_result &before,
				       uint64_t expected_wallet_revision,
				       uint64_t expected_bank_revision,
				       currency_revision_policy revision_policy,
				       std::optional<currency_prepared_mutation> *prepared)
{
	if (!prepared ||
	    (revision_policy != currency_revision_policy::sql_legacy &&
	     revision_policy != currency_revision_policy::flatfile_legacy &&
	     revision_policy != currency_revision_policy::bank_only) ||
	    !vector_valid(payload.wallet_delta) || !vector_valid(payload.bank_delta))
		return EINVAL;
	const bool bank_only = revision_policy == currency_revision_policy::bank_only;
	if (bank_only &&
	    (std::any_of(payload.wallet_delta.amount.begin(), payload.wallet_delta.amount.end(),
			 [](int64_t amount) { return amount != 0; }) ||
	     std::all_of(payload.bank_delta.amount.begin(), payload.bank_delta.amount.end(),
			 [](int64_t amount) { return amount == 0; })))
		return EINVAL;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		if (before.wallet.amount[index] < 0 || before.wallet.amount[index] > INT_MAX ||
		    before.bank.amount[index] < 0 || before.bank.amount[index] > INT_MAX)
			return EILSEQ;
	const bool rebase = (revision_policy == currency_revision_policy::sql_legacy &&
			     currency_command_is_rebasable_reward(payload)) ||
			    (revision_policy == currency_revision_policy::flatfile_legacy &&
			     currency_command_is_rebasable_reward(payload));
	constexpr uint64_t wildcard = std::numeric_limits<uint64_t>::max();
	if (!rebase && ((!bank_only && expected_wallet_revision != wildcard &&
			 expected_wallet_revision != before.wallet_revision) ||
			(expected_bank_revision != wildcard &&
			 expected_bank_revision != before.bank_revision)))
		return ESTALE;
	auto after = before;
	const auto apply = [](int64_t current, int64_t delta, int64_t *next) -> unsigned int
	{
		// The current value is bounded above, so subtraction avoids signed
		// overflow even for an extreme positive delta. INT64_MIN is rejected.
		if (delta < 0 && current < -delta)
			return ENOSPC;
		if (delta > 0 && current > static_cast<int64_t>(INT_MAX) - delta)
			return ERANGE;
		*next = current + delta;
		return 0;
	};
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		const auto wallet_error = apply(before.wallet.amount[index],
						payload.wallet_delta.amount[index],
						&after.wallet.amount[index]);
		const auto bank_error = apply(before.bank.amount[index],
					      payload.bank_delta.amount[index],
					      &after.bank.amount[index]);
		// SQL historically reports either insufficient holding before either
		// overflow at the same denomination. Flatfile checks wallet then bank.
		if (revision_policy == currency_revision_policy::sql_legacy &&
		    (wallet_error == ENOSPC || bank_error == ENOSPC))
			return ENOSPC;
		if (wallet_error)
			return wallet_error;
		if (bank_error)
			return bank_error;
	}
	if ((!bank_only && before.wallet_revision == wildcard) || before.bank_revision == wildcard)
		return ERANGE;
	if (!bank_only)
		++after.wallet_revision;
	++after.bank_revision;
	*prepared = currency_prepared_mutation(payload, before, after);
	return 0;
}

namespace
{
bool currency_codec_admit(size_t outer, size_t fixed, size_t extra,
			  bool (*reserve)(size_t, void *) noexcept, void *context) noexcept
{
	return reserve && outer <= SIZE_MAX - fixed && extra <= SIZE_MAX - outer - fixed &&
	       reserve(outer + fixed + extra, context);
}
}

// Exact original account identity algorithm with prospective canonical-string
// requests. Caller counts its actual output and frames in outer_live; this leaf
// retains no owner and exposes no account/source/admission capability.
bool currency_account_key_bounded(const char *account_name, uint8_t racewar,
				  critical_entity_key *key,
				  bool (*reserve)(size_t, void *) noexcept, void *context,
				  size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) ||  \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || !defined(__linux__) ||             \
	!defined(__x86_64__) || !defined(OPENSSL_VERSION_MAJOR) || OPENSSL_VERSION_MAJOR != 3 || \
	!defined(OPENSSL_VERSION_MINOR) || OPENSSL_VERSION_MINOR != 0 ||                         \
	!defined(OPENSSL_VERSION_PATCH) || OPENSSL_VERSION_PATCH != 13 ||                        \
	defined(OPENSSL_NO_DEPRECATED_3_0)
	(void)account_name;
	(void)racewar;
	(void)key;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	if (!key || !reserve)
		return false;
	if (sizeof(void *) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	const size_t candidate_frame = sizeof(critical_entity_key);
	if (!currency_codec_admit(outer_live, candidate_frame, 0, reserve, context))
		return false;
	critical_entity_key candidate{};
	bool completed = false;
	try
	{
		{
			size_t length = 0;
			if (valid_name(account_name, &length))
			{
				const size_t fixed =
					sizeof(std::string) +
					sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
					sizeof(SHA256_CTX);
				const size_t constructor_heap = length > 15 ? length + 1 : 0;
				if (currency_codec_admit(outer_live, candidate_frame,
							 fixed + constructor_heap + sizeof(void *),
							 reserve, context))
				{
					std::string canonical(account_name, length);
					std::transform(
						canonical.begin(), canonical.end(),
						canonical.begin(), [](unsigned char ch)
						{ return static_cast<char>(std::tolower(ch)); });
					bool appended = true;
					for (unsigned step = 0; step < 2; ++step)
					{
						const size_t current =
							canonical.capacity() > 15 ?
								canonical.capacity() + 1 :
								0;
						size_t request = 0;
						if (canonical.size() == canonical.capacity())
						{
							// Genuine libstdc++13 _M_create growth, including the
							// old string heap while its replacement is allocated.
							const size_t capacity =
								canonical.capacity();
							if (capacity > (SIZE_MAX - 1) / 2)
							{
								appended = false;
								break;
							}
							request = 2 * capacity + 1;
						}
						if (request > SIZE_MAX - fixed - current ||
						    !currency_codec_admit(outer_live,
									  candidate_frame,
									  fixed + current + request,
									  reserve, context))
						{
							appended = false;
							break;
						}
						canonical.push_back(
							step ? static_cast<char>(racewar) : '\0');
					}
					if (appended)
					{
						std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
						SHA256_CTX digest_context;
						const size_t current =
							canonical.capacity() > 15 ?
								canonical.capacity() + 1 :
								0;
						// Pinned OpenSSL3.0.13 combined x86-64 SHA generator:
						// SHA256 SZ=4/rounds=64; real Linux AVX2 schedule,
						// metadata, six GPR saves, alignment, saved return and
						// red-zone pointer. Other dispatch paths are smaller.
						const size_t assembly_frames =
							2 * 4 * 64 + 4 * sizeof(void *) +
							6 * sizeof(uint64_t) + (256 * 4 - 1) +
							2 * sizeof(void *);
						const size_t c_small_frames =
							16 * sizeof(SHA_LONG) +
							12 * sizeof(unsigned int) +
							sizeof(SHA_LONG) + sizeof(int) +
							sizeof(void *);
						const size_t c_normal_frames =
							16 * sizeof(SHA_LONG) +
							11 * sizeof(unsigned int) +
							2 * sizeof(int) + 2 * sizeof(void *);
						const size_t c_block_frames =
							std::max(c_small_frames, c_normal_frames);
						const size_t block_frames =
							std::max(assembly_frames, c_block_frames);
						// md32_common Update/Final and SHA256 HASH_MAKE_STRING
						// own these fixed scalar locals; the digest block frame
						// can coexist with Final's p/n. No EVP/provider heap.
						const size_t init_frames =
							sizeof(void *) + sizeof(int);
						const size_t update_frames =
							(2 * sizeof(void *) + sizeof(size_t)) +
							(2 * sizeof(void *) + sizeof(SHA_LONG) +
							 sizeof(size_t)) +
							sizeof(int);
						const size_t final_frames =
							2 * sizeof(void *) + sizeof(void *) +
							sizeof(size_t) + sizeof(unsigned long) +
							sizeof(unsigned int) + sizeof(int);
						const size_t digest_frames =
							block_frames +
							std::max(init_frames,
								 std::max(update_frames,
									  final_frames));
						if (currency_codec_admit(
							    outer_live, candidate_frame,
							    fixed + current + digest_frames,
							    reserve, context))
						{
							// Preserve the original SHA256 value and
							// exact canonical lower-name/NUL/racewar byte stream.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
							const bool hashed =
								SHA256_Init(&digest_context) == 1 &&
								SHA256_Update(&digest_context,
									      canonical.data(),
									      canonical.size()) ==
									1 &&
								SHA256_Final(digest.data(),
									     &digest_context) == 1;
#pragma GCC diagnostic pop
							if (hashed)
							{
								uint64_t identity =
									get_u64(digest.data());
								if (!identity)
									identity = 1;
								candidate = {
									critical_entity_type::account,
									identity
								};
								completed = true;
							}
						}
					}
				}
			}
		}
	}
	catch (...)
	{
		completed = false;
	}
	// Canonical string/digest have died. Candidate remains an
	// honest current frames; caller drops them in its post-return rebase.
	const bool refreshed =
		currency_codec_admit(outer_live, candidate_frame, 0, reserve, context);
	if (!completed || !refreshed)
		return false;
	*key = candidate;
	return true;
#endif
}

bool currency_command_decode_payload_bounded(const critical_command &command,
					     currency_command_payload *payload,
					     bool (*reserve)(size_t, void *) noexcept,
					     void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve)
		return false;
	const size_t fixed = sizeof(currency_command_payload) + 2 * sizeof(critical_entity_key);
	if (!currency_codec_admit(outer_live, fixed, 0, reserve, context))
		return false;
	currency_command_payload decoded{};
	critical_entity_key account_key{};
	critical_entity_key player_key{ critical_entity_type::player, 0 };
	bool completed = false;
	try
	{
		do
		{
			if (command.type != critical_command_type::account_bank ||
			    command.payload_version != CURRENCY_COMMAND_PAYLOAD_VERSION ||
			    command.payload.size() != CURRENCY_COMMAND_PAYLOAD_BYTES ||
			    !currency_codec_admit(outer_live, fixed, 2 * sizeof(currency_vector),
						  reserve, context))
				break;
			const size_t name_length = command.payload[NAME_LENGTH_OFFSET];
			if (!name_length || name_length > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
				break;
			bool padding = true;
			for (size_t index = NAME_OFFSET + name_length; index < WALLET_OFFSET;
			     ++index)
				if (command.payload[index])
					padding = false;
			for (size_t index = BANK_OFFSET + 32; index < command.payload.size();
			     ++index)
				if (command.payload[index])
					padding = false;
			if (!padding)
				break;
			decoded.pid = get_u32(command.payload.data() + PID_OFFSET);
			decoded.racewar = command.payload[RACEWAR_OFFSET];
			decoded.reason = static_cast<currency_reason_type>(
				get_u16(command.payload.data() + REASON_OFFSET));
			decoded.reason_id = static_cast<int64_t>(
				get_u64(command.payload.data() + REASON_ID_OFFSET));
			memcpy(decoded.account_name.data(), command.payload.data() + NAME_OFFSET,
			       name_length);
			decoded.wallet_delta =
				decode_vector(command.payload.data() + WALLET_OFFSET);
			decoded.bank_delta = decode_vector(command.payload.data() + BANK_OFFSET);
			size_t checked_length = 0;
			if (!decoded.pid || !valid_reason(decoded.reason) ||
			    !valid_name(decoded.account_name.data(), &checked_length) ||
			    checked_length != name_length || !vector_valid(decoded.wallet_delta) ||
			    !vector_valid(decoded.bank_delta) ||
			    (!any_delta(decoded) &&
			     decoded.reason != currency_reason_type::corpse_lifecycle) ||
			    outer_live > SIZE_MAX - fixed ||
			    !currency_account_key_bounded(decoded.account_name.data(),
							  decoded.racewar, &account_key, reserve,
							  context, outer_live + fixed))
				break;
			player_key.id = decoded.pid;
			if (!currency_codec_admit(outer_live, fixed, 0, reserve, context))
				break;
			completed = command.keys.size() == 2 &&
				    command.expected_revisions.size() == 2 &&
				    critical_entity_key_equal(command.keys[0], player_key) &&
				    critical_entity_key_equal(command.keys[1], account_key) &&
				    critical_entity_key_equal(command.expected_revisions[0].key,
							      player_key) &&
				    critical_entity_key_equal(command.expected_revisions[1].key,
							      account_key);
		} while (false);
	}
	catch (...)
	{
		completed = false;
	}
	const bool refreshed = currency_codec_admit(outer_live, fixed, 0, reserve, context);
	if (!completed || !refreshed)
		return false;
	*payload = decoded;
	return true;
}

// This is the complete named SOURCE union of unchanged and additive typed decoder,
// not a retained-storage baseline, allocator/runtime bound or emitted stack.
// SOURCE-PINS.json identifies the exact ordinary GNU13/OpenSSL controls.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
constexpr size_t currency_decode_P = sizeof(void *), currency_decode_N = sizeof(size_t),
		 currency_decode_B = sizeof(bool), currency_decode_C = sizeof(char);
using currency_decode_iterator = std::string::iterator;
using currency_decode_predicate =
	decltype([](int64_t amount) { return amount != std::numeric_limits<int64_t>::min(); });
using currency_decode_transform =
	decltype([](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
using currency_decode_iter_pred = __gnu_cxx::__ops::_Iter_pred<currency_decode_predicate>;
using currency_decode_iter_negate = __gnu_cxx::__ops::_Iter_negate<currency_decode_predicate>;

// _M_data(this,result); _M_local_data -> pointer_to -> addressof -> __addressof.
constexpr size_t currency_decode_data = 2 * currency_decode_P;
constexpr size_t currency_decode_local_data = 8 * currency_decode_P;
constexpr size_t currency_decode_is_local =
	currency_decode_P + currency_decode_B + currency_decode_data + currency_decode_local_data;
constexpr size_t currency_decode_capacity =
	currency_decode_P + currency_decode_N + currency_decode_is_local;
constexpr size_t currency_decode_string_size = currency_decode_P + currency_decode_N;
constexpr size_t currency_decode_allocator_ref = 2 * currency_decode_P;
constexpr size_t currency_decode_string_max = currency_decode_P + currency_decode_N +
					      currency_decode_allocator_ref + currency_decode_P +
					      currency_decode_N;

// _S_allocate(a,n,p,result), allocator_traits::allocate, allocator<char>::allocate
// with the actual constant-evaluation predicate, __new_allocator<char>::allocate
// (this,n,hint,result), _M_max_size and declared scalar operator new boundary.
// alignof(char) does not select aligned-new. Constant-evaluation bodies do not run.
constexpr size_t currency_decode_allocate =
	(3 * currency_decode_P + currency_decode_N) + (2 * currency_decode_P + currency_decode_N) +
	(2 * currency_decode_P + currency_decode_N + currency_decode_B) +
	(3 * currency_decode_P + currency_decode_N) + (currency_decode_P + currency_decode_N) +
	(currency_decode_N + currency_decode_P);
// traits/allocator/new_allocator deallocate, real C++20 predicate, sized delete
// declaration. The scalar char alignment discards aligned-delete at compile time.
constexpr size_t currency_decode_deallocate =
	(2 * currency_decode_P + currency_decode_N) +
	(2 * currency_decode_P + currency_decode_N + currency_decode_B) +
	(2 * currency_decode_P + currency_decode_N) + (currency_decode_P + currency_decode_N);
// _S_copy -> char_traits<char>::assign OR copy -> builtin memcpy declaration.
// Sum of both genuine branches includes their actual consteval predicates.
constexpr size_t currency_decode_copy =
	(2 * currency_decode_P + currency_decode_N) + (2 * currency_decode_P + currency_decode_B) +
	(3 * currency_decode_P + currency_decode_N + currency_decode_B) +
	(3 * currency_decode_P + currency_decode_N);
constexpr size_t currency_decode_set_length =
	(currency_decode_P + currency_decode_N) + (currency_decode_P + currency_decode_N) +
	currency_decode_data + (2 * currency_decode_P + currency_decode_B) + currency_decode_C;
// _M_create(this,capacity-ref,old-capacity,result), three max_size calls, allocator
// reference and full actual allocation chain. The length-error declaration owns
// its pointer argument; valid <=50-byte account inputs cannot select that error.
constexpr size_t currency_decode_create =
	(3 * currency_decode_P + currency_decode_N) + 3 * currency_decode_string_max +
	currency_decode_allocator_ref + currency_decode_allocate + currency_decode_P;
constexpr size_t currency_decode_destroy = (currency_decode_P + currency_decode_N) +
					   currency_decode_allocator_ref + currency_decode_data +
					   currency_decode_deallocate;
constexpr size_t currency_decode_dispose =
	currency_decode_P + currency_decode_is_local + currency_decode_destroy;
// ~basic_string, dispose, implicit _Alloc_hider/allocator/new_allocator destruction.
constexpr size_t currency_decode_string_cleanup = 4 * currency_decode_P + currency_decode_dispose;

// Genuine pointer+length+const-allocator constructor, not default-string or row
// copy. Default allocator object/ctor/base ctor, _Alloc_hider const-copy ctor,
// allocator/base copy constructors and temporary allocator/base destruction.
// _M_construct forward path uses pointer std::distance/category/__distance,
// true forward tag, dnew, guard ctor/dtor, copy_chars, length and buffer setters.
// Its one-pointer guard OBJECT is already admitted by the original constructor
// `fixed + constructor_heap + sizeof(void*)`; only its call scopes enter supplement.
constexpr size_t currency_decode_string_constructor =
	(3 * currency_decode_P + currency_decode_N) + sizeof(std::allocator<char>) +
	2 * currency_decode_P + 7 * currency_decode_P + 2 * currency_decode_P +
	currency_decode_local_data +
	(3 * currency_decode_P + sizeof(std::forward_iterator_tag) + currency_decode_N) +
	(2 * currency_decode_P + sizeof(std::ptrdiff_t)) +
	(currency_decode_P + sizeof(std::random_access_iterator_tag)) +
	(2 * currency_decode_P + sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag)) +
	currency_decode_create + 2 * currency_decode_P + (currency_decode_P + currency_decode_N) +
	(currency_decode_P + currency_decode_B) + 2 * currency_decode_P + currency_decode_P +
	currency_decode_dispose + 3 * currency_decode_P + currency_decode_copy +
	currency_decode_data + currency_decode_set_length;
// push_back(this,char,size), size/capacity, _M_mutate(this,pos,len1,s,len2,
// how_much,new_capacity,r), its length/capacity/create/copy/dispose/setters,
// final char assignment and set_length. Actual loop is iterative: no depth/count
// multiplier. Old/new heap overlap remains in the original prospective requests.
constexpr size_t currency_decode_string_append =
	(currency_decode_P + currency_decode_C + currency_decode_N) + currency_decode_string_size +
	currency_decode_capacity + (3 * currency_decode_P + 5 * currency_decode_N) +
	2 * currency_decode_string_size + currency_decode_capacity + currency_decode_create +
	3 * currency_decode_copy + 2 * currency_decode_data + currency_decode_dispose +
	2 * currency_decode_P + (currency_decode_P + currency_decode_N) +
	(2 * currency_decode_P + currency_decode_B) + currency_decode_data +
	currency_decode_set_length;
// Real string begin/end normal_iterator constructors, ==/two base calls,
// dereference and two prefix increments, parameter/argument iterator cleanups,
// lambda copy/cleanup and lambda(char)->tolower(int) result. Fresh glibc ctype.h
// additionally exposes optimized C++ extern-inline tolower->__ctype_tolower_loc;
// include its actual zero-argument pointer result as well as the int formal/result.
constexpr size_t currency_decode_transform_source =
	4 * sizeof(currency_decode_iterator) + sizeof(currency_decode_transform) +
	2 * (5 * currency_decode_P + sizeof(currency_decode_iterator)) +
	(6 * currency_decode_P + currency_decode_N + sizeof(currency_decode_iterator)) +
	(6 * currency_decode_P + currency_decode_B) + 2 * (2 * currency_decode_P) +
	2 * (2 * currency_decode_P) + 7 * currency_decode_P + 3 * currency_decode_P +
	currency_decode_P + sizeof(unsigned char) + sizeof(char) + 2 * sizeof(int) +
	currency_decode_P;
// vector_valid->all_of->find_if_not->__find_if_not->__find_if RA. True lambda,
// iter_pred and iter_negate sizes, their constructors/move/copy/destruction,
// iterator-category tag/trip_count and actual negate->lambda->limits::min.
constexpr size_t currency_decode_all_of_source =
	(currency_decode_P + currency_decode_B) + 4 * currency_decode_P +
	(2 * currency_decode_P + sizeof(currency_decode_predicate) + currency_decode_B) +
	(3 * currency_decode_P + sizeof(currency_decode_predicate)) +
	(sizeof(currency_decode_predicate) + sizeof(currency_decode_iter_pred)) +
	(currency_decode_P + sizeof(currency_decode_predicate)) + 4 * currency_decode_P +
	(3 * currency_decode_P + sizeof(currency_decode_iter_pred)) +
	(sizeof(currency_decode_iter_pred) + sizeof(currency_decode_iter_negate)) +
	(currency_decode_P + sizeof(currency_decode_predicate)) + 4 * currency_decode_P +
	(currency_decode_P + sizeof(std::random_access_iterator_tag)) +
	(3 * currency_decode_P + sizeof(currency_decode_iter_negate) +
	 sizeof(std::random_access_iterator_tag) + sizeof(std::ptrdiff_t)) +
	(2 * currency_decode_P + currency_decode_B) +
	(currency_decode_P + sizeof(int64_t) + currency_decode_B) + sizeof(int64_t) +
	4 * (2 * currency_decode_P) + 8 * currency_decode_P;
// Full local/declared scalar helpers: valid_name including strnlen boundary;
// valid_reason; any_delta and actual fixed-array indexing; get_u16/u32/u64;
// decode_vector's formals/index/uint64 result plus its admitted vector object.
constexpr size_t currency_decode_scalar_source =
	(2 * currency_decode_P + currency_decode_N + currency_decode_B + currency_decode_P +
	 2 * currency_decode_N) +
	(sizeof(currency_reason_type) + currency_decode_B) +
	(currency_decode_P + currency_decode_N + currency_decode_B +
	 2 * (2 * currency_decode_P + currency_decode_N)) +
	(currency_decode_P + sizeof(uint16_t)) +
	(currency_decode_P + 2 * sizeof(uint32_t) + sizeof(unsigned int)) +
	(currency_decode_P + 2 * sizeof(uint64_t) + sizeof(unsigned int)) +
	(currency_decode_P + currency_decode_N + 2 * currency_decode_P + currency_decode_N);
// Original decoder full formals, fixed/name_length/indices/checked_length,
// completed/padding/refreshed. Original account child full formals, 17 actual
// size_t declarations, step, completed/appended/hashed/refreshed and identity.
// Four runtime std::max calls use two const-ref arguments, returned ref, bool.
constexpr size_t currency_decode_lexical_source =
	(4 * currency_decode_P + 6 * currency_decode_N + 4 * currency_decode_B) +
	(4 * currency_decode_P + sizeof(uint8_t) + 18 * currency_decode_N + sizeof(unsigned int) +
	 5 * currency_decode_B + sizeof(uint64_t)) +
	4 * (3 * currency_decode_P + currency_decode_B);
// Admission helper own arguments/result and indirect reserve call boundary.
// Descendant callback implementation is caller-owned, never copied here.
constexpr size_t currency_decode_admission_source = 2 * currency_decode_P + 3 * currency_decode_N +
						    currency_decode_B + currency_decode_P +
						    currency_decode_N + currency_decode_B;
// Actual vector size/data/_M_data_ptr/subscript, array data/subscript, generated
// aggregate assignments and key_equal. Sum one closure per named selected family;
// iterative/repeated calls have no additional simultaneous source depth.
constexpr size_t currency_decode_access_source =
	(currency_decode_P + currency_decode_N) + 5 * currency_decode_P +
	(2 * currency_decode_P + currency_decode_N) + 2 * currency_decode_P +
	// Public string data(this,result) -> internal _M_data(this,result).
	4 * currency_decode_P + (2 * currency_decode_P + currency_decode_N) +
	// payload, currency_vector, its amount array, name array, entity-key assignments.
	5 * (2 * currency_decode_P) + sizeof(critical_entity_key) +
	(2 * currency_decode_P + currency_decode_B) + (3 * currency_decode_P + currency_decode_N);
// Exact unchanged account child SHA allowance, pinned to the published fixed SHA
// controls. This term is already retained by its SHA admission, so not supplement.
constexpr size_t currency_decode_sha_source =
	std::max(2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) +
			 2 * sizeof(void *),
		 std::max(16 * sizeof(SHA_LONG) + 12 * sizeof(unsigned int) + sizeof(SHA_LONG) +
				  sizeof(int) + sizeof(void *),
			  16 * sizeof(SHA_LONG) + 11 * sizeof(unsigned int) + 2 * sizeof(int) +
				  2 * sizeof(void *))) +
	std::max(sizeof(void *) + sizeof(int),
		 std::max(4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(SHA_LONG) + sizeof(int),
			  3 * sizeof(void *) + sizeof(size_t) + sizeof(unsigned long) +
				  sizeof(unsigned int) + sizeof(int)));
// Typed counterpart adds two real outcome objects, account_result, two typed
// return values, two unnamed bad_alloc catch references, and the admission helper
// (outer/fixed/extra, reserve/context/outcome-reference, allowed/return bool).
// This union conservatively includes original bool-return carriers too.
constexpr size_t currency_decode_typed_source = 5 * sizeof(currency_command_bounded_result) +
						2 * currency_decode_P + 3 * currency_decode_P +
						3 * currency_decode_N + 2 * currency_decode_B;
constexpr size_t currency_decode_supplement_source =
	currency_decode_lexical_source + currency_decode_scalar_source +
	currency_decode_admission_source + currency_decode_access_source +
	currency_decode_string_constructor + currency_decode_string_append +
	currency_decode_transform_source + currency_decode_all_of_source +
	currency_decode_string_cleanup + currency_decode_typed_source;
constexpr size_t currency_decode_full_source =
	currency_decode_supplement_source + currency_decode_sha_source + sizeof(void *);
#endif
}

bool currency_command_decode_payload_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	*output = currency_decode_full_source;
	return true;
#else
	(void)output;
	return false;
#endif
}

bool currency_command_decode_payload_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	*output = currency_decode_supplement_source;
	return true;
#else
	(void)output;
	return false;
#endif
}

bool currency_command_decode_payload_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	*output = sizeof(currency_command_payload) + 2 * sizeof(critical_entity_key);
	return true;
#else
	(void)output;
	return false;
#endif
}

#include <new>
namespace
{
bool currency_status_codec_admit(size_t outer, size_t fixed, size_t extra,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 currency_command_bounded_result &outcome) noexcept
{
	const bool allowed = currency_codec_admit(outer, fixed, extra, reserve, context);
	if (!allowed)
		outcome = currency_command_bounded_result::capacity;
	return allowed;
}
}

// Additive typed resource result; all original bool APIs above remain exact.
// Outcome records actual arithmetic/reserve denial or std::bad_alloc at its
// real failing branch; a later successful CURRENT refresh does not erase it.
currency_command_bounded_result currency_account_key_bounded_status(
	const char *account_name, uint8_t racewar, critical_entity_key *key,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) ||  \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || !defined(__linux__) ||             \
	!defined(__x86_64__) || !defined(OPENSSL_VERSION_MAJOR) || OPENSSL_VERSION_MAJOR != 3 || \
	!defined(OPENSSL_VERSION_MINOR) || OPENSSL_VERSION_MINOR != 0 ||                         \
	!defined(OPENSSL_VERSION_PATCH) || OPENSSL_VERSION_PATCH != 13 ||                        \
	defined(OPENSSL_NO_DEPRECATED_3_0)
	(void)account_name;
	(void)racewar;
	(void)key;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return currency_command_bounded_result::capacity;
#else
	if (!key || !reserve)
		return currency_command_bounded_result::invalid;
	if (sizeof(void *) != 8 || sizeof(SHA_LONG) != 4)
		return currency_command_bounded_result::capacity;
	currency_command_bounded_result outcome = currency_command_bounded_result::invalid;
	const size_t candidate_frame = sizeof(critical_entity_key);
	if (!currency_status_codec_admit(outer_live, candidate_frame, 0, reserve, context, outcome))
		return currency_command_bounded_result::capacity;
	critical_entity_key candidate{};
	bool completed = false;
	try
	{
		{
			size_t length = 0;
			if (valid_name(account_name, &length))
			{
				const size_t fixed =
					sizeof(std::string) +
					sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
					sizeof(SHA256_CTX);
				const size_t constructor_heap = length > 15 ? length + 1 : 0;
				if (currency_status_codec_admit(outer_live, candidate_frame,
								fixed + constructor_heap +
									sizeof(void *),
								reserve, context, outcome))
				{
					std::string canonical(account_name, length);
					std::transform(
						canonical.begin(), canonical.end(),
						canonical.begin(), [](unsigned char ch)
						{ return static_cast<char>(std::tolower(ch)); });
					bool appended = true;
					for (unsigned step = 0; step < 2; ++step)
					{
						const size_t current =
							canonical.capacity() > 15 ?
								canonical.capacity() + 1 :
								0;
						size_t request = 0;
						if (canonical.size() == canonical.capacity())
						{
							// Genuine libstdc++13 _M_create growth, including the
							// old string heap while its replacement is allocated.
							const size_t capacity =
								canonical.capacity();
							if (capacity > (SIZE_MAX - 1) / 2)
							{
								outcome =
									currency_command_bounded_result::
										capacity;
								appended = false;
								break;
							}
							request = 2 * capacity + 1;
						}
						if (request > SIZE_MAX - fixed - current ||
						    !currency_status_codec_admit(
							    outer_live, candidate_frame,
							    fixed + current + request, reserve,
							    context, outcome))
						{
							outcome = currency_command_bounded_result::
								capacity;
							appended = false;
							break;
						}
						canonical.push_back(
							step ? static_cast<char>(racewar) : '\0');
					}
					if (appended)
					{
						std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
						SHA256_CTX digest_context;
						const size_t current =
							canonical.capacity() > 15 ?
								canonical.capacity() + 1 :
								0;
						// Pinned OpenSSL3.0.13 combined x86-64 SHA generator:
						// SHA256 SZ=4/rounds=64; real Linux AVX2 schedule,
						// metadata, six GPR saves, alignment, saved return and
						// red-zone pointer. Other dispatch paths are smaller.
						const size_t assembly_frames =
							2 * 4 * 64 + 4 * sizeof(void *) +
							6 * sizeof(uint64_t) + (256 * 4 - 1) +
							2 * sizeof(void *);
						const size_t c_small_frames =
							16 * sizeof(SHA_LONG) +
							12 * sizeof(unsigned int) +
							sizeof(SHA_LONG) + sizeof(int) +
							sizeof(void *);
						const size_t c_normal_frames =
							16 * sizeof(SHA_LONG) +
							11 * sizeof(unsigned int) +
							2 * sizeof(int) + 2 * sizeof(void *);
						const size_t c_block_frames =
							std::max(c_small_frames, c_normal_frames);
						const size_t block_frames =
							std::max(assembly_frames, c_block_frames);
						// md32_common Update/Final and SHA256 HASH_MAKE_STRING
						// own these fixed scalar locals; the digest block frame
						// can coexist with Final's p/n. No EVP/provider heap.
						const size_t init_frames =
							sizeof(void *) + sizeof(int);
						const size_t update_frames =
							(2 * sizeof(void *) + sizeof(size_t)) +
							(2 * sizeof(void *) + sizeof(SHA_LONG) +
							 sizeof(size_t)) +
							sizeof(int);
						const size_t final_frames =
							2 * sizeof(void *) + sizeof(void *) +
							sizeof(size_t) + sizeof(unsigned long) +
							sizeof(unsigned int) + sizeof(int);
						const size_t digest_frames =
							block_frames +
							std::max(init_frames,
								 std::max(update_frames,
									  final_frames));
						if (currency_status_codec_admit(
							    outer_live, candidate_frame,
							    fixed + current + digest_frames,
							    reserve, context, outcome))
						{
							// Preserve the original SHA256 value and
							// exact canonical lower-name/NUL/racewar byte stream.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
							const bool hashed =
								SHA256_Init(&digest_context) == 1 &&
								SHA256_Update(&digest_context,
									      canonical.data(),
									      canonical.size()) ==
									1 &&
								SHA256_Final(digest.data(),
									     &digest_context) == 1;
#pragma GCC diagnostic pop
							if (hashed)
							{
								uint64_t identity =
									get_u64(digest.data());
								if (!identity)
									identity = 1;
								candidate = {
									critical_entity_type::account,
									identity
								};
								completed = true;
							}
						}
					}
				}
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		completed = false;
		outcome = currency_command_bounded_result::capacity;
	}
	catch (...)
	{
		completed = false;
	}
	// Canonical string/digest have died. Candidate remains an
	// honest current frames; caller drops them in its post-return rebase.
	const bool refreshed = currency_status_codec_admit(outer_live, candidate_frame, 0, reserve,
							   context, outcome);
	if (!refreshed)
		return currency_command_bounded_result::capacity;
	if (!completed)
		return outcome;
	*key = candidate;
	return currency_command_bounded_result::ok;
#endif
}

currency_command_bounded_result currency_command_decode_payload_bounded_status(
	const critical_command &command, currency_command_payload *payload,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve)
		return currency_command_bounded_result::invalid;
	currency_command_bounded_result outcome = currency_command_bounded_result::invalid;
	const size_t fixed = sizeof(currency_command_payload) + 2 * sizeof(critical_entity_key);
	if (!currency_status_codec_admit(outer_live, fixed, 0, reserve, context, outcome))
		return currency_command_bounded_result::capacity;
	currency_command_payload decoded{};
	critical_entity_key account_key{};
	critical_entity_key player_key{ critical_entity_type::player, 0 };
	bool completed = false;
	try
	{
		do
		{
			if (command.type != critical_command_type::account_bank ||
			    command.payload_version != CURRENCY_COMMAND_PAYLOAD_VERSION ||
			    command.payload.size() != CURRENCY_COMMAND_PAYLOAD_BYTES ||
			    !currency_status_codec_admit(outer_live, fixed,
							 2 * sizeof(currency_vector), reserve,
							 context, outcome))
				break;
			const size_t name_length = command.payload[NAME_LENGTH_OFFSET];
			if (!name_length || name_length > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
				break;
			bool padding = true;
			for (size_t index = NAME_OFFSET + name_length; index < WALLET_OFFSET;
			     ++index)
				if (command.payload[index])
					padding = false;
			for (size_t index = BANK_OFFSET + 32; index < command.payload.size();
			     ++index)
				if (command.payload[index])
					padding = false;
			if (!padding)
				break;
			decoded.pid = get_u32(command.payload.data() + PID_OFFSET);
			decoded.racewar = command.payload[RACEWAR_OFFSET];
			decoded.reason = static_cast<currency_reason_type>(
				get_u16(command.payload.data() + REASON_OFFSET));
			decoded.reason_id = static_cast<int64_t>(
				get_u64(command.payload.data() + REASON_ID_OFFSET));
			memcpy(decoded.account_name.data(), command.payload.data() + NAME_OFFSET,
			       name_length);
			decoded.wallet_delta =
				decode_vector(command.payload.data() + WALLET_OFFSET);
			decoded.bank_delta = decode_vector(command.payload.data() + BANK_OFFSET);
			size_t checked_length = 0;
			if (!decoded.pid || !valid_reason(decoded.reason) ||
			    !valid_name(decoded.account_name.data(), &checked_length) ||
			    checked_length != name_length || !vector_valid(decoded.wallet_delta) ||
			    !vector_valid(decoded.bank_delta) ||
			    (!any_delta(decoded) &&
			     decoded.reason != currency_reason_type::corpse_lifecycle))
				break;
			if (outer_live > SIZE_MAX - fixed)
			{
				outcome = currency_command_bounded_result::capacity;
				break;
			}
			const auto account_result = currency_account_key_bounded_status(
				decoded.account_name.data(), decoded.racewar, &account_key, reserve,
				context, outer_live + fixed);
			if (account_result != currency_command_bounded_result::ok)
			{
				outcome = account_result;
				break;
			}
			player_key.id = decoded.pid;
			if (!currency_status_codec_admit(outer_live, fixed, 0, reserve, context,
							 outcome))
				break;
			completed = command.keys.size() == 2 &&
				    command.expected_revisions.size() == 2 &&
				    critical_entity_key_equal(command.keys[0], player_key) &&
				    critical_entity_key_equal(command.keys[1], account_key) &&
				    critical_entity_key_equal(command.expected_revisions[0].key,
							      player_key) &&
				    critical_entity_key_equal(command.expected_revisions[1].key,
							      account_key);
		} while (false);
	}
	catch (const std::bad_alloc &)
	{
		completed = false;
		outcome = currency_command_bounded_result::capacity;
	}
	catch (...)
	{
		completed = false;
	}
	const bool refreshed =
		currency_status_codec_admit(outer_live, fixed, 0, reserve, context, outcome);
	if (!refreshed)
		return currency_command_bounded_result::capacity;
	if (!completed)
		return outcome;
	*payload = decoded;
	return currency_command_bounded_result::ok;
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
// sha256.c SHA256_Init memset, md32_common Update memcpy/memset, Final memset
// and OPENSSL_cleanse are sequential with block calls, not nested in them.
// mem_clr.c owns ptr/len and the volatile loaded memset function-pointer result,
// then the actual memset declared arguments/result. Native libc remains separate.
constexpr size_t currency_sha_memcpy_source = 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t currency_sha_memset_source = 2 * sizeof(void *) + sizeof(int) + sizeof(size_t);
constexpr size_t currency_sha_cleanse_source =
	sizeof(void *) + sizeof(size_t) + sizeof(void *) + currency_sha_memset_source;
constexpr size_t currency_sha_block_source = std::max(
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *),
	std::max(16 * sizeof(SHA_LONG) + 12 * sizeof(unsigned int) + sizeof(SHA_LONG) +
			 sizeof(int) + sizeof(void *),
		 16 * sizeof(SHA_LONG) + 11 * sizeof(unsigned int) + 2 * sizeof(int) +
			 2 * sizeof(void *)));
static_assert(currency_sha_block_source >= currency_sha_memcpy_source);
static_assert(currency_sha_block_source >= currency_sha_memset_source);
static_assert(currency_sha_block_source >= currency_sha_cleanse_source);
// Therefore the unchanged original block + max(Init,Update,Final) admission
// dominates the complete phase graph including these memory leaves. No unrelated
// spare margin or extra supplement is used; decode21 numerical profiles stay exact.

// Narrow actual account-key closure, derived from its complete original body.
// No decoder reason/padding/vector/all_of terms enter this account-only export.
constexpr size_t currency_account_lexical_source =
	4 * currency_decode_P + sizeof(uint8_t) + 18 * currency_decode_N + sizeof(unsigned int) +
	5 * currency_decode_B + sizeof(uint64_t) + 4 * (3 * currency_decode_P + currency_decode_B);
constexpr size_t currency_account_scalar_source =
	// valid_name(this-name/length, index, bool) -> declared strnlen input/limit/result.
	2 * currency_decode_P + currency_decode_N + currency_decode_B + currency_decode_P +
	2 * currency_decode_N +
	// get_u64 input, value, byte index and returned uint64.
	currency_decode_P + 2 * sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t currency_account_access_source =
	// digest array.data, canonical.data -> _M_data, string size/capacity,
	// entity-key assignment formal refs and its real braced temporary.
	2 * currency_decode_P + 4 * currency_decode_P + currency_decode_string_size +
	currency_decode_capacity + 2 * currency_decode_P + sizeof(critical_entity_key);
constexpr size_t currency_account_typed_source =
	// outcome and returned enum, bad_alloc reference, typed admit's true args/results.
	2 * sizeof(currency_command_bounded_result) + currency_decode_P + 3 * currency_decode_P +
	3 * currency_decode_N + 2 * currency_decode_B;
constexpr size_t currency_account_supplement_source =
	currency_account_lexical_source + currency_account_scalar_source +
	currency_account_access_source + currency_decode_admission_source +
	currency_decode_string_constructor + currency_decode_string_append +
	currency_decode_transform_source + currency_decode_string_cleanup +
	currency_account_typed_source;
constexpr size_t currency_account_full_source =
	currency_account_supplement_source + currency_decode_sha_source + sizeof(void *);
#endif
}

bool currency_account_key_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	*output = currency_account_full_source;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool currency_account_key_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	*output = currency_account_supplement_source;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool currency_account_key_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&        \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) &&  \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4)
		return false;
	*output = sizeof(critical_entity_key);
	return true;
#else
	(void)output;
	return false;
#endif
}
