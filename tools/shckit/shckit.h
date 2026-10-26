// Read, check, lint, inspect, patch and translate asmsh assembly, the SuperH
// dialect Hitachi's SHC compiler writes and its assembler, asmsh, reads.
// SHCASM is the SHC phase that writes it; the patches recognize its lines by
// their exact bytes.
#ifndef SHCKIT_H
#define SHCKIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes, by the functions that return them. docs/patches.md lists the
// refusals each patch can return.
enum {
    SHCKIT_OK,

    // shckit_read
    SHCKIT_NOT_CP932, // a byte outside CP932, or a lead byte with no trail
    SHCKIT_CONTROL,   // a control character other than a tab or line end
    SHCKIT_UNENDED,   // an empty text, or a last line without a line end

    // shckit_apply
    SHCKIT_UNREADABLE,    // an unknown line, or a continuation joining nothing
    SHCKIT_PAST_END,      // a line after .END that isn't blank or a comment
    SHCKIT_OTHER_ALIGN,   // a code section's .ALIGN 4 that isn't SHCASM's line
    SHCKIT_SWITCH,        // a pool item Lnn-Lnn, a switch table's entry
    SHCKIT_POOL_LABEL,    // a pool label line not in SHCASM's 34-byte form
    SHCKIT_ABOVE,         // a pool label whose line above isn't an instruction
    SHCKIT_ITEM,          // a pool item not in SHCASM's form, or not a literal
    SHCKIT_SHAPE,         // pool items not words, at most one pad, then longs
    SHCKIT_NO_LONG,       // a pool without a .DATA.L
    SHCKIT_DUPLICATE,     // a pool label defined twice
    SHCKIT_POOLED,        // .POOL, .NOPOOL, or a MOV.W, MOV.L or MOVA of #x
    SHCKIT_NOT_HELD,      // an instruction the opcode table doesn't hold
    SHCKIT_SLOT,          // a delayed branch whose slot shckit can't place
    SHCKIT_SLOT_LABEL,    // a label on a slot or above it, below its branch
    SHCKIT_REACHED,       // a pool not right below the slot of a jump or return
    SHCKIT_GUARDED,       // a pool the BRA above it branches over
    SHCKIT_USE,           // a pool label used other than by a load or MOVA
    SHCKIT_NO_ITEM,       // a load or MOVA of no whole pool item of its size
    SHCKIT_UNUSED,        // a pool item no load or MOVA uses
    SHCKIT_CONFLICT,      // align-pool and use-assembler-pools together
    SHCKIT_OFFSET,        // an operand that offsets a code label, $ or PC
    SHCKIT_ADDRESS,       // a pool item's address used other than to read it
    SHCKIT_LOCAL_LABEL,   // a ?label in the block a pool's label opens
    SHCKIT_NO_SUCH_PATCH, // a bit that names no patch
    SHCKIT_TWICE,         // a text already patched

    // shckit_write
    SHCKIT_FULL, // the text needs more than cap bytes

    // any function that allocates
    SHCKIT_NOMEM, // allocation failed

    // shckit_inspect and shckit_inspect_all
    SHCKIT_INSPECT_INPUT, // a function or producer inspect can't take
    SHCKIT_CHECK_FAILED,  // a text check refuses

    // shckit_translate, which also returns SHCKIT_UNREADABLE,
    // SHCKIT_PAST_END, SHCKIT_NOT_HELD, SHCKIT_SLOT and SHCKIT_FULL
    SHCKIT_UNTRANSLATED, // a directive, operand or continuation it won't write
    SHCKIT_WORD,         // $, the GOT, or a word operator but STARTOF or SIZEOF
    SHCKIT_GROUPING,     // ~ or ^ between operands, or operators grouped apart
    SHCKIT_BRANCH,       // a branch GNU as can't relocate as asmsh does
    SHCKIT_STRING,       // a string other than an .SDATA of printable bytes
    SHCKIT_WAITING,      // a literal waiting at .SECTION, .END or the end
    SHCKIT_RESERVED,     // a name translate writes for STARTOF or SIZEOF
};

// A text: a copy of the bytes read, and its lines as the patches leave
// them. Functions that take a const text only read it, and texts share no
// mutable state.
struct shckit_text;

// Pointer arguments must not be NULL unless a comment says they may be.
// Line indexes count from 0. Original line numbers count from 1, with 0
// for none: struct shckit_line's input, and the line numbers shckit_read and
// shckit_apply report. A pool is an Lnn label line of a code section and the
// unlabeled .DATA and .RES lines directly below it, as SHCASM writes literal
// pools and switch tables.

