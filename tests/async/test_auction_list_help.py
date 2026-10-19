#!/usr/bin/env python3
"""Execute the SQL auction list formatter with fortification and boundary inputs."""
from pathlib import Path
import resource
import subprocess
import tempfile
from _paths import ROOT, SRC, extract_function

HARNESS = r'''
#include "core/safe_format.h"
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
#define MAX_STRING_LENGTH 65536
#define MAX_INPUT_LENGTH 1024
#define TRUE 1
#define LOG_SYS 0
#define AUCTION_STATUS_OPEN 1
static_assert(__USE_FORTIFY_LEVEL == 3);
using P_char = void *;
char buff[MAX_STRING_LENGTH];
void *DB = nullptr;
vector<string> output, queries;
struct EqSort {
    vector<string> keywords = {"horns", "nose", "head"};
    string getKeyword(int i) { return keywords.at(i); }
    int getSize() { return keywords.size(); }
    bool isKeyword(const char *key) {
        for (const auto &word : keywords) if (word == key) return true;
        return false;
    }
    string getDescString(const char *key) { return key; }
} catalogue, *sorter = &catalogue;
void logit(int, const char *, ...) { abort(); }
void send_to_char(const char *text, P_char) { output.emplace_back(text); }
bool isname(const char *key, const char *names) {
    istringstream input(names); string word;
    while (input >> word) if (word == key) return true;
    return false;
}
unsigned long mysql_real_escape_string(void *, char *dest, const char *src, unsigned long n) {
    memcpy(dest, src, n); dest[n] = 0; return n;
}
#define FALSE 0
#define GET_PID(ch) 0
#define IS_TRUSTED(ch) false
bool qry(const char *format, ...) {
    char query[MAX_STRING_LENGTH * 2];
    va_list args; va_start(args, format);
    int n = vsnprintf(query, sizeof(query), format, args); va_end(args);
    assert(n >= 0 && n < int(sizeof(query)));
    queries.emplace_back(query); return true;
}
struct MYSQL_RES {} empty_rows;
using MYSQL_ROW = char **;
MYSQL_RES *mysql_store_result(void *) { return &empty_rows; }
int mysql_num_rows(MYSQL_RES *) { return 0; }
MYSQL_ROW mysql_fetch_row(MYSQL_RES *) { return nullptr; }
void mysql_free_result(MYSQL_RES *) {}
string pad_ansi(const char *text, int, int = 0) { return text; }
string format_time(long) { return ""; }
'''
CHECKS = r'''
void run(const string &input) {
    char args[MAX_STRING_LENGTH] = {};
    assert(input.size() < MAX_INPUT_LENGTH);
    strcpy(args, input.c_str());
    memset(buff, 'X', sizeof(buff)); // Do not rely on an already-empty display buffer.
    output.clear(); queries.clear();
    assert(auction_list(nullptr, args));
}
void help(const string &input, bool syntax) {
    run(input);
    assert(queries.empty());
    assert(output.size() == (syntax ? 2 : 1));
    if (syntax) assert(output.front().find("Auction list syntax:") != string::npos);
    string expected = "&+WValid keywords: &+Y" + catalogue.keywords.front();
    for (size_t i = 1; i < catalogue.keywords.size(); ++i)
        expected += ", " + catalogue.keywords[i];
    expected += ".\n\r";
    assert(output.back() == expected.substr(0, sizeof(buff) - 1));
    assert(buff[output.back().size()] == '\0');
}
int main() {
    for (const char *input : {"plate 1000", "plate", "1", "nonsense", "%s%n", "unknown extra args"})
        help(input, true);
    help("sort", false);
    help("s", false);
    for (const char *input : {"", "all", "a"}) {
        run(input);
        assert(queries.size() == 1);
        assert(queries.front().find("where status = 1  order by") != string::npos);
        assert(output.front() == "&+WAuctions closing soon:\r\n");
        assert(output.back().find("No auctions") != string::npos);
    }
    run("player synthetic");
    assert(queries.size() == 1);
    assert(queries.front().find("and seller_name like 'Synthetic'") != string::npos);
    assert(output.front() == "&+WAuctions by &nSynthetic&+W:\r\n");
    run("sort horns nose");
    assert(queries.size() == 1);
    assert(queries.front().find("and id_keywords like '% horns,%' and id_keywords like '% nose,%'") != string::npos);
    assert(output.front().find("Auctions for items") != string::npos);
    run("player");
    assert(queries.empty() && output.front().find("Please enter the name") != string::npos);
    run("sort not-a-keyword");
    assert(queries.empty() && output.front().find("is an invalid keyword") != string::npos);
    // Initial text, a later keyword, and the final punctuation can each fill the buffer.
    for (int boundary = 0; boundary < 3; ++boundary) {
        catalogue.keywords = {"horns", "nose", "head"};
        if (boundary == 0) catalogue.keywords[0] = string(MAX_STRING_LENGTH, 'x');
        if (boundary == 1) catalogue.keywords[1] = string(MAX_STRING_LENGTH, 'y');
        if (boundary == 2) catalogue.keywords = {string(MAX_STRING_LENGTH - 22, 'z')};
        help("plate", true);
        help("sort", false);
    }
}
'''

source = (SRC / "auction_houses.c").read_text()
start = source.rindex("bool auction_list(P_char ch, char *args)")
end = source.index("\n// syntax: auction info", start)
with tempfile.TemporaryDirectory(prefix="auction-list-") as directory:
    path = Path(directory)
    harness, binary = path / "harness.cpp", path / "harness"
    harness.write_text(HARNESS + extract_function(
        "interp.c", "void half_chop(char *string, char *arg1, char *arg2)") +
        "\n" + source[start:end] + CHECKS)
    subprocess.run(["g++", "-std=c++20", "-O2", "-g", "-U_FORTIFY_SOURCE", "-D_FORTIFY_SOURCE=3",
                    f"-I{SRC}", str(harness), str(SRC / "safe_format.c"),
                    "-o", str(binary)], check=True)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    subprocess.run([str(binary)], check=True)
print("auction list help, filtering and truncation boundaries passed")
