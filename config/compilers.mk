# Compiler selection, patches and flags

# SHC 5.0 R32 builds game code; R10 builds most SDK code.
# TOOLCHAIN.md#releases gives the evidence for each release.
# build/shckit patches the assembly R10 writes for these sources, with the
# patches named for each, before asmsh assembles it. TOOLCHAIN.md#patched-r10
# says why each source takes its patches, and shckit's docs/patches.md what
# each writes
PATCHED_C := sdk/ninja/njControl.c sdk/ninja/njMotion.c sdk/ninja/njMotionE.c \
             sdk/ninja/njGetMotion.c sdk/ninja/njModel.c sdk/ninja/njModelE.c \
             sdk/ninja/njModelS.c sdk/ninja/njMotLinkS.c sdk/ninja/njMotLinkSrc.c \
             sdk/ninja/njMotSrc.c sdk/ninja/njMotionS.c src/draw/action.c \
             src/draw/action_link.c
build/sdk/ninja/njControl.obj: private PATCHES := align-pool
build/sdk/ninja/njMotion.obj: private PATCHES := use-assembler-pools drop-r0-move-nop
build/sdk/ninja/njMotionE.obj: private PATCHES := drop-inline-align use-assembler-pools drop-r0-move-nop
build/sdk/ninja/njGetMotion.obj: private PATCHES := use-assembler-pools drop-r0-move-nop
build/sdk/ninja/njModel.obj build/stub/sdk/ninja/njModel.obj: private PATCHES := drop-inline-align align-pool
build/sdk/ninja/njModelE.obj: private PATCHES := drop-inline-align align-pool
build/sdk/ninja/njModelS.obj: private PATCHES := drop-inline-align align-pool
build/sdk/ninja/njMotLinkS.obj: private PATCHES := use-assembler-pools drop-r0-move-nop
build/sdk/ninja/njMotLinkSrc.obj: private PATCHES := drop-inline-align use-assembler-pools drop-r0-move-nop fill-call-slot
build/sdk/ninja/njMotSrc.obj: private PATCHES := drop-inline-align use-assembler-pools drop-r0-move-nop
build/sdk/ninja/njMotionS.obj: private PATCHES := use-assembler-pools drop-r0-move-nop
build/src/draw/action.obj: private PATCHES := use-assembler-pools
build/src/draw/action_link.obj: private PATCHES := use-assembler-pools
# ALIGN names a section and an alignment, which tools/align.py declares for the
# section in the patched text before asmsh assembles it. action.c's code ends
# at 0x8c0419c4, and action_link.c's starts at 0x8c0419e0, after 28 zero bytes:
# 0x8c0419c4 rounded up to 32 is 0x8c0419e0. With its code section on 32,
# action_link.c's object starts there, and the splicer writes the 28 bytes as
# the linker's fill. R10 declares the section on 4 for this source, and writes
# the same text under -extra=a=c00 as under its -extra=a=400: c00 adds the
# game's a=800, which aligns R32's code to 32. R10's shcasm.exe holds the
# operands ALIGN=4, 8 and 16, and no ALIGN=32, and -align16, R10's one
# alignment option, declares 16 and changes the code. lnk 6.0 and 6.0A,
# linking the two objects from 0x8c040de0, start action_link.c's code at
# 0x8c0419c4 where it declares 4 and at 0x8c0419e0 where it declares 32,
# whether or not the link command gives align_section, which keeps lnk from
# warning that the objects' P sections declare different alignments. Their D
# rows meet at 0x8c1b0c50, and action.c's B row ends where text.c's starts, so
# an object linked between the two holds no data or bss; one with neither and
# an empty code section on 32 would leave this layout too. This is the
# accepted reconstruction of the boundary, which the layout and these trials
# support; it does not show how the original was built
build/src/draw/action_link.obj: private ALIGN := P 32
# R10's assembly of njCameraMotion.c takes functions from its donor's, as
# TOOLCHAIN.md#grafted-r10 says
GRAFTED_C := sdk/ninja/njCameraMotion.c
DONOR_C := sdk/ninja/njCameraMotionOld.c
build/sdk/ninja/njCameraMotion.obj: private PATCHES := use-assembler-pools drop-r0-move-nop
# MOVIE.BIN's own SDK members, under modules/, build as SDK code with R10
MOVIE_SDK := $(shell find modules/movie/sdk -name '*.c')
# LOADER.BIN links its own SDK 1.20J members, beside its own code
LOADER_SDK := $(shell find loader/sdk -name '*.c')
LOADER_C := $(filter-out $(LOADER_SDK),$(shell find loader -name '*.c'))
# R10 takes #pragma inline_asm and #pragma asm only with -code=asmcode, so a
# source with either compiles to assembly, which R32's asmsh assembles. A
# patched or grafted source takes its own route. $(call inline_asm,PATHS)
# names each C source in PATHS, or under a directory there, that holds
# either, as tools/tags.py reads them: a directive, never a comment or a
# literal that mentions one. It stops make when tags.py fails, since a
# partial list would move a source off its route
inline_asm = $(shell PYTHONPYCACHEPREFIX=$(PYTHONPYCACHEPREFIX) python3 tools/tags.py --inline $1)$(if $(filter-out 0,$(.SHELLSTATUS)),$(error tools/tags.py --inline failed; make can't tell which sources hold inline assembly))
INLINE_C := $(call inline_asm,sdk src $(MOVIE_SDK))
ASMCODE := $(filter-out $(PATCHED_C) $(GRAFTED_C) $(DONOR_C),$(INLINE_C))
# R26 builds Shinobi's overlay library
R26_C := sdk/shinobi/ov_main.c
# R32 builds the C runtime, with RUNTIME's flags
RUNTIME_C := $(shell find sdk/runtime -name '*.c')
# R32 also builds SDK 1.30J's GD and ADX decoders and its GD file system
R32_C := sdk/shinobi/gdDec.c sdk/sofdec/adx_dcda.c \
         sdk/shinobi/gdctl.c sdk/shinobi/gdfs.c sdk/shinobi/gdfsdir.c \
         sdk/shinobi/gdfshn.c sdk/shinobi/gdfsif.c
# SDK 1.30J's Kamui members build as game code, with R32 and the game's flags
KAMUI_C := $(wildcard sdk/ninja/*_kamui_lib.c)

# Compiler flags change object bytes; TOOLCHAIN.md#flags says how
SHCFLAGS := -cpu=sh4 -endian=little
GAME     := $(SHCFLAGS) -fpu=single -speed -extra=a=800 -macsave=0
SDK      := $(SHCFLAGS)
RUNTIME  := $(SHCFLAGS) -fpu=single

# asmsh assembles FSCA and FSRRA only under -private
ASMFLAGS := -cpu=sh4 -endian=little -sjis -private

# $(call own,SOURCES,FLAGS) adds FLAGS to GAME for each source, a path less
# its .c. A per-source flag names the source's stub object too, where it has
# one, and is private: the object depends on its stub, which would otherwise
# inherit the flag from it and take it twice
own = $(foreach s,$1,$(eval build/$s.obj build/stub/$s.obj: private GAME += $2))
$(call own,modules/advertise/game_bug modules/al_race/menu \
    modules/al_race/racer \
    modules/b_chaos0/chaos0 modules/b_chaos0/clock modules/b_chaos2/chaos2 \
    modules/b_chaos4/camera modules/b_chaos4/chaos4 \
    modules/sboard/pillar modules/sboard/sand_frog \
    modules/sboard/sand_gate modules/sboard/sand_smoke \
    modules/stg02/windy modules/stg03/cart_stopper modules/stg04/highway \
    modules/stg07/aoki_switch modules/stg08/big_icicle modules/stg12/shelter \
    src/camera/editor src/draw/state src/enemy/snake src/main \
    src/object/simple src/object/switch src/player/e102 \
    src/save/save src/vmu/screen,-noinline)
$(call own,modules/advertise/character_select,-inline=10)
$(call own,modules/stg08/snow_door src/path/curve,-aggressive=2)
$(call own,src/system/binary,-optimize=0)
# -size takes -speed's place: SHC warns when given both
build/modules/b_e101_r/files.obj build/stub/modules/b_e101_r/files.obj: private GAME := $(patsubst -speed,-size,$(GAME)) -optimize=0
# Event sources omit -extra=a=800, including shared event code
build/events/% build/stub/events/%: GAME := $(filter-out -extra=a=800,$(GAME))
# The loader's own code takes -extra=a=c00 in place of -extra=a=800, and sees
# the SDK's headers and the loader's, not the game's
$(LOADER_C:%.c=build/%.obj): GAME := $(patsubst -extra=a=800,-extra=a=c00,$(GAME))
$(LOADER_C:%.c=build/%.obj): GAME_INC := sdk/include,.
build/sdk/ninja/kmutil_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/ninja/njCameraMotion.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njDebug.obj: SDK += -double=float -macsave=0 -extra=a=400
build/sdk/ninja/njCreateLight.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njDeleteLight.obj: SDK += -extra=a=400
build/sdk/ninja/njDrawLine3D.obj: SDK += -macsave=0
build/sdk/ninja/njDrawPolygon2D.obj: SDK += -extra=a=400
build/sdk/ninja/njDrawTriangle3D.obj: SDK += -macsave=0
build/sdk/ninja/njGenerateFogDensity.obj: SDK += -double=float -extra=a=400
build/sdk/ninja/njGenerateFogTable.obj: SDK += -macsave=0 -extra=a=400
build/sdk/ninja/njGenerateFogTable3.obj: SDK += -double=float -macsave=0 -extra=a=400
build/sdk/ninja/njGetMotion.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njInitCacheTexture.obj: SDK += -extra=a=400
build/sdk/ninja/njLoadTextureLN.obj: SDK += -size -extra=a=400
build/sdk/ninja/njLoadTexturePvmFile.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/ninja/njLtSrcs.obj: SDK += -double=float -size -macsave=0 -extra=a=400
# njMem_ alone takes bit 0x40, as a=440: under a=400, njMemCopy2's DT lands at
# 0x8c6572fe, ahead of its load
build/sdk/ninja/njMem.obj: SDK += -double=float -size -macsave=0 -extra=a=440
build/sdk/ninja/njModel.obj build/stub/sdk/ninja/njModel.obj: private SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njModelE.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njModelS.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njMotLinkS.obj: SDK += -macsave=0 -extra=a=400
build/sdk/ninja/njMotLinkSrc.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njMotSrc.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njMotion.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njMotionE.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njMotionS.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/sdk/ninja/njNearClipping.obj: SDK += -extra=a=400
build/sdk/ninja/njPalette.obj: SDK += -size -extra=a=400
build/sdk/ninja/njReleaseTextureAll.obj: SDK += -extra=a=400
build/sdk/ninja/njReleaseTextureNum.obj: SDK += -size
build/sdk/ninja/njSetTextureParam.obj: SDK += -extra=a=400
build/sdk/ninja/njSystem.obj: SDK += -size -extra=a=400
build/sdk/ninja/njTexFile.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/ninja/njTexList.obj: SDK += -size -extra=a=400
build/sdk/ninja/njTexStride.obj: SDK += -macsave=0 -extra=a=400
build/sdk/ninja/njTexinner.obj: SDK += -size -extra=a=400
build/sdk/ninja/njTexpvr.obj: SDK += -size -extra=a=400
build/sdk/ninja/njTexture.obj: SDK += -size -extra=a=400
build/sdk/ninja/njkm.obj: SDK += -extra=a=400
build/sdk/ninja/njtri_wiredpoly.obj: SDK += -extra=a=400
build/sdk/ninja/njtri_wiredpoly_with_tex.obj: SDK += -extra=a=400
build/sdk/shinobi/fm.obj: SDK += -size -extra=a=400
build/sdk/shinobi/fmcrc.obj: SDK += -extra=a=400
build/sdk/shinobi/gdDec.obj: SDK += -fpu=single -macsave=0
build/sdk/shinobi/gdctl.obj: SDK += -fpu=single
build/sdk/shinobi/gdfs.obj: SDK += -fpu=single -size -macsave=0
build/sdk/shinobi/gdfsdir.obj: SDK += -fpu=single -size
build/sdk/shinobi/gdfshn.obj: SDK += -fpu=single -size
build/sdk/shinobi/gdfsif.obj: SDK += -fpu=single
build/sdk/shinobi/kdapi.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/shinobi/lcd.obj: SDK += -optimize=0
build/sdk/shinobi/lcdvms.obj: SDK += -optimize=0
build/sdk/shinobi/mpapi.obj: SDK += -macsave=0 -extra=a=400
build/sdk/shinobi/mpdrv.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/shinobi/njpad.obj: SDK += -optimize=0
build/sdk/shinobi/pd_kbd.obj: SDK += -optimize=0
build/sdk/shinobi/pdmain.obj: SDK += -optimize=0 -macsave=0 -extra=a=400
build/sdk/shinobi/pdvib.obj: SDK += -macsave=0 -extra=a=400
build/sdk/shinobi/rfiber.obj: SDK += -macsave=0 -extra=a=400
build/sdk/shinobi/sd_2nd_host_cmd_init_lst.obj: SDK += -macsave=0
build/sdk/shinobi/sd_2nd_host_cmd_send_lst.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_aica_reset_keep.obj: SDK += -speed
build/sdk/shinobi/sd_aica_reset_rel.obj: SDK += -speed
build/sdk/shinobi/sd_drv_init.obj: SDK += -size
build/sdk/shinobi/sd_memblk_cpu_transfer.obj: SDK += -size
build/sdk/shinobi/sd_memblk_dma_transfer.obj: SDK += -size
build/sdk/shinobi/sd_memblk_get_stat.obj: SDK += -size
build/sdk/shinobi/sd_memblk_que_push.obj: SDK += -size
build/sdk/shinobi/sd_memblk_transfer.obj: SDK += -size
build/sdk/shinobi/sd_midi_get_stat.obj: SDK += -size
build/sdk/shinobi/sd_module_open_port.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_cur_adr.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_pstm_get_err.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_flg.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_fx_lev.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_pan.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_pstm_get_pitch.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_speed.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_total_smp_frame.obj: SDK += -size
build/sdk/shinobi/sd_pstm_get_vol.obj: SDK += -size
build/sdk/shinobi/sd_pstm_open_port.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_pstm_play.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_pstm_set_base_vol.obj: SDK += -macsave=0
build/sdk/shinobi/sd_pstm_set_pan.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_pstm_set_pitch.obj: SDK += -size
build/sdk/shinobi/sd_pstm_set_use_ch.obj: SDK += -macsave=0
build/sdk/shinobi/sd_pstm_stop.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_pstm_stop_all.obj: SDK += -macsave=0
build/sdk/shinobi/sd_set_host_cmd.obj: SDK += -size -macsave=0
build/sdk/shinobi/sd_snd_mem_allocate_bank.obj: SDK += -size
build/sdk/shinobi/sd_snd_mem_get_bank_stat.obj: SDK += -size
build/sdk/shinobi/sd_snd_set_fx_out_prm.obj: SDK += -size
build/sdk/shinobi/sd_snd_set_fx_prg.obj: SDK += -size
build/sdk/shinobi/sg_fhref.obj: SDK += -speed -macsave=0 -extra=a=400
build/sdk/shinobi/sg_flcommon.obj: SDK += -extra=a=400
build/sdk/shinobi/sg_playtime.obj: SDK += -speed -extra=a=400
build/sdk/shinobi/sg_rdalahd.obj: SDK += -extra=a=400
build/sdk/shinobi/sg_syb2.obj: SDK += -optimize=0 -extra=a=400
build/sdk/shinobi/sg_sybt.obj: SDK += -optimize=0
build/sdk/shinobi/sg_sycf.obj: SDK += -optimize=0
build/sdk/shinobi/sg_syfb.obj: SDK += -extra=a=400
build/sdk/shinobi/sg_syrtc.obj: SDK += -macsave=0 -extra=a=400
build/sdk/shinobi/sg_userflag.obj: SDK += -speed -extra=a=400
build/sdk/shinobi/sg_userhis.obj: SDK += -extra=a=400
build/sdk/shinobi/sh4tmr.obj: SDK += -macsave=0 -extra=a=400
build/sdk/shinobi/syCache.obj: SDK += -speed -extra=a=400
build/sdk/shinobi/syChain.obj: SDK += -speed -extra=a=400
build/sdk/shinobi/syIntCtrl.obj: SDK += -speed
build/sdk/shinobi/syMalloc.obj: SDK += -speed -macsave=0 -extra=a=400
build/sdk/shinobi/sy_g2_dma_open.obj: SDK += -macsave=0
build/sdk/shinobi/sy_g2_dma_reset.obj: SDK += -macsave=0
build/sdk/shinobi/sy_g2_dma_resume_all.obj: SDK += -macsave=0
build/sdk/shinobi/sy_g2_dma_suspend_all.obj: SDK += -macsave=0
build/sdk/shinobi/sy_g2_sys_init.obj: SDK += -macsave=0
build/sdk/shinobi/sybtcfg.obj: SDK += -size -extra=a=400
build/sdk/shinobi/vms_rw.obj: SDK += -macsave=0 -extra=a=400
build/sdk/shinobi/vmsapi.obj: SDK += -extra=a=400
build/sdk/shinobi/vmscmd.obj: SDK += -extra=a=400
build/sdk/shinobi/vmsio.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/shinobi/vmsphys.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/shinobi/vmsutil.obj: SDK += -size -extra=a=400
build/sdk/shinobi/ov_main.obj: SDK += -fpu=single -speed -extra=a=c00
build/sdk/sofdec/adx_bsc.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/sofdec/adx_dcd.obj: SDK += -double=float -extra=a=400
build/sdk/sofdec/adx_dcd2.obj: SDK += -macsave=0 -extra=a=400
build/sdk/sofdec/adx_dcda.obj: SDK += -fpu=single -extra=a=400
build/sdk/sofdec/adx_fs.obj: SDK += -size -extra=a=400
build/sdk/sofdec/adx_sjd.obj: SDK += -size -extra=a=400
build/sdk/sofdec/adx_spsd.obj: SDK += -size -extra=a=400
build/sdk/sofdec/adx_stms.obj: SDK += -macsave=0 -extra=a=400
build/sdk/sofdec/adx_tlk2.obj: SDK += -extra=a=400
build/sdk/sofdec/adx_tsvr.obj: SDK += -size -macsave=0 -extra=a=400
build/sdk/sofdec/adx_xpnd.obj: SDK += -macsave=0 -extra=a=400
build/sdk/sofdec/adx_tlk.obj build/stub/sdk/sofdec/adx_tlk.obj: private SDK += -size -macsave=0 -extra=a=400 -double=float
build/sdk/sofdec/mw_rna.obj build/stub/sdk/sofdec/mw_rna.obj: private SDK += -size -macsave=0 -extra=a=400
build/sdk/sofdec/mw_sdcs.obj build/stub/sdk/sofdec/mw_sdcs.obj: private SDK += -size -macsave=0 -extra=a=400
build/sdk/sofdec/sj_mem.obj: SDK += -size -extra=a=400
build/sdk/sofdec/sj_rbf.obj: SDK += -size -extra=a=400
build/src/system/thread.obj: SDK += -speed
build/src/draw/action.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/src/draw/action_link.obj: SDK += -double=float -size -macsave=0 -extra=a=400
build/modules/movie/sdk/cmpint64.obj: SDK += -size
build/modules/movie/sdk/sfd_buf.obj: SDK += -macsave=0
build/modules/movie/sdk/sfd_lib.obj: SDK += -macsave=0
build/modules/movie/sdk/sfd_mpv.obj: SDK += -size -macsave=0
build/modules/movie/sdk/sfd_ply.obj: SDK += -size
build/modules/movie/sdk/sfd_tim.obj: SDK += -macsave=0
build/modules/movie/sdk/sfd_trn.obj: SDK += -size -macsave=0
build/modules/movie/sdk/sfd_vom.obj: SDK += -size
build/modules/movie/sdk/mpv_cmc.obj: SDK += -macsave=0
build/modules/movie/sdk/mpv_err.obj: SDK += -speed
build/modules/movie/sdk/mpv_frm.obj: SDK += -speed
build/modules/movie/sdk/mpv_lib.obj: SDK += -speed -noinline
build/modules/movie/sdk/mpvadec.obj: SDK += -speed -noloop -macsave=0 -extra=a=400
build/modules/movie/sdk/gsc_gl.obj: SDK += -size
build/modules/movie/sdk/gsc_hn.obj: SDK += -size
build/modules/movie/sdk/gsc_if.obj: SDK += -size
build/modules/movie/sdk/mps_dec.obj: SDK += -size
build/modules/movie/sdk/mps_lib.obj: SDK += -macsave=0
build/modules/movie/sdk/mwsfd.obj: SDK += -size -macsave=0 -extra=a=400
build/modules/movie/sdk/mwsfdsub.obj build/stub/modules/movie/sdk/mwsfdsub.obj: private SDK += -size -macsave=0 -extra=a=400
build/modules/movie/sdk/mwmng.obj: SDK += -extra=a=400
build/modules/movie/sdk/mwrnv.obj: SDK += -extra=a=400
build/modules/movie/sdk/rnv_disp.obj: SDK += -size -extra=a=400
build/modules/movie/sdk/rnv_tex.obj: SDK += -extra=a=400
build/modules/movie/sdk/sjrbf_gl.obj: SDK += -size -macsave=0
build/loader/sdk/dma_workaround_for_sh_lnk.obj: SDK += -size -extra=a=400
build/loader/sdk/fballoc_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
build/loader/sdk/gdDec.obj: SDK += -macsave=0 -extra=a=400
build/loader/sdk/gdctl.obj: SDK += -size -extra=a=400
build/loader/sdk/gdfs.obj: SDK += -size -macsave=0 -extra=a=400
build/loader/sdk/gdfsdir.obj: SDK += -size -extra=a=400
build/loader/sdk/gdfshn.obj: SDK += -size -extra=a=400
build/loader/sdk/hwinit_workaround_for_sh_lnk.obj: SDK += -size -extra=a=400
build/loader/sdk/interupt_workaround_for_sh_lnk.obj: SDK += -size -extra=a=400
build/loader/sdk/kmcalbac_workaround_for_sh_lnk.obj: SDK += -size -extra=a=400
build/loader/sdk/kmdevice_workaround_for_sh_lnk.obj: SDK += -extra=a=400
build/loader/sdk/kmframe_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
build/loader/sdk/kmglobal_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
build/loader/sdk/kmimem_workaround_for_sh_lnk.obj: SDK += -extra=a=400
build/loader/sdk/kmsurfac_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
# kmRender's control is set through a cast used as an lvalue, which R10 accepts
# with warning 1022
build/loader/sdk/kmsurfac_workaround_for_sh_lnk.obj: private ADMITTED := 1022 (W) First operand of "=" is not lvalue
build/loader/sdk/kmtex_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
build/loader/sdk/kmvertex_workaround_for_sh_lnk.obj: SDK += -size -extra=a=400
build/loader/sdk/kmvtxbuf_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
build/loader/sdk/kmvtxlst_workaround_for_sh_lnk.obj: SDK += -extra=a=400
build/loader/sdk/tactrl_workaround_for_sh_lnk.obj: SDK += -macsave=0 -extra=a=400
build/loader/sdk/video_workaround_for_sh_lnk.obj: SDK += -size -macsave=0 -extra=a=400
# These runtime members build without -fpu=single, in double precision
build/sdk/runtime/_duchek.obj: RUNTIME := $(SHCFLAGS)
build/sdk/runtime/_log10.obj: RUNTIME := $(SHCFLAGS)
build/sdk/runtime/_unpack.obj: RUNTIME := $(SHCFLAGS)
build/sdk/runtime/_xti.obj: RUNTIME := $(SHCFLAGS)
build/sdk/runtime/_xtostr.obj: RUNTIME := $(SHCFLAGS)
# The runtime members in events/shared/sdk/ take the C runtime's flags
build/events/shared/sdk/%.obj: GAME := $(RUNTIME)
# These SDK members link code, constants, data and bss into PSG, CSG, DSG and
# BSG, and Shinobi's others into SHC's P, C, D and B
build/sdk/ninja/%.obj build/stub/sdk/ninja/%.obj: private SDK += -section=p=PSG,c=CSG,d=DSG,b=BSG
# njTexLoad_ holds its bss in BSG32, so njTexLoad.c's flags are set whole
# here, in place of the -section the other Ninja members add
build/sdk/ninja/njTexLoad.obj build/stub/sdk/ninja/njTexLoad.obj: private SDK := $(SDK) -size -macsave=0 -extra=a=400 -section=p=PSG,c=CSG,d=DSG,b=BSG32
$(KAMUI_C:%.c=build/%.obj) $(KAMUI_C:%.c=build/stub/%.obj): private GAME += -section=p=PSG,c=CSG,d=DSG,b=BSG
build/sdk/sofdec/%.obj build/stub/sdk/sofdec/%.obj: private SDK += -section=p=PSG,c=CSG,d=DSG,b=BSG
SG := _0sbver fm fmcrc gdDec gdctl gdfs gdfsdir gdfshn gdfsif lcd lcdvms \
      njpad pd_kbd pdmain sg_fhref sg_flcommon sg_playtime sg_rdalahd \
      sg_syCbl sg_syb2 sg_sybt sg_sycf sg_syfb sg_syrtc sg_userflag \
      sg_userhis sh4tmr syCache syChain syIntCtrl syMalloc sySq
$(SG:%=build/sdk/shinobi/%.obj): SDK += -section=p=PSG,c=CSG,d=DSG,b=BSG
build/sdk/shinobi/sd_%.obj build/stub/sdk/shinobi/sd_%.obj: private SDK += -section=p=PSG,c=CSG,d=DSG,b=BSG
# A library's handle lies in DSGLH: ov_main.c's handle is all its data, and
# the GD file systems and the movie middleware put theirs there by #pragma
# section LH under d=DSG
build/sdk/shinobi/ov_main.obj: SDK += -section=p=PSG,c=CSG,d=DSGLH,b=BSG
build/modules/movie/sdk/%.obj build/stub/modules/movie/sdk/%.obj: private SDK += -section=p=PSG,c=CSG,d=DSG,b=BSG
build/loader/sdk/%.obj build/stub/loader/sdk/%.obj: private SDK += -section=p=PSG,c=CSG,d=DSG,b=BSG
