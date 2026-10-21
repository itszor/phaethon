/* Phaethon (Sinusoidal Tendencies, 1993): what libroger needs to know about
 * the game.
 *
 * !Run RMEnsures Squash, sets Phaethon$Dir and Zone$Path (the !Zones
 * application beside it: the zones, their music and high scores), with a
 * 1068K WimpSlot runs !Phaethon. That is the intro, packed: Acorn's squeeze,
 * then a loader that hooks the SWI vector and decrypts the intro from its
 * own SWI handler. The intro (the System Interrupt credits over a
 * starfield, with its own voice generator) waits for a key, then *Loads
 * !RunImage at &8000 from a stub in screen memory, decrypts it and runs it.
 * !RunImage protects itself again - a patch loop that relies on the ARM2's
 * prefetch, its own SWI vector handlers chaining to the OS's - and
 * unpacks the game above itself (&68EDC), then starts it at &569C8.
 *
 * So the run has four stages, switched at the addresses control reaches
 * (dispatch()): the packed layers interpreted; the intro translated
 * (genintro/, from the image as it is at &8108, decrypted); !RunImage's
 * protection and unpacking interpreted; the game translated (gen/, from
 * the image at &569C8). ../tools/unpack writes both images, with these
 * same configuration and stages. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <roger.h>

extern const struct roger_image recomp_image, recompI_image;   /* gen{,intro}/entries.c */

/* the packed stages: no code translated, everything interpreted */
static uint8_t nocode[0x90000 / 32];
static const struct roger_image packed = {0x8000, 0x8000 + 199924, NULL, 0, nocode, 'P'};
static const struct roger_image unpacking = {0x8000, 0x98000, NULL, 0, nocode, 'U'};

#define INTRO_START 0x8108u      /* the intro, decrypted: its loader's last words decrypted, run */
#define GAME_LOAD   0x8000u      /* !RunImage, loaded and decrypted by the intro's stub */
#define GAME_START  0x569C8u     /* the game, unpacked */

/* Interpreted silently (any other untranslated address stops the run, or
 * with --trace-gaps is logged and interpreted): page zero, where the
 * loaders leave branches they go through; while packed, everything; while
 * the intro runs, its stub, which it copies into screen memory to load
 * !RunImage over the intro. */
static uint32_t interpreted[] = {0, 0x20, 0x8000, 0x4000000, 0, 0};
static void interpret(uint32_t from, uint32_t to, uint32_t from2, uint32_t to2)
{
    memset(interpreted, 0, sizeof interpreted);
    interpreted[0] = from; interpreted[1] = to;
    interpreted[2] = from2; interpreted[3] = to2;
}

static int stage;
static int dispatch(uint32_t a)
{
    switch (stage) {
    case 0:
        if (a != INTRO_START) return 0;
        roger_set_image(&recompI_image);
        interpret(0, 0x20, 0x1FE0000, 0x1FE0200);
        break;
    case 1:
        if (a != GAME_LOAD) return 0;
        roger_set_image(&unpacking);
        interpret(0, 0x20, 0x8000, 0x4000000);
        break;
    case 2:
        if (a != GAME_START) return 0;
        roger_set_image(&recomp_image);
        interpret(0, 0, 0, 0);
        break;
    default:
        return 0;
    }
    stage++;
    return 0;
}

static const char *const vars[] = {
    "Phaethon$Dir", "!Phaethon",
    /* !Run's SetMacro (<Phaethon$Dir> expanded, as reading it does): where
     * !Zones may be, the first beside !Phaethon */
    "Zone$Path", "!Phaethon.^.!Zones.,!Phaethon.!Zones.,ADFS::0.!Zones.,ADFS::1.!Zones.,RAM:!Zones.",
    NULL,
};
static const char *const prefixes[] = {NULL};

static struct roger_config config = {
    .title = "Phaethon (recompiled)",
    .image_path = "../game/!Phaethon/!Phaethon,fffff843-9fb27a36",
    .image = &packed,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = vars,
    .directory = "<Phaethon$Dir>",
    .command_line = "<Phaethon$Dir>.!Phaethon",
    .initial_mode = 12,                 /* the desktop's: the intro sets its own */
    .wimpslot = 1068 * 1024,            /* !Run's *WimpSlot 584k 1068k */
    .interpreted = interpreted,
    .call_addr = dispatch,
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