// Read len bytes of asmsh assembly, in CP932 as SHC's
// -code=asmcode output is. src must address len readable bytes, which the
// text copies. Lines end at LF, and a CR right before an LF belongs to the
// line end. The reader checks CP932's byte structure, not whether CP932
// assigns each pair. The reader types lines without validating them: a text
// with SHCKIT_UNKNOWN lines reads, and check reports them or apply refuses
// them.
//
// On success *text is the new text. On error *text is NULL. line may be
// NULL; otherwise *line gets the number of the line at fault, or 0 on
// success, for an empty text, and for SHCKIT_NOMEM.
int shckit_read(struct shckit_text **text, const void *src, size_t len,
                size_t *line);

// Line kinds. Reading assigns a kind from the line's bytes alone; whether a
// continuation joins the statement above is decided by check and apply.
enum {
    SHCKIT_BLANK,        // blanks or nothing
    SHCKIT_COMMENT,      // a comment after any blanks
    SHCKIT_LABEL,        // a label in column 0, then at most a comment
    SHCKIT_INSTRUCTION,  // a mnemonic and its operands, after any label
    SHCKIT_DIRECTIVE,    // a known directive and its operands, after any label
    SHCKIT_CONTINUATION, // + in column 0, then a nonempty operand field
    SHCKIT_UNKNOWN,      // anything else
};

// Directives the reader knows, spelled in capitals. Any other directive, or
// one of these with operands the reader doesn't accept, makes the line
// SHCKIT_UNKNOWN.
enum {
    SHCKIT_SECTION, // .SECTION name,CODE or DATA,ALIGN=n, without blanks
    SHCKIT_ALIGN,   // .ALIGN n, n in decimal
    SHCKIT_DATA_B,
    SHCKIT_DATA_W,
    SHCKIT_DATA_L,
    SHCKIT_RES_B,
    SHCKIT_RES_W,
    SHCKIT_RES_L,
    SHCKIT_SDATA,
    SHCKIT_FDATA_S,
    SHCKIT_DATAB_B, // .DATAB.B count,value, a block of count items
    SHCKIT_DATAB_W,
    SHCKIT_DATAB_L,
    SHCKIT_IMPORT,
    SHCKIT_EXPORT,
    SHCKIT_POOL,   // no operands: asmsh writes waiting literals here
    SHCKIT_NOPOOL, // no operands: asmsh writes none after the slot above
    SHCKIT_END,
    SHCKIT_EQU,    // label: .EQU value
    SHCKIT_ASSIGN, // label: .ASSIGN value, which a later .ASSIGN redefines
};

// Bytes at..at+len of a line. len is 0 for a part the line doesn't have.
// Spans are byte offsets and lengths, not characters: the text is CP932 and
// is never decoded.
struct shckit_span {
    size_t at;
    size_t len;
};

// A line as the reader typed it. bytes holds the line without its line end
// and isn't NUL-terminated. code is true in a code section, including P,
// which holds the lines above any .SECTION. A line a patch rewrote keeps
// its input; a line a patch inserted has input 0. pool numbers the pools
// from 1 in text order, and is 0 on other lines.
struct shckit_line {
    const unsigned char *bytes;
    size_t len;
    bool crlf; // ends with CR LF rather than LF
    bool code;
    int kind;
    int directive;               // for SHCKIT_DIRECTIVE, else -1
    struct shckit_span label;    // without its colon
    struct shckit_span name;     // the mnemonic or directive
    struct shckit_span operands; // the operand field, without blanks around
    struct shckit_span comment;  // from the ; to the end
    size_t input;                // original line number
    int patch;                   // the patch that wrote or rewrote it, or -1
    size_t pool;
};

// The number of lines, and the line at index, or NULL past the last. A line
// pointer, and the bytes it points to, stay valid until the next
// shckit_apply or shckit_free on the text, whatever apply returns.
size_t shckit_count(const struct shckit_text *text);
const struct shckit_line *shckit_line(const struct shckit_text *text,
                                      size_t index);

// The next operand of a line's operand field. Set *at to 0 for the first;
// each call moves it past the operand returned. A comma inside a string or
// parentheses doesn't end an operand, the blanks around one aren't part of
// it, and one can be empty. Returns false when none is left. Only this
// line's field is read, not its continuations'.
bool shckit_operand(const struct shckit_line *line, size_t *at,
                    struct shckit_span *operand);

// What an instruction does to the flow of control.
enum {
    SHCKIT_DELAYED = 1,         // the next instruction, its delay slot, runs
                                // before the branch takes effect
    SHCKIT_NO_FALL_THROUGH = 2, // control never passes the instruction and
                                // its slot
    SHCKIT_SLOT_ILLEGAL = 4,    // illegal in a delay slot
};

