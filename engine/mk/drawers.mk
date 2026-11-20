# The Dreamcast drawers' test: the easy walk's sends, engine/dc/sa1/easy.h,
# which make dc compiles into Ninja's easy draws

# engine-test runs engine/tests/easy_sends.c where tools/sh4zam holds
# SH4ZAM's headers, which the walk includes and make dc fetches: the walk's
# sends as they stand, built for the host over the slice's prelude, into a
# store queue of the test's own, with no product fused into a sum, as make dc
# compiles them. The test reads engine/tests/host/easy_batch.h for the walk's
# batch.h, whose kernels no host compiles. The compiler is the slice's clang,
# whose sanitizer ends the run at C the standard leaves undefined, an offset
# from a null pointer among it. The sanitizer traps there, and the test
# catches the trap and names its case, so the test links no runtime of the
# sanitizer's and engine-test needs no more of clang than the slice does. The
# walk casts a pointer to 32 bits where it opens a polygon, which no send
# does, so the rule turns those two warnings off
DRAWERS_SH4ZAM := $(wildcard tools/sh4zam/fetched.txt)
build/engine/tests/easy_sends: engine/tests/easy_sends.c engine/tests/host/easy_batch.h $(wildcard engine/dc/sa1/*.h) \
		engine/dc/sa_dc.h engine/sa1/sa_sa1.h engine/sa1/rules.h $(ENGINE_INCLUDE)/sa_engine.h $(NATIVE_PRELUDE) \
		$(DRAWERS_SH4ZAM) engine/mk/drawers.mk $(ENGINE_CLANG)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CC) -std=gnu17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -Wno-pointer-to-int-cast \
		-Wno-int-to-pointer-cast -fsanitize=undefined -fsanitize-trap=undefined -DNONMATCHING \
		-include engine/native/prelude.h -isystem tools/sh4zam/include -idirafter sdk/include -I$(ENGINE_INCLUDE) \
		-Iengine/sa1 -Iengine/dc -I. -o $@ $<
engine-test: engine-drawers
engine-drawers: $(if $(DRAWERS_SH4ZAM),build/engine/tests/easy_sends)
	$(Q)$(if $(DRAWERS_SH4ZAM),build/engine/tests/easy_sends)
	@$(call status,drawers,$(if $(DRAWERS_SH4ZAM),the easy walk's sends,no SH4ZAM under tools/sh4zam$(comma) \
		which make dc fetches: the easy walk's sends go unrun))
.PHONY: engine-drawers
