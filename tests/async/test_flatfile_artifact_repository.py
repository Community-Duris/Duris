#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-artifact-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_artifact_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_artifact_repository_harness.cpp",
            rel("flatfile_artifact_repository.c"),
            rel("player_snapshot_codec.c"),
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
            rel("flatfile_authority_transaction.c"),
            rel("flatfile_store.c"),
            "-lcrypto",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    run_result = subprocess.run(
        [str(binary), str(temporary_path / "state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())

    runtime_binary = temporary_path / "flatfile_artifact_bind_runtime_test"
    runtime_compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_artifact_bind_runtime_harness.cpp",
            rel("sql.c"),
            rel("flatfile_artifact_repository.c"),
            rel("player_snapshot_codec.c"),
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
            rel("flatfile_authority_transaction.c"),
            rel("flatfile_store.c"),
            "-Wl,--gc-sections",
            "-lcrypto",
            "-pthread",
            "-o",
            str(runtime_binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if runtime_compile_result.returncode:
        raise SystemExit(runtime_compile_result.stdout)
    runtime_result = subprocess.run(
        [str(runtime_binary), str(temporary_path / "runtime-state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if runtime_result.returncode:
        raise SystemExit(runtime_result.stdout)
    print(runtime_result.stdout.strip())

    gameplay_binary = temporary_path / "flatfile_artifact_gameplay_runtime_test"
    gameplay_compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_artifact_runtime_harness.cpp",
            rel("artifact.c"),
            rel("flatfile_artifact_repository.c"),
            rel("player_snapshot_codec.c"),
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
            rel("flatfile_authority_transaction.c"),
            rel("flatfile_store.c"),
            "-Wl,--gc-sections",
            "-lcrypto",
            "-pthread",
            "-o",
            str(gameplay_binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if gameplay_compile_result.returncode:
        raise SystemExit(gameplay_compile_result.stdout)
    gameplay_result = subprocess.run(
        [str(gameplay_binary), str(temporary_path / "gameplay-state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if gameplay_result.returncode:
        raise SystemExit(gameplay_result.stdout)
    print(gameplay_result.stdout.strip())

    artifact_source = (SRC / "artifact.c").read_text()
    expiry_start = artifact_source.index("void event_artifact_check_poof_sql(")
    expiry_end = artifact_source.index("\nvoid event_artifact_wars_sql(", expiry_start)
    expiry_body = artifact_source[expiry_start:expiry_end]
    if "flatfile_artifact_find_next_expired(" not in expiry_body:
        raise AssertionError("client-free expiry event does not select from flat authority")
    if "flatfile_artifact_expire(" not in expiry_body:
        raise AssertionError("client-free expiry event does not clear flat authority")

    wars_start = artifact_source.index("void event_artifact_wars_sql(")
    wars_end = artifact_source.index("\nvoid event_arti_hunt_sql(", wars_start)
    wars_body = artifact_source[wars_start:wars_end]
    if "flatfile_artifact_war_owners(" not in wars_body:
        raise AssertionError("client-free artifact-war event does not group flat authority")
    if "flatfile_artifact_apply_war_burn(" not in wars_body:
        raise AssertionError("client-free artifact-war event does not update flat timers")

    binding_start = artifact_source.index("void event_artifact_check_bind_sql(")
    binding_end = artifact_source.index("\nvoid arti_fixit_sql(", binding_start)
    binding_body = artifact_source[binding_start:binding_end]
    if "flatfile_artifact_list(" not in binding_body:
        raise AssertionError("client-free binding maintenance does not page flat authority")
    if binding_body.count("artifact_bind_maintenance_update(") != 3:
        raise AssertionError("binding maintenance does not persist every binding outcome")

    remove_start = artifact_source.index("bool remove_owned_artifact_sql(")
    remove_end = artifact_source.index("\nvoid remove_all_artifacts_sql(", remove_start)
    remove_body = artifact_source[remove_start:remove_end]
    if "flatfile_artifact_remove_owned(" not in remove_body:
        raise AssertionError("client-free owned artifact removal bypasses flat authority")

    remove_all_start = artifact_source.index("void remove_all_artifacts_sql(P_char ch)\n{")
    remove_all_end = artifact_source.index("\n// This is a wrapper function", remove_all_start)
    if "flatfile_artifact_release_player(" not in artifact_source[remove_all_start:remove_all_end]:
        raise AssertionError("client-free bulk artifact removal bypasses flat authority")

    list_signature = "void list_artifacts_sql(P_char ch, int type, bool Godlist, bool allArtis)\n{"
    list_start = artifact_source.index(list_signature)
    list_end = artifact_source.index("\nvoid arti_remove_sql(", list_start)
    list_body = artifact_source[list_start:list_end]
    if "flatfile_artifact_list(" not in list_body:
        raise AssertionError("client-free artifact list bypasses flat authority")
    if "requires a completed durable backend" in list_body:
        raise AssertionError("client-free artifact list remains disabled")

    player_start = artifact_source.index("void arti_player_sql(P_char ch, char *arg)\n{")
    player_body = artifact_source[player_start:]
    if "flatfile_artifact_list(" not in player_body:
        raise AssertionError("client-free per-player artifact list bypasses flat authority")
    if "requires MySQL support" in player_body:
        raise AssertionError("client-free per-player artifact list remains disabled")

    reset_start = artifact_source.index("void arti_reset_sql(P_char ch, char *arg)\n{")
    reset_end = artifact_source.index("\nP_char find_mob_in_game(", reset_start)
    reset_body = artifact_source[reset_start:reset_end]
    if "flatfile_artifact_bind_update(" not in reset_body:
        raise AssertionError("client-free keyed soul reset bypasses flat authority")
    if "flatfile_artifact_bind_reset_all(" not in reset_body:
        raise AssertionError("client-free reset-all bypasses flat authority")

    feed_min_start = artifact_source.index("void artifact_feed_to_min_sql(")
    feed_min_end = artifact_source.index("\nvoid artifact_switch_check(", feed_min_start)
    feed_min_body = artifact_source[feed_min_start:feed_min_end]
    if "flatfile_artifact_extend_timer(" not in feed_min_body:
        raise AssertionError("client-free minimum feed bypasses flat timer authority")

    feed_start = artifact_source.index("void artifact_feed_sql(")
    feed_end = artifact_source.index("\nP_char load_dummy_char(", feed_start)
    feed_body = artifact_source[feed_start:feed_end]
    if "flatfile_artifact_gameplay_update(" not in feed_body:
        raise AssertionError("client-free missing-row feed bypasses flat authority")

    fix_start = artifact_source.index("void arti_fixit_sql(P_char ch)\n{")
    fix_end = artifact_source.index("\nvoid arti_sync_sql(", fix_start)
    fix_body = artifact_source[fix_start:fix_end]
    if "flatfile_artifact_list(" not in fix_body:
        raise AssertionError("client-free artifact fixit bypasses flat selection")
    if "flatfile_artifact_repair_player_binding(" not in fix_body:
        raise AssertionError("client-free artifact fixit bypasses atomic flat repair")

    syncdb_start = artifact_source.index("void arti_syncdb_sql(P_char ch)\n{")
    syncdb_end = artifact_source.index("\n// Resets the 'soul'", syncdb_start)
    syncdb_body = artifact_source[syncdb_start:syncdb_end]
    if "flatfile_item_repository_list_active_player_items(" not in syncdb_body:
        raise AssertionError("client-free artifact syncdb bypasses saved item authority")
    if "flatfile_artifact_reconcile_players(" not in syncdb_body:
        raise AssertionError("client-free artifact syncdb bypasses artifact reconciliation")

    remove_entry_start = artifact_source.index("void arti_remove_sql(int vnum, bool mortalToo)\n{")
    remove_entry_end = artifact_source.index("\nvoid setupMortArtiList_sql(", remove_entry_start)
    if "flatfile_artifact_erase(" not in artifact_source[remove_entry_start:remove_entry_end]:
        raise AssertionError("client-free artifact removal bypasses catalog erase")

    clear_start = artifact_source.index("void arti_clear_sql(P_char ch, char *arg)\n{")
    clear_end = artifact_source.index("\nvoid arti_poof_sql(", clear_start)
    if "flatfile_artifact_erase(" not in artifact_source[clear_start:clear_end]:
        raise AssertionError("client-free staff artifact clear bypasses catalog erase")

    boot_source = (SRC / "db.c").read_text()
    boot_start = boot_source.index("void boot_db(int mini_mode)\n{")
    boot_end = boot_source.index("\nvoid update_stat_data()", boot_start)
    boot_body = boot_source[boot_start:boot_end]
    if "flatfile_artifact_ensure(" not in boot_body:
        raise AssertionError("flat boot does not establish artifact authority")
    if boot_body.index("flatfile_artifact_ensure(") > boot_body.index("ne_init_events();"):
        raise AssertionError("artifact authority is established after boot zone resets")
