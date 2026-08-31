"""Runtime regression for logit() recreating missing parent directories."""

from _paths import SRC
import shlex
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory() as tmp:
    tmp_path = Path(tmp)
    utility_object = tmp_path / "utility.o"
    harness_source = tmp_path / "log_directory_harness.cpp"
    harness_binary = tmp_path / "log_directory_harness"
    target = tmp_path / "missing" / "nested" / "status"
    mysql_includes = shlex.split(
        subprocess.check_output(["mariadb_config", "--include"], text=True)
    )

    harness_source.write_text(
        """
#include <fstream>
#include <string>

void logit(const char *filename, const char *format, ...);

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    logit(argv[1], "missing-directory retry marker %d", 42);
    std::ifstream input(argv[1]);
    std::string contents((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
    return contents.find("missing-directory retry marker 42") == std::string::npos;
}
"""
    )

    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-ffunction-sections",
            "-fdata-sections",
            "-I",
            str(SRC),
            *mysql_includes,
            "-c",
            str(SRC / "utility.c"),
            "-o",
            str(utility_object),
        ],
        check=True,
    )
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wl,--gc-sections",
            str(harness_source),
            str(utility_object),
            "-o",
            str(harness_binary),
        ],
        check=True,
    )
    subprocess.run([str(harness_binary), str(target)], check=True)
    assert target.is_file()
