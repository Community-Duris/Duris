#include "kingdom/kingdom_internal.h"
#include "flatfile/flatfile_association_repository.h"
#include "flatfile/flatfile_authority_transaction.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
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

namespace
{
/* On-disk layout, mirrored from kingdom_db.c's flat-file half: magic 8,
 * version u32, payload length u32, revision u64, SHA-256 32; then a u32 record
 * count and, per realm, assoc_id first:
 *
 *     version 1    4 x i32, 4 x i64, i64, 2 x i32                = 64 bytes
 *     version 2    all of that, then 16 guard slots of
 *                  (i32 class, i32 level) and the champion's i32  = 196 bytes
 *
 * Reading the image back independently is what pins the layout, so the roster
 * is spelled out here in the same terms rather than deferred to a constant
 * from the code under test; the slot count is copied for the same reason the
 * record maximum below is. */
constexpr size_t header_size = 8 + 4 + 4 + 8 + 32;
constexpr size_t guard_slots = 16;
constexpr size_t record_size = 4 * 4 + 4 * 8 + 8 + 2 * 4 + guard_slots * (4 + 4) + 4;
/* Header field offsets, for the hand-edited images below. */
constexpr size_t version_offset = 8;
constexpr size_t payload_length_offset = 12;
constexpr size_t revision_offset = 16;
constexpr size_t digest_offset = 24;
/* highest_claim is the fourth i32 of a record. */
constexpr size_t highest_claim_offset = 12;
/* Mirrors kingdom_db.c's kingdom_realm_maximum: the record count a file may
 * declare before the decoder refuses the file rather than allocate for it.
 * A copy, because that constant lives in kingdom_db.c's anonymous namespace;
 * if the two ever drift the over-cap case below stops rejecting and fails. */
constexpr int32_t kingdom_realm_cap = 65536;

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
	fs::create_directories(root / "domains");
	fs::create_directories(root / "metadata");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
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

/* Whole-file overwrite, left owner-only. The permissions are set explicitly
 * rather than inherited: flatfile_read() rejects an authority carrying any
 * group or other bit, so a file this helper created fresh under a umask would
 * be refused on its metadata and every "rejected" expectation below would pass
 * without the decoder ever seeing the bytes. */
void write_file(const fs::path &path, const std::vector<uint8_t> &bytes)
{
	{
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		file.write(reinterpret_cast<const char *>(bytes.data()),
			   static_cast<std::streamsize>(bytes.size()));
	}
	fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write,
			fs::perm_options::replace);
}

/* Store `value` little-endian as four bytes at `at`; aborts the run when the
 * image is too short to hold the edit. */
void put_le32(std::vector<uint8_t> &bytes, size_t at, int32_t value)
{
	require(bytes.size() >= at + 4, "image is too short for the edit under test");
	uint32_t bits = static_cast<uint32_t>(value);
	for (size_t index = 0; index < 4; ++index)
		bytes[at + index] = static_cast<uint8_t>((bits >> (index * 8)) & 0xff);
}

/* Rewrite the header's SHA-256 field over the payload that follows the header,
 * so a hand-edited image is sound in every way decode_catalog() checks except
 * the one field under test. encode_catalog() checksums the PAYLOAD only (count
 * + records), so a record edit needs this and a header edit does not. */
