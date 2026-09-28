// Included after the preparation fixture and allocation fault helpers.
void codec_tests()
{
	auto prepared = prepare(fixture());
	std::vector<uint8_t> encoded;
	assert(economic_baseline_encode(*prepared, &encoded) == error::ok);
	assert(encoded == REFERENCE_WITNESS);
	const auto expected_plan = prepared->encoded_plan();
	auto retained = prepare(fixture());
	assert(economic_baseline_decode(encoded, &retained) == error::ok);
	assert(retained->encoded_plan() == expected_plan);
	for (size_t size = 0; size < encoded.size(); ++size)
	{
		assert(economic_baseline_decode(std::span(encoded).first(size), &retained) !=
		       error::ok);
		assert(retained->encoded_plan() == expected_plan);
	}
	auto reject = [&](const std::vector<uint8_t> &bad)
	{
		assert(economic_baseline_decode(bad, &retained) != error::ok);
		assert(retained->encoded_plan() == expected_plan);
	};
	for (size_t offset : { size_t(0), size_t(4), size_t(6), size_t(8), size_t(12), size_t(116),
			       size_t(192 + 36), size_t(192 + 6 * 112 + 10) })
	{
		auto bad = encoded;
		bad[offset] ^= 1;
		reject(bad);
	}
	auto bad = encoded;
	bad.push_back(0);
	reject(bad);
	bad = encoded;
	std::fill_n(bad.begin() + 184, 8, 255);
	reject(bad);
	bad = encoded;
	std::fill_n(bad.begin() + 192 + 40, 8, 255);
	reject(bad);
	bad = encoded;
	std::swap_ranges(bad.begin() + 192, bad.begin() + 192 + 112, bad.begin() + 192 + 112);
	reject(bad);
	bad = encoded;
	std::copy_n(bad.begin() + 192, 112, bad.begin() + 192 + 112);
	reject(bad);
	bad = encoded;
	std::swap_ranges(bad.begin() + 192 + 6 * 112, bad.begin() + 192 + 6 * 112 + 88,
			 bad.begin() + 192 + 6 * 112 + 88);
	reject(bad);
	// Changed but valid source bytes are distinct evidence, not authenticated
	// corruption detection. A receipt owner must compare the original hashes.
	bad = encoded;
	bad[192 + 80] ^= 1;
	assert(economic_baseline_decode(bad, &retained) == error::ok);
	assert(retained->encoded_plan() != expected_plan);
	assert(retained->plan().metadata.operation_id.bytes ==
	       prepared->plan().metadata.operation_id.bytes);
	retained = prepare(fixture());
	auto shuffled = fixture();
	std::reverse(shuffled.holdings.begin(), shuffled.holdings.end());
	std::reverse(shuffled.items.begin(), shuffled.items.end());
	auto reordered = prepare(shuffled);
	std::vector<uint8_t> canonical;
	assert(economic_baseline_encode(*reordered, &canonical) == error::ok &&
	       canonical == encoded);
	auto moved = std::move(*reordered);
	canonical = { 99 };
	assert(economic_baseline_encode(*reordered, &canonical) != error::ok &&
	       canonical == std::vector<uint8_t>{ 99 });
	assert(economic_baseline_encode(moved, &canonical) == error::ok && canonical == encoded);
	size_t encode_failures = 0, decode_failures = 0;
	for (size_t target = 1; target < 1024; ++target)
	{
		canonical = { 99 };
		allocation_seen = 0;
		allocation_target = target;
		const auto status = economic_baseline_encode(*prepared, &canonical);
		allocation_target = 0;
		if (status == error::ok)
			break;
		assert(status == error::capacity && canonical == std::vector<uint8_t>{ 99 });
		++encode_failures;
	}
	for (size_t target = 1; target < 1024; ++target)
	{
		allocation_seen = 0;
		allocation_target = target;
		const auto status = economic_baseline_decode(encoded, &retained);
		allocation_target = 0;
		assert(retained->encoded_plan() == expected_plan);
		if (status == error::ok)
			break;
		assert(status == error::capacity);
		++decode_failures;
	}
	assert(encode_failures > 0 && encode_failures < 1023 && decode_failures > 20 &&
	       decode_failures < 1023);
	auto maximum = fixture();
	maximum.holdings.clear();
	maximum.items.clear();
	for (uint64_t n = 1; n <= ECONOMIC_BASELINE_MAX_HOLDINGS; ++n)
		maximum.holdings.push_back({ { id(1), economic_account_kind::wallet, n, 0 },
					     { 1, 2, 3, 4 },
					     UINT64_MAX,
					     digest(1) });
	for (uint64_t n = 1; n <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++n)
		maximum.items.push_back({ { n,
					    { { item_owner_type::player, 7, 0 },
					      1,
					      n - 1,
					      UINT64_MAX,
					      item_custody_state::active } },
					  digest(1) });
	auto full = prepare(maximum);
	assert(economic_baseline_encode(*full, &canonical) == error::ok);
	assert(canonical.size() == ECONOMIC_BASELINE_MAX_BYTES && canonical.size() == 872144);
	assert(economic_baseline_decode(canonical, &retained) == error::ok);
	assert(retained->encoded_plan() == full->encoded_plan());
	critical_command command;
	economic_accounting_plan bound;
	assert(economic_baseline_command_build(*full, 123456, &command) == error::ok);
	assert(command.payload.size() == ECONOMIC_BASELINE_COMMAND_BYTES);
	assert(economic_baseline_command_plan(command, *retained, &bound) == error::ok);
	std::vector<uint8_t> bound_bytes, record_bytes;
	assert(economic_plan_encode(bound, &bound_bytes) == error::ok);
	flatfile_accounting_record record;
	record.command = command;
	record.plan = bound_bytes;
	assert(flatfile_accounting_record_encode(record, &record_bytes) ==
	       flatfile_accounting_status::ok);
	flatfile_accounting_record roundtrip;
	assert(flatfile_accounting_record_decode(record_bytes, &roundtrip) ==
	       flatfile_accounting_status::ok);
	assert(roundtrip.plan == bound_bytes);
	canonical.push_back(0);
	assert(economic_baseline_decode(canonical, &retained) == error::capacity);
	assert(retained->encoded_plan() == full->encoded_plan());
	std::cout
		<< "baseline witness codec: reference bytes, truncations, corruption, canonical order, maximum batch; "
		<< encode_failures << " encode and " << decode_failures
		<< " decode allocation failures passed\n";
}
