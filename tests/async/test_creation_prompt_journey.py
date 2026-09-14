#!/usr/bin/env python3
"""Real Telnet account/character creation, correction, confirmation and relog.

Each scenario boots an isolated flat-file server with synthetic accounts. No
production account, database, or listener is used. The existing recovery fixture
provides the real socket client and server lifecycle, not mocked nanny handlers.
"""

from pathlib import Path
import subprocess
import tempfile

from test_account_recovery_journey import (
    ACCOUNT, OLD_PASSWORD, EMAIL, IsolatedServer, MudClient,
    build_flatfile_server, create_account, enter_account_name, require,
)

PASSWORD_PROMPT = "Please enter your password:  "
CONFIRM_PASSWORD = "Please re-enter the same password to confirm:  "
MENU = "Please select an option: "
NAME_PROMPT = "Enter your new name:  "
RACE_MENU = "Your selection: "
KEEP = "Do you want to keep this character? (Y/N/Q)"


def reply(client, answer, prompt):
    client.send(answer)
    return client.expect(prompt, timeout=5)


def enter_email(client):
    enter_account_name(client)
    client.expect("is this correct?")
    reply(client, "y", "email address:  ")


def account_name_retry(client, server):
    enter_account_name(client)
    client.expect("is this correct?")
    reply(client, "?", "is this correct?")
    reply(client, "n", "Account Name: ")
    reply(client, "123", "Account Name: ")
    reply(client, ACCOUNT, "is this correct?")
    reply(client, "y", "email address:  ")


def email_retry(client, server):
    enter_email(client)
    reply(client, "bad-email", "Email address:  ")
    reply(client, "first@example.invalid", "is this correct?")
    reply(client, "?", "is this correct?")
    reply(client, "n", "Please enter your email address:  ")
    reply(client, EMAIL, "is this correct?")
    reply(client, "y", PASSWORD_PROMPT)


def password_and_summary_retry(client, server):
    enter_email(client)
    reply(client, EMAIL, "is this correct?")
    reply(client, "y", PASSWORD_PROMPT)
    for invalid in ("", "    ", "abc", "alllowercase"):
        reply(client, invalid, PASSWORD_PROMPT)
    reply(client, OLD_PASSWORD, CONFIRM_PASSWORD)
    reply(client, "Different9!", "Passwords do not match!")
    client.expect(PASSWORD_PROMPT)
    reply(client, OLD_PASSWORD, CONFIRM_PASSWORD)
    reply(client, OLD_PASSWORD, "Is this information correct?")
    reply(client, "?", "Is this information correct?")
    reply(client, "n", "Please enter your account name: ")
    # Starting over must not have saved the abandoned registration.
    reply(client, ACCOUNT, "is this correct?")
    reply(client, "y", "email address:  ")
    reply(client, EMAIL, "is this correct?")
    reply(client, "y", PASSWORD_PROMPT)
    reply(client, OLD_PASSWORD, CONFIRM_PASSWORD)
    reply(client, OLD_PASSWORD, "Is this information correct?")
    reply(client, "y", "PRESS RETURN")
    reply(client, "", MENU)
    # The same confirmation handler also serves account-menu password changes.
    reply(client, "6", PASSWORD_PROMPT)
    reply(client, OLD_PASSWORD, CONFIRM_PASSWORD)
    reply(client, OLD_PASSWORD, MENU)
    wire = bytes(client.transcript)
    require(OLD_PASSWORD.encode() not in wire, "password was echoed to the player")
    require(b"\xff\xfb\x01" in wire, "password input never disabled Telnet echo")
    require(wire.rfind(b"\xff\xfc\x01") > wire.rfind(b"\xff\xfb\x01"),
            "Telnet echo was not restored after confirmation")


def start_character(client):
    reply(client, "2", NAME_PROMPT)


