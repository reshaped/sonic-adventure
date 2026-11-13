/* the Dreamcast engine's batch kernels, written in SH4ZAM's style. SH4ZAM
 * turns one vector a call, and its dots take their vectors in fixed
 * registers. so a C loop of them reloads a fixed vector, such as the light,
 * each pass. each loop here is inline assembly that holds that vector in its
 * registers. the kernels know no game. a point is three floats, a color a
 * word and a screen five floats.
 *
 * a kernel PREFs ZAM_PREFETCH_AHEAD bytes, two lines, ahead of its reads of
 * an array. so a PREF may reach past the array into other RAM. there it
 * fills a line as a load would. a caller keeps DMA from writing what a PREF
 * can reach while a kernel runs.
 *
 * FPSCR must hold PR and SZ clear, as GCC leaves them. an asm that changes
 * SZ or FR sets it back before it ends. an interrupt taken in between must
 * save and restore FPSCR and both banks */
#ifndef BATCH_H
#define BATCH_H

#include <zam.h>
#include <stdint.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

#define ZAM_PREFETCH_AHEAD 64

/* the screen the kernels project onto. scale_x and scale_y take a view-space
 * x and y at z = 1 to the screen. near is the near plane's z */
typedef struct zam_screen {
    float cx, cy;
    float scale_x, scale_y;
    float near;
} zam_screen_t;

/* project count points, three floats each, by XMTRX onto screen. each point
 * takes 16 bytes at heads: control, its x and y on the screen and its 1/z.
 * these are a tile accelerator vertex's first four words. return how many
 * points lie no farther than screen->near, by the z XMTRX gives them.
 *
 * heads must start on 32 bytes. MOVCA.L claims a line without reading
 * memory. a line that began before a pair would put stale bytes ahead of it.
 * the line before heads and the two lines past them must be free. pass 0
 * writes the line before, and the asm keeps XMTRX's words in the two after.
 * FPSCR must hold DN set and no exception enabled, as the CPU's reset value
 * does. the asm's FTRVs read whatever FR3 and FR7 held before it. its last
 * pass may turn XMTRX's kept words as points and divide by their z.
 *
 * the asm folds the screen's scales into XMTRX. M is XMTRX, and P scales x
 * by scale_x and y by scale_y. the asm turns the points by P * M. a point's
 * x on the screen is its turned x times its 1/z, plus cx. its y is alike,
 * plus cy. P's z row is (0, 0, 1, 0). for finite M and points, P * M's z row
 * is thus M's. so a point's z and 1/z are those M gives. the fold rounds
 * each word of P * M toward zero, by under 2^-23 of it. a point's x on the
 * screen then moves by under 2^-23 * scale_x / z * S. S sums |m * p| for m
 * in M's x row and p the point's matching word. y moves likewise, by scale_y
 * and M's y row. S grows with the point's distance from the points' origin
 * and with the origin's distance from the eye. FTRV's, the product's and the
 * sum's roundings add to the move. for a z past 2^126, DN flushes 1/z to 0,
 * so the point lies at (cx, cy). the asm sets P * M's w row to zero but for
 * near in w. a point loaded with a w of 1 thus turns with near in w. its
 * test of z reads that w. the asm keeps M in the two lines past the heads,
 * and XMTRX holds it again after.
 *
 * each pair of points fills a 32-byte line, which MOVCA.L claims before the
 * pass writes it. pass k first divides for pair k - 1's b and turns pair k.
 * it then writes pair k - 1 into its line and divides for pair k's a. at
 * the start of a pass past pass 0, FR8 and FR9 hold the last pair's a,
 * turned. FR10 holds a's 1/z, whose FDIV is in flight. FR12 and FR13 hold
 * that pair's b, turned, and FR6 and FR7 hold b's z and near. FR14 holds 1,
 * and FR11 and FR15 hold cx and cy. at most two instructions issue a cycle.
 * so each 1/z is read 13 or more cycles after its FDIV. each FDIV issues 11
 * or more cycles after the one before. pass 0 writes the line before heads.
 * the 1 and 0 the asm puts in FR6 and FR7 keep pass 0 from counting a b.
 * the count's half plus one passes run, and the last loads no word past the
 * points. for an odd count it loads the last point as a, from a copy just
 * before M. it then loads b from M's first three words. for an even count it
 * loads both from M's first six. no result keeps what it loads from M. for
 * an even count the asm subtracts the last pass's count of a. for an odd
 * count the tail writes only a. the last line's second half then keeps the
 * copy and what the cache held. NEGC of -1 gives 1 - T, a point's nearness.
 * a pass PREFs ZAM_PREFETCH_AHEAD bytes past its pair's start and reads 24
 * bytes of points, under a line. so its PREFs skip no line */
