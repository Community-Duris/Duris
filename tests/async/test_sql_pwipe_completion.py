from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/sql.c").read_text()
start = source.rindex("bool sql_pwipe(int code_verify)")
end = source.index("void sql_log_player_login", start)
body = source[start:end]

assert 'logit(LOG_DEBUG, "sql_pwipe: COMPLETED!");' in body
assert 'send_to_all("WIPE COMPLETED!");' in body
assert body.index('redis_clear_pwipe_state()') < body.index('sql_pwipe: COMPLETED!')
assert body.index('sql_pwipe: COMPLETED!') < body.index('return TRUE;')
assert body.count('sql_pwipe: COMPLETED!') == 1

print("sql pwipe completion reporting checks passed")
