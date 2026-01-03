// migrate_common.c
// shared utility functions for pfile migration tool

#include "migrate_common.h"

// global vars
P_Guild guild_list = NULL;
char buf[MAX_STRING_LENGTH];

// format seconds into human readable time
static void format_time(double seconds, char *buf, size_t buflen) {
    if (seconds < 60) {
        snprintf(buf, buflen, "%ds", (int)seconds);
    } else if (seconds < 3600) {
        int mins = (int)(seconds / 60);
        int secs = (int)seconds % 60;
        snprintf(buf, buflen, "%dm%02ds", mins, secs);
    } else {
        int hours = (int)(seconds / 3600);
        int mins = ((int)seconds % 3600) / 60;
        int secs = (int)seconds % 60;
        snprintf(buf, buflen, "%dh%02dm%02ds", hours, mins, secs);
    }
}

void progress_init(struct progress_bar *pb, int total, const char *prefix) {
    pb->total = total;
    pb->current = 0;
    pb->prefix = prefix;
    gettimeofday(&pb->start_time, NULL);
    progress_update(pb, 0);
}

void progress_update(struct progress_bar *pb, int current) {
    pb->current = current;

    struct timeval now;
    gettimeofday(&now, NULL);
    double elapsed = (now.tv_sec - pb->start_time.tv_sec) +
                     (now.tv_usec - pb->start_time.tv_usec) / 1000000.0;

    int width = 30;
    int filled = (pb->total > 0) ? (current * width) / pb->total : 0;
    int percent = (pb->total > 0) ? (current * 100) / pb->total : 0;

    char elapsed_str[32], eta_str[32];
    format_time(elapsed, elapsed_str, sizeof(elapsed_str));

    if (current > 0 && current < pb->total) {
        double rate = current / elapsed;
        double remaining = (pb->total - current) / rate;
        format_time(remaining, eta_str, sizeof(eta_str));
    } else if (current >= pb->total) {
        snprintf(eta_str, sizeof(eta_str), "0s");
    } else {
        snprintf(eta_str, sizeof(eta_str), "--");
    }

    printf("\r%-12s [", pb->prefix);
    for (int i = 0; i < width; i++) {
        if (i < filled) printf("=");
        else printf("-");
    }
    printf("] %3d%% %d/%d  %s<%s", percent, current, pb->total, elapsed_str, eta_str);
    fflush(stdout);
}

void progress_finish(struct progress_bar *pb) {
    progress_update(pb, pb->total);
    printf("\n");
}

// minimal stubs for functions we don't need
void logit(int type, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[debug] ");
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// fread_string from db.c - reads until ~
char *fread_string(FILE *fl) {
    static char buf[MAX_STRING_LENGTH];
    char *ptr = buf;
    char c;

    do {
        c = getc(fl);
    } while (isspace(c));

    if (c == '~') {
        return str_dup("");
    }

    while (c != '~' && !feof(fl)) {
        if (c == '\n' || c == '\r') {
            *ptr++ = ' ';
        } else {
            *ptr++ = c;
        }
        c = getc(fl);
    }
    *ptr = '\0';

    // trim trailing space
    while (ptr > buf && isspace(*(ptr-1))) {
        *--ptr = '\0';
    }

    return str_dup(buf);
}

void free_mig_obj(struct mig_obj *obj) {
    if (!obj) return;
    if (obj->name) free(obj->name);
    if (obj->short_descr) free(obj->short_descr);
    if (obj->description) free(obj->description);
    if (obj->action_descr) free(obj->action_descr);
    free_mig_obj(obj->contains);
    free_mig_obj(obj->next);
    free(obj);
}

void free_mig_affect(struct mig_affect *af) {
    while (af) {
        struct mig_affect *next = af->next;
        if (af->wear_off_char) free(af->wear_off_char);
        if (af->wear_off_room) free(af->wear_off_room);
        free(af);
        af = next;
    }
}

void free_mig_player(struct mig_player *p) {
    if (!p) return;
    if (p->short_descr) free(p->short_descr);
    if (p->long_descr) free(p->long_descr);
    if (p->description) free(p->description);
    if (p->title) free(p->title);
    if (p->poof_in) free(p->poof_in);
    if (p->poof_out) free(p->poof_out);
    if (p->poof_in_sound) free(p->poof_in_sound);
    if (p->poof_out_sound) free(p->poof_out_sound);
    if (p->granted_cmds) free(p->granted_cmds);
    free_mig_affect(p->affects);
    for (int i = 0; i < MIG_MAX_WEAR; i++)
        free_mig_obj(p->equipment[i]);
    free_mig_obj(p->inventory);
    free(p);
}
