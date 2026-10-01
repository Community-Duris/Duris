#!/usr/bin/env python3
"""The login screen names each enabled server mode on one blinking line."""

import os
import subprocess
import tempfile
from pathlib import Path

from _paths import SRC

ROOT = Path(__file__).resolve().parents[2]
account = (SRC / "account.c").read_text()
nanny = (SRC / "nanny.c").read_text()
makefile = (SRC / "Makefile").read_text()
example = (ROOT / ".env.example").read_text()

# Both account-name prompts go through the helper that prints the banner.
prompt = 'SEND_TO_Q("Please enter your account name: ", d);'
assert account.count(prompt) == 1 and nanny.count(prompt) == 0
helper_start = account.index("void send_account_name_prompt(P_desc d)")
helper = account[helper_start : account.index("\n}\n", helper_start)]
assert prompt in helper
assert helper.index("login_mode_banner(") < helper.index(prompt)
for accessor in (
    "duris_staging_enabled()",
    "chaos_mud_enabled()",
    "creation_all_races_enabled()",
    "creation_all_classes_enabled()",
):
    assert accessor in helper
assert account.count("send_account_name_prompt(d);") == 1
assert nanny.count("send_account_name_prompt(d);") == 1
assert "account/login_mode_banner.o" in makefile
assert "DURIS_STAGING=FALSE" in example
print("[PASS] both account-name prompts print the mode banner first")

HARNESS = r"""
#include "account/login_mode_banner.h"
#include "net/ansi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* unicode.c's table setter references the server's corruption panic. */
void panic_corruption(const char *, const char *, ...)
{
	abort();
}

static void escaped(const char *text)
{
	for (const char *c = text; *c; ++c)
		if (*c == '\x1b')
			printf("\\e");
		else if (*c == '\r')
			printf("\\r");
		else if (*c == '\n')
			printf("\\n");
		else
			putchar(*c);
}

static void show(const char *name, bool staging, bool chaos, bool races, bool classes)
{
	const std::string banner = login_mode_banner(staging, chaos, races, classes);
	static char rendered[MAX_STRING_LENGTH];
	AnsiString(banner.c_str()).term(rendered, TL_BLINK);
	printf("%s\t", name);
	escaped(banner.c_str());
	putchar('\t');
	escaped(rendered);
	putchar('\n');
}

int main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "staging"))
	{
		printf("%d\n", duris_staging_enabled() ? 1 : 0);
		return 0;
	}
	show("none", false, false, false, false);
	show("staging", true, false, false, false);
	show("chaos", false, true, false, false);
	show("all", true, true, true, true);
	return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="duris-login-mode-banner-") as temporary:
    harness = Path(temporary) / "harness.cpp"
    harness.write_text(HARNESS)
    binary = Path(temporary) / "login_mode_banner"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc",
            str(harness),
            "src/account/login_mode_banner.c",
            "src/net/ansi.c",
            "src/net/unicode.c",
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    lines = subprocess.run(
        [str(binary)], check=True, capture_output=True, text=True
    ).stdout.splitlines()
    cases = {name: (raw, rendered) for name, raw, rendered in (line.split("\t", 2) for line in lines)}

    assert cases["none"] == ("", ""), cases["none"]
    assert cases["staging"][0] == "&=LM*** &=LYSTAGING&=LM ***&n\\r\\n"
    assert cases["chaos"][0] == "&=LM*** &=LRCHAOS&=LM ***&n\\r\\n"
    assert cases["all"][0] == (
        "&=LM*** &=LYSTAGING&=LW | &=LRCHAOS&=LW | &=LGALL-RACES"
        "&=LW | &=LCALL-CLASSES&=LM ***&n\\r\\n"
    )
    rendered = cases["all"][1]
    # Every colored run blinks (5) on black (40) with a bright foreground (1;3x).
    for foreground in ("35", "33", "37", "31", "32", "36"):
        assert f"\\e[0;1;{foreground};5;40m" in rendered, (foreground, rendered)
    assert rendered.endswith("\\e[m\\r\\n"), rendered
    print("[PASS] enabled modes render in order as one blinking all-caps line")

    for value, expected in (("TRUE", "1"), ("true", "1"), ("FALSE", "0"), ("", "0"), (None, "0")):
        environment = {key: item for key, item in os.environ.items() if key != "DURIS_STAGING"}
        if value is not None:
            environment["DURIS_STAGING"] = value
        result = subprocess.run(
            [str(binary), "staging"], check=True, capture_output=True, text=True, env=environment
        ).stdout.strip()
        assert result == expected, (value, result)
    print("[PASS] DURIS_STAGING is enabled only by TRUE")

print("login mode banner regression passed")
