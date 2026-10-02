#ifndef PLAYER_QUARANTINE_RECOVERY_H
#define PLAYER_QUARANTINE_RECOVERY_H

#include "player/player_save_journal.h"
#include "player/player_load_repository.h"

struct player_recovery_creation
{
	critical_command command;
	item_transfer_result receipt;
};

// Pure reconstruction after the native owner has verified every original command
// and successful receipt. Missing evidence never becomes an inferred grant.
bool player_quarantine_recovery_build(const player_load_result &current,
				      const std::vector<player_snapshot> &frames,
				      const std::vector<player_recovery_creation> &creations,
				      player_save_recovery_record *record, std::string *error);
bool player_quarantine_recovery_state_matches(const player_load_result &current,
					      const player_save_recovery_record &record,
					      bool applied);
bool player_quarantine_recovery_projection_equal(const player_snapshot &left,
						 const player_snapshot &right);
player_load_request player_quarantine_recovery_request(const player_save_recovery_record &);
bool player_quarantine_recovery_creation_proofs_sql(MYSQL *, const player_save_recovery_record &);
// A native receipt is committed with the replacement. It binds the entire
// prepared record and remains immutable while later ordinary saves advance.
bool player_quarantine_recovery_sql_receipt(MYSQL *, const player_save_recovery_record &,
					    bool write, bool *present);
std::string player_quarantine_recovery_receipt_filename(const player_save_recovery_record &);
bool player_quarantine_recovery_receipt_bytes(const player_save_recovery_record &,
					      std::vector<uint8_t> *);
void player_quarantine_recovery_revalidate_selected();

// Listener-free owners: callers must hold the isolated restore/server-stop
// boundary. SQL and flat-file verification never execute a creation command.
bool player_quarantine_recovery_prepare_sql(MYSQL *, int pid, const std::vector<critical_command> &,
					    const std::string &backend_identity,
					    player_save_recovery_record *, std::string *error);
bool player_quarantine_recovery_resume_sql(MYSQL *, int pid, const std::string &backend_identity,
					   std::string *error);
bool player_quarantine_recovery_prepare_flatfile(const std::string &root, int pid,
						 const std::vector<critical_command> &,
						 const std::string &backend_identity,
						 player_save_recovery_record *, std::string *error);
bool player_quarantine_recovery_resume_flatfile(const std::string &root, int pid,
						const std::string &backend_identity,
						std::string *error);

#endif
