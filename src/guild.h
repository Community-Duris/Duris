/*
   this is for generic guilds.. I HATE changing all 20 guilds
   or so when something smiple changes.
   */

/* flags (just after number: - = send_to_char or act needed,
   I = insert to Gbuf3, no flag = copy to Gbuf3 */

#define GUILD_ERR 1 /* - */

#define GUILD_WRONG_CLASS 2 /* - */
#define GUILD_EXPLIST_PRE 3 /* - */ /* before showing do_exp info.. */
#define GUILD_EXPLIST_AFT 4 /* - */ /* after showing do_exp info.. */
/* Message that lists available practices when practices are free. */
#define GUILD_PRAC_LPRE 5
/* I: message shown when the player tries to practice with no practices left. */
#define GUILD_NOPRACS 6
#define GUILD_PRE_SKILLS 7 /* I */ /* before showing skill list */
#define GUILD_AFT_SKILLS 8 /* I */ /* after the same */

#define GUILD_NO_SUCH_SKILL 9 /* - */
#define GUILD_NOT_CLASS_SKI 10 /* - */
#define GUILD_TOO_LOW_LVL 11 /* - */
#define GUILD_NOPRACS_PRAC 12 /* - */ /* no pracs left when doing actual prac */
#define GUILD_FULL_SKILL 13 /* - */
#define GUILD_PRACCING 14
#define GUILD_REACHED_FULL 15 /* - */
