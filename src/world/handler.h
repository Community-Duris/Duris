int can_prime_class_use_item(P_char, P_obj);

// A committed corpse raise may need to defer player serialization until the
// durable item rows have been rehydrated.  The login hook clears this fence
// only after the player's authoritative snapshot has been loaded.
bool corpse_raise_player_save_fenced(P_char);
void corpse_raise_player_ready(P_char);
