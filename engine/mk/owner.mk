# The submission owner's tests: one owner of the Dreamcast's TA lists and
# frame, which make dc links as config/dreamcast.mk says
# (ENGINE.md#submission)

# engine-test runs engine/tests/owner_slots.c on every host: the owner's
# frame beside a model of the chip that the test keeps, where each picture
# renders from its own bank over its own background plane, and each flip goes
# on at the event its frame's rate gives it. owner.c compiles as it stands,
# with the machine's words the test's by its WORD, which
# engine/tests/host/machine.h gives under OWNER_SLOTS, and a 32-bit KMDWORD
# under -Dlong=int. It casts between pointers and KMDWORDs, which a 64-bit
# host's compiler warns of, so it compiles under -w here, and under make dc's
# warnings for the Dreamcast
OWNER_SLOTS_DEPS := engine/tests/owner_slots.c engine/dc/owner.c engine/dc/owner.h engine/tests/host/machine.h \
	sdk/include/kamui.h sdk/include/kmistat.h engine/mk/owner.mk $(ENGINE_CC)
build/engine/tests/owner-slots/slots: $(OWNER_SLOTS_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -O1 -std=gnu17 -Wall -Wextra -Werror -Iengine/dc -idirafter sdk/include -c -o $(@D)/harness.o $<
	$(Q)$(CC) -O1 -w -fno-strict-aliasing -std=gnu17 -Dlong=int -DOWNER_SLOTS -Iengine/tests/host -Isdk/include \
		-c -o $(@D)/owner.o engine/dc/owner.c
	$(Q)$(CC) -o $@ $(@D)/harness.o $(@D)/owner.o
engine-test: engine-test-owner
engine-test-owner: build/engine/tests/owner-slots/slots
	$(Q)build/engine/tests/owner-slots/slots
	@$(call status,owner,each picture's bank and background plane at its render)
.PHONY: engine-test-owner

# The names the owner takes from Kamui's two objects, which make dc compiles
# without them, and which owner_diff renames out of them
OWNER_SURFACE := kmRender kmFlipFrameBuffer kmResetPointer kmRenderDirect kmRenderTexture kmRenderTextureDirect
OWNER_INTERRUPT := kmiInitInterrupt kmiInterruptHandlerSH kmiFinalizeRender
owner_renames = $(foreach n,$1,-D$n=kamui_$n)

# engine-test runs engine/tests/owner_diff.c on x86-64 Linux: Kamui's frame and
# the owner's, each built for the host from its sources as they stand, through
# the same events, overflows and timeouts among them, with their traces equal.
# Kamui's C is old, and the file's -Dlong=int gives it a 32-bit KMDWORD
ifeq ($(HOST),Linux x86_64)
OWNER_DIFF_RUN := 120 100
OWNER_DIFF_CFLAGS := -O1 -w -Wno-return-mismatch -Wno-int-conversion -Wno-incompatible-pointer-types -fno-pie \
	-fno-strict-aliasing -std=gnu17 -Dlong=int -Iengine/tests/host -Isdk/include
OWNER_DIFF_FRAME := sdk/ninja/kmsurfac_kamui_lib.c sdk/ninja/interupt_kamui_lib.c
OWNER_DIFF_DEPS := engine/tests/owner_diff.c engine/tests/host/machine.h $(OWNER_DIFF_FRAME) sdk/include/kamui.h \
	sdk/include/kmistat.h engine/dc/owner.h engine/mk/owner.mk $(ENGINE_CC)
# the harness itself reads the host's headers ahead of the tree's
build/engine/tests/owner-diff/harness.o: $(OWNER_DIFF_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -O1 -fno-pie -std=gnu17 -Wall -Wextra -idirafter sdk/include -c -o $@ $<
build/engine/tests/owner-diff/kamui: build/engine/tests/owner-diff/harness.o $(OWNER_DIFF_DEPS)
	$(Q)$(CC) $(OWNER_DIFF_CFLAGS) -no-pie -Wl,--unresolved-symbols=ignore-all -o $@ $< $(OWNER_DIFF_FRAME)
build/engine/tests/owner-diff/owner: build/engine/tests/owner-diff/harness.o engine/dc/owner.c $(OWNER_DIFF_DEPS)
	$(Q)$(CC) $(OWNER_DIFF_CFLAGS) $(call owner_renames,$(OWNER_SURFACE)) -c -o $(@D)/surface.o \
		sdk/ninja/kmsurfac_kamui_lib.c
	$(Q)$(CC) $(OWNER_DIFF_CFLAGS) $(call owner_renames,$(OWNER_INTERRUPT)) -c -o $(@D)/interrupt.o \
		sdk/ninja/interupt_kamui_lib.c
	$(Q)$(CC) $(OWNER_DIFF_CFLAGS) -no-pie -Wl,--unresolved-symbols=ignore-all -o $@ $< $(@D)/surface.o \
		$(@D)/interrupt.o engine/dc/owner.c
build/engine/tests/owner-diff/equal: build/engine/tests/owner-diff/kamui build/engine/tests/owner-diff/owner
	$(Q)build/engine/tests/owner-diff/kamui $(OWNER_DIFF_RUN) > $@.kamui
	$(Q)build/engine/tests/owner-diff/owner $(OWNER_DIFF_RUN) > $@.owner
	$(Q)cmp $@.kamui $@.owner
	@$(call status,owner,Kamui's frame and the owner's on the chip's model)
	$(Q)touch $@
# the owner's frame at 60 fps, through the same events, with no flip timed out
build/engine/tests/owner-diff/sixty: build/engine/tests/owner-diff/owner
	$(Q)build/engine/tests/owner-diff/owner -6 $(OWNER_DIFF_RUN) > $@.owner
	@$(call status,owner,the owner's frame at 60 fps on the chip's model)
	$(Q)touch $@
engine-test: build/engine/tests/owner-diff/equal build/engine/tests/owner-diff/sixty
endif