// The flow bits of the instruction at index, from the opcode table of
// SH-4 instructions as asmsh assembles them with -cpu=sh4 -private
// (docs/opcodes.md). -1 for a line that isn't an instruction, an index past
// the last line, or an instruction whose mnemonic, size and operand forms no
// row holds. The table matches operand forms, not values. An instruction's
// operands include those of the continuations below it; a comma must end
// the field above each continuation or start its own, or the result is -1.
int shckit_flow(const struct shckit_text *text, size_t index);

// Rules: check's (docs/check.md), then lint's (docs/lint.md). lint reports a
// line it can't read by check's SHCKIT_UNKNOWN_LINE. Findings for one line
// come in this order.
enum {
    SHCKIT_UNKNOWN_LINE,
    SHCKIT_UNKNOWN_INSTRUCTION,
    SHCKIT_DATA_IN_FLOW,
    SHCKIT_DELAY_SLOT,
    SHCKIT_NO_SLOT,
    SHCKIT_AFTER_END,
    SHCKIT_COLUMNS,
    SHCKIT_TAB,
    SHCKIT_TRAILING_BLANK,
    SHCKIT_LINE_ENDS,
    SHCKIT_EXPORT_COMMENT,
    SHCKIT_RULES,
};

// A line a rule refuses: its index, and the rule.
struct shckit_finding {
    size_t index;
    int rule;
};

// Check text by check's rules. On success *found is an array of *count
// findings, sorted by index and then rule, each pair once, for the caller
// to release with free(); *found may be NULL when *count is 0. On error
// *found is NULL and *count is 0.
//
// If a line up to the first .END is one shckit can't read or an instruction
// the opcode table doesn't hold, those lines are the only findings.
// Otherwise the flow rules run, and a line past .END can break only
// after-end.
int shckit_check(const struct shckit_text *text, struct shckit_finding **found,
                 size_t *count);

// Check text by every one of lint's rules, whatever the text holds, and
// return the findings as shckit_check does.
int shckit_lint(const struct shckit_text *text, struct shckit_finding **found,
                size_t *count);

// A rule's name and one-line definition, or NULL outside the enum.
const char *shckit_rule_name(int rule);
const char *shckit_rule_definition(int rule);

// Patches (docs/patches.md), in the order shckit_apply runs them.
// align-pool and use-assembler-pools lay pools out in incompatible ways, so
// they never run together.
enum {
    SHCKIT_DROP_INLINE_ALIGN,
    SHCKIT_ALIGN_POOL,
    SHCKIT_USE_ASSEMBLER_POOLS,
    SHCKIT_DROP_R0_MOVE_NOP,
    SHCKIT_FILL_CALL_SLOT,
    SHCKIT_PATCHES,
};

// Apply the patches whose bits (1U << SHCKIT_DROP_INLINE_ALIGN, ...) are
// set, in enum order, reading the text as asmsh does, to its first .END. With
// no bits set, apply does nothing and returns SHCKIT_OK.
//
// Before any patch runs, apply refuses a bit that names no patch
// (SHCKIT_NO_SUCH_PATCH), align-pool with use-assembler-pools
// (SHCKIT_CONFLICT), a line up to the first .END that shckit can't read
// (SHCKIT_UNREADABLE), and a line past it that isn't blank or a comment
// (SHCKIT_PAST_END). On any error, a patch's refusal and running out of
// memory included, apply leaves the text as it was. A text is patched once:
// after a call with bits set succeeds, a later one with bits set returns
// SHCKIT_TWICE, unless its bits fail the first two checks.
//
// line and patch may be NULL. Otherwise *line gets the original number of
// the line the result names, or 0 for none, and *patch the patch that
// failed, or -1 for none. Each line's input and patch say which lines the
// patches kept, rewrote and inserted.
int shckit_apply(struct shckit_text *text, unsigned patches, size_t *line,
                 int *patch);

// A patch's name, as the command line spells it, or NULL outside the enum.
const char *shckit_patch_name(int patch);

// The patch with that name, or -1.
int shckit_patch(const char *name);

// Write the text's bytes, line ends included, into dst[0..cap), with no NUL
// after them. *out, if not NULL, gets the text's size, even on SHCKIT_FULL.
// With dst NULL it measures without writing, and cap is ignored. A size over
// cap returns SHCKIT_FULL and writes nothing, as does a size past SIZE_MAX,
// with *out SIZE_MAX. dst must not overlap the text.
int shckit_write(const struct shckit_text *text, void *dst, size_t cap,
                 size_t *out);

