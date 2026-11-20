/* the Dreamcast's crash screen, as crash.h describes it. it draws 3 by 5
 * glyphs, each dot SCALE pixels square, into the frame buffer FB_R_SOF1 names.
 * it calls nothing of the renderer's and divides nothing, so an exception or
 * the vblank interrupt can run it whatever the game was doing.
 *
 * the screen's lines, top to bottom. CRASH and the exception's code, HANG and
 * the vblanks since the last frame, or port_stop's words and number, with
 * the name it was given on a line of its own. PC, the instruction that
 * faulted, that the vblank stopped or that port_stop returns to. a crash's
 * PR, where its function returns to. SR, the status register there. SP,
 * the stopped code's stack pointer, and
 * ADDR, the address a faulting access asked for. MODE and STAGE, the game's
 * mode, and its stage and act as stage_get_stage_act gives them. a crash
 * then shows R0 to R14, four to a line. last come the callers: each word of
 * the stopped code's stack that addresses the instruction after a call's
 * delay slot, behind its offset from SP, nearest first. they are what the
 * stack holds, not an unwinding: a word a returned function left there shows
 * too. make dc's symbols for the build that drew the screen turn each
 * address into a name */
#include "engine/dc/crash.h"

#include <machine.h>
#include <sg_chain.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "engine/dc/dreamcast.h"
#include "main.h"
#include "stage/stage.h"

/* the PowerVR's registers that say which buffer the screen reads and how
 * deep its pixels are, and where video memory lies for 32-bit access. video
 * memory takes no 8-bit write, so the screen is written a word at a time */
#define REG(address) (*(volatile Uint32 *)(address))
#define FB_R_CTRL REG(0xa05f8044)
#define FB_R_SOF1 REG(0xa05f8050)
#define FB_R_SOF2 REG(0xa05f8054)
#define FB_DEPTH_SHIFT 2
#define FB_DEPTH_MASK 3
#define VIDEO_MEMORY 0xa5000000
#define VIDEO_OFFSET 0x007ffffc
/* the CPU's registers for the exception it last took: its code, and the
 * address an address error's access asked for */
#define EXPEVT_AT 0xff000024
#define EXPEVT REG(EXPEVT_AT)
#define TEA REG(0xff00000c)

/* SR's bits that block exceptions and interrupts and select the second
 * register bank, and its interrupt mask */
#define SR_BL 0x10000000
#define SR_RB 0x20000000
#define SR_IMASK 0xf0

/* the exceptions the screen takes: an address error on a read and on a
 * write, and an illegal instruction, alone and in a delay slot. the entry
 * below tests each as its code over CODE_STEP */
#define CODE_ADDRESS_READ 0x0e0
#define CODE_ADDRESS_WRITE 0x100
#define CODE_ILLEGAL 0x180
#define CODE_ILLEGAL_SLOT 0x1a0
#define CODE_STEP 0x20
/* where the CPU enters a general exception, from VBR */
#define GENERAL_VECTOR 0x100

/* the picture is WIDTH pixels across. the text starts inside the title-safe
 * area, in cells of a glyph and a dot's gap */
#define WIDTH 640
#define LEFT 40
#define TOP 44
#define SCALE 3
#define GLYPH_W 3
#define GLYPH_H 5
#define CELL_W ((GLYPH_W + 1) * SCALE)
#define CELL_H ((GLYPH_H + 1) * SCALE)
#define COLUMNS 44
/* the most bytes a pixel takes */
#define PIXEL_BYTES 4

/* the vblanks with no frame and no picture that make a hang, half a minute
 * at 60 Hz, and the vblanks between its writes after that. a load takes
 * less than half a minute unless a read stalls or slows to a crawl. GDFS
 * times a read out after 20 seconds without a sector. stopping the read can
 * take 40 seconds more. the screen can then show over a load that recovers,
 * until the game's next picture */
#define CRASH_HANG 1800
#define HANG_AGAIN 30

/* main memory: MEMORY_SIZE bytes from MEMORY, in each view. an address reads
 * it where its low VIEW_SHIFT bits fall there and it lies under
 * VIEWS_END, past which the CPU has no view of it. the game's code runs in
 * two of the views, CODE_VIEW and CODE_VIEW_LOW, and a module loads through
 * the lower one */
#define MEMORY 0x0c000000
#define MEMORY_SIZE 0x01000000
#define VIEW_SHIFT 29
#define VIEWS_END 0xc0000000
#define CODE_VIEW 0x8c
#define CODE_VIEW_LOW 0x0c
#define CODE_VIEW_SHIFT 24
/* the words of the stopped code's stack the screen reads, and the callers
 * it shows, three to a line */
#define STACK_WORDS 512
#define CALLERS_SHOWN 24
#define CALLERS_A_LINE 3
/* a call returns CALL_RETURN bytes past its instruction, after the delay
 * slot. the calls are JSR @Rn, BSRF Rn and BSR */
#define CALL_RETURN 4
#define IS_JSR(op) (((op) & 0xf0ff) == 0x400b)
#define IS_BSRF(op) (((op) & 0xf0ff) == 0x0003)
#define IS_BSR(op) (((op) & 0xf000) == 0xb000)

/* the words of the stack a report runs on */
#define OWN_STACK_WORDS 1024

/* each glyph's rows from the top, three bits a row, the left dot the high
 * bit */
#define G(a, b, c, d, e)                                                       \
    (Uint16)((a) << 12 | (b) << 9 | (c) << 6 | (d) << 3 | (e))
static const Uint16 digits[10] = {
    G(7, 5, 5, 5, 7), G(2, 6, 2, 2, 7), G(7, 1, 7, 4, 7), G(7, 1, 7, 1, 7),
    G(5, 5, 7, 1, 1), G(7, 4, 7, 1, 7), G(7, 4, 7, 5, 7), G(7, 1, 1, 1, 1),
    G(7, 5, 7, 5, 7), G(7, 5, 7, 1, 7),
};
static const Uint16 letters[26] = {
    G(7, 5, 7, 5, 5), G(6, 5, 6, 5, 6), G(7, 4, 4, 4, 7), G(6, 5, 5, 5, 6),
    G(7, 4, 7, 4, 7), G(7, 4, 7, 4, 4), G(7, 4, 5, 5, 7), G(5, 5, 7, 5, 5),
    G(7, 2, 2, 2, 7), G(1, 1, 1, 5, 7), G(5, 5, 6, 5, 5), G(4, 4, 4, 4, 7),
    G(5, 7, 7, 5, 5), G(7, 5, 5, 5, 5), G(7, 5, 5, 5, 7), G(7, 5, 7, 4, 4),
    G(7, 5, 5, 7, 1), G(7, 5, 6, 5, 5), G(7, 4, 7, 1, 7), G(7, 2, 2, 2, 2),
    G(5, 5, 5, 5, 7), G(5, 5, 5, 5, 2), G(5, 5, 7, 7, 5), G(5, 5, 2, 5, 5),
    G(5, 5, 2, 2, 2), G(7, 1, 2, 4, 7),
};
/* a hex digit past 9 is a small letter, so that no b or d reads as an 8 or
 * a 0 */
static const Uint16 small[6] = {
    G(0, 3, 5, 5, 3), G(4, 4, 6, 5, 6), G(0, 3, 4, 4, 3),
    G(1, 1, 3, 5, 3), G(2, 5, 7, 4, 3), G(3, 4, 6, 4, 4),
};
#define GLYPH_DOT G(0, 0, 0, 0, 2)
#define GLYPH_UNDERSCORE G(0, 0, 0, 0, 7)
#define GLYPH_HYPHEN G(0, 0, 7, 0, 0)

/* the hang's handler runs after the vblank chain's others */
#define CHAIN_PRIORITY 0x7fffffff

/* what the entry keeps of the code an exception stopped, before anything
 * else runs: PR, then R0 to R14. R0 to R7 are the bank's that SR.RB selected
 * at the entry, which the exception set */
enum { SAVED_PR, SAVED_R0, SAVED_WORDS = SAVED_R0 + 15 };
static Uint32 saved[SAVED_WORDS];
/* the reports begun: 0 until an exception is taken. past 1, a report itself
 * faulted */
static Uint32 taken;
static Uint32 own_stack[OWN_STACK_WORDS];

/* where the game stopped. a crash's is the first exception's, and nothing
 * writes it again */
static struct {
    Uint32 code;
    Uint32 pc;
    Uint32 sr;
    Uint32 sp;
    Uint32 address;
    Uint32 pr;
    Uint32 r[15];
} stopped;
/* the lines from the top down to the lowest any screen has drawn */
static int lines_drawn;

/* TRUE once crash_frame has joined the vblank's chain. the vblanks since a
 * frame last ended or a picture was last sent to be shown, those until a
 * hang next writes, and _nj_vsync_count_ as the last vblank found it */
static int started;
static volatile Uint32 idle;
static Uint32 again;
static Uint32 sent;

static void report(void) __attribute__((noreturn, used));
static void report_again(void) __attribute__((noreturn, used));

/* the general exceptions' entry. the CPU enters it with SR.BL set, where a
 * second exception would reset the console, and with SR.RB set. so it runs
 * on registers alone until it has a stack of its own: the stopped code's
 * R15 may be what failed.
 *
 * crash_vector is the trampoline crash_install copies over Shinobi's stub at
 * VBR + GENERAL_VECTOR. it keeps R0 in DBR, which the game leaves unused,
 * since an exception inside a handler finds the handler's own R0 there.
 *
 * crash_entry gives any code but the four the screen takes to Shinobi's
 * stub, with R0 put back in the jump's delay slot: TRAPA is one, which the
 * game runs on from. for one of the four it keeps the registers in saved,
 * moves to own_stack and calls report, or report_again where a report had
 * begun */
/* a macro's value as the assembler reads it */
#define TEXT(value) #value
#define AS(value) TEXT(value)
/* clang-format off */
__asm__("\t.section\t.text.crash_entry,\"ax\",@progbits\n"
        "\t.align\t2\n"
        "\t.global\t_crash_vector\n"
        "_crash_vector:\n"
        "\tldc\tr0, dbr\n"
        "\tmov.l\t0f, r0\n"
        "\tjmp\t@r0\n"
        "\tnop\n"
        "\t.align\t2\n"
        "0:\t.long\t_crash_entry\n"
        "\t.global\t_crash_vector_end\n"
        "_crash_vector_end:\n"
        "_crash_entry:\n"
        "\tmov.l\t1f, r0\n"
        "\tmov.l\t@r0, r0\n"
        "\tshlr2\tr0\n"
        "\tshlr2\tr0\n"
        "\tshlr\tr0\n"
        "\tcmp/eq\t#(" AS(CODE_ADDRESS_READ) " / " AS(CODE_STEP) "), r0\n"
        "\tbt\t5f\n"
        "\tcmp/eq\t#(" AS(CODE_ADDRESS_WRITE) " / " AS(CODE_STEP) "), r0\n"
        "\tbt\t5f\n"
        "\tcmp/eq\t#(" AS(CODE_ILLEGAL) " / " AS(CODE_STEP) "), r0\n"
        "\tbt\t5f\n"
        "\tcmp/eq\t#(" AS(CODE_ILLEGAL_SLOT) " / " AS(CODE_STEP) "), r0\n"
        "\tbt\t5f\n"
        "\tmov.l\t2f, r0\n"
        "\tjmp\t@r0\n"
        "\tstc\tdbr, r0\n"
        "5:\n"
        "\tmov.l\t3f, r0\n"
        "\tmov.l\t@r0, r0\n"
        "\ttst\tr0, r0\n"
        "\tbf\t6f\n"
        "\tmov.l\t4f, r0\n"
        "\tmov.l\tr14, @-r0\n"
        "\tmov.l\tr13, @-r0\n"
        "\tmov.l\tr12, @-r0\n"
        "\tmov.l\tr11, @-r0\n"
        "\tmov.l\tr10, @-r0\n"
        "\tmov.l\tr9, @-r0\n"
        "\tmov.l\tr8, @-r0\n"
        "\tmov.l\tr7, @-r0\n"
        "\tmov.l\tr6, @-r0\n"
        "\tmov.l\tr5, @-r0\n"
        "\tmov.l\tr4, @-r0\n"
        "\tmov.l\tr3, @-r0\n"
        "\tmov.l\tr2, @-r0\n"
        "\tmov.l\tr1, @-r0\n"
        "\tstc\tdbr, r1\n"
        "\tmov.l\tr1, @-r0\n"
        "\tsts\tpr, r1\n"
        "\tmov.l\tr1, @-r0\n"
        "\tmov.l\t7f, r15\n"
        "\tmov.l\t8f, r0\n"
        "\tjmp\t@r0\n"
        "\tnop\n"
        "6:\n"
        "\tmov.l\t7f, r15\n"
        "\tmov.l\t9f, r0\n"
        "\tjmp\t@r0\n"
        "\tnop\n"
        "\t.align\t2\n"
        "1:\t.long\t" AS(EXPEVT_AT) "\n"
        "2:\t.long\t_syIntExpISRTop\n"
        "3:\t.long\t%a0\n"
        "4:\t.long\t%a1\n"
        "7:\t.long\t%a2\n"
        "8:\t.long\t%a3\n"
        "9:\t.long\t%a4\n"
        :
        : "i"(&taken), "i"(&saved[SAVED_WORDS]),
          "i"(&own_stack[OWN_STACK_WORDS]), "i"(report), "i"(report_again));
/* clang-format on */
/* the trampoline's words, from its first to the word after its last */
extern const Uint32 crash_vector[];
extern const Uint32 crash_vector_end[];

static Uint32 glyph(char c)
{
    if (c >= '0' && c <= '9') {
        return digits[c - '0'];
    }
    if (c >= 'a' && c <= 'f') {
        return small[c - 'a'];
    }
    if (c >= 'A' && c <= 'Z') {
        return letters[c - 'A'];
    }
    switch (c) {
    case '.':
        return GLYPH_DOT;
    case '_':
        return GLYPH_UNDERSCORE;
    case '-':
        return GLYPH_HYPHEN;
    default:
        return 0;
    }
}

/* draw c in the cell at column and line, white on black, in the buffer the
 * screen shows. an interlaced picture keeps its two fields' lines by turns
 * in one buffer, so a line of the picture is a line of the buffer either
 * way. a cell's rows start on a word and end on one at each depth, since
 * LEFT, CELL_W and WIDTH are multiples of 4 pixels */
static void put(int column, int line, char c)
{
    Uint32 depth = (FB_R_CTRL >> FB_DEPTH_SHIFT) & FB_DEPTH_MASK;
    Uint32 bytes = depth == 3 ? 4 : depth == 2 ? 3 : 2;
    Uint32 first = FB_R_SOF1 & VIDEO_OFFSET;
    Uint32 second = FB_R_SOF2 & VIDEO_OFFSET;
    Uint32 words = (CELL_W * bytes) >> 2;
    Uint32 bits = glyph(c);
    /* a row of the cell as the buffer holds it. a lit pixel's bytes are all
     * 0xff at each depth, so their order within a word doesn't matter */
    Uint32 row[CELL_W * PIXEL_BYTES / sizeof(Uint32)];
    volatile Uint32 *to;
    Uint8 *byte;
    Uint32 i;
    int glyph_row;
    int dot;
    int y;

    if (second != 0 && second < first) {
        first = second;
    }
    to = (volatile Uint32 *)(VIDEO_MEMORY + first +
                             ((((TOP + (line * CELL_H)) * WIDTH) + LEFT +
                               (column * CELL_W)) *
                              bytes));
    for (glyph_row = 0; glyph_row <= GLYPH_H; glyph_row++) {
        byte = (Uint8 *)row;
        for (dot = 0; dot <= GLYPH_W; dot++) {
            int lit = glyph_row < GLYPH_H && dot < GLYPH_W &&
                      ((bits >> (((GLYPH_H - 1 - glyph_row) * GLYPH_W) +
                                 (GLYPH_W - 1 - dot))) &
                       1);

            for (i = 0; i < SCALE * bytes; i++) {
                *byte++ = lit ? 0xff : 0x00;
            }
        }
        for (y = 0; y < SCALE; y++) {
            for (i = 0; i < words; i++) {
                to[i] = row[i];
            }
            to += (WIDTH * bytes) >> 2;
        }
    }
}

static void draw(int line, const char *text)
{
    int column;

    for (column = 0; column < COLUMNS; column++) {
        put(column, line, *text ? *text++ : ' ');
    }
    if (line >= lines_drawn) {
        lines_drawn = line + 1;
    }
}

/* blank the lines below line that an earlier screen drew. callers left there
 * would read as this screen's */
static void blank_below(int line)
{
    while (++line < lines_drawn) {
        draw(line, "");
    }
}

/* copy from to to in capitals, up to end, and return where it stopped */
static char *copy(char *to, const char *from, char *end)
{
    while (*from && to < end) {
        char c = *from++;

        *to++ = (char)(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
    }
    return to;
}

/* write value's low digits hex digits, the highest first */
static char *hex(char *to, Uint32 value, int digits, char *end)
{
    int shift;

    for (shift = (digits - 1) * 4; shift >= 0 && to < end; shift -= 4) {
        Uint32 digit = (value >> shift) & 15;

        *to++ = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
    }
    return to;
}

/* write label, a space and value's eight digits, behind a space where the
 * line has text already */
static char *field(char *to, char *start, const char *label, Uint32 value,
                   char *end)
{
    if (to != start) {
        to = copy(to, " ", end);
    }
    to = copy(to, label, end);
    to = copy(to, " ", end);
    return hex(to, value, 8, end);
}

/* TRUE where address names bytes of main memory in a view the CPU reads,
 * aligned to bytes, a power of two */
static int in_memory(Uint32 address, Uint32 bytes)
{
    Uint32 offset = (address & ((1U << VIEW_SHIFT) - 1)) - MEMORY;

    return address < VIEWS_END && (address & (bytes - 1)) == 0 &&
           offset < MEMORY_SIZE && bytes <= MEMORY_SIZE - offset;
}

/* TRUE where word could be a return address of the game's: the address, in
 * one of the views its code runs in, of the instruction after a call's
 * delay slot */
static int returns_from_call(Uint32 word)
{
    Uint32 view = word >> CODE_VIEW_SHIFT;
    Uint16 op;

    if ((view != CODE_VIEW && view != CODE_VIEW_LOW) ||
        !in_memory(word - CALL_RETURN, sizeof(Uint16))) {
        return FALSE;
    }
    op = *(const Uint16 *)(word - CALL_RETURN);
    return IS_JSR(op) || IS_BSRF(op) || IS_BSR(op);
}

/* write the first line, what and number, then name on a line of its own
 * where it isn't null. it returns the next line */
static int show_what(const char *what, Uint32 number, const char *name)
{
    char text[COLUMNS + 1];
    char *end = text + COLUMNS;
    char *to;
    int line = 0;

    to = copy(text, what, end);
    to = copy(to, " ", end);
    to = hex(to, number, 8, end);
    *to = 0;
    draw(line++, text);
    if (name != NULL) {
        to = copy(text, name, end);
        *to = 0;
        draw(line++, text);
    }
    return line;
}

/* write stopped's PC, PR and SR, then its SP and ADDR, from line on. PR
 * shows where pr is TRUE, and ADDR where the code is an address error's */
static int show_where(int line, int pr)
{
    char text[COLUMNS + 1];
    char *end = text + COLUMNS;
    char *to;

    to = field(text, text, "PC", stopped.pc, end);
    if (pr) {
        to = field(to, text, "PR", stopped.pr, end);
    }
    to = field(to, text, "SR", stopped.sr, end);
    *to = 0;
    draw(line++, text);
    to = field(text, text, "SP", stopped.sp, end);
    if (stopped.code == CODE_ADDRESS_READ ||
        stopped.code == CODE_ADDRESS_WRITE) {
        to = field(to, text, "ADDR", stopped.address, end);
    }
    *to = 0;
    draw(line++, text);
    return line;
}

static int show_game(int line)
{
    char text[COLUMNS + 1];
    char *end = text + COLUMNS;
    char *to;

    to = field(text, text, "MODE", (Uint32)game_mode, end);
    to = field(to, text, "STAGE", (Uint32)stage_get_stage_act(), end);
    *to = 0;
    draw(line++, text);
    return line;
}

/* write stopped's R0 to R14, four to a line behind the first one's number */
static int show_registers(int line)
{
    char text[COLUMNS + 1];
    char *end = text + COLUMNS;
    char *to = text;
    int i;

    for (i = 0; i < 15; i++) {
        if ((i & 3) == 0) {
            to = copy(text, "R", end);
            to = hex(to, (Uint32)i, 1, end);
        }
        to = copy(to, " ", end);
        to = hex(to, stopped.r[i], 8, end);
        if ((i & 3) == 3 || i == 14) {
            *to = 0;
            draw(line++, text);
        }
    }
    return line;
}

/* write the callers the stack at sp holds, as the header says, and blank the
 * lines below them. it reads the stack only where sp is a word's address in
 * main memory, and no further than memory's end in sp's view */
static void show_callers(int line, Uint32 sp)
{
    char text[COLUMNS + 1];
    char *end = text + COLUMNS;
    char *to = text;
    const Uint32 *stack = (const Uint32 *)sp;
    Uint32 words;
    Uint32 i;
    int shown = 0;

    if (!in_memory(sp, sizeof(Uint32))) {
        draw(line, "NO STACK AT SP");
        blank_below(line);
        return;
    }
    words = (MEMORY_SIZE - ((sp & ((1U << VIEW_SHIFT) - 1)) - MEMORY)) >> 2;
    if (words > STACK_WORDS) {
        words = STACK_WORDS;
    }
    for (i = 0; i < words && shown < CALLERS_SHOWN; i++) {
        Uint32 word = stack[i];

        if (!returns_from_call(word)) {
            continue;
        }
        to = hex(to, i * sizeof(Uint32), 3, end);
        to = copy(to, " ", end);
        to = hex(to, word, 8, end);
        to = copy(to, "  ", end);
        if (++shown % CALLERS_A_LINE == 0) {
            *to = 0;
            draw(line++, text);
            to = text;
        }
    }
    *to = 0;
    draw(line, text);
    blank_below(line);
}

/* clear SR.BL and mask every interrupt. an exception then enters
 * crash_entry again, where under SR.BL it would reset the console, and no
 * interrupt's handler runs */
static void unblock(void)
{
    Uint32 sr;

    __asm__("stc sr,%0" : "=r"(sr));
    sr = (sr & ~(Uint32)SR_BL) | SR_IMASK;
    __asm__ volatile("ldc %0,sr" : : "r"(sr));
}

/* crash_entry's call for the first exception it takes, on own_stack with
 * SR.BL set. it reads what the CPU kept of the stopped code into stopped
 * before anything else can fault: SPC, SSR and SGR hold its PC, SR and R15
 * until the next exception. its R0 to R7 are the other bank's, or saved's
 * where it ran in this one, as a handler does. the game can't go on, so it
 * stays here */
static void report(void)
{
    Uint32 other[8];
    int i;

    __asm__("stc spc,%0" : "=r"(stopped.pc));
    __asm__("stc ssr,%0" : "=r"(stopped.sr));
    __asm__("stc sgr,%0" : "=r"(stopped.sp));
    __asm__("stc r0_bank,%0" : "=r"(other[0]));
    __asm__("stc r1_bank,%0" : "=r"(other[1]));
    __asm__("stc r2_bank,%0" : "=r"(other[2]));
    __asm__("stc r3_bank,%0" : "=r"(other[3]));
    __asm__("stc r4_bank,%0" : "=r"(other[4]));
    __asm__("stc r5_bank,%0" : "=r"(other[5]));
    __asm__("stc r6_bank,%0" : "=r"(other[6]));
    __asm__("stc r7_bank,%0" : "=r"(other[7]));
    stopped.code = EXPEVT;
    stopped.address = TEA;
    stopped.pr = saved[SAVED_PR];
    for (i = 0; i < 15; i++) {
        stopped.r[i] =
            i < 8 && !(stopped.sr & SR_RB) ? other[i] : saved[SAVED_R0 + i];
    }
    taken = 1;
    unblock();
    i = show_what("CRASH", stopped.code, NULL);
    i = show_where(i, TRUE);
    i = show_game(i);
    i = show_registers(i);
    show_callers(i, stopped.sp);
    for (;;) {
    }
}

/* crash_entry's call for an exception a report took: the report faulted.
 * the first exception's record stands. the screen shows it again without
 * the game's state or the stack, which may be what faulted, and then the
 * second exception's code and PC. a third exception stops here unshown */
static void report_again(void)
{
    char text[COLUMNS + 1];
    char *end = text + COLUMNS;
    char *to;
    Uint32 pc;
    int line;

    if (taken++ > 1) {
        for (;;) {
        }
    }
    __asm__("stc spc,%0" : "=r"(pc));
    unblock();
    line = show_what("CRASH", stopped.code, NULL);
    line = show_where(line, TRUE);
    to = field(text, text, "REPORT FAULTED", EXPEVT, end);
    to = field(to, text, "PC", pc, end);
    *to = 0;
    draw(line, text);
    blank_below(line);
    for (;;) {
    }
}

void crash_install(void)
{
    Uint32 *vector = (Uint32 *)((char *)get_vbr() + GENERAL_VECTOR);
    const Uint32 *from;
    Uint32 *to = vector;

    for (from = crash_vector; from < crash_vector_end; from++) {
        *to++ = *from;
    }
    /* the operand cache holds the words until they are written back, and
     * the instruction cache may hold the stub's */
    syCacheOCWB(vector, (Uint32)((char *)to - (char *)vector));
    syCacheICI();
}

/* the vblank chain's handler, which the vblank interrupt runs. SPC, SSR and
 * SGR hold what the interrupt stopped, and its PR and registers are the
 * master's to restore, out of reach here */
static void hang_vblank(void *arg)
{
    int line;

    (void)arg;
    /* a picture sent to be shown is the game running too. the count moves
     * as the game asks a flip, ahead of the flip itself. the movie player,
     * and each other loop on njWaitVSync, ends no frame, and the opening's
     * movies play for longer than CRASH_HANG */
    if (sent != _nj_vsync_count_) {
        sent = _nj_vsync_count_;
        idle = 0;
        again = 0;
        return;
    }
    if (++idle < CRASH_HANG) {
        return;
    }
    if (again != 0) {
        again--;
        return;
    }
    again = HANG_AGAIN;
    __asm__("stc spc,%0" : "=r"(stopped.pc));
    __asm__("stc ssr,%0" : "=r"(stopped.sr));
    __asm__("stc sgr,%0" : "=r"(stopped.sp));
    stopped.code = 0;
    line = show_what("HANG", idle, NULL);
    line = show_where(line, FALSE);
    line = show_game(line);
    show_callers(line, stopped.sp);
}

void crash_frame(void)
{
    if (!started) {
        started = TRUE;
        syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK, hang_vblank,
                          CHAIN_PRIORITY, NULL);
    }
    idle = 0;
    again = 0;
}

void port_stop(const char *what, const char *name, Uint32 number)
{
    int line;

    /* nothing of the game's runs again, a handler least of all */
    set_imask(IMASK_ALL);
    stopped.pc = (Uint32)__builtin_return_address(0);
    stopped.sp = (Uint32)__builtin_frame_address(0);
    __asm__("stc sr,%0" : "=r"(stopped.sr));
    stopped.code = 0;
    line = show_what(what, number, name);
    line = show_where(line, FALSE);
    line = show_game(line);
    show_callers(line, stopped.sp);
    for (;;) {
    }
}
