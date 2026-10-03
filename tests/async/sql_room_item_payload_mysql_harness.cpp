// Reuse the maintained native SQL fixture helpers and pooled coordinator owner.
// Its original main is linked but never executed by this focused payload case.
#define main maintained_item_transfer_fixture_main
#include "item_transfer_mysql_harness.cpp"
#undef main
#include "persistence/sql_room_item_payload.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "account/account_load.h"

static uint64_t room_scalar(MYSQL *connection, const std::string &sql)
{
	return scalar(connection, sql.c_str());
}

static std::vector<uint8_t> encode_exact_graph(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode(items, &encoded) ==
	       player_snapshot_codec_result::ok);
	return encoded;
}

static void check_cold_payload(MYSQL *connection, const std::vector<player_item_snapshot> &expected)
{
	execute(connection, "START TRANSACTION");
	std::vector<uint64_t> roots;
	assert(sql_room_item_payload_roots(connection, &roots));
	assert(std::find(roots.begin(), roots.end(), expected[0].object_uid) != roots.end());
	sql_room_item_graph graph;
	const bool read = sql_room_item_payload_read(connection, expected[0].object_uid, &graph);
	if (!read)
		fprintf(stderr, "exact payload read refused errno=%d sql=%s\n", errno,
			mysql_error(connection));
	assert(read);
	assert(graph.items.size() == expected.size() && graph.identities.size() == expected.size());
	for (size_t index = 0; index < expected.size(); ++index)
	{
		graph.items[index].equipment_slot = 0;
		const auto &identity = graph.identities[index];
		assert(identity.item_uid == expected[index].object_uid &&
		       identity.item_revision == 2);
		assert(identity.root_item_uid == expected[0].object_uid &&
		       identity.owner.type == item_owner_type::room);
		assert(identity.owner.id == 22800 && identity.owner.context_id == 0 &&
		       identity.quantity == 1);
	}
	assert(encode_exact_graph(graph.items) == encode_exact_graph(expected));
	execute(connection, "ROLLBACK");
}

