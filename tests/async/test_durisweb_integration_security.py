#!/usr/bin/env python3
"""Contracts for the remediated DurisWeb integration findings."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WS = (SRC / "websocket.c").read_text()
HANDLERS = (SRC / "ws_handlers.c").read_text()
AUTH = (SRC / "ws_auth.h").read_text()
GMCP = (SRC / "gmcp.c").read_text()
HEADER = (SRC / "websocket.h").read_text()
STRUCTS = (SRC / "structs.h").read_text()

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
hook_set = HANDLERS[
    HANDLERS.index("void ws_cmd_durisweb_hook_set") :
    HANDLERS.index("void ws_broadcast_durisweb_hook_state")
]
assert hook_set.index("durisweb_verified") < hook_set.index('cJSON_GetObjectItem(data, "hook")')
assert "ws_is_durisweb_mud_gated_hook" in hook_set
assert "cJSON_IsBool(enabled_json)" in hook_set
assert "request_json->valuestring[0] == '\\0'" in hook_set
assert "set_durisweb_hook_enabled(hook_id, enabled)" in hook_set
assert '"durisweb_hook_set"' in HANDLERS
assert '{ "durisweb_hook_set", ws_cmd_durisweb_hook_set }' in HANDLERS

properties = (SRC / "properties.c").read_text()
setter = properties[properties.index("bool set_durisweb_hook_enabled") :]
assert "persist_durisweb_hook_property" in setter
assert "apply_properties()" in setter
assert "ws_broadcast_durisweb_hook_state()" in setter
assert '#define PROPERTIES_FILE "lib/duris.properties"' in properties
assert 'fopen(PROPERTIES_FILE ".new", "w")' in properties
assert 'rename(PROPERTIES_FILE ".new", PROPERTIES_FILE)' in properties

# Private presence fields and invisible staff require an explicit scope.
assert "durisweb_presence_character_visible" in HANDLERS
assert HANDLERS.count("if (durisweb_private_presence_enabled())") >= 2

# The command surface is table-driven and the formerly advertised dead text
# categories are absent.
assert "ws_cmd_handler handler" in HANDLERS
assert "for (const auto &entry : handlers)" in HANDLERS
assert "TEXT_COMBAT" not in (SRC / "json_utils.h").read_text()

print("DurisWeb integration security contracts passed")
