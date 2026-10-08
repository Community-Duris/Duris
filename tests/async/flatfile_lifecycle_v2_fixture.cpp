// Independent V2 coverage contract checked with real native EAB codecs.
// Does not call lifecycle installation or the unpublished native V2 encoder.
#include "../../scripts/qualify_flatfile_economic_lifecycle.h"
#include "economy/economic_baseline_adapter.h"
#include <fstream>
#include <iostream>
#include <iterator>

static restore_economic_authority::bytes input(const char *path, size_t limit)
{
	std::ifstream file(path, std::ios::binary);
	restore_economic_authority::need(bool(file));
	restore_economic_authority::bytes value{ std::istreambuf_iterator<char>(file), {} };
	restore_economic_authority::need(value.size() <= limit);
	return value;
}
int main(int argc, char **argv)
{
	using namespace restore_economic_lifecycle;
	try
	{
		need(argc == 5);
		const auto version = static_cast<uint32_t>(std::stoul(argv[3]));
		if (std::string(argv[1]) == "frame")
		{
			uint32_t actual = 0;
			const auto body =
				frame(argv[2], argv[4], "DURELR\0", receipt_limit, {}, &actual);
			need(actual == version);
			std::cout << body.size() << "\n";
			return 0;
		}
		need(std::string(argv[1]) == "coverage");
		const auto encoded = input(argv[2], restore_economic_baseline::witness_limit);
		const auto keys = input(argv[4], 3071 * 40);
		need(keys.size() % 40 == 0);
		std::set<account_key> mappings;
		reader in{ keys };
		while (in.offset != keys.size())
			need(mappings.insert(in.fixed<40>()).second);
		std::optional<economic_prepared_baseline> native;
		const bool accepted = economic_baseline_decode(encoded, &native) ==
				      economic_accounting_error::ok;
		bool roundtrip = false;
		if (accepted)
		{
			std::vector<uint8_t> canonical;
			need(native.has_value() && economic_baseline_encode(*native, &canonical) ==
							   economic_accounting_error::ok);
			roundtrip = canonical == encoded;
		}
		bool independent = false;
		digest coverage{};
		try
		{
			identity lineage{};
			lineage[0] = 1;
			digest original{};
			original.fill(0x31);
			coverage = coverage_digest(version, lineage, original, encoded, mappings);
			independent = true;
		}
		catch (...)
		{
		}
		std::cout << "{\"native_baseline_accepted\":" << (accepted ? "true" : "false")
			  << ",\"native_baseline_roundtrip\":" << (roundtrip ? "true" : "false")
			  << ",\"independent_accepted\":" << (independent ? "true" : "false")
			  << ",\"coverage\":\"" << restore_economic_baseline::hex(coverage)
			  << "\"}\n";
		return 0;
	}
	catch (...)
	{
		std::cerr << "lifecycle_v2_contract_refused\n";
		return 1;
	}
}
