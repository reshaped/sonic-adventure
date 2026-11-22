# This test enters through the PC game's runner and replaces only sa1_main.
# The nested target enables native.mk's regular generated-source checks.
.PHONY: engine-test-player-terrain
engine-test-player-terrain:
	$(Q)$(MAKE) $(NATIVE)/player-terrain
	$(Q)$(NATIVE)/player-terrain $(NATIVE_DISC) 0

TERRAIN_SRC := $(NATIVE)/tests/player_terrain.o $(NATIVE)/tests/pc_terrain.o
$(TERRAIN_SRC): $(NATIVE)/tests/%.o: engine/tests/%.c engine/tests/player_terrain.h $(NATIVE_PASSES) engine/mk/player_terrain.mk
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_pass,$(addprefix -I,$(NATIVE_GAME_INC)) -Iengine/tests,r32,$(GAME))

$(NATIVE)/tests/pc_terrain_main.o: engine/tests/pc_terrain_main.c engine/native/native.h $(NATIVE_NEEDS) engine/mk/player_terrain.mk
	@$(call status,cc,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -Wall -Wextra -Werror -c -o $@ $<

$(NATIVE)/player-terrain: private NATIVE_WRAPS += sa1_main
$(NATIVE)/player-terrain: $(NATIVE_LINK_OBJ) $(NATIVE_PLAY_OBJ) $(TERRAIN_SRC) $(NATIVE)/tests/pc_terrain_main.o $(NATIVE_LINKED) engine/mk/player_terrain.mk
	@$(call status,link,$@)
	$(Q)$(NATIVE_CC) $(NATIVE_LINK_FLAGS) -o $@ $(filter %.o,$^) $(PC_LIBS) -lstdc++ -lm $(NATIVE_CRT_END)

-include $(TERRAIN_SRC:.o=.d)
