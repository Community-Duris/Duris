#!/usr/bin/env python3
"""Compile and run the real DurisWeb challenge/HMAC helper."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = r'''
#include "ws_auth.h"
#include <openssl/hmac.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static void sign_for(const char *secret, const char *challenge, char output[65])
{
    char message[128];
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    int message_length = std::snprintf(message, sizeof(message), "%ld:%s",
                                       std::time(nullptr) / 60, challenge);
    HMAC(EVP_sha256(), secret, std::strlen(secret),
         reinterpret_cast<unsigned char *>(message), message_length, digest, &length);
    for (unsigned int i = 0; i < length; ++i)
        std::snprintf(output + i * 2, 3, "%02x", digest[i]);
    output[64] = '\0';
}

int main()
{
    char challenge[65] = {};
    char signature[65] = {};
    time_t expires = 0;
    setenv("DURISWEB_SECRET", "current-key", 1);
    setenv("DURISWEB_SECRET_PREVIOUS", "previous-key", 1);
    if (!ws_issue_durisweb_challenge(challenge, &expires) || std::strlen(challenge) != 64 ||
        expires <= std::time(nullptr)) return 1;
    sign_for("current-key", challenge, signature);
    if (!ws_verify_durisweb_signature(signature, challenge, expires)) return 2;
    sign_for("previous-key", challenge, signature);
    if (!ws_verify_durisweb_signature(signature, challenge, expires)) return 3;
    challenge[0] = challenge[0] == 'a' ? 'b' : 'a';
    if (ws_verify_durisweb_signature(signature, challenge, expires)) return 4;
    if (ws_verify_durisweb_signature(signature, challenge, std::time(nullptr) - 1)) return 5;

    time_t window = 0;
    unsigned int attempts = 0;
    for (unsigned int i = 0; i < 5; ++i) {
        if (ws_auth_rate_limited(&window, &attempts, 5, 60)) return 6;
        ws_auth_record_failure(&window, &attempts, 5, 60);
    }
    if (!ws_auth_rate_limited(&window, &attempts, 5, 60)) return 7;
    ws_auth_reset(&window, &attempts);
    if (window || attempts) return 8;
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ws-auth-") as directory:
    source = Path(directory) / "auth_runtime.cpp"
    binary = Path(directory) / "auth_runtime"
    source.write_text(SOURCE)
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
         str(source), "-lcrypto", "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("DurisWeb authentication runtime passed")