def character_name_retry(client, server, *, policy=False, invalid=False):
    create_account(client, OLD_PASSWORD)
    start_character(client)
    if invalid:
        reply(client, "123", "Illegal character name, please try another.")
        client.expect(NAME_PROMPT)
    reply(client, "Taverek", "Is this correct?")
    reply(client, "?", "Please type Yes or No")
    if policy:
        reply(client, "y", "meet these criteria?")
        reply(client, "?", "Please type Yes or No")
    reply(client, "n", NAME_PROMPT)
    # One replacement input must be consumed as the name, not discarded.
    reply(client, "Veralis", "You chose the name Veralis")
    client.expect("Is this correct?")
    reply(client, "y", "meet these criteria?")
    reply(client, "y", RACE_MENU)


def reach_race(client):
    create_account(client, OLD_PASSWORD)
    start_character(client)
    reply(client, "Taverek", "Is this correct?")
    reply(client, "y", "meet these criteria?")
    reply(client, "y", RACE_MENU)


def creation_help_and_back(client, server):
    reach_race(client)
    reply(client, "?", RACE_MENU)
    reply(client, "x", "Press return to go back to the race selection menu.")
    reply(client, "", RACE_MENU)
    reply(client, "y", "Press Return and choose your race with care...")
    reply(client, "", RACE_MENU)
    # Uppercase is advertised as help, not an unavailable race selection.
    help_output = reply(client, "H", "[Press Return or Enter to return to the Race Menu]")
    require("not currently available" not in help_output, "race help rejected as a selection")
    reply(client, "", RACE_MENU)
    reply(client, "h", "Male or Female")
    reply(client, "z", RACE_MENU)
    # Mindflayers have no sex choice: their class menu promises a return to race.
    for race in ("Illithid", "Planetbound Illithid"):
        reply(client, race, "Class Selection")
        client.expect("z) Return to previous menu (selecting your race).")
        client.expect(RACE_MENU)
        reply(client, "z", RACE_MENU)
    reply(client, "h", "Male or Female")
    reply(client, "m", "H (for Hardcore), N (for Normal)")
    reply(client, "n", "Class Selection")
    client.expect(RACE_MENU)
    reply(client, "W", "[Press Return or Enter to return to the Class Menu]")
    reply(client, "", "Class Selection")
    client.expect(RACE_MENU)
    reply(client, "z", "Male or Female")
    reply(client, "f", "H (for Hardcore), N (for Normal)")


def finish_character_choices(client, *, exercise_retries=False, choose_hardcore=False):
    reply(client, "h", "Male or Female")
    if exercise_retries:
        reply(client, "?", "Please select either F")
    reply(client, "m", "H (for Hardcore), N (for Normal)")
    if exercise_retries:
        reply(client, "?", "H (for Hardcore), N (for Normal)")
    reply(client, "h" if choose_hardcore else "n", "Class Selection")
    client.expect(RACE_MENU)
    if exercise_retries:
        reply(client, "?", "Class Selection")
        client.expect(RACE_MENU)
        reply(client, "4", "This is not an allowed class for your race!")
        client.expect("Class Selection")
        client.expect(RACE_MENU)
    reply(client, "w", "Alignment only affects")
    client.expect(RACE_MENU)
    if exercise_retries:
        reply(client, "?", "Please choose an alignment: ")
    reply(client, "g", RACE_MENU)
    if exercise_retries:
        reply(client, "?", "Please choose a real hometown: ")
    reply(client, "p", "Press return to continue")
    reply(client, "", "first bonus")
    for next_bonus in ("second bonus", "third bonus", "fourth bonus"):
        if exercise_retries:
            reply(client, "?", "bonus:")
        reply(client, "s", next_bonus)
    if exercise_retries:
        reply(client, "?", "fourth bonus:")
    reply(client, "s", "swap stats (Y/N): ")
    if exercise_retries:
        reply(client, "?", "swap stats (Y/N): ")
        reply(client, "y", "Enter two letters separated by a space to swap:")
        reply(client, "bogus", "swap stats (Y/N): ")
        reply(client, "y", "Enter two letters separated by a space to swap:")
        reply(client, "s d", "swap more stats (Y/N): ")
    reply(client, "n", KEEP)


