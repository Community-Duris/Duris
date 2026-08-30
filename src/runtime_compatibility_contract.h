#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 173;
constexpr const char *RUNTIME_MYSQL8_METADATA_FINGERPRINT =
	"faa4bc0cfec07dd5cd960cce8306503fd774820e72ee344ec98398b15f4d78ed";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"7ab4d5acca0f4125c5a95114dd375df859768c4c2ade97abe6fb68b73ecb19e1";
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0005_level_cap_singleton";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 5;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"a83264e2f9241e328bcf76eefa192a0d10b0b4122917e56ef12ad39a35bc6132";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"4974eb5bf494251d83fc9c3f0c381a61f3afbd80e4eae6f9f291ea5b5c77b77f";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"31d99c8d9bd9bbc551046cb448e75e8a61bd9d2d3ab4c58795865d096b3e055e";
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
