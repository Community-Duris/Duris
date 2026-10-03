from _paths import ROOT, SRC, extract_function
from pathlib import Path
import subprocess
import tempfile

comm = (SRC / "comm.c").read_text(encoding="utf-8", errors="replace")
mccp = (SRC / "mccp.c").read_text(encoding="utf-8", errors="replace")
nanny = (SRC / "nanny.c").read_text(encoding="utf-8", errors="replace")

# Fragmented Telnet commands must remain buffered until the next socket read.
assert "memmove(bp, buf + i, len - i);" in comm
assert "bp += len - i;" in comm

# The parser must not inspect the command or option byte until present.
assert "if (buflen < 2)" in mccp
assert mccp.count("if (buflen < 3)") >= 5

# Re-enabling client echo must be one complete Telnet negotiation command.
echo_on = nanny[nanny.index("void echo_on(P_desc d)") : nanny.index("void echo_off(P_desc d)")]
assert "{ IAC, WONT, TELOPT_ECHO }" in echo_on
assert "write_to_descriptor_binary(d, on_string, sizeof(on_string));" in echo_on
assert "TELOPT_NAOFFD" not in echo_on
assert "TELOPT_NAOCRD" not in echo_on

# Compile the production Telnet parser and input-framing functions themselves.
template_path = ROOT / "tests/async/telnet_input_runtime_harness.cpp"
generated = template_path.read_text(encoding="utf-8")
generated = generated.replace(
    "/* PARSE_TELNET_OPTIONS_IMPLEMENTATION */",
    extract_function("mccp.c", "int parse_telnet_options("),
)
generated = generated.replace(
    "/* PROCESS_INPUT_IMPLEMENTATION */",
    extract_function("comm.c", "int process_input(P_desc t)"),
)
assert "/* PARSE_TELNET_OPTIONS_IMPLEMENTATION */" not in generated
assert "/* PROCESS_INPUT_IMPLEMENTATION */" not in generated

build_dir = ROOT / "bin/tests"
build_dir.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="telnet-input-", dir=build_dir) as directory:
    directory_path = Path(directory)
    source_path = directory_path / "telnet_input_runtime.cpp"
    binary_path = directory_path / "telnet_input_runtime"
    source_path.write_text(generated, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Wlogical-op",
            "-D__NO_MYSQL__",
            f"-I{ROOT / 'src'}",
            f"-I{ROOT / 'src/no_mysql'}",
            str(source_path),
            "-o",
            str(binary_path),
        ],
        check=True,
        cwd=ROOT,
        timeout=120,
    )
    subprocess.run([str(binary_path)], check=True, cwd=ROOT, timeout=30)