def keep_confirmation(client, server):
    reach_race(client)
    finish_character_choices(client, exercise_retries=True)
    for invalid in ("?", "", "    "):
        reply(client, invalid, KEEP)
    reply(client, "n", "Discarding this character.")
    client.expect(MENU)
    reply(client, "1", "Account currently doesn't have any characters (0/")
    client.expect(MENU)


def quit_confirmation(client, server):
    reach_race(client)
    finish_character_choices(client)
    reply(client, "q", "Come back again real soon.")
    client.socket.settimeout(5)
    while client.socket.recv(4096):
        pass
    other = MudClient(server.plain_port)
    try:
        enter_account_name(other)
        other.expect("Please enter your password:")
        reply(other, OLD_PASSWORD, "PRESS RETURN")
        reply(other, "", MENU)
        reply(other, "1", "Account currently doesn't have any characters (0/")
        other.expect(MENU)
    finally:
        other.close()


def normal_after_hardcore(client, server):
    reach_race(client)
    reply(client, "h", "Male or Female")
    reply(client, "m", "H (for Hardcore), N (for Normal)")
    reply(client, "h", "Class Selection")
    client.expect(RACE_MENU)
    reply(client, "z", "Male or Female")
    reply(client, "z", RACE_MENU)
    finish_character_choices(client)
    reply(client, "y", "PRESS RETURN")
    reply(client, "", "Your starter kit is ready")
    client.expect("Pos: standing >")
    score = reply(client, "score", "Pos: standing >")
    require("HardCore pts:" not in score, "Normal choice retained the previous Hardcore flag")
    require("Taverek" in score, "score did not display the new character")
    reply(client, "save", "Save complete for Taverek.")
    # A resident linkdead character reconnects immediately by design. Restart
    # the server to exercise disk reload and the saved-character confirmation.
    command = server.process.args
    server.shutdown()
    server.output.close()
    server.output_path = server.run_root / "restarted.out"
    server.output = server.output_path.open("w", encoding="utf-8")
    server.process = subprocess.Popen(
        command, cwd=server.run_root, env=server.environment,
        text=True, stdout=server.output, stderr=subprocess.STDOUT,
    )
    other = MudClient(server.plain_port)
    try:
        enter_account_name(other)
        other.expect("Please enter your password:")
        reply(other, OLD_PASSWORD, "PRESS RETURN")
        reply(other, "", MENU)
        reply(other, "1", "Taverek")
        reply(other, "1", "(Y/N)")
        reply(other, "?", "Please answer (Y/N): ")
        reply(other, "n", "Taverek")
        reply(other, "1", "(Y/N)")
        reply(other, "y", "Pos: standing >")
        score = reply(other, "score", "Pos: standing >")
        require("Taverek" in score and "HardCore pts:" not in score,
                "saved character did not reconnect with Normal mode")
    finally:
        other.close()


SCENARIOS = (
    ("account-name rejection", account_name_retry),
    ("email correction", email_retry),
    ("password mismatch and account-summary restart", password_and_summary_retry),
    ("invalid character name", lambda c, s: character_name_retry(c, s, invalid=True)),
    ("character-name rejection", character_name_retry),
    ("name-policy rejection", lambda c, s: character_name_retry(c, s, policy=True)),
    ("creation help and back navigation", creation_help_and_back),
    ("selection retries and final discard", keep_confirmation),
    ("final quit flushes its farewell without saving a character", quit_confirmation),
    ("Hardcore correction, world entry, save/restart and relog", normal_after_hardcore),
)


def run_journeys(binary):
    failures = []
    for name, scenario in SCENARIOS:
        environment = {"CREATION_ALL_RACES": "TRUE"} if scenario is creation_help_and_back else None
        with IsolatedServer(binary, environment) as server:
            client = MudClient(server.plain_port)
            try:
                scenario(client, server)
                server.shutdown()
                print(f"PASS creation journey: {name}", flush=True)
            except AssertionError as error:
                failures.append(f"{name}: {error}")
                print(f"FAIL creation journey: {name}: {error}", flush=True)
            finally:
                client.close()
    require(not failures, "Creation journey failures:\n" + "\n".join(failures))


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="creation-prompts-build-") as build_tmp:
        run_journeys(build_flatfile_server(Path(build_tmp)))
