#!/usr/bin/env python3
"""Bounded asynchronous collector listing detail pipeline regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def test_flatfile_adapter(directory: Path) -> None:
    # Compile the real selected-backend adapter; stub only configuration and
    # repository I/O. No database, configured persistence root, or live service.
    source = directory / "flatfile-adapter.cpp"
    binary = directory / "flatfile-adapter"
    source.write_text(r'''#include "economy/collector_catalog_source.h"
#include "flatfile/flatfile_collector_repository.h"
#include <cassert>
#include <cerrno>
#include <string>

static const char *configured_root = nullptr;
static unsigned reads = 0;
static bool supplied_found = true;
static std::string supplied_error;
static auto supplied_result = flatfile_collector_repository_result::ok;
const char *persistence_mode_flatfile_root() { return configured_root; }
flatfile_collector_repository_result flatfile_collector_repository_read_bootstrap(
    const std::string &, collector_bootstrap_snapshot *, std::string *)
{
    assert(false && "listing read must not bootstrap the catalog");
    return flatfile_collector_repository_result::invalid;
}
flatfile_collector_repository_result flatfile_collector_repository_read_listing(
    const std::string &root, uint64_t listing, collector_listing_detail *, bool *found,
    std::string *error)
{
    assert(root == "fixture-authority-root" && listing == 42);
    ++reads;
    *found = supplied_found;
    *error = supplied_error;
    return supplied_result;
}
int main()
{
    collector_listing_detail detail;
    bool found = false;
    unsigned int code = 0;
    std::string error;
    assert(!collector_listing_source_load(0, detail, found, code, error));
    assert(code == EINVAL && reads == 0);
    for (const char *root : {static_cast<const char *>(nullptr), ""}) {
        configured_root = root;
        assert(!collector_listing_source_load(42, detail, found, code, error));
        assert(code == ENOENT && !error.empty() && reads == 0);
    }
    configured_root = "fixture-authority-root";
    supplied_error = "stale error";
    for (bool present : {true, false}) {
        supplied_found = present;
        assert(collector_listing_source_load(42, detail, found, code, error));
        assert(found == present && code == 0 && error.empty());
    }
    assert(reads == 2);
    supplied_result = flatfile_collector_repository_result::io_error;
    supplied_error = "fixture read failure";
    assert(!collector_listing_source_load(42, detail, found, code, error));
    assert(code == EIO && error == supplied_error && reads == 3);
    supplied_result = flatfile_collector_repository_result::invalid;
    supplied_error.clear();
    assert(!collector_listing_source_load(42, detail, found, code, error));
    assert(code == EILSEQ && error == "flat-file collector listing read failed" && reads == 4);
}
''')
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-I", str(ROOT / "src"), "-I", str(ROOT / "src/no_mysql"),
        str(ROOT / "src/economy/collector_catalog_source.c"), str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True, timeout=10)
    print("collector flat-file listing adapter: configured root, missing root, found/not-found and errors PASS")


def main() -> None:
    pipeline_source = (ROOT / "src/economy/collector_listing_pipeline.c").read_text()
    selected_source = (ROOT / "src/economy/collector_catalog_source.c").read_text()
    comm_source = (ROOT / "src/net/comm.c").read_text()
    assert "std::condition_variable" in pipeline_source
    assert "COLLECTOR_LISTING_MAX_PENDING" in pipeline_source
    assert "collector_listing_source_load" in pipeline_source
    assert "collector_repository_read_listing" in selected_source
    assert "sql_pool_acquire" in selected_source
    flatfile_listing = selected_source.split("bool collector_listing_source_load", 1)[1]
    flatfile_listing = flatfile_listing.split("#ifdef __NO_MYSQL__", 1)[1].split("#else", 1)[0]
    assert "persistence_mode_flatfile_root()" in flatfile_listing
    assert "flatfile_collector_repository_read_listing(" in flatfile_listing
    assert "collector_listing_pipeline_init()" in comm_source
    assert "collector_listing_pipeline_shutdown()" in comm_source
    assert "P_char" not in (ROOT / "src/economy/collector_listing_pipeline.h").read_text()
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-listing-pipeline-", dir=output) as directory:
        binary = Path(directory) / "collector-listing-pipeline"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O2",
                "-I",
                str(ROOT / "src"),
                str(ROOT / "src/economy/collector_policy.c"),
                str(ROOT / "src/economy/collector_listing_pipeline.c"),
                str(ROOT / "tests/async/collector_listing_pipeline_harness.cpp"),
                "-pthread",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)
        test_flatfile_adapter(Path(directory))


if __name__ == "__main__":
    main()
