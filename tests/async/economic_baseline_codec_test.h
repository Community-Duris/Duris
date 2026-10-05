// This exercises the production preparation/codec/command contracts. It does
// not substitute a hand-built witness or establish native cutover authority.
void baseline_equipment_tests()
{
	assert(economic_baseline_batch{}.witness_version == 2);
	auto singleton = [](item_owner_type owner, uint16_t slot)
	{
		auto value = fixture();
		value.witness_version = 2;
		value.holdings.clear();
		value.items = {
			{ { 101, { { owner, 7, 0 }, 101, 0, 3, item_custody_state::active, slot } },
			  digest(31) }
		};
		return value;
	};
	for (auto owner : { item_owner_type::player, item_owner_type::native_mobile })
	{
		std::vector<uint16_t> slots = { 0, 1, 43 };
		if (owner == item_owner_type::player)
			slots.push_back(UINT16_MAX);
		auto carried = prepare(singleton(owner, 0));
		std::vector<uint8_t> carried_bytes;
		assert(economic_baseline_encode(*carried, &carried_bytes) == error::ok);
		critical_command carried_command;
		assert(economic_baseline_command_build(*carried, 123456, &carried_command) ==
		       error::ok);
		for (auto slot : slots)
		{
			auto original = prepare(singleton(owner, slot));
			std::vector<uint8_t> witness;
			assert(economic_baseline_encode(*original, &witness) == error::ok);
			assert(witness.size() == 288 && witness[3] == '2' && witness[4] == 2);
			assert(witness[248] == static_cast<uint8_t>(slot) &&
			       witness[249] == static_cast<uint8_t>(slot >> 8));
			std::optional<economic_prepared_baseline> restored;
			assert(economic_baseline_decode(witness, &restored) == error::ok);
			assert(restored->witness().witness_version == 2);
			assert(restored->witness().items[0].snapshot.position.equipment_slot ==
			       slot);
			assert(restored->encoded_plan() == original->encoded_plan());
			std::vector<uint8_t> repeated;
			assert(economic_baseline_encode(*restored, &repeated) == error::ok &&
			       repeated == witness);
			critical_command command;
			assert(economic_baseline_command_build(*original, 123456, &command) ==
			       error::ok);
			economic_accounting_plan resolved;
			assert(economic_baseline_command_plan(command, *restored, &resolved) ==
			       error::ok);
			std::vector<uint8_t> frozen_plan;
			assert(economic_plan_encode(resolved, &frozen_plan) == error::ok);
			assert(resolved.items_before[0].position.equipment_slot == slot &&
			       economic_item_position_equal(resolved.items_before[0].position,
							    resolved.items_after[0].position));
			if (slot)
			{
				assert(witness != carried_bytes &&
				       command.payload != carried_command.payload &&
				       command.accounting_intent !=
					       carried_command.accounting_intent);
				assert(original->plan().metadata.domain_digest !=
				       carried->plan().metadata.domain_digest);
				assert(original->plan().metadata.intent_digest !=
				       carried->plan().metadata.intent_digest);
				assert(economic_baseline_command_plan(command, *carried,
								      &resolved) != error::ok);
				std::vector<uint8_t> unchanged;
				assert(economic_plan_encode(resolved, &unchanged) == error::ok &&
				       unchanged == frozen_plan);
			}
			const auto retained_plan = restored->encoded_plan();
			for (size_t offset : { size_t(3), size_t(4), size_t(202), size_t(250) })
			{
				auto malformed = witness;
				malformed[offset] ^= 1;
				assert(economic_baseline_decode(malformed, &restored) != error::ok);
				assert(restored->encoded_plan() == retained_plan);
			}
			for (size_t length = 0; length < witness.size(); ++length)
			{
				assert(economic_baseline_decode(std::span(witness).first(length),
								&restored) != error::ok);
				assert(restored->encoded_plan() == retained_plan);
			}
		}
	}
	auto legacy = singleton(item_owner_type::player, 0);
	legacy.witness_version = 1;
	auto old = prepare(legacy);
	std::vector<uint8_t> old_bytes;
	assert(economic_baseline_encode(*old, &old_bytes) == error::ok && old_bytes.size() == 280);
	std::optional<economic_prepared_baseline> old_restored;
	assert(economic_baseline_decode(old_bytes, &old_restored) == error::ok);
	assert(old_restored->witness().witness_version == 1 &&
	       old_restored->encoded_plan() == old->encoded_plan());
	legacy.items[0].snapshot.position.equipment_slot = 1;
	rejected(legacy, error::invalid_version);
	for (uint16_t version : { uint16_t(0), uint16_t(3) })
	{
		auto unsupported = singleton(item_owner_type::player, 0);
		unsupported.witness_version = version;
		rejected(unsupported, error::invalid_version);
	}
	// Every negative changes only the slot on an otherwise accepted owner.
	for (auto owner :
	     { item_owner_type::room, item_owner_type::container, item_owner_type::corpse,
	       item_owner_type::locker, item_owner_type::auction, item_owner_type::system,
	       item_owner_type::destruction, item_owner_type::shopkeeper,
	       item_owner_type::collector, item_owner_type::pet })
	{
		auto value = singleton(owner, 0);
		auto &position = value.items[0].snapshot.position;
		if (owner == item_owner_type::system || owner == item_owner_type::destruction)
			position.owner.id = 0;
		if (owner == item_owner_type::pet)
			position.owner.context_id = 1;
		if (owner == item_owner_type::destruction)
			position.state = item_custody_state::destroyed;
		auto sentinel = prepare(value);
		const auto original_plan = sentinel->encoded_plan();
		position.equipment_slot = 1;
		assert(economic_baseline_prepare(value, &sentinel) != error::ok);
		assert(sentinel->encoded_plan() == original_plan);
	}
	for (auto owner : { item_owner_type::player, item_owner_type::native_mobile })
	{
		auto tree = singleton(owner, 43);
		auto child = tree.items[0];
		child.snapshot.uid = 102;
		child.snapshot.position.parent_uid = 101;
		child.snapshot.position.equipment_slot = 0;
		child.source_digest = digest(32);
		tree.items.push_back(child);
		auto original = prepare(tree);
		std::vector<uint8_t> encoded;
		assert(economic_baseline_encode(*original, &encoded) == error::ok);
		std::optional<economic_prepared_baseline> restored;
		assert(economic_baseline_decode(encoded, &restored) == error::ok);
		assert(restored->encoded_plan() == original->encoded_plan());
		assert(restored->witness().items[0].snapshot.position.equipment_slot == 43 &&
		       restored->witness().items[1].snapshot.position.equipment_slot == 0);
		tree.items[1].snapshot.position.equipment_slot = 1;
		const auto original_plan = restored->encoded_plan();
		assert(economic_baseline_prepare(tree, &restored) != error::ok);
		assert(restored->encoded_plan() == original_plan);
	}
	auto mobile_overflow = singleton(item_owner_type::native_mobile, 44);
	std::optional<economic_prepared_baseline> sentinel = prepare(fixture());
	const auto sentinel_plan = sentinel->encoded_plan();
	assert(economic_baseline_prepare(mobile_overflow, &sentinel) != error::ok &&
	       sentinel->encoded_plan() == sentinel_plan);
	auto equipped = singleton(item_owner_type::player, 1);
	equipped.items[0].snapshot.position.state = item_custody_state::quarantined;
	assert(economic_baseline_prepare(equipped, &sentinel) != error::ok &&
	       sentinel->encoded_plan() == sentinel_plan);
	std::cout
		<< "baseline equipment: EAB2 exact replay and fingerprints; EAB1 retained; invalid equipment refused\n";
}

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
	assert(canonical.size() == ECONOMIC_BASELINE_V1_MAX_BYTES && canonical.size() == 872144);
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
	// The new maximum is checked separately from the unchanged legacy bound.
	auto maximum_v2 = maximum;
	maximum_v2.witness_version = 2;
	auto full_v2 = prepare(maximum_v2);
	std::vector<uint8_t> canonical_v2;
	assert(economic_baseline_encode(*full_v2, &canonical_v2) == error::ok);
	assert(canonical_v2.size() == ECONOMIC_BASELINE_MAX_BYTES && canonical_v2.size() == 920144);
	std::optional<economic_prepared_baseline> retained_v2;
	assert(economic_baseline_decode(canonical_v2, &retained_v2) == error::ok);
	assert(retained_v2->encoded_plan() == full_v2->encoded_plan());
	// The complete new maximum must also survive its original command and
	// flat authority record framing, not just the independent witness codec.
	assert(economic_baseline_command_build(*full_v2, 123456, &command) == error::ok);
	assert(command.payload.size() == ECONOMIC_BASELINE_COMMAND_BYTES);
	assert(economic_baseline_command_plan(command, *retained_v2, &bound) == error::ok);
	assert(economic_plan_encode(bound, &bound_bytes) == error::ok);
	std::vector<uint8_t> wire_v2;
	assert(critical_command_encode(command, &wire_v2) == critical_command_codec_result::ok);
	critical_command decoded_v2;
	assert(critical_command_decode(wire_v2.data(), wire_v2.size(), &decoded_v2) ==
	       critical_command_codec_result::ok);
	assert(critical_command_equal(command, decoded_v2));
	record.command = command;
	record.plan = bound_bytes;
	assert(flatfile_accounting_record_encode(record, &record_bytes) ==
	       flatfile_accounting_status::ok);
	assert(flatfile_accounting_record_decode(record_bytes, &roundtrip) ==
	       flatfile_accounting_status::ok);
	assert(roundtrip.plan == bound_bytes && critical_command_equal(roundtrip.command, command));
	canonical_v2.push_back(0);
	assert(economic_baseline_decode(canonical_v2, &retained_v2) == error::capacity);
	assert(retained_v2->encoded_plan() == full_v2->encoded_plan());
	baseline_equipment_tests();
	canonical.push_back(0);
	assert(economic_baseline_decode(canonical, &retained) == error::capacity);
	assert(retained->encoded_plan() == full->encoded_plan());
	std::cout
		<< "baseline witness codec: reference bytes, truncations, corruption, canonical order, maximum batch; "
		<< encode_failures << " encode and " << decode_failures
		<< " decode allocation failures passed\n";
}