int main()
{
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	std::vector<player_item_snapshot> seed_expected;
	MYSQL *connection = open_pool_test_connection();
	assert(connection);
#ifdef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
	{
		economic_sql_real_pool_lifecycle pool_lifecycle;
#endif
		assert(critical_operation_id_generate(&run_operation));
		ensure_collector_boundary_fixture(connection);
		execute(connection,
			"INSERT INTO player_data(pid,name,account_name) VALUES(60491,'ExactPayloadFixture','ExactPayloadFixture')");
		// Install the real zero-revision login baselines before retaining history.
		// The complete native boot keeps its baseline readiness guards unchanged.
		assert(account_load_repair(connection, "ExactPayloadFixture") >= 0);
		for (const char *table : { "currency_wallet_baseline", "epic_balance_baseline",
					   "combat_frag_baseline" })
			assert(room_scalar(connection, std::string("SELECT COUNT(*) FROM ") +
							       table + " WHERE pid=60491") == 1);
		const auto creator = operation(1), lineage = operation(2), epoch = operation(3);
		execute(connection,
			"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
				operation_hex(creator) +
				"'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')");
		execute(connection,
			"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,creating_operation_id) VALUES(UNHEX('" +
				operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
				"'),1,NULL,1,REPEAT(CHAR(0),32),UNHEX('" + operation_hex(creator) +
				"'))");
		execute(connection,
			"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(UNHEX('" +
				operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
				"'),0)");
		item_uid_allocator_reset_for_tests();
		assert(item_uid_allocator_reserve(connection, 3));
		root_uid = item_uid_allocator_next();
		child_uid = item_uid_allocator_next();
		const uint64_t grandchild = item_uid_allocator_next();
		const item_owner_identity system = { item_owner_type::system, 0, 0 };
		const item_owner_identity player = { item_owner_type::player, 60491, 0 };
		const item_owner_identity room = { item_owner_type::room, 22800, 0 };
		std::vector<player_item_snapshot> exact;
		for (size_t index = 0; index < 3; ++index)
		{
			auto next = runtime_item(index == 0 ? root_uid :
						 index == 1 ? child_uid :
							      grandchild,
						 INT64_C(9007199254740993) + index, 60 + index);
			// This route is ordinary inventory only; artifact custody is excluded.
			next.extra_flags &= ~ITEM_ARTIFACT;
			// Match actual set_obj_affected_extra state: the retained baseline
			// precedes the effect, and live flags include that effect's bits.
			// An incoherent synthetic flag/list pair correctly refuses restore.
			next.dynamic_affects.insert(next.dynamic_affects.begin(),
						    { TAG_ALTERED_EXTRA2, 0, next.extra2_flags });
			next.extra2_flags |= next.dynamic_affects[1].extra2;
			next.vnum = index < 2 ? 48 : 5;
			next.type = index < 2 ? 15 : 1;
			next.equipment_slot = 0;
			next.parent_index = index ? index - 1 : -1;
			exact.push_back(std::move(next));
		}
		auto creation = payload(system, player, item_transfer_reason::creation,
					owner_revision(connection, system),
					owner_revision(connection, player),
					ITEM_TRANSFER_ABSENT_REVISION);
		creation.item_count = 3;
		creation.items[2] = { grandchild, root_uid,
				      child_uid,  ITEM_TRANSFER_ABSENT_REVISION,
				      5,	  item_custody_state::absent };
		for (size_t index = 0; index < creation.item_count; ++index)
			creation.items[index].vnum = exact[index].vnum;
		attach_blob(&creation, exact);
		const auto created = critical_command_repository_apply(
			connection,
			accounted_item_transfer(operation(4), creation, lineage, epoch, 60491,
						economic_source_kind::starter_grant));
		assert(created.outcome == critical_apply_outcome::applied && !created.error_code);
		// Use the ordinary native save writer, then read its actual physical state.
		// The drop source is these existing native rows and original UIDs, never a
		// sidecar/legacy projection or a prototype used to fill absent fields.
		player_snapshot checkpoint{};
		checkpoint.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		checkpoint.pid = 60491;
		checkpoint.revision = 1;
		checkpoint.components = PLAYER_COMPONENT_INVENTORY | PLAYER_COMPONENT_EQUIPMENT;
		checkpoint.items = exact;
		assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
		       player_save_apply_outcome::applied);
		player_load_request request{};
		request.request_id = 60491;
		request.pid = 60491;
		request.account_name = "ExactPayloadFixture";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		auto source = player_load_repository_execute(connection, request);
		assert(source.outcome == player_load_outcome::applied &&
		       source.snapshot.items.size() == exact.size());
		for (size_t index = 0; index < exact.size(); ++index)
		{
			// SQL transports spellbook metadata as JSON text. Verify those physical
			// bytes before converting to the typed representation used by runtime
			// capture; no other captured field may be filled from a prototype.
			auto &item = source.snapshot.items[index];
			assert(item.extra_descriptions.size() == 1);
			auto &book = item.extra_descriptions[0];
			assert(book.keyword == "SPELLBOOK" && book.spellbook &&
			       book.description == "[4,9,15]" && book.spell_ids.empty());
			book.description.clear();
			book.spell_ids = { 4, 9, 15 };
		}
		// Encode the complete graph: a singleton cannot retain a child's parent index.
		assert(encode_exact_graph(source.snapshot.items) == encode_exact_graph(exact));

		auto drop = creation;
		drop.from_owner = player;
		drop.to_owner = room;
		drop.reason = item_transfer_reason::player_drop;
		drop.reason_id = room.id;
		drop.selected_item_uid = root_uid;
		drop.target_root_item_uid = root_uid;
		drop.expected_from_revision = owner_revision(connection, player);
		drop.expected_to_revision = owner_revision(connection, room);
		for (size_t index = 0; index < drop.item_count; ++index)
		{
			drop.items[index].expected_item_revision = 1;
			drop.items[index].expected_state = item_custody_state::active;
		}
		attach_blob(&drop, source.snapshot.items);
		execute(connection, "START TRANSACTION");
		sql_room_item_payload_batch prepared;
		const bool prepared_ok = sql_room_item_payload_prepare(connection, drop, &prepared);
		if (!prepared_ok)
			fprintf(stderr, "room prepare refused errno=%d sql=%s\n", errno,
				mysql_error(connection));
		assert(prepared_ok);
		execute(connection, "ROLLBACK");
		auto altered = drop;
		altered.reason_id = 0;
		execute(connection, "START TRANSACTION");
		assert(!sql_room_item_payload_prepare(connection, altered, &prepared));
		execute(connection, "ROLLBACK");
		// A missing physical properties projection cannot prove that the native
		// literal checkpoint wrote its extra2/dynamic-effect payload.
		execute(connection, "START TRANSACTION");
		execute(connection, "UPDATE player_items SET item_properties=NULL WHERE obj_uid=" +
					    std::to_string(root_uid));
		assert(!sql_room_item_payload_prepare(connection, drop, &prepared) &&
		       errno == ESTALE);
		execute(connection, "ROLLBACK");
		// Fail after earlier payload rows, then after physical deletion begins. Each
		// failure must roll back custody, immutable rows, source projection and root
		// evidence together before a successful lost-reply run.
		const std::vector<std::pair<std::string, std::string>> faults = {
			{ "BEFORE INSERT ON sql_room_item_payload",
			  "NEW.item_uid=" + std::to_string(grandchild) },
			{ "AFTER DELETE ON player_items",
			  "OLD.obj_uid=" + std::to_string(root_uid) }
		};
		for (size_t index = 0; index < faults.size(); ++index)
		{
			const auto failed = accounted_item_transfer(operation(6 + index), drop,
								    lineage, epoch, 60491);
			execute(connection,
				"CREATE TRIGGER sql_room_item_fault " + faults[index].first +
					" FOR EACH ROW BEGIN IF " + faults[index].second +
					" THEN SIGNAL SQLSTATE '45000' SET MYSQL_ERRNO=1644,MESSAGE_TEXT='room payload rollback fixture'; END IF; END");
			const auto refused = critical_command_repository_apply(connection, failed);
			assert(refused.outcome != critical_apply_outcome::applied &&
			       refused.outcome != critical_apply_outcome::already_applied);
			execute(connection, "DROP TRIGGER sql_room_item_fault");
			assert(owner_revision(connection, player) == drop.expected_from_revision &&
			       owner_revision(connection, room) == drop.expected_to_revision);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id=60491 AND item_revision=1 AND state=1") ==
			       3);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM player_items WHERE pid=60491") == 3);
			assert(scalar(connection, "SELECT COUNT(*) FROM sql_room_item_payload") ==
			       0);
			for (const char *table :
			     { "item_ownership_ledger", "critical_operation_inbox",
			       "critical_outbox", "economic_accounting_operation" })
				assert(room_scalar(connection,
						   std::string("SELECT COUNT(*) FROM ") + table +
							   " WHERE operation_id=UNHEX('" +
							   operation_hex(failed.operation_id) +
							   "')") == 0);
		}
		const auto command =
			accounted_item_transfer(operation(5), drop, lineage, epoch, 60491);
		economic_sql_commit_reply_loss_fixture::arm();
		const auto applied =
			exercise_sql_coordinator(command, "exact-room-payload", false,
						 critical_apply_outcome::already_applied);
		economic_sql_commit_reply_loss_fixture::verify();
		assert(applied.outcome == critical_apply_outcome::already_applied);
		assert(scalar(connection, "SELECT COUNT(*) FROM sql_room_item_payload") == 3);
		assert(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE pid=60491") ==
		       0);
		for (const char *table : { "player_item_runtime_state", "player_item_affects",
					   "player_item_extra_descr" })
			assert(room_scalar(connection,
					   std::string("SELECT COUNT(*) FROM ") + table) == 0);
		// The coordinator fixture ACKs and restarts its journal before returning.
		check_cold_payload(connection, exact);
		mysql_close(connection);
		connection = open_pool_test_connection();
		assert(connection);
		check_cold_payload(connection, exact);
		const auto refusal = [&]
		{
			execute(connection, "START TRANSACTION");
			sql_room_item_graph sentinel;
			sentinel.owner_revision = 999;
			assert(!sql_room_item_payload_read(connection, root_uid, &sentinel));
			assert(sentinel.owner_revision == 999 && sentinel.items.empty());
			execute(connection, "ROLLBACK");
		};
		execute(connection, "START TRANSACTION");
		execute(connection,
			"UPDATE item_current_owner SET item_revision=item_revision+1 WHERE item_uid=" +
				std::to_string(root_uid));
		std::vector<uint64_t> stale_roots;
		assert(sql_room_item_payload_roots(connection, &stale_roots) &&
		       std::find(stale_roots.begin(), stale_roots.end(), root_uid) !=
			       stale_roots.end());
		sql_room_item_graph graph;
		assert(!sql_room_item_payload_read(connection, root_uid, &graph));
		execute(connection, "ROLLBACK");
		const auto retained = read_blob(
			connection, ("SELECT payload FROM sql_room_item_payload WHERE item_uid=" +
				     std::to_string(grandchild))
					    .c_str());
		execute(connection,
			"UPDATE sql_room_item_payload SET payload=X'00000000' WHERE item_uid=" +
				std::to_string(grandchild));
		refusal();
		execute(connection, "UPDATE sql_room_item_payload SET payload=UNHEX('" +
					    bytes_hex(retained) +
					    "') WHERE item_uid=" + std::to_string(grandchild));
		execute(connection,
			"UPDATE critical_operation_inbox SET failure_stage=1 WHERE operation_id=UNHEX('" +
				operation_hex(command.operation_id) + "')");
		refusal();
		execute(connection,
			"UPDATE critical_operation_inbox SET failure_stage=0 WHERE operation_id=UNHEX('" +
				operation_hex(command.operation_id) + "')");
		execute(connection, "START TRANSACTION");
		execute(connection, "DELETE FROM sql_room_item_payload");
		assert(sql_room_item_payload_roots(connection, &stale_roots) &&
		       std::find(stale_roots.begin(), stale_roots.end(), root_uid) !=
			       stale_roots.end());
		bool enrolled = false;
		assert(sql_room_item_payload_present(connection, grandchild, &enrolled) &&
		       enrolled);
		assert(!sql_room_item_payload_read(connection, root_uid, &graph));
		execute(connection, "ROLLBACK");
		execute(connection, "START TRANSACTION");
		execute(connection,
			"UPDATE season_reset_state SET season_epoch=season_epoch+1 WHERE state_id=1");
		assert(sql_room_item_payload_roots(connection, &stale_roots) &&
		       std::find(stale_roots.begin(), stale_roots.end(), root_uid) ==
			       stale_roots.end());
		assert(!sql_room_item_payload_read(connection, root_uid, &graph));
		execute(connection, "ROLLBACK");
		check_cold_payload(connection, exact);
		seed_expected = exact;
		mysql_close(connection);
#ifdef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
	}
#endif
	// Export only after durable ACK and the production pool's clean shutdown.
	// This isolated fixture retains historical lineage and payload evidence;
	// complete active-epoch lifecycle installation is a separate qualification.
	if (const char *export_seed = std::getenv("DURIS_SQL_ROOM_ITEM_SEED_EXPORT");
	    export_seed && std::strcmp(export_seed, "1") == 0)
	{
		connection = open_pool_test_connection();
		assert(connection);
		execute(connection, "UPDATE economic_lineage_state SET active_epoch=NULL");
		check_cold_payload(connection, seed_expected);
		mysql_close(connection);
		const auto encoded = encode_exact_graph(seed_expected);
		std::printf("ROOM_ITEM_PAYLOAD_SEED uid=%llu payload=%s\n",
			    static_cast<unsigned long long>(seed_expected[0].object_uid),
			    bytes_hex(encoded).c_str());
	}
	std::puts(
		"PASS: exact native source, atomic schema2 drop and two rollback faults, ACK, two cold SQL connections, corrupt/stale/missing provenance and season refusal; no gameplay publication claim");
}
