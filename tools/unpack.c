/* Run Phaethon's packed program with libroger's ARM2 interpreter until
 * control arrives at an address for the Nth time, then save memory.
 *
 *   unpack FILE --stop=ADDR[,N] --dump=FROM:TO --write=OUT [--jumps]
 *
 * FILE is loaded at &8000 and entered there, with the game's files and
 * system variables as !Run sets them (recomp/game.c). Nothing is
 * translated. --jumps prints each transfer of control to an address more
 * than 64K from the last one (the layers' hand-overs), to find the stops;
 * --peek=ADDR the registers each time ADDR is reached; --redirect=FROM:TO
 * carries on at TO when FROM is first reached (to skip to the part of a
 * program that does the unpacking); --trace=ICOUNT:N each of N instructions
 * from then on, with the registers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <roger.h>

static uint8_t nocode[0x100000 / 32];
static struct roger_image image = {0x8000, 0x8000 + 0x100000, NULL, 0, nocode, 'P'};

static uint32_t stop = ~0u, nth = 1, from, to, last, peek = ~0u, redir_from = ~0u, redir_to;
static int jumps;
static unsigned long long trace_from = ~0ull, trace_n;
static const char *out;

static int arg(const char *a)
{
    if (!strncmp(a, "--stop=", 7)) {
        char *e;
        stop = strtoul(a + 7, &e, 16);
        if (*e == ',') nth = strtoul(e + 1, 0, 10);
        return 1;
    }
    if (!strncmp(a, "--dump=", 7)) {
        char *e;
        from = strtoul(a + 7, &e, 16);
        to = strtoul(e + 1, 0, 16);
        return 1;
    }
    if (!strncmp(a, "--write=", 8)) { out = a + 8; return 1; }
    if (!strcmp(a, "--jumps")) { jumps = 1; return 1; }
    if (!strncmp(a, "--trace=", 8)) {
        char *e;
        trace_from = strtoull(a + 8, &e, 10);
        trace_n = strtoull(e + 1, 0, 10);
        return 1;
    }
    if (!strncmp(a, "--peek=", 7)) { peek = strtoul(a + 7, 0, 16); return 1; }
    if (!strncmp(a, "--redirect=", 11)) {
        char *e;
        redir_from = strtoul(a + 11, &e, 16);
        redir_to = strtoul(e + 1, 0, 16);
        return 1;
    }
    return 0;
}

static int stop_hook(uint32_t a)
{
    if (jumps && (a - last + 0x10000) > 0x20000)
        printf("%llu: %08x -> %08x\n", (unsigned long long)icount, last, a);
    last = a;
    if (a == redir_from) {
        redir_from = ~0u;
        npc = redir_to;
        return 1;
    }
    if (icount >= trace_from && trace_n) {
        trace_n--;
        printf("%llu %08x %08x:", (unsigned long long)icount, a, *(uint32_t *)(MEM + a));
        for (int k = 0; k < 15; k++) printf(" %x", R[k]);
        printf("\n");
    }
    if (a == peek) {
        printf("%llu:", (unsigned long long)icount);
        for (int k = 0; k < 15; k++) printf(" %08x", R[k]);
        printf("\n");
    }
    if (a != stop || !icount || --nth) return 0;
    if (out) {
        FILE *f = fopen(out, "wb");
        if (!f) { perror(out); exit(1); }
        fwrite(MEM + from, 1, to - from, f);
        fclose(f);
    }
    printf("stopped at %08x after %llu instructions; wrote %08x-%08x to %s\n", a,
           (unsigned long long)icount, from, to, out ? out : "(nothing)");
    for (int k = 0; k < 15; k++) printf("R%d=%08x%s", k, R[k], k % 5 == 4 ? "\n" : " ");
    exit(0);
}

/* as recomp/game.c */
static const char *const prefixes[] = {NULL};
static const char *const variables[] = {"Phaethon$Dir", "!Phaethon", "Zone$Path", "!Phaethon.^.!Zones.,!Phaethon.!Zones.,ADFS::0.!Zones.,ADFS::1.!Zones.,RAM:!Zones.", NULL};


static struct roger_config config = {
    .title = "unpack",
    .image = &image,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = variables,
    .directory = "<Phaethon$Dir>",
    .initial_mode = 12,
    .wimpslot = 1068 * 1024,
    .interpret_gaps = 1,
    .arg = arg,
    .call_addr = stop_hook,
    .usage = "  --stop=ADDR[,N] --dump=FROM:TO --write=FILE --jumps --peek=ADDR --redirect=FROM:TO --trace=ICOUNT:N   (hex addresses)\n",
};

int main(int argc, char **argv)
{
    for (int k = 1; k < argc; k++)
        if (argv[k][0] != '-') {                        /* the file: the (empty) image's extent */
            struct stat st;
            if (!stat(argv[k], &st) && st.st_size <= (long)sizeof nocode * 32) image.end = 0x8000 + st.st_size;
        }
    roger_main(argc, argv, &config);
}