ALWAYS_INLINE int zam_xmtrx_project_points(const float *points, uint32_t count,
                                           const zam_screen_t *screen,
                                           void *heads, uint32_t control)
{
    const float *from = points;
    const float *by = &screen->cx;
    /* 20 bytes into the line before pass 0's line. each pass leaves to 20
     * bytes into the line it writes */
    uint32_t to = (uint32_t)heads - 64 + 20;
    /* the two lines past the heads, where the asm keeps M */
    uint32_t kept = ((uint32_t)heads + 16 * count + 31) & ~(uint32_t)31;
    uint32_t passes = count >> 1;
    int near_points = 0;
    uint32_t ahead;
    int a_near, b_near;

    /* P goes into the FR bank, and FRCHG trades it for M. FTRV then turns
     * each of M's columns by P. P's w row reaches only P * M's w row, which
     * the asm then sets. so P's FR3 and FR7 need no value. its FR11 and FR15
     * take cx and cy. the loop adds them once the second FRCHG brings P's
     * bank back. a count of 0 branches past it all. after the count's half of
     * passes, the code at label 5 readies the last pass. SUBC leaves -1 in
     * count for an odd count and 0 for an even one. the loop's end and the
     * tail test it. a count of 1 goes to label 5 first. the asm clobbers
     * memory. as a memory operand, the lines it writes would take a register
     * of their own */
    __asm__ volatile(
        "tst %[count],%[count]\n\t"
        "bt 4f\n\t"
        "fmov.s @%[by]+,fr11\n\t"
        "fmov.s @%[by]+,fr15\n\t"
        "fmov.s @%[by]+,fr0\n\t"
        "fmov.s @%[by]+,fr5\n\t"
        "fldi0 fr8\n\t"
        "fldi0 fr9\n\t"
        "fldi0 fr1\n\t"
        "fldi0 fr2\n\t"
        "fldi0 fr4\n\t"
        "fldi0 fr6\n\t"
        "fldi1 fr10\n\t"
        "fldi0 fr12\n\t"
        "fldi0 fr13\n\t"
        "fldi0 fr14\n\t"
        "movca.l %[control],@%[kept]\n\t"
        "add #32,%[kept]\n\t"
        "movca.l %[control],@%[kept]\n\t"
        "add #32,%[kept]\n\t"
        "frchg\n\t"
        "fschg\n\t"
        "fmov dr14,@-%[kept]\n\t"
        "fmov dr12,@-%[kept]\n\t"
        "fmov dr10,@-%[kept]\n\t"
        "fmov dr8,@-%[kept]\n\t"
        "fmov dr6,@-%[kept]\n\t"
        "fmov dr4,@-%[kept]\n\t"
        "fmov dr2,@-%[kept]\n\t"
        "fmov dr0,@-%[kept]\n\t"
        "fschg\n\t"
        "ftrv xmtrx,fv0\n\t"
        "ftrv xmtrx,fv4\n\t"
        "ftrv xmtrx,fv8\n\t"
        "ftrv xmtrx,fv12\n\t"
        "fldi0 fr3\n\t"
        "fldi0 fr7\n\t"
        "fldi0 fr11\n\t"
        "fmov.s @%[by],fr15\n\t"
        "frchg\n\t"
        "fldi1 fr6\n\t"
        "tst %[passes],%[passes]\n\t"
        "bf.s 1f\n\t"
        "fldi0 fr7\n"
        "5:\n\t"
        "shlr %[count]\n\t"
        "subc %[count],%[count]\n\t"
        "mov %[from],%[by]\n\t"
        "mov #1,%[passes]\n\t"
        "bf.s 1f\n\t"
        "mov %[kept],%[from]\n\t"
        "fmov.s @%[by]+,fr0\n\t"
        "fmov.s @%[by]+,fr1\n\t"
        "fmov.s @%[by],fr2\n\t"
        "fmov.s fr2,@-%[from]\n\t"
        "fmov.s fr1,@-%[from]\n\t"
        "fmov.s fr0,@-%[from]\n"
        "1:\n\t"
        "fmov.s @%[from]+,fr0\n\t"
        "fdiv fr6,fr14\n\t"
        "fmov.s @%[from]+,fr1\n\t"
        "mov %[from],%[ahead]\n\t"
        "fmov.s @%[from]+,fr2\n\t"
        "add #12,%[to]\n\t"
        "fldi1 fr3\n\t"
        "fcmp/gt fr7,fr6\n\t"
        "ftrv xmtrx,fv0\n\t"
        "movca.l %[control],@%[to]\n\t"
        "fmov.s @%[from]+,fr4\n\t"
        "fmul fr10,fr8\n\t"
        "fmov.s @%[from]+,fr5\n\t"
        "fmul fr10,fr9\n\t"
        "fmov.s @%[from]+,fr6\n\t"
        "add #16,%[to]\n\t"
        "fadd fr11,fr8\n\t"
        "fldi1 fr7\n\t"
        "fadd fr15,fr9\n\t"
        "add %[past_z],%[ahead]\n\t"
        "ftrv xmtrx,fv4\n\t"
        "fmov.s fr10,@-%[to]\n\t"
        "fldi1 fr10\n\t"
        "negc %[minus_one],%[b_near]\n\t"
        "mov.l %[control],@(4,%[to])\n\t"
        "add %[b_near],%[near_points]\n\t"
        "fdiv fr2,fr10\n\t"
        "fmov.s fr9,@-%[to]\n\t"
        "fcmp/gt fr3,fr2\n\t"
        "fmov fr1,fr9\n\t"
        "fmul fr14,fr12\n\t"
        "fmov.s fr8,@-%[to]\n\t"
        "fmul fr14,fr13\n\t"
        "fmov fr0,fr8\n\t"
        "add #28,%[to]\n\t"
        "fadd fr11,fr12\n\t"
        "pref @%[ahead]\n\t"
        "fadd fr15,fr13\n\t"
        "fmov.s fr14,@-%[to]\n\t"
        "negc %[minus_one],%[a_near]\n\t"
        "fldi1 fr14\n\t"
        "add %[a_near],%[near_points]\n\t"
        "fmov.s fr13,@-%[to]\n\t"
        "dt %[passes]\n\t"
        "fmov.s fr12,@-%[to]\n\t"
        "fmov fr4,fr12\n\t"
        "bf.s 1b\n\t"
        "fmov fr5,fr13\n\t"
        "cmp/pl %[count]\n\t"
        "bt 5b\n\t"
        "tst %[count],%[count]\n\t"
        "bf 2f\n\t"
        "bra 3f\n\t"
        "sub %[a_near],%[near_points]\n"
        "2:\n\t"
        "add #12,%[to]\n\t"
        "movca.l %[control],@%[to]\n\t"
        "add #16,%[to]\n\t"
        "fmul fr10,fr8\n\t"
        "fmul fr10,fr9\n\t"
        "fadd fr11,fr8\n\t"
        "fadd fr15,fr9\n\t"
        "fmov.s fr10,@-%[to]\n\t"
        "fmov.s fr9,@-%[to]\n\t"
        "fmov.s fr8,@-%[to]\n"
        "3:\n\t"
        "fschg\n\t"
        "fmov @%[kept]+,xd0\n\t"
        "fmov @%[kept]+,xd2\n\t"
        "fmov @%[kept]+,xd4\n\t"
        "fmov @%[kept]+,xd6\n\t"
        "fmov @%[kept]+,xd8\n\t"
        "fmov @%[kept]+,xd10\n\t"
        "fmov @%[kept]+,xd12\n\t"
        "fmov @%[kept]+,xd14\n\t"
        "fschg\n"
        "4:"
        : [from] "+r"(from), [to] "+r"(to), [kept] "+r"(kept),
          [passes] "+r"(passes), [count] "+r"(count),
          [near_points] "+r"(near_points), [by] "+r"(by), [ahead] "=&r"(ahead),
          [a_near] "=&r"(a_near), [b_near] "=&r"(b_near)
        : [past_z] "I08"(ZAM_PREFETCH_AHEAD - 8), [minus_one] "r"(-1),
          [control] "z"(control)
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9",
          "fr10", "fr11", "fr12", "fr13", "fr14", "fr15", "t", "memory");
    return near_points;
}

/* the store queues. once QACR0 and QACR1 hold ZAM_SQ_AREA(address), a queue
 * filled at ZAM_SQ_BASE | (address & ZAM_SQ_ADDRESS) goes to address */
#define ZAM_QACR0 (*(volatile uint32_t *)0xff000038)
#define ZAM_QACR1 (*(volatile uint32_t *)0xff00003c)
#define ZAM_SQ_AREA(address) (((address) >> 24) & 0x1c)
#define ZAM_SQ_ADDRESS 0x03ffffff
#define ZAM_SQ_BASE 0xe0000000

/* project count points, three floats each, by XMTRX onto screen, and light
 * each by its normal at normals. each point's 16 bytes go to out through
 * the store queues. they hold its x and y on the screen, 1/z by FDIV and its
 * intensity. x * scale_x * (1/z) + cx gives x on the screen, rounded in that
 * order. y is alike, by scale_y and cy. the intensity is FIPR's dot of light
 * with the normal, given a w of 1. where that dot is not over light's w, the
 * intensity is light's w. return how many points lie no farther than
 * screen->near, by the z XMTRX gives them.
 *
 * out must start on 32 bytes. the call sets QACR0 and QACR1 to out's area.
 * each pair of points fills a store queue, which PREF then sends to its line
 * of out. a lone last point fills only the queue's first half. PREF also
 * sends the queue's stale second half, into the 16 bytes of out past the
 * point. OCBI drops each line from the operand cache unwritten, so that
 * reads of out see memory.
 *
 * a pass of the loop works on a pair, a in FV0 and b in FV4. FV12 holds
 * light and FPUL holds near. FR8 to FR11 hold scale_x, scale_y, cx and cy.
 * the pass FIPRs both normals. it then loads the points over the normals'
 * first three words, which FIPR has read. each dot goes to its word once
 * FIPR gives it. once FTRV has the point, light's w replaces a dot not over
 * it. each FDIV issues once FCMP/GT has tested z against near. ADDC of 0
 * counts a point past it. OCBI drops the pair's line while a divides. the
 * products go down from each point's dot by pre-decrement. the pointers then
 * end at the pair's start and at b's. a pass PREFs ZAM_PREFETCH_AHEAD bytes
 * ahead of the normals and of the points. it reads 24 bytes of each, under a
 * line, so its PREFs skip no line. the asm reads count and zero after it
 * writes its outputs. they are early clobbers so that neither input shares a
 * register with an output */
ALWAYS_INLINE int zam_xmtrx_project_lit_points(const float *points,
                                               const float *normals,
                                               uint32_t count,
                                               const zam_screen_t *screen,
                                               shz_vec4_t light, void *out)
{
    register float lx __asm__("fr12") = light.x;
    register float ly __asm__("fr13") = light.y;
    register float lz __asm__("fr14") = light.z;
    register float lw __asm__("fr15") = light.w;
    uint32_t at = (uint32_t)out;
    /* a's dot's word in its queue, and b's */
    uint32_t a = ((at & ZAM_SQ_ADDRESS) | ZAM_SQ_BASE) + 12;
    uint32_t b = a + 16;
    uint32_t line = at;
    uint32_t normal_ahead = (uint32_t)normals + ZAM_PREFETCH_AHEAD - 24;
    uint32_t point_ahead = (uint32_t)points + ZAM_PREFETCH_AHEAD - 24;
    uint32_t passes = count >> 1;
    uint32_t beyond = 0;
    const float *by = &screen->cx;

    ZAM_QACR0 = ZAM_SQ_AREA(at);
    ZAM_QACR1 = ZAM_SQ_AREA(at);
    __asm__ volatile(
        "fmov.s @%[by]+,fr10\n\t"
        "fmov.s @%[by]+,fr11\n\t"
        "fmov.s @%[by]+,fr8\n\t"
        "fmov.s @%[by]+,fr9\n\t"
        "fmov.s @%[by],fr0\n\t"
        "flds fr0,fpul\n\t"
        "tst %[passes],%[passes]\n\t"
        "bt 4f\n"
        "1:\n\t"
        "fmov.s @%[normal]+,fr0\n\t"
        "fmov.s @%[normal]+,fr1\n\t"
        "fmov.s @%[normal]+,fr2\n\t"
        "fldi1 fr3\n\t"
        "fipr fv12,fv0\n\t"
        "fmov.s @%[normal]+,fr4\n\t"
        "add #24,%[normal_ahead]\n\t"
        "fmov.s @%[normal]+,fr5\n\t"
        "fmov.s @%[normal]+,fr6\n\t"
        "fldi1 fr7\n\t"
        "fipr fv12,fv4\n\t"
        "fmov.s @%[point]+,fr0\n\t"
        "add #24,%[point_ahead]\n\t"
        "fmov.s @%[point]+,fr1\n\t"
        "fmov.s @%[point]+,fr2\n\t"
        "fmov.s fr3,@%[a]\n\t"
        "fcmp/gt fr15,fr3\n\t"
        "fldi1 fr3\n\t"
        "ftrv xmtrx,fv0\n\t"
        "fmov.s @%[point]+,fr4\n\t"
        "fmov.s @%[point]+,fr5\n\t"
        "fmov.s @%[point]+,fr6\n\t"
        "bt 2f\n\t"
        "fmov.s fr15,@%[a]\n"
        "2:\n\t"
        "fmov.s fr7,@%[b]\n\t"
        "fcmp/gt fr15,fr7\n\t"
        "fldi1 fr7\n\t"
        "ftrv xmtrx,fv4\n\t"
        "pref @%[normal_ahead]\n\t"
        "fsts fpul,fr3\n\t"
        "fmul fr8,fr0\n\t"
        "pref @%[point_ahead]\n\t"
        "bt 3f\n\t"
        "fmov.s fr15,@%[b]\n"
        "3:\n\t"
        "fcmp/gt fr3,fr2\n\t"
        "fldi1 fr3\n\t"
        "fmul fr9,fr1\n\t"
        "fsts fpul,fr7\n\t"
        "fdiv fr2,fr3\n\t"
        "ocbi @%[line]\n\t"
        "fmul fr8,fr4\n\t"
        "addc %[zero],%[beyond]\n\t"
        "fcmp/gt fr7,fr6\n\t"
        "fldi1 fr7\n\t"
        "fmul fr9,fr5\n\t"
        "add #32,%[line]\n\t"
        "fdiv fr6,fr7\n\t"
        "addc %[zero],%[beyond]\n\t"
        "dt %[passes]\n\t"
        "fmul fr3,fr0\n\t"
        "fmov.s fr3,@-%[a]\n\t"
        "fmul fr3,fr1\n\t"
        "fadd fr10,fr0\n\t"
        "fmul fr7,fr4\n\t"
        "fadd fr11,fr1\n\t"
        "fmov.s fr7,@-%[b]\n\t"
        "fmul fr7,fr5\n\t"
        "fadd fr10,fr4\n\t"
        "fmov.s fr1,@-%[a]\n\t"
        "fadd fr11,fr5\n\t"
        "fmov.s fr0,@-%[a]\n\t"
        "fmov.s fr5,@-%[b]\n\t"
        "fmov.s fr4,@-%[b]\n\t"
        "pref @%[a]\n\t"
        "add #44,%[a]\n\t"
        "bf/s 1b\n\t"
        "add #44,%[b]\n"
        "4:\n\t"
        "mov %[count],%[passes]\n\t"
        "shlr %[passes]\n\t"
        "bf 6f\n\t"
        "fmov.s @%[normal]+,fr0\n\t"
        "fmov.s @%[normal]+,fr1\n\t"
        "fmov.s @%[normal],fr2\n\t"
        "fldi1 fr3\n\t"
        "fipr fv12,fv0\n\t"
        "fmov.s @%[point]+,fr0\n\t"
        "fmov.s @%[point]+,fr1\n\t"
        "fmov.s @%[point],fr2\n\t"
        "fmov.s fr3,@%[a]\n\t"
        "fcmp/gt fr15,fr3\n\t"
        "fldi1 fr3\n\t"
        "ftrv xmtrx,fv0\n\t"
        "bt 5f\n\t"
        "fmov.s fr15,@%[a]\n"
        "5:\n\t"
        "fsts fpul,fr3\n\t"
        "fcmp/gt fr3,fr2\n\t"
        "fmul fr8,fr0\n\t"
        "fldi1 fr3\n\t"
        "fmul fr9,fr1\n\t"
        "fdiv fr2,fr3\n\t"
        "addc %[zero],%[beyond]\n\t"
        "ocbi @%[line]\n\t"
        "fmul fr3,fr0\n\t"
        "fmov.s fr3,@-%[a]\n\t"
        "fmul fr3,fr1\n\t"
        "fadd fr10,fr0\n\t"
        "fadd fr11,fr1\n\t"
        "fmov.s fr1,@-%[a]\n\t"
        "fmov.s fr0,@-%[a]\n\t"
        "pref @%[a]\n"
        "6:"
        : [normal] "+&r"(normals), [point] "+&r"(points), [a] "+&r"(a),
          [b] "+&r"(b), [line] "+&r"(line), [normal_ahead] "+&r"(normal_ahead),
          [point_ahead] "+&r"(point_ahead), [passes] "+&r"(passes),
          [beyond] "+&r"(beyond), [by] "+&r"(by)
        : [count] "r"(count), [zero] "r"(0), "f"(lx), "f"(ly), "f"(lz), "f"(lw)
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9",
          "fr10", "fr11", "fpul", "t", "memory");
    return (int)(count - beyond);
}

/* the dots of the vector at v with XMTRX's first three columns. v is three
 * floats, with a w of 0. the result is XMTRX's transpose times v. its dot
 * with a direction is v's dot with that direction as XMTRX turns it. the asm
 * moves the columns out of XMTRX into FV8, FV0 and FV12 by six FMOV.D.
 * each FIPR then takes the lanes shz_vec4_dot3 gives it, column k's word i
 * in lane i. v comes by address, so its words load straight into FV4 */
ALWAYS_INLINE shz_vec3_t zam_xmtrx_transform_vec3_transpose(const float *v)
{
    register float vx __asm__("fr4") = v[0];
    register float vy __asm__("fr5") = v[1];
    register float vz __asm__("fr6") = v[2];
    register float vw __asm__("fr7") = 0.0F;
    register float x __asm__("fr11");
    register float y __asm__("fr3");
    register float z __asm__("fr15");

    __asm__ volatile("fschg\n\t"
                     "fmov xd0,dr8\n\t"
                     "fmov xd4,dr0\n\t"
                     "fmov xd8,dr12\n\t"
                     "fmov xd2,dr10\n\t"
                     "fmov xd6,dr2\n\t"
                     "fmov xd10,dr14\n\t"
                     "fschg\n\t"
                     "fipr fv4,fv8\n\t"
                     "fipr fv4,fv0\n\t"
                     "fipr fv4,fv12"
                     : "=f"(x), "=f"(y), "=f"(z)
                     : "f"(vx), "f"(vy), "f"(vz), "f"(vw)
                     : "fr0", "fr1", "fr2", "fr8", "fr9", "fr10", "fr12",
                       "fr13", "fr14");
    return shz_vec3_init(x, y, z);
}

/* v's dot with each of XMTRX's four columns: XMTRX's transpose times v. the
 * result's dot with a point, given a w of 1, is v's dot with the turned
 * point. the asm moves each column out of XMTRX into FV8 or FV0 by two
 * FMOV.D. it FIPRs each with v in FV4, in
 * zam_xmtrx_transform_vec3_transpose's lanes. FV8 takes the first, third and
 * last columns, and each result leaves FR11 before the next. the results end
 * in FV4 in v's place. so FV12, which the ABI has each function keep for its
 * caller, goes unused */
ALWAYS_INLINE shz_vec4_t zam_xmtrx_transform_vec4_transpose(shz_vec4_t v)
{
    register float x __asm__("fr4") = v.x;
    register float y __asm__("fr5") = v.y;
    register float z __asm__("fr6") = v.z;
    register float w __asm__("fr7") = v.w;

    __asm__ volatile("fschg\n\t"
                     "fmov xd0,dr8\n\t"
                     "fmov xd2,dr10\n\t"
                     "fmov xd4,dr0\n\t"
                     "fmov xd6,dr2\n\t"
                     "fschg\n\t"
                     "fipr fv4,fv8\n\t"
                     "fipr fv4,fv0\n\t"
                     "fmov fr11,fr0\n\t"
                     "fschg\n\t"
                     "fmov xd8,dr8\n\t"
                     "fmov xd10,dr10\n\t"
                     "fschg\n\t"
                     "fipr fv4,fv8\n\t"
                     "fmov fr11,fr1\n\t"
                     "fschg\n\t"
                     "fmov xd12,dr8\n\t"
                     "fmov xd14,dr10\n\t"
                     "fschg\n\t"
                     "fipr fv4,fv8\n\t"
                     "fmov fr0,fr4\n\t"
                     "fmov fr3,fr5\n\t"
                     "fmov fr1,fr6\n\t"
                     "fmov fr11,fr7"
                     : "+f"(x), "+f"(y), "+f"(z), "+f"(w)
                     :
                     : "fr0", "fr1", "fr2", "fr3", "fr8", "fr9", "fr10",
                       "fr11");
    return shz_vec4_init(x, y, z, w);
}

/* whether any of count points at points dots v over bound. each is three
 * floats, with a w of 1. a NaN dot is not over it. the loop leaves at the
 * first point whose dot FCMP/GT finds over bound. where none is, CMP/HI of
 * the end leaves T clear. a pass PREFs once, ZAM_PREFETCH_AHEAD bytes past
 * the point it loads, and reads 12 bytes. so its PREFs skip no line */
ALWAYS_INLINE bool zam_vec4_dot_any_above(shz_vec4_t v, const float *points,
                                          uint32_t count, float bound)
{
    register float vx __asm__("fr4") = v.x;
    register float vy __asm__("fr5") = v.y;
    register float vz __asm__("fr6") = v.z;
    register float vw __asm__("fr7") = v.w;
    const float *end = points + 3 * count;
    uint32_t ahead = (uint32_t)points + ZAM_PREFETCH_AHEAD;
    bool above;

    __asm__("cmp/hi %[from],%[end]\n\t"
            "bf 2f\n"
            "1:\n\t"
            "pref @%[ahead]\n\t"
            "fmov.s @%[from]+,fr8\n\t"
            "fmov.s @%[from]+,fr9\n\t"
            "fmov.s @%[from]+,fr10\n\t"
            "fldi1 fr11\n\t"
            "fipr fv4,fv8\n\t"
            "add #12,%[ahead]\n\t"
            "fcmp/gt %[bound],fr11\n\t"
            "bt 2f\n\t"
            "cmp/hi %[from],%[end]\n\t"
            "bt 1b\n"
            "2:\n\t"
            "movt %[above]"
            : [from] "+r"(points), [ahead] "+r"(ahead), [above] "=r"(above)
            : [end] "r"(end), [bound] "f"(bound), "f"(vx), "f"(vy), "f"(vz),
              "f"(vw), "m"(*(const float (*)[])points)
            : "fr8", "fr9", "fr10", "fr11", "t");
    return above;
}

/* zam_vec4_dot_palette_pairs' asm to hold entries a and b to the palette,
 * then branch to label back */
#define ZAM_HOLD_PAIR(back)                                                    \
    "cmp/pz %[a]\n\t"                                                          \
    "bt 1f\n\t"                                                                \
    "mov #0,%[a]\n"                                                            \
    "1:\n\t"                                                                   \
    "cmp/gt %[last],%[a]\n\t"                                                  \
    "bf 2f\n\t"                                                                \
    "mov %[last],%[a]\n"                                                       \
    "2:\n\t"                                                                   \
    "cmp/pz %[b]\n\t"                                                          \
    "bt 3f\n\t"                                                                \
    "mov #0,%[b]\n"                                                            \
    "3:\n\t"                                                                   \
    "cmp/gt %[last],%[b]\n\t"                                                  \
    "bf " back "\n\t"                                                          \
    "bra " back "\n\t"                                                         \
    "mov %[last],%[b]\n"

/* light 2 * pairs points by a palette of entries 0 to entries - 1. pairs is
 * at least 1, and light's w must be 0. each normal at *normals is three
 * floats, loaded with a w of 1. its dot with light picks entry
 * (int)(dot * half + half). half is (entries - 1) / 2. the entry is held to
 * the palette. a dot of -1 picks entry 0, and 1 the last. an entry's base
 * color is base[2 * entry], and its offset color is offsets[2 * entry].
 * each point's two go to *colors, base then offset. the call steps *normals
 * and *colors past the points.
 *
 * a pass of the loop works on three pairs. it scales, converts and tests the
 * pair whose FIPRs the last pass issued. it looks up and stores the colors
 * of the pair it converted the pass before. it FIPRs the next pair's normals
 * with light, which FV4 holds throughout. the code before the loop begins
 * the first two pairs. the code after it ends the last two. a single pair is
 * begun twice, so that no normal past the points' is read, and ended once.
 * (int)(dot * half + half) is FMUL and FADD by half, then FTRC.
 * (uint32_t)(a | b) > entries - 1 holds for any pair with an entry outside
 * the palette. the test reads the entries before they scale by 8 into byte
 * offsets. where it holds, the pair branches to a copy of ZAM_HOLD_PAIR past
 * the end, and back. a pass PREFs once, ZAM_PREFETCH_AHEAD bytes past the
 * pair it loads. a pair's normals take 24 bytes, under a line, so the PREFs
 * skip no line */
ALWAYS_INLINE void
zam_vec4_dot_palette_pairs(shz_vec4_t light, const float **normals,
                           uint32_t **colors, int pairs, const uint32_t *base,
                           const uint32_t *offsets, int entries)
{
    register float lx __asm__("fr4") = light.x;
    register float ly __asm__("fr5") = light.y;
    register float lz __asm__("fr6") = light.z;
    register float lw __asm__("fr7") = light.w;
    /* the loop's passes, one for each pair after the first two, or -1 for a
     * single pair */
    int passes = pairs - 2;
    uint32_t ahead;
    int a, b, k;
    uint32_t color, offset;

    __asm__(
        "mov %[normal],%[ahead]\n\t"
        "add %[ahead_bytes],%[ahead]\n\t"
        "pref @%[ahead]\n\t"
        "fmov.s @%[normal]+,fr8\n\t"
        "fmov.s @%[normal]+,fr9\n\t"
        "fmov.s @%[normal]+,fr10\n\t"
        "fmov %[one],fr11\n\t"
        "fipr fv4,fv8\n\t"
        "fmov.s @%[normal]+,fr0\n\t"
        "fmov.s @%[normal]+,fr1\n\t"
        "fmov.s @%[normal]+,fr2\n\t"
        "fmov %[one],fr3\n\t"
        "fipr fv4,fv0\n\t"
        "cmp/pz %[passes]\n\t"
        "bt .Lsecond%=\n\t"
        "add #-24,%[normal]\n\t"
        ".Lsecond%=:\n\t"
        "fmul %[half],fr11\n\t"
        "fmov.s @%[normal]+,fr8\n\t"
        "fmul %[half],fr3\n\t"
        "fmov.s @%[normal]+,fr9\n\t"
        "fadd %[half],fr11\n\t"
        "fmov.s @%[normal]+,fr10\n\t"
        "fadd %[half],fr3\n\t"
        "fmov.s @%[normal]+,fr0\n\t"
        "ftrc fr11,fpul\n\t"
        "fmov %[one],fr11\n\t"
        "fmov.s @%[normal]+,fr1\n\t"
        "add #24,%[ahead]\n\t"
        "sts fpul,%[a]\n\t"
        "ftrc fr3,fpul\n\t"
        "fipr fv4,fv8\n\t"
        "fmov %[one],fr3\n\t"
        "fmov.s @%[normal]+,fr2\n\t"
        "sts fpul,%[b]\n\t"
        "mov %[a],%[color]\n\t"
        "fipr fv4,fv0\n\t"
        "or %[b],%[color]\n\t"
        "pref @%[ahead]\n\t"
        "cmp/hi %[last],%[color]\n\t"
        "bt .Lhold_begun%=\n\t"
        ".Lbegun%=:\n\t"
        "cmp/pz %[passes]\n\t"
        "bf .Llast%=\n\t"
        "tst %[passes],%[passes]\n\t"
        "bt .Lend%=\n\t"
        ".Lpass%=:\n\t"
        "fmul %[half],fr11\n\t"
        "mov %[a],%[k]\n\t"
        "shll2 %[k]\n\t"
        "fmov.s @%[normal]+,fr8\n\t"
        "fmul %[half],fr3\n\t"
        "fmov.s @%[normal]+,fr9\n\t"
        "add %[k],%[k]\n\t"
        "mov.l @(%[k],%[base]),%[color]\n\t"
        "fadd %[half],fr11\n\t"
        "mov.l @(%[k],%[offsets]),%[offset]\n\t"
        "mov %[b],%[k]\n\t"
        "mov.l %[color],@%[lit]\n\t"
        "fmov.s @%[normal]+,fr10\n\t"
        "fadd %[half],fr3\n\t"
        "fmov.s @%[normal]+,fr0\n\t"
        "ftrc fr11,fpul\n\t"
        "fmov %[one],fr11\n\t"
        "fmov.s @%[normal]+,fr1\n\t"
        "shll2 %[k]\n\t"
        "add #24,%[ahead]\n\t"
        "add %[k],%[k]\n\t"
        "sts fpul,%[a]\n\t"
        "ftrc fr3,fpul\n\t"
        "mov.l @(%[k],%[base]),%[color]\n\t"
        "fipr fv4,fv8\n\t"
        "fmov %[one],fr3\n\t"
        "fmov.s @%[normal]+,fr2\n\t"
        "mov.l %[color],@(8,%[lit])\n\t"
        "sts fpul,%[b]\n\t"
        "mov.l %[offset],@(4,%[lit])\n\t"
        "mov %[a],%[color]\n\t"
        "fipr fv4,fv0\n\t"
        "or %[b],%[color]\n\t"
        "pref @%[ahead]\n\t"
        "cmp/hi %[last],%[color]\n\t"
        "mov.l @(%[k],%[offsets]),%[offset]\n\t"
        "bt .Lhold_passed%=\n\t"
        ".Lpassed%=:\n\t"
        "dt %[passes]\n\t"
        "mov.l %[offset],@(12,%[lit])\n\t"
        "bf.s .Lpass%=\n\t"
        "add #16,%[lit]\n\t"
        ".Lend%=:\n\t"
        "fmul %[half],fr11\n\t"
        "mov %[a],%[k]\n\t"
        "shll2 %[k]\n\t"
        "fmul %[half],fr3\n\t"
        "add %[k],%[k]\n\t"
        "mov.l @(%[k],%[base]),%[color]\n\t"
        "fadd %[half],fr11\n\t"
        "mov.l @(%[k],%[offsets]),%[offset]\n\t"
        "mov %[b],%[k]\n\t"
        "mov.l %[color],@%[lit]\n\t"
        "fadd %[half],fr3\n\t"
        "ftrc fr11,fpul\n\t"
        "shll2 %[k]\n\t"
        "add %[k],%[k]\n\t"
        "sts fpul,%[a]\n\t"
        "ftrc fr3,fpul\n\t"
        "mov.l @(%[k],%[base]),%[color]\n\t"
        "mov.l %[color],@(8,%[lit])\n\t"
        "sts fpul,%[b]\n\t"
        "mov.l %[offset],@(4,%[lit])\n\t"
        "mov %[a],%[color]\n\t"
        "or %[b],%[color]\n\t"
        "cmp/hi %[last],%[color]\n\t"
        "mov.l @(%[k],%[offsets]),%[offset]\n\t"
        "bt .Lhold_ended%=\n\t"
        ".Lended%=:\n\t"
        "mov.l %[offset],@(12,%[lit])\n\t"
        "add #16,%[lit]\n\t"
        ".Llast%=:\n\t"
        "mov %[a],%[k]\n\t"
        "shll2 %[k]\n\t"
        "add %[k],%[k]\n\t"
        "mov.l @(%[k],%[base]),%[color]\n\t"
        "mov.l @(%[k],%[offsets]),%[offset]\n\t"
        "mov %[b],%[k]\n\t"
        "shll2 %[k]\n\t"
        "mov.l %[color],@%[lit]\n\t"
        "add %[k],%[k]\n\t"
        "mov.l %[offset],@(4,%[lit])\n\t"
        "mov.l @(%[k],%[base]),%[color]\n\t"
        "mov.l @(%[k],%[offsets]),%[offset]\n\t"
        "mov.l %[color],@(8,%[lit])\n\t"
        "mov.l %[offset],@(12,%[lit])\n\t"
        "bra .Ldone%=\n\t"
        "add #16,%[lit]\n\t"
        /* clang-format off */
        ".Lhold_begun%=:\n\t" ZAM_HOLD_PAIR(".Lbegun%=")
        ".Lhold_passed%=:\n\t" ZAM_HOLD_PAIR(".Lpassed%=")
        ".Lhold_ended%=:\n\t" ZAM_HOLD_PAIR(".Lended%=")
        /* clang-format on */
        ".Ldone%=:"
        : [normal] "+r"(*normals), [lit] "+r"(*colors), [passes] "+r"(passes),
          [ahead] "=&r"(ahead), [a] "=&r"(a), [b] "=&r"(b),
          [color] "=&r"(color), [offset] "=&r"(offset), [k] "=&z"(k),
          "=m"(*(uint32_t (*)[])(*colors))
        : [base] "r"(base), [offsets] "r"(offsets), [last] "r"(entries - 1),
          [half] "f"((float)(entries - 1) * 0.5F), [one] "f"(1.0F),
          [ahead_bytes] "I08"(ZAM_PREFETCH_AHEAD), "f"(lx), "f"(ly), "f"(lz),
          "f"(lw), "m"(*(const float (*)[])(*normals)),
          "m"(*(const uint32_t (*)[])base), "m"(*(const uint32_t (*)[])offsets)
        : "fr0", "fr1", "fr2", "fr3", "fr8", "fr9", "fr10", "fr11", "fpul",
          "t");
}

#undef ZAM_HOLD_PAIR

/* give 2 * pairs points, pairs at least 1, their colors at colors. each
 * point's entry at entries is a signed byte, its distance from the entry at
 * middle. an entry's base and offset colors lie side by side, at middle plus
 * the distance times 8 bytes. they move as one 8-byte FMOV, for which the
 * asm sets FPSCR.SZ.
 *
 * a pass of the loop stores from DR0 and DR2 the pair the pass before
 * loaded. it loads its own pair's colors by the entries the pass before
 * read. it then reads the next pair's entries, a's by way of next. the code
 * before the loop begins the first pair, and the code after it stores the
 * last. the loop reads two entries past the last pair's. each pair's colors
 * go down from the end of its 16 bytes, by pre-decrement. the asm clobbers
 * memory. as a memory operand, the palette would take a register for its
 * address, 1024 bytes below middle */
ALWAYS_INLINE void zam_palette_pairs(const int8_t *entries, uint32_t *colors,
                                     int pairs, const uint32_t *middle)
{
    int a, b, next;

    __asm__ volatile(
        "mov.b @%[entry]+,%[a]\n\t"
        "mov.b @%[entry]+,%[b]\n\t"
        "fschg\n\t"
        "add #16,%[lit]\n\t"
        "shad %[three],%[a]\n\t"
        "shad %[three],%[b]\n\t"
        "add %[middle],%[a]\n\t"
        "add %[middle],%[b]\n\t"
        "fmov @%[a],dr0\n\t"
        "fmov @%[b],dr2\n\t"
        "mov.b @%[entry]+,%[a]\n\t"
        "dt %[pairs]\n\t"
        "bt.s 2f\n\t"
        "mov.b @%[entry]+,%[b]\n"
        "1:\n\t"
        "shad %[three],%[a]\n\t"
        "fmov dr2,@-%[lit]\n\t"
        "shad %[three],%[b]\n\t"
        "fmov dr0,@-%[lit]\n\t"
        "add %[middle],%[a]\n\t"
        "mov.b @%[entry]+,%[next]\n\t"
        "add %[middle],%[b]\n\t"
        "fmov @%[a],dr0\n\t"
        "dt %[pairs]\n\t"
        "fmov @%[b],dr2\n\t"
        "mov %[next],%[a]\n\t"
        "mov.b @%[entry]+,%[b]\n\t"
        "bf.s 1b\n\t"
        "add #32,%[lit]\n"
        "2:\n\t"
        "fmov dr2,@-%[lit]\n\t"
        "fmov dr0,@-%[lit]\n\t"
        "fschg"
        : [entry] "+r"(entries), [lit] "+r"(colors), [pairs] "+r"(pairs),
          [a] "=&r"(a), [b] "=&r"(b), [next] "=&r"(next)
        : [middle] "r"(middle), [three] "r"(3)
        : "fr0", "fr1", "fr2", "fr3", "t", "memory");
}

#endif
