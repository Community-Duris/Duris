#include "telemetry/telemetry_battle_contract.h"

#include <array>
#include <cstdio>
#include <cstdint>

/* Test-only batch framing: u32 byte length followed by canonical fact bytes.
 * No journal or repository format is introduced by this qualification harness. */
int main(int argc, char **argv)
{
	if (argc != 2)
		return 2;
	std::FILE *input = std::fopen(argv[1], "rb");
	if (!input)
		return 2;
	std::array<std::uint8_t, TELEMETRY_BATTLE_WIRE_BYTES * TELEMETRY_BATTLE_PACKET_MAX_FACTS>
		bytes{};
	std::array<telemetry_battle_fact, TELEMETRY_BATTLE_PACKET_MAX_FACTS> facts{};
	std::array<std::uint8_t, 4U> header{};
	for (;;)
	{
		const auto read = std::fread(header.data(), 1U, header.size(), input);
		if (read == 0U && std::feof(input))
			break;
		if (read != header.size())
		{
			std::fclose(input);
			return 2;
		}
		const auto length = (std::uint32_t{ header[0] } << 24U) |
				    (std::uint32_t{ header[1] } << 16U) |
				    (std::uint32_t{ header[2] } << 8U) | header[3];
		if (length > bytes.size() || std::fread(bytes.data(), 1U, length, input) != length)
		{
			std::fclose(input);
			return 2;
		}
		bool valid = length != 0U && length % TELEMETRY_BATTLE_WIRE_BYTES == 0U;
		const auto count = length / TELEMETRY_BATTLE_WIRE_BYTES;
		for (std::size_t index = 0U; valid && index < count; ++index)
			valid = telemetry_battle_fact_decode(
				bytes.data() + index * TELEMETRY_BATTLE_WIRE_BYTES,
				TELEMETRY_BATTLE_WIRE_BYTES, &facts[index]);
		valid = valid && telemetry_battle_packet_is_valid(facts.data(), count);
		std::printf("%u\n", static_cast<unsigned>(valid));
	}
	return std::fclose(input) == 0 ? 0 : 2;
}
