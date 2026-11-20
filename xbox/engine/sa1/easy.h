/* the Xbox port's SA1 easy models: engine/dc/sa1/easy.h's walk, into a run
 * of the renderer's for each meshset where one takes it (sa1/emit.h), and
 * else into Kamui's list as the Dreamcast's. the walk's kernel is
 * xbox/engine/batch.h's. the walk reads Ninja's words by their SH-4 names,
 * which tools/pclink.py binds to the C names: Clang takes an asm label only
 * ahead of a name's first use, so they are declared here, before
 * sa1/header.h's uses, with the labels sdk/ninja/njModelE.c gives them */
#ifndef XBOX_SA1_EASY_H
#define XBOX_SA1_EASY_H

#include <kamui.h>
#include <ninja.h>

#include "sdk/ninja/njkamui.h"

extern Uint32 sa1_live_control __asm__("__nj_control_3d_flag_");
extern Uint32 sa1_live_attr_and __asm__("__nj_constant_attr_and_");
extern Uint32 sa1_live_attr_or __asm__("__nj_constant_attr_or_");
extern struct nj_parameterkm sa1_live_header __asm__("__nj_parameterkm_");
extern Uint32 sa1_live_texture __asm__("__nj_current_texture");
extern Uint32 sa1_live_texture_bank __asm__("__nj_current_texture_bank");
extern NJS_TEXLIST *sa1_live_texlist __asm__("__nj_current_texlist");
extern Uint32 sa1_live_sq_base __asm__("__nj_sq_base_");
extern Uint32 **sa1_live_list __asm__("__nj_listtype_");
extern Uint32 *sa1_live_color_mode __asm__("__nj_color_mode_");

#include "sa1/emit.h"
#include "../../../engine/dc/sa1/easy.h"

#endif