void reseal(std::vector<uint8_t> &bytes)
{
	require(bytes.size() >= header_size, "image is too short to reseal");
	SHA256(bytes.data() + header_size, bytes.size() - header_size,
	       bytes.data() + digest_offset);
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

/* Build a sound on-disk image holding `count` minimally-sane records:
 * assoc_id 1..count (positive and strictly increasing, so the layout check
 * passes) and every other field at its zero default (so record_is_sane()
 * passes). The magic is restated here rather than shared with kingdom_db.c
 * on purpose -- an independent statement of the format is what makes the
 * round trip evidence. Used to put the decoder's record-count cap on its
 * boundary: at the cap this is a valid catalogue, one past it nothing but
 * the cap can be what rejects it. */
std::vector<uint8_t> catalog_image(int32_t count, int32_t version = 2)
{
	/* A version 1 record stops before the roster; a version 2 one carries
	 * it. Both are valid images, which is the point of taking the version:
	 * the decoder must still read the older layout. */
	const size_t width = version >= 2 ? record_size : record_size - (guard_slots * 8 + 4);
	const size_t payload_size = 4 + static_cast<size_t>(count) * width;
	std::vector<uint8_t> image(header_size + payload_size, 0);
	const uint8_t magic[8] = { 'D', 'U', 'R', 'K', 'I', 'N', 'G', 0 };
	memcpy(image.data(), magic, sizeof(magic));
	put_le32(image, version_offset, version);
	put_le32(image, payload_length_offset, static_cast<int32_t>(payload_size));
	image[revision_offset] = 1; /* revision 1: little-endian, the rest zero */
	put_le32(image, header_size, count);
	for (int32_t index = 0; index < count; ++index)
		put_le32(image, header_size + 4 + static_cast<size_t>(index) * width, index + 1);
	reseal(image);
	return image;
}

/* Install `image` as the authority under `root`, seed the map with `seed` so
 * an emptied map afterwards proves the load ran, and require that the load
 * rejects the file WHOLE (false, and no realms left behind). Aborts the run
 * with `message` when the image is accepted instead. */
void require_rejected(const fs::path &root, const std::vector<uint8_t> &image,
		      const kingdom_realm &seed, const char *message)
{
	write_file(authority_of(root), image);
	kingdom_realms.clear();
	kingdom_realms[seed.assoc_id] = seed;
	require(!kingdom_db_load_all(), message);
	require(kingdom_realms.empty(), "a rejected load left stale realms in the map");
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

	/* --- FLUSH side: an insane record is dropped individually, not with its
	 * batch. This is the writer's half of the per-record rule; the reader's
	 * half (decode_catalog dropping one bad on-disk record) is the section
	 * after it. --- */
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
	passed("an insane record is dropped from a flush batch individually");

	/* --- DECODE side: one insane record in an otherwise-valid catalogue is
	 * dropped on load and the file heals at the next publish.
	 *
	 * Only the payload is checksummed, so an edited record can be resealed
	 * from outside: the image below carries a valid magic, version, payload
	 * length, revision, digest and assoc_id ordering, and differs from a
	 * published file in exactly one field. Its own root, so the `state`
	 * catalogue and its revision series are untouched. --- */
	const fs::path heal = base / "heal";
	prepare_root(heal);
	persistence_root = heal.string();
	kingdom_realms.clear();
	require(kingdom_db_save_realm(realm3) && kingdom_db_save_realm(realm7) &&
			kingdom_db_save_realm(realm12),
		"seeding the heal catalogue failed");
	require(stored_ids(heal) == std::vector<int>({ 3, 7, 12 }),
		"the heal catalogue is not the three sorted records");

	std::vector<uint8_t> edited = read_file(authority_of(heal));
	/* Record index 1 is association 7. Push its highest_claim past
	 * KINGDOM_MAX_SQUARES and leave its assoc_id alone, so the file's
	 * strictly-increasing order still holds and only record_is_sane() can be
	 * what rejects it. */
	put_le32(edited, header_size + 4 + record_size + highest_claim_offset,
		 KINGDOM_MAX_SQUARES + 1);
	/* The same edit UNSEALED is a whole-file rejection. Pinning that first is
	 * what proves the resealed image below reaches the per-record path at all
	 * rather than passing for some unrelated reason. */
	require_rejected(heal, edited, realm3,
			 "an unsealed record edit was accepted: the records are not checksummed");
	reseal(edited);
	write_file(authority_of(heal), edited);

	const uint64_t heal_revision = revision_of(heal);
	kingdom_realms.clear();
	kingdom_realms[99] = realm3;
	require(kingdom_db_load_all(), "one insane record failed the whole catalogue");
	require(kingdom_realms.size() == 2 && kingdom_realms.count(3) && kingdom_realms.count(12) &&
			!kingdom_realms.count(7),
		"the insane record survived the load, or a sane neighbour went with it");
	require(same_persisted_fields(kingdom_realms[3], realm3) &&
			same_persisted_fields(kingdom_realms[12], realm12),
		"a surviving record was disturbed by its neighbour's rejection");
	require(stored_ids(heal) == std::vector<int>({ 3, 7, 12 }),
		"the load rewrote the authority instead of only decoding it");
	require(kingdom_db_save_realm(realm3), "the publish over a dropped record failed");
	require(stored_ids(heal) == std::vector<int>({ 3, 12 }),
		"the dropped record is still on disk after the next publish");
	require(revision_of(heal) == heal_revision + 1,
		"the healing publish did not advance the revision by exactly one");
	require(kingdom_db_load_all() && kingdom_realms.size() == 2 && !kingdom_realms.count(7),
		"the healed catalogue did not reload as the two survivors");
	passed("one insane on-disk record is dropped on load and gone at the next publish");

	/* --- header-level rejections other than checksum and truncation.
	 * Each edit below leaves the payload, and so the digest, valid: what
	 * rejects the file is the named header field on its own. --- */
	const std::vector<uint8_t> sound = read_file(authority_of(heal));

	std::vector<uint8_t> broken = sound;
	broken[0] = static_cast<uint8_t>(broken[0] ^ 0xff);
	require_rejected(heal, broken, realm3, "a file with the wrong magic was accepted");

	/* 3 is one past the version this build writes. 2 is NOT tested here any
	 * more and must not be: the roster change made 2 the current version,
	 * and 1 is still read on purpose so a server upgraded across that change
	 * does not present every realm as never having existed. Both of those
	 * are checked as ACCEPTANCES below. */
	broken = sound;
	put_le32(broken, version_offset, 3);
	require_rejected(heal, broken, realm3, "a file of an unknown version was accepted");

	broken = sound;
	put_le32(broken, version_offset, 0);
	require_rejected(heal, broken, realm3, "a file of version 0 was accepted");

	broken = sound;
	std::fill(broken.begin() + static_cast<std::ptrdiff_t>(revision_offset),
		  broken.begin() + static_cast<std::ptrdiff_t>(digest_offset),
		  static_cast<uint8_t>(0));
	require_rejected(heal, broken, realm3, "a file with revision 0 was accepted");

	write_file(authority_of(heal), sound);
	kingdom_realms.clear();
	require(kingdom_db_load_all() && kingdom_realms.size() == 2,
		"the sound authority did not load again after the header cases");
	passed("bad magic, unknown version and zero revision are rejected");

	/* --- the record-count cap, on its boundary. Both images below are sound
	 * in every other respect and carry a full, in-order, sane payload, so the
	 * declared count is the ONLY thing that can separate them: a short payload
	 * would have been rejected by the length or cursor check instead and would
	 * have proved nothing about the cap. --- */
	const fs::path cap = base / "cap";
	prepare_root(cap);
	persistence_root = cap.string();
	write_file(authority_of(cap), catalog_image(kingdom_realm_cap));
	kingdom_realms.clear();
	require(kingdom_db_load_all() &&
			kingdom_realms.size() == static_cast<size_t>(kingdom_realm_cap),
		"a catalogue of exactly the cap's many records was rejected");
	require_rejected(cap, catalog_image(kingdom_realm_cap + 1), realm3,
			 "a catalogue one record past the cap was accepted");
	passed("the record-count cap admits the cap itself and rejects one past it");

	/* --- a VERSION 1 file still loads, with empty rosters ---
	 * The roster change of 2026-09-04 moved the file to version 2. A server
	 * upgraded across it has a version 1 authority on disk, and refusing that
	 * file would present every realm on the server as never having existed --
	 * the single worst reading of "the format changed". So version 1 must
	 * still decode, and a version 1 record means exactly what it says: no
	 * guard was ever bought, because guards were derived from land then. */
	const fs::path older = base / "v1";
	prepare_root(older);
	persistence_root = older.string();
	write_file(authority_of(older), catalog_image(3, 1));
	kingdom_realms.clear();
	require(kingdom_db_load_all() && kingdom_realms.size() == 3,
		"a version 1 authority file was rejected");
	for (const auto &entry : kingdom_realms)
		for (int slot = 0; slot < 16; ++slot)
			require(entry.second.guards[slot].level == 0,
				"a version 1 record decoded with a guard on its roster");
	passed("a version 1 authority still loads, with every roster empty");

	/* Back to the catalogue the remaining sections measure. */
	persistence_root = state.string();

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

	/* --- paid guild + roster: the shared recovery journal completes both
	 * after-images after an interruption between them. --- */
	const fs::path paid = base / "paid";
	prepare_root(paid);
	persistence_root = paid.string();
	flatfile_association_record paid_guild = {};
	paid_guild.association_id = 50;
	paid_guild.name = "Paid Guild";
	paid_guild.platinum = 7;
	kingdom_realm paid_realm = make_realm(50, 8, 574424, 8, 0, 0, 0, 0, 1700025200, 0, 0);
	paid_realm.guards[0].guard_class = 1;
	paid_realm.guards[0].level = KINGDOM_GUARD_BASE_LEVEL;
	std::string transaction_error;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(!kingdom_db_save_payment_pair(paid.string(), paid_guild, paid_realm,
					      &transaction_error),
		"fault injection did not interrupt the paid kingdom commit");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(fs::exists(paid / "domains/association_catalog") &&
			!fs::exists(authority_of(paid)) &&
			fs::exists(paid / "domains/.critical-authority-transaction"),
		"interruption did not stop between the guild and realm after-images");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(paid.string(), &transaction_error),
			"could not acquire the paid kingdom recovery lock");
		require(flatfile_authority_transaction_recover(paid.string(), lock,
							       &transaction_error) ==
				flatfile_authority_transaction_result::ok,
			("paid kingdom recovery failed: " + transaction_error).c_str());
	}
	std::vector<flatfile_association_record> paid_guilds;
	require(flatfile_association_list(paid.string(), &paid_guilds, &transaction_error) ==
				flatfile_association_result::ok &&
			paid_guilds.size() == 1 && paid_guilds[0].association_id == 50 &&
			paid_guilds[0].platinum == 7,
		"recovery did not preserve the paid guild after-image");
	kingdom_realms.clear();
	require(kingdom_db_load_all() && kingdom_realms.size() == 1 &&
			kingdom_realms[50].guards[0].guard_class == 1 &&
			kingdom_realms[50].guards[0].level == KINGDOM_GUARD_BASE_LEVEL &&
			!fs::exists(paid / "domains/.critical-authority-transaction"),
		"recovery did not preserve the bought roster after-image");
	passed("paid guild and roster recover as one authority transaction");

	/* --- payment_pending gates the generic flush (lane A's durability rule):
	 * a record whose paired guild write has not landed must stay off disk and
	 * stay dirty until kingdom_upkeep_retry_pending() clears the pair. --- */
	persistence_root = state.string();
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
