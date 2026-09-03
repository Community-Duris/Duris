#!/usr/bin/env python3
"""Contracts for the remediated DurisWeb integration findings."""

from _paths import SRC
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
WS = (SRC / "websocket.c").read_text()
HANDLERS = (SRC / "ws_handlers.c").read_text()
AUTH = (SRC / "ws_auth.h").read_text()
GMCP = (SRC / "gmcp.c").read_text()
HEADER = (SRC / "websocket.h").read_text()
STRUCTS = (SRC / "structs.h").read_text()
EXPECTED_MUD_HOOKS = (
    "auction_new",
    "auction_bid",
    "auction_close",
    "player_presence",
    "mud_shutdown",
    "wholist",
    "admin_delete_character",
    "donation_delivery",
)

# Site and new-character bans cover WebSocket clients after trusted proxy
# metadata has been applied.
assert WS.index('strncasecmp(line, "X-Forwarded-For:"') < WS.index("bannedsite(d->host, 0)")
create = HANDLERS[HANDLERS.index("void ws_cmd_create_character") :]
assert "bannedsite(d->host, 1)" in create

# Login and registration are bounded both on the descriptor and across
# reconnects from the same resolved client host.
assert "WS_LOGIN_MAX_ATTEMPTS 5" in HEADER
assert "WS_REGISTER_MAX_ATTEMPTS 3" in HEADER
assert "WS_IP_RATE_SLOTS" in HANDLERS
assert "ws_player_auth_attempt(d, 0)" in HANDLERS
assert "ws_player_auth_attempt(d, 1)" in HANDLERS
for field in ("websocket_login_attempts", "websocket_register_attempts"):
    assert field in STRUCTS

# Login and registration conflicts no longer reveal which identity exists.
login = HANDLERS[HANDLERS.index("void ws_cmd_login") : HANDLERS.index("void ws_cmd_game")]
registration = HANDLERS[
    HANDLERS.index("void ws_cmd_register") : HANDLERS.index("static void ws_add_chargen_race")
]
assert '"Account not found"' not in login
assert '"Invalid password"' not in login
assert login.count('"Invalid account or password"') >= 4
assert '"An account with that name already exists"' not in registration
assert '"Email address is already in use"' not in registration

# Production only exposes the plaintext listener on loopback behind a trusted
# TLS terminator, and browser origins are exact-match allow-listed.
assert 'getenv("DURIS_WEBSOCKET_LISTEN_ADDRESS")' in WS
assert "websocket_address_is_loopback" in WS
assert 'getenv("DURIS_WEBSOCKET_ALLOWED_ORIGINS")' in WS
assert 'strncasecmp(line, "Origin:", 7)' in WS
assert 'websocket_send_http_rejection(d, "403 Forbidden")' in WS

# Service authentication is a one-time, expiring challenge bound into the
# HMAC and supports a previous key during rotation.
assert "RAND_bytes" in AUTH
assert '"%ld:%s", minute, challenge' in AUTH
assert 'getenv("DURISWEB_SECRET_PREVIOUS")' in AUTH
assert "durisweb_auth_challenge_expires" in STRUCTS
assert '"durisweb_challenge"' in HANDLERS
assert '"Core.AuthChallenge"' in GMCP

# Hook mutation stays on the authenticated command plane. Authorization is
# checked before request parsing, ids are exact-whitelisted, and success is
# coupled to durable property mutation plus the existing state broadcast.
hook_id_block = HANDLERS[
    HANDLERS.index("static const char *const DURISWEB_MUD_GATED_HOOKS[]") :
    HANDLERS.index("#define DURISWEB_MUD_GATED_HOOK_COUNT")
]
assert tuple(re.findall(r'"([a-z_]+)"', hook_id_block)) == EXPECTED_MUD_HOOKS

hook_state = HANDLERS[
    HANDLERS.index("void ws_cmd_durisweb_hook_state") :
    HANDLERS.index("static void ws_send_durisweb_hook_set_response")
]
assert hook_state.index("durisweb_verified") < hook_state.index(
    "ws_build_durisweb_hook_state_json"
)
assert '{ "durisweb_hook_state", ws_cmd_durisweb_hook_state }' in HANDLERS

