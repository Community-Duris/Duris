#!/usr/bin/env python3
"""Execute private-chest hydration and asynchronous description serialization."""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, SRC

sql = (SRC / 'sql_player.c').read_text()
start = sql.rindex('void sql_load_private_chest_items(')
loader = sql[start:sql.index('\n// migration helpers', start)]
worker = (SRC / 'locker_async.c').read_text()
builder = worker[worker.index('struct la_buf\n{'):worker.index('/* Only locker metadata')]

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "sql/item_extra_descr_codec.h"
#include <mysql/mysql.h>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_set>
#define LOCKER_ASYNC_SCRIPT_INIT 65536
MYSQL *DB = reinterpret_cast<MYSQL *>(1);
static MYSQL_RES rows{};
static std::string cells[28];
static char *values[28];
static bool fetched;
static obj_data loaded{};
static index_data indexes[2]{};
P_index obj_index = indexes;
void logit(const char *, const char *, ...) {}
char *str_dup(const char *value) { return strdup(value); }
char *sql_escape_string(const char *value) { return strdup(value); }
MYSQL_RES *db_query(const char *format, ...) {
    char query[2048]; va_list args; va_start(args, format);
    vsnprintf(query, sizeof(query), format, args); va_end(args);
    assert(strstr(query, "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, item_material"));
    fetched = false; return &rows;
}
MYSQL_ROW mysql_fetch_row(MYSQL_RES *) {
    if (fetched) return nullptr; fetched = true; return values;
}
void mysql_free_result(MYSQL_RES *) {}
int real_object(int vnum) { assert(vnum == 700); return 1; }
P_obj read_object(int, int) {
    loaded = {}; loaded.bitvector = 11; loaded.bitvector2 = 12;
    loaded.bitvector3 = 13; loaded.bitvector4 = 14; loaded.bitvector5 = 15;
    loaded.material = 16; return &loaded;
}
void extract_obj(P_obj, int) { std::abort(); }
static int sql_validate_loaded_item_type(P_obj, int type, const char *) { return type; }
bool sql_persistence_item_owner_matches_identity(unsigned long long uid, const char *,
    unsigned long long owner, unsigned long long chest, const char *) {
    assert(uid == 42 && owner == 8 && chest == 9); return true;
}
static void sql_load_item_affects_from_table(int, P_obj, const char *) {}
static bool sql_load_item_extra_descr_from_table(int, P_obj, const char *) { return true; }
void obj_to_obj(P_obj obj, P_obj chest) { chest->contains = obj; }
static P_obj sql_load_locker_items_filtered(int, int, int, int) { return nullptr; }
bool obj_can_nest(P_obj, P_obj) { return true; }
void recalc_container_weight(P_obj) {}
'''

DRIVER = r'''
int main() {
    for (bool stored : {false, true}) {
        for (int i = 0; i < 28; ++i) { cells[i] = "0"; values[i] = cells[i].data(); }
        cells[0] = "1"; cells[1] = "700"; cells[20] = "42"; cells[21] = "91";
        for (int i = 22; i < 28; ++i) cells[i] = std::to_string(i + 100);
        for (int i = 0; i < 28; ++i) values[i] = cells[i].data();
        for (int i = 16; i < 20; ++i) values[i] = nullptr;
        if (!stored) for (int i = 22; i < 28; ++i) values[i] = nullptr;
        obj_data chest{};
        sql_load_private_chest_items(8, 9, &chest);
        assert(chest.contains == &loaded && loaded.obj_uid == 42 && loaded.condition == 91);
        assert(loaded.bitvector == (stored ? 122U : 11U));
        assert(loaded.bitvector2 == (stored ? 123U : 12U));
        assert(loaded.bitvector3 == (stored ? 124U : 13U));
        assert(loaded.bitvector4 == (stored ? 125U : 14U));
        assert(loaded.bitvector5 == (stored ? 126U : 15U));
        assert(loaded.material == (stored ? 127 : 16));
    }
    char keyword[] = "note", description[] = "same", other[] = "different";
    extra_descr_data distinct{keyword, other, nullptr};
    extra_descr_data duplicate{keyword, description, &distinct};
    extra_descr_data first{keyword, description, &duplicate};
    obj_data item{}; item.R_num = 1; item.ex_description = &first;
    indexes[1].virtual_number = 700;
    la_buf buffer{}; assert(la_buf_init(&buffer, 64));
    assert(emit_item_sql(&buffer, &item, 8, 9, 0));
    std::string text = buffer.data;
    const std::string marker = "INSERT INTO locker_item_extra_descr";
    size_t count = 0, offset = 0;
    while ((offset = text.find(marker, offset)) != std::string::npos) { ++count; ++offset; }
    assert(count == 2);
    assert(text.find("'note', 'same'") != std::string::npos);
    assert(text.find("'note', 'different'") != std::string::npos);
    la_buf_free(&buffer);
    puts("PASS private-chest item flags/material, NULL prototype fallback, and duplicate-free asynchronous descriptions");
}
'''

with tempfile.TemporaryDirectory(prefix='locker-item-payload-') as temporary:
    source, binary = Path(temporary) / 'harness.cpp', Path(temporary) / 'harness'
    source.write_text(PRELUDE + loader + builder + DRIVER)
    subprocess.run(['g++', '-std=c++20', '-g', '-O1', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-Isrc', *subprocess.check_output(['mysql_config', '--cflags'], text=True).split(),
                    str(source), 'src/sql/item_extra_descr_codec.c', '-o', str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ,
        'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
