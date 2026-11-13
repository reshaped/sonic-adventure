# Play math's rules: engine/numeric/'s library, the SH-4's float operations
# that play reads, as a host computes them to the SH-4's bits
# (ENGINE.md#numbers). A program that plays on a host links one build of it.
# This file alone names each build's objects and what compiles them, and a
# fragment whose program links one takes both from here

# A source outside the library includes its header, play_math.h, and nothing
# else of it, as engine/tests/boundary.py holds. PLAY_MATH_INCLUDE is the
# flag that finds the header, and PLAY_MATH_HEADER what such a source's
# object waits on
PLAY_MATH_INCLUDE := -Iengine/numeric
PLAY_MATH_HEADER  := engine/numeric/play_math.h

# The library builds four ways, and a link takes the objects of one. The
# reference is math.c, written to be read. The fast path is fast.c, which
# includes it and gives its bits faster: it guesses by the C library's
# sqrtf, so its link takes -lm. Each gives the default build's answers, or
# compiled with SA_NUMERIC_CONSOLE the console's. Each name below takes DIR,
# the directory one compiler's objects lie under, and gives a build's
# objects there, as $(call play_math_fast,DIR) gives the fast path's
play_math_reference         = $1/math.o
play_math_fast              = $1/fast.o
play_math_reference_console = $1/console/math.o
play_math_fast_console      = $1/console/fast.o

# The fast path's tables of sines, one for each of the two answers, where
# the rules at this file's end write them. A compile of the fast path
# searches its table's directory
PLAY_MATH_SINE         := build/engine/numeric/sine.h
PLAY_MATH_SINE_CONSOLE := build/engine/numeric/sine-console.h

# $(eval $(call play_math,DIR,COMPILE,NEEDS,LABEL)) gives the objects under
# DIR their rules. COMPILE, a compiler and its flags, compiles each, and each
# waits on NEEDS too: the fragment that gives COMPILE, and what must be in
# place before COMPILE runs. Where LABEL isn't empty, each compile prints it
# as its status. A source of the library finds the header and the sources it
# includes beside it, and fast.c its build's table of sines where the host
# writes it. DIR gets the rules of all four builds, and make runs those a
# link names
PLAY_MATH_DEPS := $(PLAY_MATH_HEADER) engine/mk/play_math.mk
PLAY_MATH_HOST := build/engine/numeric
define play_math
$(call play_math_reference,$1) $(call play_math_fast,$1): $1/%.o: engine/numeric/%.c \
		engine/numeric/sh4_fpu_approx.h $$(PLAY_MATH_DEPS) $3
	$(if $4,@$$(call status,$4,$$<))
	$$(Q)mkdir -p $$(@D)
	$$(Q)$2 $$(PLAY_MATH_TABLES) -c -o $$@ $$<
$(call play_math_reference_console,$1) $(call play_math_fast_console,$1): $1/console/%.o: engine/numeric/%.c \
		engine/numeric/sh4_fpu_approx.h $$(PLAY_MATH_DEPS) $3
	$(if $4,@$$(call status,$4,$$<))
	$$(Q)mkdir -p $$(@D)
	$$(Q)$2 -DSA_NUMERIC_CONSOLE $$(PLAY_MATH_TABLES) -c -o $$@ $$<
$(call play_math_fast,$1): private PLAY_MATH_TABLES := -I$$(patsubst %/,%,$$(dir $$(PLAY_MATH_SINE)))
$(call play_math_fast_console,$1): private PLAY_MATH_TABLES := -I$$(patsubst %/,%,$$(dir $$(PLAY_MATH_SINE_CONSOLE)))
$(call play_math_fast,$1): engine/numeric/math.c $$(PLAY_MATH_SINE)
$(call play_math_fast_console,$1): engine/numeric/math.c $$(PLAY_MATH_SINE_CONSOLE)
endef

# The host's own builds, which CC compiles under the engine's flags. The
# tables' writers and the engine's tests link them. Each waits on CC's
# record, ENGINE_CC, as each program CC links below does
$(eval $(call play_math,$(PLAY_MATH_HOST),$$(CC) $$(ENGINE_CFLAGS) -O2,engine/engine.mk $(ENGINE_CC)))

# fast.c's table, the sine's words of the first half turn, one for each of
# the two answers. sine.c writes each from the host's reference of that
# build, so the table can't part from it, and its words are that reference's
# integers, so every compiler's fast.c reads the one table
$(PLAY_MATH_SINE): engine/numeric/sine.c $(call play_math_reference,$(PLAY_MATH_HOST))
$(PLAY_MATH_SINE_CONSOLE): engine/numeric/sine.c $(call play_math_reference_console,$(PLAY_MATH_HOST))
$(PLAY_MATH_SINE) $(PLAY_MATH_SINE_CONSOLE): $(PLAY_MATH_DEPS) engine/engine.mk $(ENGINE_CC)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(ENGINE_CFLAGS) -O2 -o $@.bin $(filter %.c %.o,$^)
	$(Q)$@.bin > $@ && rm $@.bin

# The fast path against the reference: engine/tests/fast.c holds fast.c to
# math.c's bits in each of the two answers, which the fast path's object
# holds under ref_ names
PLAY_MATH_FAST_TEST := engine/tests/fast.c $(PLAY_MATH_DEPS) $(ENGINE_CC)
build/engine/tests/fast: $(PLAY_MATH_FAST_TEST) $(call play_math_fast,$(PLAY_MATH_HOST))
build/engine/tests/fast-console: $(PLAY_MATH_FAST_TEST) $(call play_math_fast_console,$(PLAY_MATH_HOST))
build/engine/tests/fast build/engine/tests/fast-console:
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(ENGINE_CFLAGS) -O2 $(PLAY_MATH_INCLUDE) -o $@ engine/tests/fast.c $(filter %.o,$^) -lm
engine-test: engine-play-math
engine-play-math: build/engine/tests/fast build/engine/tests/fast-console
	$(Q)build/engine/tests/fast
	$(Q)build/engine/tests/fast-console
	@$(call status,numeric,fast.c gives math.c's bits$(comma) on each of the two answers)
.PHONY: engine-play-math