hook_set = HANDLERS[
    HANDLERS.index("void ws_cmd_durisweb_hook_set") :
    HANDLERS.index("void ws_broadcast_durisweb_hook_state")
]
assert hook_set.index("durisweb_verified") < hook_set.index('cJSON_GetObjectItem(data, "hook")')
assert "ws_is_durisweb_mud_gated_hook" in hook_set
assert "cJSON_IsBool(enabled_json)" in hook_set
assert "request_json->valuestring[0] == '\\0'" in hook_set
assert "strlen(request_json->valuestring) > 128" in hook_set
assert "set_durisweb_hook_enabled(hook_id, enabled)" in hook_set
assert '"durisweb_hook_set"' in HANDLERS
assert '{ "durisweb_hook_set", ws_cmd_durisweb_hook_set }' in HANDLERS

auction_remove = HANDLERS[
    HANDLERS.index("void ws_cmd_durisweb_auction_remove") :
    HANDLERS.index("void ws_cmd_durisweb_hook_set")
]
# Authorization precedes parsing, the request is bounded, and the website never
# reaches the auction tables: removal is submitted as the MUD's own actor-less
# critical command.
assert auction_remove.index("durisweb_verified") < auction_remove.index(
    'cJSON_GetObjectItem(data, "auctionId")'
)
assert "request_json->valuestring[0] == '\\0'" in auction_remove
assert "strlen(request_json->valuestring) > 128" in auction_remove
assert "auction_action::remove" in auction_remove
# cJSON numbers are doubles, so a fractional id must be rejected outright rather
# than truncated onto a neighbouring auction.
assert "floor(auction_json->valuedouble)" in auction_remove
assert "auction_transaction_submit_background(payload, NULL)" in auction_remove
assert "payload.actor_pid" not in auction_remove
assert '{ "durisweb_auction_remove", ws_cmd_durisweb_auction_remove }' in HANDLERS

# Removal stages items back to the seller and never touches an actor wallet, so
# the shared validator must accept the actor-less payload the handler submits.
auction_command = (SRC / "economy/auction_command.c").read_text()
validator = auction_command[auction_command.index("bool valid_payload("):]
assert "payload.action != auction_action::remove" in validator[:validator.index("\n}")]

properties = (SRC / "properties.c").read_text()
setter = properties[properties.index("bool set_durisweb_hook_enabled") :]
assert "persist_durisweb_hook_property" in setter
assert "ws_is_durisweb_mud_gated_hook(hook_id)" in setter
assert "apply_properties()" in setter
assert "ws_broadcast_durisweb_hook_state()" in setter
assert '#define PROPERTIES_FILE "lib/duris.properties"' in properties
assert 'fopen(PROPERTIES_FILE ".new", "w")' in properties
assert 'fprintf(target, "%s=%.3f\\n", key, value)' in properties
assert 'rename(PROPERTIES_FILE ".new", PROPERTIES_FILE)' in properties
assert setter.index("persist_durisweb_hook_property") < setter.index(
    "ws_broadcast_durisweb_hook_state"
)

# The shipped property file and MUD operator configuration table must contain
# exactly the same eight property-backed ids, with no website-only inventions.
property_file = (ROOT / "lib/duris.properties").read_text()
property_ids = tuple(
    re.findall(r"^durisweb\.hook\.([a-z_]+)=", property_file, re.MULTILINE)
)
assert property_ids == EXPECTED_MUD_HOOKS

configuration = (ROOT / "docs/operations/CONFIGURATION.md").read_text()
documented_property_ids = tuple(
    re.findall(
        r"^\| `durisweb\.hook\.([a-z_]+)` \|",
        configuration,
        re.MULTILINE,
    )
)
assert documented_property_ids == EXPECTED_MUD_HOOKS

api_reference = (ROOT / "docs/reference/api/durisweb.md").read_text()
for hook_id in EXPECTED_MUD_HOOKS:
    assert f"`{hook_id}`" in api_reference
assert '`connection_log` is deliberately **not** gated here' in api_reference
assert '"cmd": "durisweb_auction_remove"' in api_reference
assert '"auctionId"' in api_reference
assert "Bidding and buy-now are deliberately **not** exposed here" in api_reference
assert '"cmd": "durisweb_hook_set"' in api_reference
for field in ('"requestId"', '"hook"', '"enabled"'):
    assert field in api_reference
assert "automatically and does not require" in api_reference
assert "may arrive before the acknowledgement" in api_reference

runbook = (ROOT / "docs/operations/RUNBOOK.md").read_text()
assert "website closes its own gate first" in runbook
assert "opens its own gate last" in runbook
assert "For `UNKNOWN`, restore" in runbook
assert "the authenticated bridge first" in runbook
assert "The terminal is always-on" in runbook

incident_response = (ROOT / "docs/operations/incident-response.md").read_text()
assert "## DurisWeb Hook Mismatch Or Bridge Failure" in incident_response
assert "never disable" in incident_response
assert "Treat an omitted" in incident_response

# Every emitter returns before constructing its payload. Player presence has
# two emitters; request/worker hooks use their own explicit refusal or drop
# behavior at the application boundary.
emitter_boundaries = (
    ("ws_broadcast_auction_new", "ws_broadcast_auction_bid", "auction_new"),
    ("ws_broadcast_auction_bid", "ws_broadcast_auction_close", "auction_bid"),
    ("ws_broadcast_auction_close", "ws_broadcast_mud_shutdown", "auction_close"),
    ("ws_broadcast_mud_shutdown", "ws_broadcast_player_login", "mud_shutdown"),
    ("ws_broadcast_player_login", "ws_broadcast_player_logout", "player_presence"),
    ("ws_broadcast_player_logout", "ws_send_wholist_to_client", "player_presence"),
    ("ws_send_wholist_to_client", "ws_cmd_request_wholist", "wholist"),
)
for start, end, hook_id in emitter_boundaries:
    emitter = HANDLERS[HANDLERS.index(start) : HANDLERS.index(end)]
    assert emitter.index(f'durisweb_hook_enabled("{hook_id}")') < emitter.index(
        "cJSON_CreateObject"
    )

admin_delete = HANDLERS[
    HANDLERS.index("void ws_cmd_admin_delete_character") :
    HANDLERS.index("void ws_cmd_rested_bonus")
]
assert admin_delete.index('durisweb_hook_enabled("admin_delete_character")') < (
    admin_delete.index('cJSON_GetObjectItem(data, "account")')
)
assert admin_delete.index('cJSON_GetObjectItem(data, "requestId")') < (
    admin_delete.index('durisweb_hook_enabled("admin_delete_character")')
)
assert 'strlen(request_id_json->valuestring) > 128' in admin_delete
disabled_delete = admin_delete[
    admin_delete.index('if (!durisweb_hook_enabled("admin_delete_character"))') :
    admin_delete.index('account_json = cJSON_GetObjectItem(data, "account")')
]
assert "ws_send_admin_delete_response(d, 0, NULL, NULL, request_id" in disabled_delete

revert = properties[
    properties.index('else if (!strcmp(command, "revert")') :
    properties.index("else\n\t\t\tshow_help = 1;")
]
assert "is_durisweb_hook_property" in revert
assert revert.index("apply_properties()") < revert.index(
    "ws_broadcast_durisweb_hook_state()"
)

donation_runtime = (SRC / "redis_donation_runtime.c").read_text()
donation_check = donation_runtime[
    donation_runtime.index("void check_donation_messages") :
    donation_runtime.index("bool redis_donation_runtime_enabled")
]
assert donation_check.index('durisweb_hook_enabled("donation_delivery")') < (
    donation_check.index("redis_donation_worker_take")
)
assert donation_check.index("if (!hook_enabled)") < donation_check.index(
    "broadcast_donation_nchat"
)

# Private presence fields and invisible staff require an explicit scope.
assert "durisweb_presence_character_visible" in HANDLERS
assert HANDLERS.count("if (durisweb_private_presence_enabled())") >= 2

# The command surface is table-driven and the formerly advertised dead text
# categories are absent.
assert "ws_cmd_handler handler" in HANDLERS
assert "for (const auto &entry : handlers)" in HANDLERS
assert "TEXT_COMBAT" not in (SRC / "json_utils.h").read_text()

print("DurisWeb integration security contracts passed")
