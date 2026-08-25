from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
structs = (ROOT / "src" / "structs.h").read_text()
header = (ROOT / "src" / "websocket.h").read_text()
ws = (ROOT / "src" / "websocket.c").read_text()
comm = (ROOT / "src" / "comm.c").read_text()

assert "WS_MAX_OUTPUT_BYTES" in header
assert "WS_CONTROL_OUTPUT_RESERVE" in header
assert "ws_output_buffer" in structs
assert "ws_output_len" in structs
assert "ws_output_offset" in structs
assert "ws_control_output_buffer" in structs
assert "websocket_flush_output" in header
assert "errno == EAGAIN || errno == EWOULDBLOCK" in ws
assert "websocket_flush_output(point)" in comm
assert "free(d->ws_output_buffer)" in ws
assert "free(d->ws_control_output_buffer)" in ws
