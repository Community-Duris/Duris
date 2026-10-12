#include <cassert>
#include <cstdio>

// The complete genuine provider is compiled. No checksum implementation is
// extracted/replaced and no journal/ownership/backend function is stubbed.
#include "player/player_save_journal.c"

namespace
{
uint32_t original_crc(uint32_t crc, const uint8_t *bytes, size_t size)
{
	for (size_t index = 0; index < size; ++index)
	{
		crc ^= bytes[index];
		for (unsigned int bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0U - (crc & 1U)));
	}
	return crc;
}
}

int main()
{
	const uint8_t known[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	assert(payload_checksum(nullptr, 0) == 0);
	assert(payload_checksum(known, sizeof(known)) == UINT32_C(0xcbf43926));
	for (uint32_t seed : { uint32_t(0), uint32_t(1), UINT32_C(0x823474cd), UINT32_MAX })
	{
		assert(crc32_update(seed, nullptr, 0) == seed);
		for (unsigned int value = 0; value < 256; ++value)
		{
			const uint8_t byte = value;
			assert(crc32_update(seed, &byte, 1) == original_crc(seed, &byte, 1));
		}
	}
	// Header-only, tiny, ordinary bound, and maximum original snapshot frames.
	for (size_t payload_size : { size_t(0), size_t(1), size_t(71), size_t(8192), size_t(65536),
				     PLAYER_SNAPSHOT_MAX_BYTES })
	{
		std::vector<uint8_t> frame(JOURNAL_HEADER_SIZE + payload_size);
		for (size_t index = 0; index < frame.size(); ++index)
			frame[index] = static_cast<uint8_t>((index * 47) ^ (index >> 8));
		for (uint32_t seed : { uint32_t(0), UINT32_C(0x12345678), UINT32_MAX })
		{
			assert(crc32_update(seed, frame.data(), frame.size()) ==
			       original_crc(seed, frame.data(), frame.size()));
			for (size_t split : { size_t(0), JOURNAL_CHECKSUM_OFFSET,
					      JOURNAL_HEADER_SIZE, frame.size() })
				assert(crc32_update(crc32_update(seed, frame.data(), split),
						    frame.data() + split, frame.size() - split) ==
				       crc32_update(seed, frame.data(), frame.size()));
		}
		const uint32_t reference = ~original_crc(
			original_crc(UINT32_MAX, frame.data(), JOURNAL_CHECKSUM_OFFSET),
			frame.data() + JOURNAL_HEADER_SIZE, payload_size);
		assert(frame_checksum(frame.data(), frame.size()) == reference);
		// The original checksum field is excluded; original payload is covered.
		frame[JOURNAL_CHECKSUM_OFFSET] ^= 0x80;
		assert(frame_checksum(frame.data(), frame.size()) == reference);
		if (payload_size)
		{
			frame.back() ^= 0x80;
			assert(frame_checksum(frame.data(), frame.size()) != reference);
		}
	}
	std::puts(
		"player save journal CRC: known vectors, original recurrence, frame and incremental controls PASS");
}