// Write text as GNU as text (docs/translate.md) into dst[0..cap), as
// shckit_write writes its bytes: *out, if not NULL, gets the size, even on
// SHCKIT_FULL, dst NULL measures, and a size over cap, or of SIZE_MAX, writes
// nothing. Every line ends with LF. translate reads the text as asmsh does,
// to its first .END. It refuses what apply refuses before any patch, an
// instruction the opcode table doesn't hold (SHCKIT_NOT_HELD), a delayed
// branch whose slot it can't place (SHCKIT_SLOT) and what its own codes
// name, and then writes nothing and gives *out 0. line may be NULL;
// otherwise *line gets the original number of the line at fault, or 0 for
// none. dst must not overlap the text.
//
// SHCKIT_OK means the text is written, not that GNU as assembles it to
// asmsh's bytes: docs/translate.md lists where they can differ.
int shckit_translate(const struct shckit_text *text, void *dst, size_t cap,
                     size_t *out, size_t *line);

// Inspection. A witness, in one function of a text, is evidence that the SHC
// release named, under the modes named, didn't compile it from C, inline text
// aside: SHC copies the text of #pragma inline_asm and #pragma asm as written.
// stack-above-entry and return-below-entry rest on an unfinished argument
// about SHC 5.0 R10's frames. A limit is a place inspect can't see past. It
// is no error: a witness found apart from it stands, but a function without a
// witness may still not be compiled from C, or may hold one a limit hides.
// docs/inspect.md lists the rules, the releases and modes each holds for, and
// their evidence, with its gaps.

// -code=machinecode and -code=asmcode, as bits.
enum { SHCKIT_MACHINECODE = 1, SHCKIT_ASMCODE = 2 };

// The compiler to rule out: a release, as docs/inspect.md names it, such as
// "5.0R10", and the modes a witness has to hold under, one bit or both. A
// release no rule holds for is a limit, not an error.
struct shckit_producer {
    const char *release;
    unsigned modes;
};

// FPSCR.SZ at the entry, which sets the width of an FMOV through memory.
// With SHCKIT_SZ_UNKNOWN, the stack rule finds a witness only where it finds
// one of the same rule at the same line under SZ 0 and under SZ 1. It meets
// each limit either meets, and stack-unknown at a line where it finds a
// witness under one alone.
enum { SHCKIT_SZ_UNKNOWN, SHCKIT_SZ_0, SHCKIT_SZ_1 };

// A function: the lines from first, its entry, up to end, which isn't one of
// them. first is a label or an instruction in a code section. Only the
// caller can tell that the lines are one whole function that every call
// enters at first; a label below it starts no other.
struct shckit_function {
    size_t first;
    size_t end;
    int sz;
};

// What a witness is. A lower kind outranks a higher one.
enum {
    SHCKIT_NO_WITNESS,
    SHCKIT_OPCODE,  // the instruction itself
    SHCKIT_OPERAND, // the instruction's operands
    SHCKIT_ABI,     // a use of R15
};

// A witness, at the line it stands at. value is R15's offset in bytes from
// its entry value once that line has run, for stack-above-entry and
// return-below-entry, with SZ unknown the nearer 0 of SZ 0's and SZ 1's; the
// constant, for small-constant; and 0 for any other.
struct shckit_witness {
    int kind;         // SHCKIT_NO_WITNESS for none
    const char *rule; // static, as docs/inspect.md names it, or NULL for none
    size_t index;     // the line's index, or SIZE_MAX for none
    int32_t value;
};

// What inspect found: the witness of the lowest kind, at the lowest index
// within it, and apart from it the limit at the lowest index. Of two limits
// at one index, continued comes first, then rule-scope or pool-word, then
// the stack rule's.
struct shckit_inspection {
    struct shckit_witness witness;
    const char *incomplete;  // static, as docs/inspect.md names it, or NULL
    size_t incomplete_index; // SIZE_MAX for none and for a limit at no line
};

// Inspect function in text for a witness against producer.
//
// Returns SHCKIT_INSPECT_INPUT for a function or producer outside the
// contracts above, such as an sz outside its enum or no modes. Each call then
// checks the whole text, and returns SHCKIT_CHECK_FAILED if shckit_check
// refuses a line. *result is set whatever inspect returns, and on error holds
// no witness or limit. The text is neither changed nor kept.
int shckit_inspect(const struct shckit_text *text,
                   const struct shckit_producer *producer,
                   const struct shckit_function *function,
                   struct shckit_inspection *result);

// Inspect as shckit_inspect does, setting *result alike, and hand over every
// witness: one for each rule at each line where it finds one, sorted by
// index and, at one index, in the order of docs/inspect.md's rules table.
// result->witness is one of them unless it is none. On success *found is an
// array of *count witnesses for the caller to release with free(); *found may
// be NULL when *count is 0. On error *found is NULL and *count is 0.
int shckit_inspect_all(const struct shckit_text *text,
                       const struct shckit_producer *producer,
                       const struct shckit_function *function,
                       struct shckit_inspection *result,
                       struct shckit_witness **found, size_t *count);

// Free a text and its lines. text may be NULL.
void shckit_free(struct shckit_text *text);

// What an error code means, in a few words; "unknown error" for any other
// value.
const char *shckit_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif
