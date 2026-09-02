#include "kingdom/kingdom_internal.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/* The one module global kingdom_db.c reads and writes. kingdom.c owns the
 * real definition; the harness owns this one so it can seed and inspect it. */
std::unordered_map<int, kingdom_realm> kingdom_realms;

static std::string persistence_root;

/* The flat-file root kingdom_db.c asks persistence_mode.c for on every call;
 * empty means "persistence never configured". */
const char *persistence_mode_flatfile_root()
{
	return persistence_root.c_str();
}

/* Swallow the module's log lines. */
void logit(const char *, const char *, ...) {}

/* Swallow engine debug output. */
void debug(const char *, ...) {}

/* Look a realm up in the harness-owned map, as kingdom.c's finder does. */
kingdom_realm *kingdom_find_realm(int assoc_id)
{
	const auto found = kingdom_realms.find(assoc_id);
	return found == kingdom_realms.end() ? nullptr : &found->second;
}

namespace
{
/* On-disk layout, mirrored from kingdom_db.c's flat-file half: magic 8,
 * version u32, payload length u32, revision u64, SHA-256 32; then a u32 record
 * count and 64 bytes per realm (4 x i32, 4 x i64, i64, 2 x i32), assoc_id
 * first. Reading it back independently is what pins the layout. */
constexpr size_t header_size = 8 + 4 + 4 + 8 + 32;
constexpr size_t record_size = 4 * 4 + 4 * 8 + 8 + 2 * 4;

/* Print the message and abort the run on a failed expectation. */
void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

/* Announce a completed section so a later failure is placed. */
void passed(const char *section)
{
	std::cout << "ok: " << section << '\n';
}

/* Create root/metadata owner-only, as the flat-file preflight provisions it. */
void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "metadata");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "metadata", fs::perms::owner_all, fs::perm_options::replace);
}

/* Where the catalogue file lives under a root. */
fs::path authority_of(const fs::path &root)
{
	return root / "metadata/kingdom_realms";
}

/* Whole-file read; empty when the file cannot be opened. */
std::vector<uint8_t> read_file(const fs::path &path)
{
	std::ifstream file(path, std::ios::binary);
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
				    std::istreambuf_iterator<char>());
}

/* Whole-file overwrite. */
void write_file(const fs::path &path, const std::vector<uint8_t> &bytes)
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	file.write(reinterpret_cast<const char *>(bytes.data()),
		   static_cast<std::streamsize>(bytes.size()));
}

/* Little-endian u64 at a byte pointer. */
uint64_t le64(const uint8_t *at)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= static_cast<uint64_t>(at[index]) << (index * 8);
	return value;
}

/* Little-endian i32 at a byte pointer. */
int32_t le32(const uint8_t *at)
{
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(at[index]) << (index * 8);
	return static_cast<int32_t>(value);
}

/* The catalogue's revision counter, straight from the header. */
uint64_t revision_of(const fs::path &root)
{
	const auto bytes = read_file(authority_of(root));
	require(bytes.size() >= header_size, "authority too short to carry a revision");
	return le64(bytes.data() + 16);
}

/* The assoc_id of every stored record, in file order. */
std::vector<int> stored_ids(const fs::path &root)
{
	const auto bytes = read_file(authority_of(root));
	require(bytes.size() >= header_size + 4, "authority too short to carry a count");
	const int32_t count = le32(bytes.data() + header_size);
	require(count >= 0 &&
			bytes.size() == header_size + 4 + static_cast<size_t>(count) * record_size,
		"authority size does not match its record count (layout drift)");
	std::vector<int> ids;
	for (int32_t index = 0; index < count; ++index)
		ids.push_back(le32(bytes.data() + header_size + 4 +
				   static_cast<size_t>(index) * record_size));
	return ids;
}

/* Build a fully populated, sane realm record. */
kingdom_realm make_realm(int assoc_id, int realm_id, int hall_vnum, int highest_claim, long mineral,
			 long wood, long fibre, long water, time_t paid_through, int arrears,
			 int missed_cycles)
{
	kingdom_realm realm;
	realm.assoc_id = assoc_id;
	realm.realm_id = realm_id;
	realm.hall_vnum = hall_vnum;
	realm.highest_claim = highest_claim;
	realm.resources[KRES_MINERAL] = mineral;
	realm.resources[KRES_WOOD] = wood;
	realm.resources[KRES_FIBRE] = fibre;
	realm.resources[KRES_WATER] = water;
	realm.upkeep_paid_through = paid_through;
	realm.arrears = arrears;
	realm.missed_cycles = missed_cycles;
	return realm;
}

/* True when every persisted field of two records agrees. */
bool same_persisted_fields(const kingdom_realm &a, const kingdom_realm &b)
{
	if (a.assoc_id != b.assoc_id || a.realm_id != b.realm_id || a.hall_vnum != b.hall_vnum ||
	    a.highest_claim != b.highest_claim || a.upkeep_paid_through != b.upkeep_paid_through ||
	    a.arrears != b.arrears || a.missed_cycles != b.missed_cycles)
		return false;
	for (int res = 0; res < KRES_MAX; res++)
		if (a.resources[res] != b.resources[res])
			return false;
	return true;
}

/* True when a loaded record carries only its persisted fields: runtime flags
 * clear and the anchor left at its unresolved defaults. */
bool loaded_defaults_hold(const kingdom_realm &realm)
{
	return !realm.dirty && !realm.payment_pending && realm.hall_rnum == 0 &&
	       realm.zone_idx == -1 && realm.hall_x == -1 && realm.hall_y == -1;
}
} // namespace

/* Drive the flat-file kingdom store through every entry point under the
 * temporary root named by argv[1]; exit 1 with a message on the first failed
 * expectation, exit 0 after the last section passes. */
int main(int argc, char **argv)
{
	require(argc == 2, "expected temporary root");
	const fs::path base = argv[1];

	const kingdom_realm realm3 = make_realm(3, 1, 570424, 8, 10, 20, 30, 40, 1700000000, 0, 0);
	const kingdom_realm realm7 = make_realm(7, 2, 570824, 24, 0, 5000000000L, 7, 0, 1700003600,
						KARR_NODES_DORMANT, 2);
	const kingdom_realm realm12 =
		make_realm(12, 3, 571224, 80, 1, 2, 3, 4, 1700007200, KARR_RINGS_REVERTING, 5);

	/* --- no root configured: every entry point refuses --- */
	persistence_root.clear();
	kingdom_realms.clear();
	require(!kingdom_db_load_all(), "load succeeded with no state root");
	require(!kingdom_db_save_realm(realm3), "save succeeded with no state root");
	require(!kingdom_db_delete_realm(3), "delete succeeded with no state root");
	kingdom_realms[3] = realm3;
	kingdom_realms[3].dirty = true;
	kingdom_db_flush_dirty();
	require(kingdom_realms[3].dirty, "flush cleared a flag it could not write");
	passed("unconfigured root refuses every write");

	/* --- empty root: absent authority is an empty world, not an error --- */
	const fs::path empty = base / "empty";
	prepare_root(empty);
	persistence_root = empty.string();
	kingdom_realms[99] = realm3;
	require(kingdom_db_load_all() && kingdom_realms.empty(),
		"absent authority did not load as an empty world");
	require(!fs::exists(authority_of(empty)), "load created an authority file");
	require(kingdom_db_delete_realm(5), "delete against a missing file was not idempotent");
	require(!fs::exists(authority_of(empty)), "delete created an authority file");
	kingdom_db_flush_dirty();
	require(!fs::exists(authority_of(empty)), "flush with nothing dirty wrote a file");
	passed("empty root loads empty and stays absent");

	/* --- save: sorted insert, revision, insane rejected --- */
	const fs::path state = base / "state";
	prepare_root(state);
	persistence_root = state.string();
	std::vector<uint64_t> revisions;
	require(kingdom_db_save_realm(realm7), "first save failed");
	revisions.push_back(revision_of(state));
	require(revisions.back() == 1, "first revision is not 1");
	require(kingdom_db_save_realm(realm3), "second save failed");
	revisions.push_back(revision_of(state));
	require(kingdom_db_save_realm(realm12), "third save failed");
	revisions.push_back(revision_of(state));
	require(stored_ids(state) == std::vector<int>({ 3, 7, 12 }),
		"records are not stored sorted by assoc_id");
	kingdom_realm insane = realm3;
	insane.assoc_id = 4;
	insane.highest_claim = KINGDOM_MAX_SQUARES + 1;
	require(!kingdom_db_save_realm(insane), "an out-of-range claim was persisted");
	insane = realm3;
	insane.assoc_id = 0;
	require(!kingdom_db_save_realm(insane), "assoc_id 0 was persisted");
	insane = realm3;
	insane.assoc_id = 4;
	insane.arrears = KARR_RINGS_REVERTING + 1;
	require(!kingdom_db_save_realm(insane), "an out-of-range arrears rung was persisted");
	require(revision_of(state) == revisions.back(), "a refused save still republished");
	require(stored_ids(state) == std::vector<int>({ 3, 7, 12 }),
		"a refused save changed the file");
	passed("save inserts sorted and refuses insane records");

	/* --- load: the multi-realm catalogue round-trips field for field --- */
	kingdom_realms.clear();
	kingdom_realms[99] = realm3;
	require(kingdom_db_load_all(), "catalogue load failed");
	require(kingdom_realms.size() == 3 && !kingdom_realms.count(99),
		"load did not replace the map with the three stored realms");
	require(same_persisted_fields(kingdom_realms[3], realm3) &&
			same_persisted_fields(kingdom_realms[7], realm7) &&
			same_persisted_fields(kingdom_realms[12], realm12),
		"a persisted field did not round trip");
	require(loaded_defaults_hold(kingdom_realms[3]) &&
			loaded_defaults_hold(kingdom_realms[7]) &&
			loaded_defaults_hold(kingdom_realms[12]),
		"a loaded realm carried runtime state (dirty, pending, or a resolved anchor)");
	passed("multi-realm save->load round trip");

	/* --- replace: same key, new values, neighbours untouched --- */
	kingdom_realm realm7b = realm7;
	realm7b.highest_claim = 48;
	realm7b.arrears = KARR_CURRENT;
	realm7b.missed_cycles = 0;
	realm7b.resources[KRES_WATER] = 12345;
	realm7b.upkeep_paid_through = 1700010800;
	require(kingdom_db_save_realm(realm7b), "replace save failed");
	revisions.push_back(revision_of(state));
	require(stored_ids(state) == std::vector<int>({ 3, 7, 12 }), "replace changed the key set");
	require(kingdom_db_load_all() && kingdom_realms.size() == 3, "reload after replace failed");
	require(same_persisted_fields(kingdom_realms[7], realm7b), "replace did not overwrite");
	require(same_persisted_fields(kingdom_realms[3], realm3) &&
			same_persisted_fields(kingdom_realms[12], realm12),
		"replace disturbed a neighbouring record");
	passed("upsert replaces in place");

	/* --- runtime flags are never encoded --- */
	kingdom_realm flagged = realm12;
	flagged.dirty = true;
	flagged.payment_pending = true;
	flagged.hall_rnum = 4242;
	flagged.zone_idx = 9;
	require(kingdom_db_save_realm(flagged), "save of a flagged record failed");
	revisions.push_back(revision_of(state));
	require(fs::file_size(authority_of(state)) == header_size + 4 + 3 * record_size,
		"record width changed: a runtime field is being encoded");
	require(kingdom_db_load_all() && loaded_defaults_hold(kingdom_realms[12]) &&
			same_persisted_fields(kingdom_realms[12], realm12),
		"dirty / payment_pending / resolved anchor leaked into the file");
	passed("dirty, payment_pending and the resolved anchor are not persisted");

	/* --- delete: idempotent on a missing record, real on a present one --- */
	require(kingdom_db_delete_realm(500), "delete of a missing record failed");
	require(revision_of(state) == revisions.back(), "deleting a missing record republished");
	require(!kingdom_db_delete_realm(0), "delete accepted assoc_id 0");
	require(!kingdom_db_delete_realm(-3), "delete accepted a negative assoc_id");
	require(kingdom_db_delete_realm(3), "delete of a present record failed");
	revisions.push_back(revision_of(state));
	require(stored_ids(state) == std::vector<int>({ 7, 12 }), "delete left the record behind");
	require(kingdom_db_delete_realm(3), "repeated delete was not idempotent");
	require(revision_of(state) == revisions.back(), "repeated delete republished");
	require(kingdom_db_load_all() && kingdom_realms.size() == 2 && !kingdom_realms.count(3),
		"deleted record came back on load");
	passed("delete is idempotent and removes only its record");

	/* --- flush_dirty merges into the on-disk catalogue --- */
	kingdom_realms.clear();
	kingdom_realm realm20 = make_realm(20, 4, 572024, 1, 0, 0, 0, 0, 1700014400, 0, 0);
	kingdom_realms[20] = realm20;
	kingdom_realms[20].dirty = true;
	kingdom_realm realm7c = realm7b;
	realm7c.highest_claim = 80;
	kingdom_realms[7] = realm7c;
	kingdom_realms[7].dirty = true;
	/* Realm 12 is ABSENT from memory (as after a short load) and must survive
	 * on disk; realm 7's in-memory copy is dirty and must be written. A clean
	 * in-memory change must NOT be written: stage one as realm 12's stand-in. */
	kingdom_realm realm12_clean_edit = realm12;
	realm12_clean_edit.highest_claim = 2;
	kingdom_realms[12] = realm12_clean_edit; /* dirty stays false */
	kingdom_db_flush_dirty();
	require(!kingdom_realms[20].dirty && !kingdom_realms[7].dirty,
		"flush did not clear the flags it wrote");
	revisions.push_back(revision_of(state));
	require(stored_ids(state) == std::vector<int>({ 7, 12, 20 }),
		"flush did not merge: a disk-only realm was lost or a new one missing");
	require(kingdom_db_load_all() && kingdom_realms.size() == 3, "reload after flush failed");
	require(same_persisted_fields(kingdom_realms[7], realm7c) &&
			same_persisted_fields(kingdom_realms[20], realm20),
		"flush did not write the dirty records");
	require(same_persisted_fields(kingdom_realms[12], realm12),
		"flush wrote a record that was not dirty");
	kingdom_db_flush_dirty();
	require(revision_of(state) == revisions.back(), "flush with nothing dirty republished");
	passed("flush_dirty merges dirty records into the on-disk catalogue");

	/* --- an insane record is dropped individually, not with its batch --- */
	kingdom_realms.clear();
	kingdom_realm realm30 = make_realm(30, 5, 573024, 9, 1, 1, 1, 1, 1700018000, 0, 0);
	kingdom_realms[30] = realm30;
	kingdom_realms[30].dirty = true;
	kingdom_realm realm31 = make_realm(31, 6, 573124, 999, 1, 1, 1, 1, 1700018000, 0, 0);
	kingdom_realms[31] = realm31;
	kingdom_realms[31].dirty = true;
	kingdom_db_flush_dirty();
	revisions.push_back(revision_of(state));
	require(!kingdom_realms[30].dirty, "the sane record of the batch was not written");
	require(kingdom_realms[31].dirty, "the insane record's flag was cleared");
	require(stored_ids(state) == std::vector<int>({ 7, 12, 20, 30 }),
		"the insane record reached the file or the sane one did not");
	kingdom_realms.clear();
	kingdom_realms[31] = realm31;
	kingdom_realms[31].dirty = true;
	kingdom_db_flush_dirty();
	require(revision_of(state) == revisions.back(),
		"a flush with only an insane record republished the catalogue");
	passed("an insane record is dropped individually");

	/* --- revision monotonicity across every publish so far --- */
	for (size_t index = 1; index < revisions.size(); ++index)
		require(revisions[index] == revisions[index - 1] + 1,
			"revision did not advance by exactly one per publish");
	passed("revision advances by one per publish");

	/* --- corruption: checksum and truncation are INVALID, not empty --- */
	const fs::path authority = authority_of(state);
	const std::vector<uint8_t> intact = read_file(authority);
	std::vector<uint8_t> corrupt = intact;
	corrupt.back() ^= 0x5a;
	write_file(authority, corrupt);
	kingdom_realms[7] = realm7c;
	require(!kingdom_db_load_all(), "checksum corruption was accepted");
	require(kingdom_realms.empty(), "a failed load left stale realms in the map");
	require(!kingdom_db_save_realm(realm3), "save overwrote a corrupt authority");
	require(read_file(authority) == corrupt, "a refused save changed the corrupt file");
	require(!kingdom_db_delete_realm(7), "delete overwrote a corrupt authority");
	require(read_file(authority) == corrupt, "a refused delete changed the corrupt file");
	kingdom_realms[7] = realm7c;
	kingdom_realms[7].dirty = true;
	kingdom_db_flush_dirty();
	require(kingdom_realms[7].dirty && read_file(authority) == corrupt,
		"flush wrote over a corrupt authority or cleared its flag");

	std::vector<uint8_t> truncated = intact;
	truncated.pop_back();
	write_file(authority, truncated);
	require(!kingdom_db_load_all() && kingdom_realms.empty(),
		"a truncated payload was accepted");
	require(!kingdom_db_save_realm(realm3) && read_file(authority) == truncated,
		"save overwrote a truncated authority");

	std::vector<uint8_t> short_header(intact.begin(), intact.begin() + 20);
	write_file(authority, short_header);
	require(!kingdom_db_load_all() && kingdom_realms.empty(),
		"a file shorter than its header was accepted");

	write_file(authority, intact);
	require(kingdom_db_load_all() && kingdom_realms.size() == 4,
		"the restored authority did not load");
	passed("corrupt checksum and truncated payload are rejected without overwrite");

	/* --- payment_pending gates the generic flush (lane A's durability rule):
	 * a record whose paired guild write has not landed must stay off disk and
	 * stay dirty until kingdom_upkeep_retry_pending() clears the pair. --- */
	kingdom_realms.clear();
	kingdom_realm realm40 = make_realm(40, 7, 574024, 3, 0, 0, 0, 0, 1700021600, 0, 0);
	kingdom_realms[40] = realm40;
	kingdom_realms[40].dirty = true;
	kingdom_realms[40].payment_pending = true;
	kingdom_db_flush_dirty();
	require(stored_ids(state) == std::vector<int>({ 7, 12, 20, 30 }),
		"flush published a record whose payment is still pending");
	require(kingdom_realms[40].dirty && kingdom_realms[40].payment_pending,
		"flush cleared the flags of a payment-pending record");
	require(revision_of(state) == revisions.back(),
		"a flush holding only a payment-pending record republished");
	passed("payment_pending keeps a record out of the generic flush");

	std::cout << "flat-file kingdom repository passed\n";
	return 0;
}
