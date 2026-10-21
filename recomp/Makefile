# Static recompilation of Phaethon: the game (gen/: !RunImage as it is once
# it has unpacked itself) and the intro (genintro/: !Phaethon once its
# loader has decrypted it), both written by ../tools/unpack, from
# SuperSpider's analyse and gen, and game.c, linked with libroger (the
# RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/code*.c) gen/entries.c
GENINTRO:= $(wildcard genintro/code*.c) genintro/entries.c
OBJ     := $(GEN:.c=.o) $(GENINTRO:.c=.o) game.o
LIBS    := $(shell pkg-config --libs sdl3) -lm -lpthread
INC     := -I$(ROGER)/include

phaethon: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

%.o: %.c $(ROGER)/include/roger.h $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# The images: ../tools/unpack runs !Phaethon with libroger's interpreter,
# with game.c's configuration, and writes memory when control reaches the
# address each image starts from (game.c): the intro at &8108, decrypted;
# the game at &569C8, unpacked - getting there from the intro's script
# loop (&9088) by the path a key press takes (&841C), which releases the
# intro's handlers and loads !RunImage.
UNPACK  := ../tools/unpack
PACKED  := ../game/!Phaethon/!Phaethon,fffff843-9fb27a36
UP      := ../unpacked

images:
	$(MAKE) -C ../tools
	mkdir -p $(UP)
	$(UNPACK) "$(PACKED)" --limit=3000000 --stop=8108 --dump=8000:23334 --write=$(UP)/intro --out=$(UP)/intro
	$(UNPACK) "$(PACKED)" --limit=12000000 --redirect=9088:841c --stop=569c8 --dump=8000:98000 \
	    --write=$(UP)/game --out=$(UP)/game

# Code/data analysis: SuperSpider, into ../analysis/<name>.json, then
# superspider gen. Each image's ../analysis/<name>.roots adds entry points
# to the analysis. The first is where game.c takes the image up, part way
# through the program (&8108, &569C8), with a return address the analysis
# can't know (so not --exec). "make roots" merges the gap entries a
# --trace-gaps run logged (untranslated.log, or LOG=...) into them, so the
# next "make regen" translates them. The two images share &8000 (one runs
# after the other): the log tags each entry with the image that was running.
SS      ?= $(HOME)/stuff/superspider
SSBIN   := $(SS)/_build/default/bin/main.exe
AN      := ../analysis
GAME    := $(UP)/game
INTRO   := $(UP)/intro
LOG     ?= untranslated.log

# --roots for ../analysis/<name>.roots, if there is one
roots = $(if $(wildcard $(AN)/$(1).roots),--roots $(AN)/$(1).roots)

# $(call analyse,name,image,base[,more options])
define analyse
	$(SSBIN) analyse "$(2)" $(3) $(4) --full $(call roots,$(1)) -o $(AN)/$(1).json
endef

regen: images
	cd $(SS) && dune build
	mkdir -p $(AN)
	$(call analyse,game,$(GAME),8000)
	$(call analyse,intro,$(INTRO),8000)
	rm -rf gen genintro
	$(SSBIN) gen "$(GAME)" 8000 $(AN)/game.json --c gen --leaders=dispatch
	$(SSBIN) gen "$(INTRO)" 8000 $(AN)/intro.json --c genintro --leaders=dispatch \
	    --prefix=I --tabprefix=recompI --files=1 --covtag=I

roots:
	python3 -I $(SS)/tools/gap_roots.py "$(GAME)" 8000 $(AN)/game.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py "$(INTRO)" 8000 $(AN)/intro.roots $(LOG)

clean:
	rm -f gen/*.o genintro/*.o game.o phaethon

.PHONY: FORCE images regen roots clean
