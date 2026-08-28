#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 171;
constexpr const char *RUNTIME_MYSQL8_METADATA_FINGERPRINT =
	"63be1814724472ecd652f9acc2868478f49ace81483217725dee97a48020dc15";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"7189aa7bf6a033a5f2f2cf270bcc348568e4e742e1dc1e3b3e35deb94c9715d2";
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0002_player_item_metadata_uniqueness";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 2;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"00e86dc65e6d5e935a50cd731d010675ade6da7fbbfdd18c4fe6fb17f88addba";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"312aa0aa354439e15bcef68403f00b7158c96627518d8be0bf85f59890ea1a90";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"b6731ff057a35a121c25fe0e9953a01b794efa9c286b479abb988a5d7f81cce8";
constexpr const char *LOOKUP_DATASET_NAME = "race_class";
constexpr unsigned LOOKUP_DATASET_VERSION = 1;
constexpr const char *RUNTIME_DB_CHARACTER_SET = "utf8mb4";
constexpr const char *RUNTIME_DB_TIME_ZONE = "+00:00";
constexpr const char *RUNTIME_DB_ISOLATION = "READ-COMMITTED";
constexpr const char *RUNTIME_DB_SQL_MODE =
	"STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION";
constexpr unsigned RUNTIME_DB_TIMEOUT_SECONDS = 10;
constexpr bool RUNTIME_DB_REMOTE_TLS_REQUIRED = true;
constexpr size_t RUNTIME_METADATA_MAX_BYTES = 4 * 1024 * 1024;

#endif
