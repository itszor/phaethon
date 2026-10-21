Phaethon (Sinusoidal Tendencies / System Interrupt, 1993) - static recompilation
================================================================================

A recipe: this repository holds no game data. It rebuilds the game as a
native executable from your own copy, recognised by content (recipe.json),
so it can be shared without distributing the game.

    ./build.sh phaethon.zip
    cd recomp && ./phaethon

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER; $SS below): it extracts the zip,
builds the unpacker (tools/unpack), which writes the two images that are
translated, analyses and translates them, and links them with libroger
(~/stuff/libroger, or make ROGER=...). Needs python3, OCaml/dune, gcc and
SDL3.

The input: phaethon.zip, a zip of the two applications as SparkFS wrote
them (the filetypes in its RISC OS extra fields): !Phaethon (the game, the
Squash module, the intro's music and the sound effects) and !Zones (the
five zones of the Normal course, their music, the high score tables).

What it makes:

    extracted/zip            the zip, via weave.py
    game/!Phaethon, !Zones   libroger's appdir (../game)
    tools/unpack             libroger's interpreter as an unpacker (unpack.c)
    unpacked/intro, game     the two images, as the program leaves them
    analysis/*.json          SuperSpider's analyses; *.roots: entry points
    recomp/gen*/             the C translations
    recomp/phaethon          the game

Committed: recipe.json, build.sh, tools/unpack.c, tools/Makefile,
analysis/*.roots, recomp/Makefile, recomp/game.c, this file.

Playing
-------

    ./phaethon              as !Run: the intro (any key or a click ends it),
                            the title page, the shop, the zones

The title page and the shop are worked with the mouse: Play, then buy at
least an engine (click a part's icon, then the item, then click again to
buy it), then Quit the Shop; the screen's map follows, and a click starts
it. In flight the mouse steers and Adjust (the right button) is the
thrust. The game also reads Return, P and F1, and Acorn's joystick module
(Joystick_Read), which it does without when it isn't there.

Tested (headless, scripted): the intro, the title page, the shop (buying
an engine and its upgrade), the first screen of the Normal course (Ajay)
- accelerating and steering. Further screens, the end of a course, the
high score entry, Enter Code, Options, the Editor and Back to Desk weren't
reached; their code is translated as far as the analysis finds it, and
untranslated code met there stops the run (--trace-gaps logs it and
carries on interpreted: make roots, make regen).

How the game is put together
----------------------------

!Run RMEnsures Squash, sets Phaethon$Dir and Zone$Path (a list of places
!Zones may be, the first beside !Phaethon) and, with a 1068K WimpSlot,
runs !Phaethon. Everything is packed and protected, in layers:

- !Phaethon is squeezed (Acorn's squeeze, not AIF), and unpacks to a
  loader ("Loader by Paul Carpenter", with Geraint Stimson's JoystickConv
  module inside it, inserted if there's no joystick module) that unpacks
  again and copies the intro to &8000.
- The intro's start-up puts its own branch in the ARM's SWI vector (&8),
  so that every SWI it makes goes through its handler: the handler puts
  the OS's word back, makes an OS_Byte with the caller's registers, hooks
  the vector again and returns through a second handler entered by the
  OS_UpdateMEMC it makes. A SWI 0 through the vector decrypts the intro.
  The loader goes on through branches it leaves in page zero (&8058 is
  overwritten with b &0).
- The intro (the System Interrupt credits, a bytecode script over a
  starfield, with a voice generator of its own and IntroMusic) runs until
  a key or a click. Then it releases its handlers, copies a stub into
  screen memory (&1FE0000) and clears &8000-&30000; the stub *Loads
  !RunImage at &8000, decrypts it (XOR with an LFSR) and runs it.
- !RunImage protects itself again. A loop patches the program from a
  table of (value, address) pairs, saving each old word - and its first
  instruction, str r1,[pc], overwrites the instruction after next, which
  on an ARM2 has already been fetched: the loop only works as the
  pipeline runs it. The patches put the program's handler in the SWI
  vector, decode bytes at &8004 from it, and pass SWIs on to the OS by
  decoding the branch the vector held (a SWI from above &20000 is sent to
  &4 instead). Then a decompressor at &68EDC unpacks the game above
  !RunImage, and it starts at &569C8.

What's translated (recomp/Makefile, make regen): the intro (genintro/:
the image as it is when control first reaches &8108, the loader's last
words decrypted) and the game (gen/: the image at &569C8, unpacked:
&8000-&98000). game.c runs the program in four stages, switched at the
addresses control reaches: the packed layers interpreted; the intro
translated; !RunImage's protection and unpacking interpreted; the game
translated. tools/unpack runs the same configuration with nothing
translated and writes each image when control reaches its address; for
the game, it skips the intro's attract loop (from &9088 to &841C, the path
a key press takes).

What SuperSpider and libroger needed
------------------------------------

libroger:

- The ARM's SWI vector (&8): a program's own branch there (or ldr pc)
  takes every SWI instruction, entered as the ARM does (SVC mode, IRQs
  off, R14_svc the return address with the caller's PSR); translated code
  leaves for it (the SWI() macro). &8 holds the OS's branch to its SWI
  handler (a reserved address, OS_SWI_ENTRY), which makes the SWI in the
  instruction before R14 and returns to the caller, so a handler can pass
  SWIs on by jumping where the OS's word led.
- The interpreter models the ARM2's pipeline: the two words after an
  instruction have been fetched before it runs, so a store to either isn't
  seen in sequence (until a branch, an exception or a SWI refills it).
- Vectors hold a chain of claimants, the last claimed called first; one
  that returns through R14 passes the call on, one that pulls the return
  address off the stack intercepts it. Before, a second OS_Claim replaced
  the first (the intro claims EventV twice, and its vsync counter, claimed
  first, was never called).
- *Load with no address loads an Absolute (&FF8) at &8000, as
  Alias$@LoadType_FF8 does.
- "^" (the parent directory) in filenames (Zone$Path's
  <Phaethon$Dir>.^.!Zones.).
- GSTrans ends a string at a control character, as GSRead does (the game
  sets Zone$Dir from a CR-terminated part of Zone$Path, with a length
  that runs to the next comma).

Regression: headless runs (60M instructions) of the other 22 games
before and after these changes - the event log, VDU stream, screen,
registers and --wav output - are byte-identical.
