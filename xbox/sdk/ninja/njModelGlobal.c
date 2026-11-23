/* Ninja's model workspace, sdk/ninja/njModelGlobal.src's: 4768 bytes of bss,
 * aligned to 32, each label where the .src has it. make pc compiles this in
 * place of the .src, which it can't assemble.
 *
 * without it, tools/pclink.py took the workspace's names for functions
 * nothing defined and gave each a stub of 16 bytes in 1ST_READ.BIN's code.
 * njModel.c's clippers then built their corners there: vtx2's four, 64
 * bytes each, ran over vtx4's stub and on past image_code_end into the
 * .text_last functions after it, and a frame later present_sprite ran
 * a corner's packet as code (a model clipped at the near plane in Lost
 * World, on a console).
 *
 * labels on one block, as _njm_inten, _njm_col2 and _njm_col are, are
 * assembly's: no C object gives three names one address. so the block is
 * top-level assembly, in .bss, its labels the C names (no leading
 * underscore on x86's ELF) */
__asm__(".bss\n"
        ".balign 32\n"
        /* 2080 bytes of no known type, each */
        ".globl polyvtx\n"
        "polyvtx:\n"
        "    .space 2080\n"
        ".globl texvtx\n"
        "texvtx:\n"
        "    .space 2080\n"
        /* struct clip_vertex vtx2[4], 64 bytes each: vtx2[2] and vtx2[3]
         * run on through vtx3 */
        ".globl vtx2\n"
        "vtx2:\n"
        "    .space 128\n"
        ".globl vtx3\n"
        "vtx3:\n"
        "    .space 128\n"
        /* struct color_vertex vtx4[4], 32 bytes each */
        ".globl vtx4\n"
        "vtx4:\n"
        "    .space 128\n"
        /* 128 bytes of no known type */
        ".globl vtxbuf\n"
        "vtxbuf:\n"
        "    .space 128\n"
        /* one 96-byte block under three names: _njm_inten, of no known type;
         * NJS_ARGB _njm_col2[2], one vertex's lit colors; and NJS_ARGB
         * _njm_col[3][2], a triangle's three corners' */
        ".globl _njm_inten\n"
        ".globl _njm_col2\n"
        ".globl _njm_col\n"
        "_njm_inten:\n"
        "_njm_col2:\n"
        "_njm_col:\n"
        "    .space 96\n"
        ".text\n");
