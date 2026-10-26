// Read, check, lint, inspect, patch and translate asmsh assembly.
//
// SHC's -code=asmcode output has one statement per line. SHCASM, the
// compiler's assembly writer, starts each field of its own lines at a fixed
// column: a label at 0, a mnemonic or directive at 10, operands at 22 and a
// comment at 33. A line starting with + continues the operands above.
//
//   _f:                              ; function: f
//             MOV.L       L10+2,R3   ; _a
//   +                     operand,operand
//
// Inline text, the body of a #pragma inline_asm function or a #pragma asm
// block, keeps the programmer's layout. The text is CP932 and is never
// decoded. The reader accepts bytes past ASCII only in comments and strings,
// and since a CP932 trail byte is never a blank, quote, comma, parenthesis,
// semicolon or line end, it finds all of those byte by byte. Every line keeps
// its bytes, so a line no patch writes is output unchanged.
#include "shckit.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// The text and its reader: a line's kind and spans come from its bytes
// alone.

// The bytes of a rewritten line, which the text frees.
struct owned {
    unsigned char *bytes;
};

// A line's bytes point into input if no patch changed it, into an owned
// buffer if a patch rewrote it, or at a static line if a patch wrote it.
struct shckit_text {
    unsigned char *input;
    struct owned *owned; // in allocation order
    size_t owned_count;
    size_t owned_cap;
    struct shckit_line *lines;
    size_t count;
    size_t refused; // original line number of apply's refusal, or 0
    bool patched;   // an apply with bits set succeeded
};

// The reader's or a patch's temporary output; committed only on success.
struct list {
    struct shckit_line *at;
    size_t count;
    size_t cap;
};

static const char *const directives[] = {
    [SHCKIT_SECTION] = ".SECTION", [SHCKIT_ALIGN] = ".ALIGN",
    [SHCKIT_DATA_B] = ".DATA.B",   [SHCKIT_DATA_W] = ".DATA.W",
    [SHCKIT_DATA_L] = ".DATA.L",   [SHCKIT_RES_B] = ".RES.B",
    [SHCKIT_RES_W] = ".RES.W",     [SHCKIT_RES_L] = ".RES.L",
    [SHCKIT_SDATA] = ".SDATA",     [SHCKIT_FDATA_S] = ".FDATA.S",
    [SHCKIT_DATAB_B] = ".DATAB.B", [SHCKIT_DATAB_W] = ".DATAB.W",
    [SHCKIT_DATAB_L] = ".DATAB.L", [SHCKIT_IMPORT] = ".IMPORT",
    [SHCKIT_EXPORT] = ".EXPORT",   [SHCKIT_POOL] = ".POOL",
    [SHCKIT_NOPOOL] = ".NOPOOL",   [SHCKIT_END] = ".END",
    [SHCKIT_EQU] = ".EQU",         [SHCKIT_ASSIGN] = ".ASSIGN",
};

// The reader skips blanks between fields, but SHCASM writes only spaces, so
// matching SHCASM's exact lines uses space.
static bool blank(unsigned byte)
{
    return byte == ' ' || byte == '\t';
}

static bool space(unsigned byte)
{
    return byte == ' ';
}

static bool solid(unsigned byte)
{
    return !blank(byte);
}

static bool digit(unsigned byte)
{
    return byte >= '0' && byte <= '9';
}

static bool letter(unsigned byte)
{
    return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z');
}

static bool starts_symbol(unsigned byte)
{
    return letter(byte) || byte == '_' || byte == '?' || byte == '$';
}

static bool in_symbol(unsigned byte)
{
    return starts_symbol(byte) || digit(byte) || byte == '.';
}

static struct shckit_span span(size_t at, size_t end)
{
    struct shckit_span part = {at, end - at};
    return part;
}

static bool all(const unsigned char *bytes, struct shckit_span part,
                bool (*test)(unsigned byte))
{
    for (size_t i = 0; i < part.len; i++) {
        if (!test(bytes[part.at + i])) {
            return false;
        }
    }
    return true;
}

static bool is(const unsigned char *bytes, struct shckit_span part,
               const char *text)
{
    return part.len == strlen(text) &&
           memcmp(bytes + part.at, text, part.len) == 0;
}

static bool is_line(const struct shckit_line *line, const unsigned char *text,
                    size_t len)
{
    return line->len == len && memcmp(line->bytes, text, len) == 0;
}

static int check_bytes(const unsigned char *bytes, size_t len)
{
    // accept ASCII, half-width katakana, and lead bytes followed by a trail
    // byte, whether or not CP932 assigns the pair. Refuse control characters
    // other than tab.
    for (size_t i = 0; i < len; i++) {
        unsigned byte = bytes[i];
        if ((byte >= 0x81 && byte <= 0x9f) || (byte >= 0xe0 && byte <= 0xfc)) {
            unsigned trail = i + 1 < len ? bytes[i + 1] : 0;
            if (trail < 0x40 || trail == 0x7f || trail > 0xfc) {
                return SHCKIT_NOT_CP932;
            }
            i++;
        } else if (byte == 0x80 || byte == 0xa0 || byte > 0xfc) {
            return SHCKIT_NOT_CP932;
        } else if ((byte < 0x20 && byte != '\t') || byte == 0x7f) {
            return SHCKIT_CONTROL;
        }
    }
    return SHCKIT_OK;
}

static size_t skip_blanks(const unsigned char *bytes, size_t len, size_t at)
{
    while (at < len && blank(bytes[at])) {
        at++;
    }
    return at;
}

static struct shckit_span trimmed(const unsigned char *bytes, size_t at,
                                  size_t end)
{
    while (at < end && blank(bytes[at])) {
        at++;
    }
    while (end > at && blank(bytes[end - 1])) {
        end--;
    }
    return span(at, end);
}

static bool field(const unsigned char *bytes, size_t len, size_t at,
                  size_t *end)
{
    // set *end to the end of the operand field from at: a ; outside a
    // string, or the line's end. False for a string that doesn't close,
    // parentheses that don't pair, or a byte outside a string that isn't
    // ASCII. "" inside a string is a quote.
    size_t depth = 0;
    while (at < len && bytes[at] != ';') {
        unsigned byte = bytes[at++];
        if (byte == '"') {
            while (at < len && (bytes[at] != '"' ||
                                (at + 1 < len && bytes[at + 1] == '"'))) {
                at += bytes[at] == '"' ? 2 : 1;
            }
            if (at == len) {
                return false;
            }
            at++;
        } else if (byte >= 0x80 || (byte == ')' && depth == 0)) {
            return false;
        } else if (byte == '(') {
            depth++;
        } else if (byte == ')') {
            depth--;
        }
    }
    *end = at;
    return depth == 0;
}

static int section_type(const struct shckit_line *line)
{
    // 1 for name,CODE,ALIGN=n, 0 for name,DATA,ALIGN=n, -1 for anything
    // else. Accept only SHC's spelling: no blanks, capitals, and n in decimal
    // without a leading zero.
    const unsigned char *bytes = line->bytes;
    struct shckit_span parts[4];
    struct shckit_span number;
    size_t count = 0;
    size_t at = 0;
    while (count < 4 && shckit_operand(line, &at, &parts[count])) {
        count++;
    }
    if (count != 3 || !all(bytes, line->operands, solid) || parts[0].len == 0 ||
        !starts_symbol(bytes[parts[0].at]) ||
        !all(bytes, parts[0], in_symbol) || parts[2].len < 7 ||
        memcmp(bytes + parts[2].at, "ALIGN=", 6) != 0) {
        return -1;
    }
    number = span(parts[2].at + 6, parts[2].at + parts[2].len);
    if (bytes[number.at] == '0' || !all(bytes, number, digit)) {
        return -1;
    }
    if (is(bytes, parts[1], "CODE")) {
        return 1;
    }
    return is(bytes, parts[1], "DATA") ? 0 : -1;
}

static bool one_value(const struct shckit_line *line)
{
    // a label and exactly one operand, which isn't empty
    struct shckit_span value;
    struct shckit_span more;
    size_t at = 0;
    return line->label.len != 0 && shckit_operand(line, &at, &value) &&
           value.len != 0 && !shckit_operand(line, &at, &more);
}

static bool equates(const struct shckit_line *line)
{
    // an .EQU or .ASSIGN: its label names a value, not a place
    return line->kind == SHCKIT_DIRECTIVE &&
           (line->directive == SHCKIT_EQU || line->directive == SHCKIT_ASSIGN);
}

static bool writes_data(const struct shckit_line *line)
{
    return line->kind == SHCKIT_DIRECTIVE && line->directive >= SHCKIT_DATA_B &&
           line->directive <= SHCKIT_DATAB_L;
}

static int directive(const unsigned char *bytes, struct shckit_span name)
{
    for (size_t i = 0; i < sizeof directives / sizeof directives[0]; i++) {
        if (is(bytes, name, directives[i])) {
            return (int)i;
        }
    }
    return -1;
}

static void statement(struct shckit_line *line, size_t at)
{
    // a directive must be one the reader knows, in capitals; a mnemonic may
    // be any word, which the opcode table judges later
    const unsigned char *bytes = line->bytes;
    size_t len = line->len;
    size_t start = at;
    size_t end = 0;
    int named = -1;

    if (bytes[at] == '.') {
        at++;
        while (at < len &&
               ((bytes[at] >= 'A' && bytes[at] <= 'Z') || bytes[at] == '.')) {
            at++;
        }
        named = directive(bytes, span(start, at));
        if (named < 0) {
            return;
        }
    } else if (letter(bytes[at])) {
        while (at < len && (letter(bytes[at]) || digit(bytes[at]) ||
                            bytes[at] == '.' || bytes[at] == '/')) {
            at++;
        }
    } else {
        return;
    }
    if ((at < len && !blank(bytes[at]) && bytes[at] != ';') ||
        !field(bytes, len, at, &end)) {
        return;
    }
    line->name = span(start, at);
    line->operands = trimmed(bytes, at, end);
    line->comment = span(end, len);
    line->directive = named;
    line->kind = named >= 0 ? SHCKIT_DIRECTIVE : SHCKIT_INSTRUCTION;
    // .SECTION places every line below it and patches read .ALIGN's
    // value, so both must be exact. .EQU and .ASSIGN need a label and one
    // value, and .POOL and .NOPOOL take no operands.
    if ((named == SHCKIT_SECTION && section_type(line) < 0) ||
        (named == SHCKIT_ALIGN &&
         (line->operands.len == 0 || !all(bytes, line->operands, digit))) ||
        (equates(line) && !one_value(line)) ||
        ((named == SHCKIT_POOL || named == SHCKIT_NOPOOL) &&
         line->operands.len != 0)) {
        line->kind = SHCKIT_UNKNOWN;
        line->directive = -1;
    }
}

static void type_line(struct shckit_line *line)
{
    // set the kind and spans from the bytes alone. The caller sets code, and
    // find_pools sets pool.
    const unsigned char *bytes = line->bytes;
    size_t len = line->len;
    size_t at = 0;
    size_t end = 0;
    struct shckit_span none = {0, 0};

    line->kind = SHCKIT_UNKNOWN;
    line->directive = -1;
    line->label = none;
    line->name = none;
    line->operands = none;
    line->comment = none;
    if (len > 0 && bytes[0] == '+') {
        if (field(bytes, len, 1, &end)) {
            line->operands = trimmed(bytes, 1, end);
            line->comment = span(end, len);
            if (line->operands.len != 0) {
                line->kind = SHCKIT_CONTINUATION;
            }
        }
        return;
    }
    if (len > 0 && starts_symbol(bytes[0])) {
        while (at < len && in_symbol(bytes[at])) {
            at++;
        }
        if (at == len || bytes[at] != ':') {
            return;
        }
        line->label = span(0, at++);
    } else if (len > 0 && !blank(bytes[0]) && bytes[0] != ';') {
        return;
    }
    at = skip_blanks(bytes, len, at);
    if (at < len && bytes[at] != ';') {
        statement(line, at);
        return;
    }
    line->comment = span(at, len);
    if (line->label.len != 0) {
        line->kind = SHCKIT_LABEL;
    } else {
        line->kind = at < len ? SHCKIT_COMMENT : SHCKIT_BLANK;
    }
}

bool shckit_operand(const struct shckit_line *line, size_t *at,
                    struct shckit_span *operand)
{
    // field() checked the field, so strings close and parentheses pair. *at
    // is an offset from the field's start, past its end after the last
    // operand.
    const unsigned char *bytes = line->bytes;
    size_t start = line->operands.at + *at;
    size_t end = line->operands.at + line->operands.len;
    size_t i = start;
    size_t depth = 0;
    bool quoted = false;

    if (line->operands.len == 0 || *at > line->operands.len) {
        return false;
    }
    while (i < end && (bytes[i] != ',' || depth != 0 || quoted)) {
        if (bytes[i] == '"') {
            quoted = !quoted;
        } else if (!quoted && bytes[i] == '(') {
            depth++;
        } else if (!quoted && bytes[i] == ')') {
            depth--;
        }
        i++;
    }
    *operand = trimmed(bytes, start, i);
    *at = i + 1 - line->operands.at;
    return true;
}

static bool pool_head(const struct shckit_line *line)
{
    // an Lnn label alone on a line of a code section, but for a comment
    struct shckit_span number = {1, line->label.len - 1};
    return line->code && line->kind == SHCKIT_LABEL && line->label.len >= 2 &&
           line->bytes[0] == 'L' && all(line->bytes, number, digit);
}

static bool pool_item(const struct shckit_line *line)
{
    return line->kind == SHCKIT_DIRECTIVE && line->label.len == 0 &&
           line->directive >= SHCKIT_DATA_B && line->directive <= SHCKIT_RES_L;
}

static void find_pools(struct shckit_text *text)
{
    struct shckit_line *lines = text->lines;
    size_t count = text->count;
    size_t pools = 0;

    for (size_t i = 0; i < count; i++) {
        lines[i].pool = 0;
    }
    for (size_t i = 0; i + 1 < count; i++) {
        size_t j = i + 1;
        if (!pool_head(&lines[i]) || !pool_item(&lines[j])) {
            continue;
        }
        lines[i].pool = ++pools;
        for (; j < count && pool_item(&lines[j]); j++) {
            lines[j].pool = pools;
        }
        i = j - 1;
    }
}

static void *grow(void *at, size_t *cap, size_t size, size_t first)
{
    // at reallocated to twice *cap items of size bytes, or first when *cap
    // is 0, with *cap set to match; or NULL, with at and *cap unchanged
    size_t want = *cap != 0 ? *cap * 2 : first;
    void *grown = want <= SIZE_MAX / size ? realloc(at, want * size) : NULL;
    if (grown != NULL) {
        *cap = want;
    }
    return grown;
}

static bool push(struct list *list, const struct shckit_line *line)
{
    if (list->count == list->cap) {
        struct shckit_line *grown =
            grow(list->at, &list->cap, sizeof *list->at, 1024);
        if (grown == NULL) {
            return false;
        }
        list->at = grown;
    }
    list->at[list->count++] = *line;
    return true;
}

static int split(struct shckit_text *text, size_t len, size_t *line)
{
    const unsigned char *bytes = text->input;
    struct list lines = {NULL, 0, 0};
    size_t start = 0;
    bool code = true;
    int error = SHCKIT_OK;
    for (size_t i = 0; error == SHCKIT_OK && i < len; i++) {
        struct shckit_line each;
        if (bytes[i] != '\n') {
            continue;
        }
        memset(&each, 0, sizeof each);
        each.bytes = bytes + start;
        each.len = i - start;
        each.crlf = i > start && bytes[i - 1] == '\r';
        each.len -= each.crlf ? 1 : 0;
        each.input = lines.count + 1;
        each.patch = -1;
        error = check_bytes(each.bytes, each.len);
        if (error != SHCKIT_OK) {
            *line = each.input;
            break;
        }
        type_line(&each);
        if (each.kind == SHCKIT_DIRECTIVE && each.directive == SHCKIT_SECTION) {
            code = section_type(&each) == 1;
        }
        each.code = code;
        error = push(&lines, &each) ? SHCKIT_OK : SHCKIT_NOMEM;
        start = i + 1;
    }
    if (error != SHCKIT_OK) {
        free(lines.at);
        return error;
    }
    text->lines = lines.at;
    text->count = lines.count;
    return SHCKIT_OK;
}

int shckit_read(struct shckit_text **text, const void *src, size_t len,
                size_t *line)
{
    const unsigned char *bytes = src;
    struct shckit_text *read = NULL;
    size_t at = 0;
    int error = SHCKIT_OK;

    *text = NULL;
    if (len == 0 || bytes[len - 1] != '\n') {
        error = SHCKIT_UNENDED;
        for (size_t i = 0; i < len; i++) {
            at += bytes[i] == '\n' ? 1 : 0;
        }
        at += len != 0 ? 1 : 0;
    }
    if (error == SHCKIT_OK) {
        read = calloc(1, sizeof *read);
        error = read != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    }
    if (error == SHCKIT_OK) {
        read->input = malloc(len);
        error = read->input != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    }
    if (error == SHCKIT_OK) {
        memcpy(read->input, bytes, len);
        error = split(read, len, &at);
    }
    if (error == SHCKIT_OK) {
        find_pools(read);
    }
    if (line != NULL) {
        *line = error == SHCKIT_OK ? 0 : at;
    }
    if (error != SHCKIT_OK) {
        shckit_free(read);
        return error;
    }
    *text = read;
    return SHCKIT_OK;
}

size_t shckit_count(const struct shckit_text *text)
{
    return text->count;
}

const struct shckit_line *shckit_line(const struct shckit_text *text,
                                      size_t index)
{
    return index < text->count ? &text->lines[index] : NULL;
}

// Statements across lines: whether a continuation joins the statement
// above it.

static bool continued(const struct shckit_text *text, size_t index)
{
    // the next line is a continuation, whether or not it joins
    return index + 1 < text->count &&
           text->lines[index + 1].kind == SHCKIT_CONTINUATION;
}

static bool ends_with_comma(const struct shckit_line *line)
{
    return line->operands.len != 0 &&
           line->bytes[line->operands.at + line->operands.len - 1] == ',';
}

static bool starts_with_comma(const struct shckit_line *line)
{
    return line->operands.len != 0 && line->bytes[line->operands.at] == ',';
}

static bool joins(const struct shckit_text *text, size_t head, size_t index)
{
    // whether the continuation at index joins the statement at head, the
    // lines between being its continuations. asmsh joins a continuation to
    // a directive whose field above ends with a comma, and to an
    // instruction with operands. shckit doesn't read an operand across
    // lines, so for an instruction a comma must also end the field above or
    // start the continuation's.
    const struct shckit_line *statement = &text->lines[head];
    const struct shckit_line *above = &text->lines[index - 1];
    if (statement->kind == SHCKIT_DIRECTIVE) {
        return ends_with_comma(above);
    }
    return statement->operands.len != 0 &&
           (ends_with_comma(above) || starts_with_comma(&text->lines[index]));
}

static bool unreadable(const struct shckit_text *text, size_t index,
                       size_t *head)
{
    // a line of unknown kind, or a continuation that joins no statement.
    // Call it for each line in order from 0, starting with *head =
    // SIZE_MAX. *head tracks the index of the statement a continuation
    // would join, or SIZE_MAX for none.
    const struct shckit_line *line = &text->lines[index];
    if (line->kind == SHCKIT_INSTRUCTION || line->kind == SHCKIT_DIRECTIVE) {
        *head = index;
        return false;
    }
    if (line->kind == SHCKIT_CONTINUATION && *head != SIZE_MAX &&
        joins(text, *head, index)) {
        return false;
    }
    *head = SIZE_MAX;
    return line->kind == SHCKIT_UNKNOWN || line->kind == SHCKIT_CONTINUATION;
}

// The opcode table (docs/opcodes.md), and the operands its forms match.

// Operand kinds. R0 and FR0 have kinds of their own for the forms that take
// only them, and takes() lets a general or floating-point register form
// accept them too.
enum {
    KIND_R0,
    KIND_RN,         // R1 to R15, or SP, which is R15
    KIND_BANK,       // R0_BANK to R7_BANK
    KIND_IMM,        // #imm
    KIND_AT,         // @Rn, or @(Rn)
    KIND_POST,       // @Rn+
    KIND_PRE,        // @-Rn
    KIND_DISP,       // @(disp,Rn)
    KIND_INDEX,      // @(R0,Rn)
    KIND_GBR_DISP,   // @(disp,GBR)
    KIND_GBR_INDEX,  // @(R0,GBR)
    KIND_PC_DISP,    // @(disp,PC)
    KIND_EXPRESSION, // an expression that names no register, as a label
    KIND_FR0,
    KIND_FRN, // FR1 to FR15
    KIND_DRN, // DR0 to DR14, even
    KIND_XDN, // XD0 to XD14, even
    KIND_FVN, // FV0, FV4, FV8 or FV12
    KIND_XMTRX,
    KIND_FPUL,
    KIND_FPSCR,
    KIND_SR,
    KIND_GBR,
    KIND_VBR,
    KIND_SSR,
    KIND_SPC,
    KIND_SGR,
    KIND_DBR,
    KIND_MACH,
    KIND_MACL,
    KIND_PR,
    KIND_PC, // PC alone, which no form takes
};

// Registers named without a number, read in any case. SP is R15.
static const struct {
    const char *name;
    int kind;
} named_registers[] = {
    {"SP", KIND_RN},     {"PC", KIND_PC},       {"XMTRX", KIND_XMTRX},
    {"FPUL", KIND_FPUL}, {"FPSCR", KIND_FPSCR}, {"SR", KIND_SR},
    {"GBR", KIND_GBR},   {"VBR", KIND_VBR},     {"SSR", KIND_SSR},
    {"SPC", KIND_SPC},   {"SGR", KIND_SGR},     {"DBR", KIND_DBR},
    {"MACH", KIND_MACH}, {"MACL", KIND_MACL},   {"PR", KIND_PR},
};

// Numbered registers: the prefix, a decimal number without leading zeros
// that is a multiple of step no greater than most, then the suffix. A name
// with any other number is a symbol.
static const struct {
    const char *prefix;
    const char *suffix;
    unsigned most;
    unsigned step;
    int kind;
} numbered_registers[] = {
    {"R", "", 15, 1, KIND_RN},   {"R", "_BANK", 7, 1, KIND_BANK},
    {"FR", "", 15, 1, KIND_FRN}, {"DR", "", 14, 2, KIND_DRN},
    {"XD", "", 14, 2, KIND_XDN}, {"FV", "", 12, 4, KIND_FVN},
};

// Words asmsh reads, in any case, as operators on the word after them.
static const char *const word_operators[] = {
    "STARTOF", "SIZEOF", "HIGH",   "LOW",  "HWORD",
    "LWORD",   "$EVEN",  "$EVEN2", "$ODD", "$ODD2",
};

// Operand names in the table's forms, as the SH-4 manual writes them: m for
// a source, n for the destination. A register named in a form, such as R0 or
// GBR, needs no entry.
static const struct {
    const char *name;
    int kind;
} form_kinds[] = {
    {"Rm", KIND_RN},
    {"Rn", KIND_RN},
    {"Rm_BANK", KIND_BANK},
    {"Rn_BANK", KIND_BANK},
    {"#imm", KIND_IMM},
    {"@Rm", KIND_AT},
    {"@Rn", KIND_AT},
    {"@Rm+", KIND_POST},
    {"@Rn+", KIND_POST},
    {"@-Rn", KIND_PRE},
    {"@(disp,Rm)", KIND_DISP},
    {"@(disp,Rn)", KIND_DISP},
    {"@(R0,Rm)", KIND_INDEX},
    {"@(R0,Rn)", KIND_INDEX},
    {"@(disp,GBR)", KIND_GBR_DISP},
    {"@(R0,GBR)", KIND_GBR_INDEX},
    {"@(disp,PC)", KIND_PC_DISP},
    {"label", KIND_EXPRESSION},
    {"FRm", KIND_FRN},
    {"FRn", KIND_FRN},
    {"DRm", KIND_DRN},
    {"DRn", KIND_DRN},
    {"XDm", KIND_XDN},
    {"XDn", KIND_XDN},
    {"FVm", KIND_FVN},
    {"FVn", KIND_FVN},
};

#define MOST_OPERANDS 3

// The opcode table of SH-4 instructions as asmsh assembles them with
// -cpu=sh4 -private (docs/opcodes.md). A row holds mnemonics, the sizes
// they take, the size GNU as writes for each, their operand forms and their
// flow bits. A size is the letter after the mnemonic's dot, or - for none,
// which asmsh reads as the instruction's own size. Mnemonics with the same
// sizes, forms and flow share a row. Mnemonics, or forms of one, whose sizes
// GNU as writes apart take rows of their own.
static const struct {
    const char *names; // separated by spaces
    const char *sizes;
    const char *gnu;   // a letter, or -, for each of sizes
    const char *forms; // separated by spaces, operands by commas
    unsigned flow;
} opcodes[] = {
    {"ADD", "-L", "--", "#imm,Rn Rm,Rn", 0},
    {"ADDC ADDV CMP/GE CMP/GT CMP/HI CMP/HS CMP/STR DIV0S DIV1 NEG NEGC NOT "
     "SHAD SHLD SUB SUBC SUBV XTRCT",
     "-L", "--", "Rm,Rn", 0},
    {"DMULS DMULU MUL", "-L", "LL", "Rm,Rn", 0},
    {"AND OR TST XOR", "-B", "BB", "#imm,@(R0,GBR)", 0},
    {"AND CMP/EQ OR TST XOR", "-L", "--", "#imm,R0 Rm,Rn", 0},
    {"BF BT", "-", "-", "label", SHCKIT_SLOT_ILLEGAL},
    {"BF/S BSR BT/S", "-", "-", "label", SHCKIT_DELAYED | SHCKIT_SLOT_ILLEGAL},
    {"BRA", "-", "-", "label",
     SHCKIT_DELAYED | SHCKIT_NO_FALL_THROUGH | SHCKIT_SLOT_ILLEGAL},
    {"BRAF", "-", "-", "Rn",
     SHCKIT_DELAYED | SHCKIT_NO_FALL_THROUGH | SHCKIT_SLOT_ILLEGAL},
    {"BSRF", "-", "-", "Rn", SHCKIT_DELAYED | SHCKIT_SLOT_ILLEGAL},
    {"CLRMAC CLRS CLRT DIV0U FRCHG FSCHG LDTLB NOP SETS SETT SLEEP", "-", "-",
     "", 0},
    {"CMP/PL CMP/PZ MOVT ROTCL ROTCR ROTL ROTR SHAL SHAR SHLL SHLL16 SHLL2 "
     "SHLL8 SHLR SHLR16 SHLR2 SHLR8",
     "-L", "--", "Rn", 0},
    {"DT", "-", "-", "Rn", 0},
    {"EXTS EXTU SWAP", "-BW", "WBW", "Rm,Rn", 0},
    {"FABS FNEG FSQRT", "-D", "--", "DRn", 0},
    {"FABS FLDI0 FLDI1 FNEG FSQRT FSRRA", "-S", "--", "FRn", 0},
    {"FADD FCMP/EQ FCMP/GT FDIV FMUL FSUB", "-D", "--", "DRm,DRn", 0},
    {"FADD FCMP/EQ FCMP/GT FDIV FMUL FSUB", "-S", "--", "FRm,FRn", 0},
    {"FCNVDS FTRC", "-D", "--", "DRm,FPUL", 0},
    {"FCNVSD FLOAT", "-D", "--", "FPUL,DRn", 0},
    {"FIPR", "-S", "--", "FVm,FVn", 0},
    {"FLDS FTRC", "-S", "--", "FRm,FPUL", 0},
    {"FLOAT FSTS", "-S", "--", "FPUL,FRn", 0},
    {"FMAC", "-S", "--", "FR0,FRm,FRn", 0},
    {"FMOV", "-D", "-D",
     "@(R0,Rm),DRn @(R0,Rm),XDn @Rm,DRn @Rm,XDn @Rm+,DRn @Rm+,XDn "
     "DRm,@(R0,Rn) DRm,@-Rn DRm,@Rn XDm,@(R0,Rn) XDm,@-Rn XDm,@Rn",
     0},
    {"FMOV", "-D", "--", "DRm,DRn DRm,XDn XDm,DRn XDm,XDn", 0},
    {"FMOV", "-S", "-S",
     "@(R0,Rm),FRn @Rm,FRn @Rm+,FRn FRm,@(R0,Rn) FRm,@-Rn FRm,@Rn", 0},
    {"FMOV", "-S", "--", "FRm,FRn", 0},
    {"FSCA", "-S", "--", "FPUL,DRn", 0},
    {"FTRV", "-S", "--", "XMTRX,FVn", 0},
    {"JMP", "-", "-", "@Rn",
     SHCKIT_DELAYED | SHCKIT_NO_FALL_THROUGH | SHCKIT_SLOT_ILLEGAL},
    {"JSR", "-", "-", "@Rn", SHCKIT_DELAYED | SHCKIT_SLOT_ILLEGAL},
    {"LDC", "-L", "LL",
     "@Rm+,DBR @Rm+,GBR @Rm+,Rn_BANK @Rm+,SPC @Rm+,SSR @Rm+,VBR", 0},
    {"LDC", "-L", "--", "Rm,DBR Rm,GBR Rm,Rn_BANK Rm,SPC Rm,SSR Rm,VBR", 0},
    {"LDC", "-L", "LL", "@Rm+,SR", SHCKIT_SLOT_ILLEGAL},
    {"LDC", "-L", "--", "Rm,SR", SHCKIT_SLOT_ILLEGAL},
    {"LDS", "-L", "LL", "@Rm+,FPSCR @Rm+,FPUL @Rm+,MACH @Rm+,MACL @Rm+,PR", 0},
    {"LDS", "-L", "--", "Rm,FPSCR Rm,FPUL Rm,MACH Rm,MACL Rm,PR", 0},
    {"MAC", "-WL", "WWL", "@Rm+,@Rn+", 0},
    {"MOV", "-B", "--", "#imm,Rn", 0},
    {"MOV", "-BWL", "LBWL",
     "@(R0,Rm),Rn @(disp,GBR),R0 @Rm,Rn @Rm+,Rn R0,@(disp,GBR) "
     "Rm,@(R0,Rn) Rm,@-Rn Rm,@Rn",
     0},
    {"MOV", "-L", "LL", "@(disp,Rm),Rn Rm,@(disp,Rn)", 0},
    {"MOV", "-L", "--", "Rm,Rn", 0},
    {"MOV", "-WL", "LWL", "@(disp,PC),Rn label,Rn", SHCKIT_SLOT_ILLEGAL},
    {"MOV", "BW", "BW", "@(disp,Rm),R0 R0,@(disp,Rn)", 0},
    {"MOV", "WL", "WL", "#imm,Rn", SHCKIT_SLOT_ILLEGAL},
    {"MOVA", "-L", "--", "#imm,R0 @(disp,PC),R0 label,R0", SHCKIT_SLOT_ILLEGAL},
    {"MOVCA", "-L", "LL", "R0,@Rn", 0},
    {"MULS MULU", "-WL", "-W-", "Rm,Rn", 0},
    {"OCBI OCBP OCBWB PREF", "-", "-", "@Rn", 0},
    {"RTE RTS", "-", "-", "",
     SHCKIT_DELAYED | SHCKIT_NO_FALL_THROUGH | SHCKIT_SLOT_ILLEGAL},
    {"STC", "-L", "LL",
     "DBR,@-Rn GBR,@-Rn Rm_BANK,@-Rn SGR,@-Rn SPC,@-Rn SR,@-Rn SSR,@-Rn "
     "VBR,@-Rn",
     0},
    {"STC", "-L", "--",
     "DBR,Rn GBR,Rn Rm_BANK,Rn SGR,Rn SPC,Rn SR,Rn SSR,Rn VBR,Rn", 0},
    {"STS", "-L", "LL", "FPSCR,@-Rn FPUL,@-Rn MACH,@-Rn MACL,@-Rn PR,@-Rn", 0},
    {"STS", "-L", "--", "FPSCR,Rn FPUL,Rn MACH,Rn MACL,Rn PR,Rn", 0},
    {"TAS", "-B", "BB", "@Rn", 0},
    {"TRAPA", "-L", "--", "#imm", SHCKIT_SLOT_ILLEGAL},
};

// An operand's token: a word, a string, or one byte of punctuation.
struct token {
    size_t at;
    size_t len;
    bool word; // a word or a string
};

static bool word_byte(unsigned byte)
{
    // a byte of a symbol, a number, or a radix such as the H' of H'FF
    return in_symbol(byte) || byte == '\'';
}

static unsigned upper(unsigned byte)
{
    return byte >= 'a' && byte <= 'z' ? byte - 'a' + 'A' : byte;
}

static bool same_text(const unsigned char *bytes, size_t len, const char *name,
                      size_t name_len)
{
    // bytes equal name, ignoring case
    if (len != name_len) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (upper(bytes[i]) != upper((unsigned char)name[i])) {
            return false;
        }
    }
    return true;
}

static bool same_name(const unsigned char *bytes, size_t len, const char *name)
{
    return same_text(bytes, len, name, strlen(name));
}

static bool next_token(const unsigned char *bytes, size_t *at, size_t end,
                       struct token *token)
{
    // *at moves past the token. "" inside a string is a quote, and field()
    // made sure every string closes.
    size_t i = skip_blanks(bytes, end, *at);
    if (i == end) {
        *at = end;
        return false;
    }
    token->at = i;
    token->word = true;
    if (bytes[i] == '"') {
        i++;
        while (i < end &&
               (bytes[i] != '"' || (i + 1 < end && bytes[i + 1] == '"'))) {
            i += bytes[i] == '"' ? 2 : 1;
        }
        i += i < end ? 1 : 0;
    } else if (word_byte(bytes[i])) {
        while (i < end && word_byte(bytes[i])) {
            i++;
        }
    } else {
        token->word = false;
        i++;
    }
    token->len = i - token->at;
    *at = i;
    return true;
}

static bool is_punctuation(const unsigned char *bytes, struct token token,
                           unsigned byte)
{
    return !token.word && bytes[token.at] == byte;
}

static bool number(const unsigned char *bytes, struct token token,
                   uint32_t *value)
{
    // a word that is a number in one of asmsh's radixes: decimal digits, or
    // H', B', Q' or D' and hexadecimal, binary, octal or decimal ones, in
    // either letter case, within 32 bits
    static const char letters[] = "HBQD";
    static const unsigned radixes[] = {16, 2, 8, 10};
    const char *letter = NULL;
    size_t at = token.at;
    unsigned radix = 10;
    uint64_t sum = 0;
    if (!token.word) {
        return false;
    }
    if (token.len > 2 && bytes[at + 1] == '\'') {
        letter = memchr(letters, (int)upper(bytes[at]), sizeof letters - 1);
        if (letter == NULL) {
            return false;
        }
        radix = radixes[letter - letters];
        at += 2;
    }
    for (; at < token.at + token.len; at++) {
        unsigned byte = upper(bytes[at]);
        unsigned figure = radix;
        if (digit(byte)) {
            figure = byte - '0';
        } else if (byte >= 'A' && byte <= 'F') {
            figure = byte - 'A' + 10;
        }
        if (figure >= radix) {
            return false;
        }
        sum = (sum * radix) + figure;
        if (sum > UINT32_MAX) {
            return false;
        }
    }
    *value = (uint32_t)sum;
    return true;
}

static bool plain_number(const unsigned char *bytes, size_t at, size_t end,
                         int64_t *value)
{
    // a number in one of asmsh's radixes, with an optional sign, and nothing
    // else: no symbol or expression. *value is as written, its digits within
    // 32 bits.
    struct token token;
    uint32_t digits = 0;
    bool negative = false;
    bool read = next_token(bytes, &at, end, &token);
    if (read && (is_punctuation(bytes, token, '-') ||
                 is_punctuation(bytes, token, '+'))) {
        negative = bytes[token.at] == '-';
        read = next_token(bytes, &at, end, &token);
    }
    if (!read || !number(bytes, token, &digits) ||
        next_token(bytes, &at, end, &token)) {
        return false;
    }
    *value = negative ? -(int64_t)digits : (int64_t)digits;
    return true;
}

static int numbered(const unsigned char *bytes, struct token token,
                    size_t which)
{
    // the number of the numbered_registers[which] register the token
    // names, or -1. No register needs more than two digits.
    const char *prefix = numbered_registers[which].prefix;
    const char *suffix = numbered_registers[which].suffix;
    size_t at = strlen(prefix);
    size_t digits = 0;
    unsigned number = 0;
    if (token.len <= at || !same_name(bytes + token.at, at, prefix)) {
        return -1;
    }
    while (at + digits < token.len && digit(bytes[token.at + at + digits]) &&
           digits < 2) {
        number = (number * 10) + (bytes[token.at + at + digits] - '0');
        digits++;
    }
    if (digits == 0 || (digits > 1 && bytes[token.at + at] == '0') ||
        number > numbered_registers[which].most ||
        number % numbered_registers[which].step != 0 ||
        !same_name(bytes + token.at + at + digits, token.len - at - digits,
                   suffix)) {
        return -1;
    }
    return (int)number;
}

static int register_kind(const unsigned char *bytes, struct token token)
{
    if (!token.word) {
        return -1;
    }
    for (size_t i = 0; i < sizeof named_registers / sizeof named_registers[0];
         i++) {
        if (same_name(bytes + token.at, token.len, named_registers[i].name)) {
            return named_registers[i].kind;
        }
    }
    for (size_t i = 0;
         i < sizeof numbered_registers / sizeof numbered_registers[0]; i++) {
        int number = numbered(bytes, token, i);
        int kind = numbered_registers[i].kind;
        if (number == 0 && kind == KIND_RN) {
            return KIND_R0;
        }
        if (number == 0 && kind == KIND_FRN) {
            return KIND_FR0;
        }
        if (number >= 0) {
            return kind;
        }
    }
    return -1;
}

static int lone_register(const unsigned char *bytes, size_t at, size_t end)
{
    // the kind of the register that is at..end's only token, or -1
    struct token token;
    struct token more;
    int kind = -1;
    if (next_token(bytes, &at, end, &token)) {
        kind = register_kind(bytes, token);
    }
    return kind >= 0 && !next_token(bytes, &at, end, &more) ? kind : -1;
}

static bool general(int kind)
{
    return kind == KIND_R0 || kind == KIND_RN;
}

static bool word_operator(const unsigned char *bytes, struct token token)
{
    for (size_t i = 0; i < sizeof word_operators / sizeof word_operators[0];
         i++) {
        if (same_name(bytes + token.at, token.len, word_operators[i])) {
            return true;
        }
    }
    return false;
}

static bool expression(const unsigned char *bytes, size_t at, size_t end)
{
    // a nonempty expression: no register, @ or #, and no word right after
    // another unless that one is a word operator. asmsh refuses each.
    struct token token;
    bool any = false;
    bool word = false;
    while (next_token(bytes, &at, end, &token)) {
        if (register_kind(bytes, token) >= 0 || (word && token.word) ||
            is_punctuation(bytes, token, '@') ||
            is_punctuation(bytes, token, '#')) {
            return false;
        }
        word = token.word && !word_operator(bytes, token);
        any = true;
    }
    return any;
}

static int parenthesized(const unsigned char *bytes, size_t at, size_t end)
{
    // the kind of an @( operand, given at..end past its @(. A comma outside
    // inner parentheses separates a displacement or R0 from the base
    // register.
    struct token token;
    size_t open = at;
    size_t comma = 0;
    size_t close = 0;
    bool parted = false;
    size_t depth = 1;
    int base = -1;
    while (depth > 0 && next_token(bytes, &at, end, &token)) {
        if (is_punctuation(bytes, token, '(')) {
            depth++;
        } else if (is_punctuation(bytes, token, ')')) {
            depth--;
            close = token.at;
        } else if (depth == 1 && is_punctuation(bytes, token, ',')) {
            if (parted) {
                return -1;
            }
            parted = true;
            comma = token.at;
        }
    }
    if (depth != 0 || next_token(bytes, &at, end, &token)) {
        return -1;
    }
    if (!parted) {
        return general(lone_register(bytes, open, close)) ? KIND_AT : -1;
    }
    base = lone_register(bytes, comma + 1, close);
    if (lone_register(bytes, open, comma) == KIND_R0) {
        if (general(base)) {
            return KIND_INDEX;
        }
        return base == KIND_GBR ? KIND_GBR_INDEX : -1;
    }
    if (!expression(bytes, open, comma)) {
        return -1;
    }
    if (general(base)) {
        return KIND_DISP;
    }
    if (base == KIND_GBR) {
        return KIND_GBR_DISP;
    }
    return base == KIND_PC ? KIND_PC_DISP : -1;
}

static int indirect(const unsigned char *bytes, size_t at, size_t end)
{
    // the kind of an @ operand, given at..end past its @
    struct token token;
    if (!next_token(bytes, &at, end, &token)) {
        return -1;
    }
    if (is_punctuation(bytes, token, '-')) {
        return general(lone_register(bytes, at, end)) ? KIND_PRE : -1;
    }
    if (is_punctuation(bytes, token, '(')) {
        return parenthesized(bytes, at, end);
    }
    if (!general(register_kind(bytes, token))) {
        return -1;
    }
    if (!next_token(bytes, &at, end, &token)) {
        return KIND_AT;
    }
    return is_punctuation(bytes, token, '+') &&
                   !next_token(bytes, &at, end, &token)
               ? KIND_POST
               : -1;
}

static int operand_kind(const unsigned char *bytes, struct shckit_span operand)
{
    // asmsh allows blanks between an operand's tokens
    size_t at = operand.at;
    size_t end = operand.at + operand.len;
    struct token token;
    int kind = lone_register(bytes, at, end);
    if (kind >= 0) {
        return kind;
    }
    if (!next_token(bytes, &at, end, &token)) {
        return -1;
    }
    if (is_punctuation(bytes, token, '#')) {
        return expression(bytes, at, end) ? KIND_IMM : -1;
    }
    if (is_punctuation(bytes, token, '@')) {
        return indirect(bytes, at, end);
    }
    return expression(bytes, operand.at, end) ? KIND_EXPRESSION : -1;
}

static int form_kind(const char *name, size_t len)
{
    // the kind a form's operand name stands for: a form_kinds name, or a
    // register's own name
    struct token token = {0, len, true};
    for (size_t i = 0; i < sizeof form_kinds / sizeof form_kinds[0]; i++) {
        if (strlen(form_kinds[i].name) == len &&
            memcmp(form_kinds[i].name, name, len) == 0) {
            return form_kinds[i].kind;
        }
    }
    return register_kind((const unsigned char *)name, token);
}

static bool takes(int wanted, int kind)
{
    return kind >= 0 &&
           (wanted == kind || (wanted == KIND_RN && kind == KIND_R0) ||
            (wanted == KIND_FRN && kind == KIND_FR0));
}

static bool fits(const char *form, size_t len, const int *kinds, size_t count)
{
    // whether the form, len bytes long, takes exactly count operands of
    // these kinds. A comma inside parentheses doesn't end a form's operand.
    size_t at = 0;
    size_t seen = 0;
    while (at < len) {
        size_t end = at;
        int depth = 0;
        while (end < len && (form[end] != ',' || depth != 0)) {
            if (form[end] == '(') {
                depth++;
            } else if (form[end] == ')') {
                depth--;
            }
            end++;
        }
        if (seen == count ||
            !takes(form_kind(form + at, end - at), kinds[seen])) {
            return false;
        }
        seen++;
        at = end + 1;
    }
    return seen == count;
}

static bool listed(const char *list, const unsigned char *bytes, size_t len)
{
    // bytes are one of the space-separated names in list, ignoring case
    while (*list != '\0') {
        size_t name = strcspn(list, " ");
        if (same_text(bytes, len, list, name)) {
            return true;
        }
        list += name;
        list += *list == ' ' ? 1 : 0;
    }
    return false;
}

static bool row_fits(size_t row, const unsigned char *name, size_t len,
                     unsigned size, const int *kinds, size_t count)
{
    const char *form = opcodes[row].forms;
    if (!listed(opcodes[row].names, name, len) ||
        strchr(opcodes[row].sizes, (int)size) == NULL) {
        return false;
    }
    for (;;) {
        size_t form_len = strcspn(form, " ");
        if (fits(form, form_len, kinds, count)) {
            return true;
        }
        if (form[form_len] == '\0') {
            return false;
        }
        form += form_len + 1;
    }
}

static bool operand_kinds(const struct shckit_text *text, size_t index,
                          int *kinds, size_t *count)
{
    // the kinds of the operands of the instruction at index and of the
    // continuations joined to it. The comma that ends one line's field or
    // starts the next one's separates their operands, and the empty operand
    // it leaves at the seam is skipped. False for more operands than a form
    // holds, or, when the instruction has operands, for a continuation that
    // doesn't join.
    *count = 0;
    for (size_t i = index; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        struct shckit_span operand;
        size_t at = 0;
        bool more = continued(text, i) && text->lines[index].operands.len != 0;
        bool lead = i > index && !ends_with_comma(line - 1);
        if (more && !joins(text, index, i + 1)) {
            return false;
        }
        while (shckit_operand(line, &at, &operand)) {
            bool trail =
                more && ends_with_comma(line) && at > line->operands.len;
            if (!lead && !trail) {
                if (*count == MOST_OPERANDS) {
                    return false;
                }
                kinds[(*count)++] = operand_kind(line->bytes, operand);
            }
            lead = false;
        }
        if (!more) {
            return true;
        }
    }
    return true;
}

static bool mnemonic(const struct shckit_line *line, size_t *len,
                     unsigned *size)
{
    // split an instruction's name into its mnemonic, the *len bytes before
    // the dot, and its size, the one letter after the dot in capitals, or -
    // for none. False for any other dot.
    const unsigned char *name = line->bytes + line->name.at;
    const unsigned char *dot = memchr(name, '.', line->name.len);
    *len = line->name.len;
    *size = '-';
    if (dot == NULL) {
        return true;
    }
    if (dot + 2 != name + line->name.len || !letter(dot[1])) {
        return false;
    }
    *size = upper(dot[1]);
    *len = (size_t)(dot - name);
    return true;
}

static bool is_instruction(const struct shckit_line *line, const char *name,
                           const char *sizes)
{
    // an instruction of that mnemonic, in any case, and one of those sizes
    size_t len = 0;
    unsigned size = '-';
    return line->kind == SHCKIT_INSTRUCTION && mnemonic(line, &len, &size) &&
           same_name(line->bytes + line->name.at, len, name) &&
           strchr(sizes, (int)size) != NULL;
}

static bool one_of(const struct shckit_line *line, const char *names)
{
    // an instruction whose mnemonic is in names, whatever its size
    size_t len = 0;
    unsigned size = '-';
    return line->kind == SHCKIT_INSTRUCTION && mnemonic(line, &len, &size) &&
           listed(names, line->bytes + line->name.at, len);
}

static int register_of(const struct shckit_line *line,
                       struct shckit_span operand)
{
    // the general register an operand names first, as in Rn, @Rn+, @-Rn and
    // @(R0,Rn), or -1. A displacement before it names none.
    struct token token;
    size_t at = operand.at;
    while (next_token(line->bytes, &at, operand.at + operand.len, &token)) {
        if (token.word) {
            if (same_name(line->bytes + token.at, token.len, "SP")) {
                return 15;
            }
            return numbered(line->bytes, token, 0);
        }
    }
    return -1;
}

static int opcode_row(const struct shckit_text *text, size_t index)
{
    // the row of the opcode table that holds the instruction at index, or -1
    const struct shckit_line *line = shckit_line(text, index);
    size_t len = 0;
    unsigned size = '-';
    int kinds[MOST_OPERANDS];
    size_t count = 0;
    if (line == NULL || line->kind != SHCKIT_INSTRUCTION ||
        !mnemonic(line, &len, &size) ||
        !operand_kinds(text, index, kinds, &count)) {
        return -1;
    }
    for (size_t i = 0; i < sizeof opcodes / sizeof opcodes[0]; i++) {
        if (row_fits(i, line->bytes + line->name.at, len, size, kinds, count)) {
            return (int)i;
        }
    }
    return -1;
}

static unsigned gnu_size(const struct shckit_line *line, int row)
{
    // the size GNU as writes for an instruction the row holds, or - for none
    const char *sizes = opcodes[row].sizes;
    size_t len = 0;
    unsigned size = '-';
    (void)mnemonic(line, &len, &size);
    return (unsigned char)opcodes[row].gnu[strchr(sizes, (int)size) - sizes];
}

int shckit_flow(const struct shckit_text *text, size_t index)
{
    int row = opcode_row(text, index);
    return row >= 0 ? (int)opcodes[row].flow : -1;
}

// Labels: the text's labels, sorted for lookup, and the definitions asmsh
// takes of them.

// A label and the index of the line that defines it, sorted for lookup.
struct name {
    const unsigned char *bytes;
    size_t len;
    size_t index;
};

static int by_name(const void *left, const void *right)
{
    // by length, then bytes, then line index
    const struct name *one = left;
    const struct name *two = right;
    int order = 0;
    if (one->len != two->len) {
        return one->len < two->len ? -1 : 1;
    }
    order = memcmp(one->bytes, two->bytes, one->len);
    if (order != 0) {
        return order;
    }
    if (one->index == two->index) {
        return 0;
    }
    return one->index < two->index ? -1 : 1;
}

static size_t name_lower_bound(const struct name *names, size_t count,
                               const unsigned char *bytes, size_t len)
{
    // the lower bound of bytes in names, sorted by by_name: the position of
    // its first definition, or where one would be
    struct name key = {bytes, len, 0};
    size_t low = 0;
    size_t high = count;
    while (low < high) {
        size_t middle = low + ((high - low) / 2);
        if (by_name(&names[middle], &key) < 0) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

static bool name_matches(const struct name *name, const unsigned char *bytes,
                         size_t len)
{
    return name->len == len && memcmp(name->bytes, bytes, len) == 0;
}

static int sorted_labels(const struct shckit_text *text, struct name **names,
                         size_t *count)
{
    // the labels of all lines, sorted by by_name for lookup and for finding
    // labels defined twice. The caller frees *names.
    *count = 0;
    *names = malloc((text->count != 0 ? text->count : 1) * sizeof **names);
    if (*names == NULL) {
        return SHCKIT_NOMEM;
    }
    for (size_t i = 0; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (line->label.len != 0) {
            struct name name = {line->bytes, line->label.len, i};
            (*names)[(*count)++] = name;
        }
    }
    qsort(*names, *count, sizeof **names, by_name);
    return SHCKIT_OK;
}

static bool asmsh_name(const unsigned char *bytes, size_t len)
{
    // whether asmsh takes the label as a name, which it refuses otherwise
    // with error 201 or 202: no ., a ? only first and followed by 1 to 15
    // bytes, and not $ alone
    if (len == 1 && bytes[0] == '$') {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] == '.' || (bytes[i] == '?' && i != 0)) {
            return false;
        }
    }
    return bytes[0] != '?' || (len >= 2 && len <= 16);
}

static bool defines(const struct shckit_line *line)
{
    // whether asmsh defines the line's label: a name it takes on a line by
    // itself, an instruction, data, a .POOL or, but for a ?label, an .EQU or
    // an .ASSIGN. On any other directive a label draws warning 811, and on an
    // .EQU or an .ASSIGN a ?label draws error 203.
    const unsigned char *bytes = line->bytes + line->label.at;
    if (line->label.len == 0 || !asmsh_name(bytes, line->label.len)) {
        return false;
    }
    return line->kind != SHCKIT_DIRECTIVE || writes_data(line) ||
           line->directive == SHCKIT_POOL || (equates(line) && bytes[0] != '?');
}

static size_t first_definition(const struct name *names, size_t count,
                               const struct shckit_line *lines,
                               const unsigned char *bytes, size_t len)
{
    // the index of the first line asmsh takes as a definition of the label
    // bytes, or SIZE_MAX
    for (size_t i = name_lower_bound(names, count, bytes, len);
         i < count && name_matches(&names[i], bytes, len); i++) {
        if (defines(&lines[names[i].index])) {
            return names[i].index;
        }
    }
    return SIZE_MAX;
}

static size_t other_definition(const struct name *names, size_t count,
                               const struct shckit_line *lines, size_t index)
{
    // the original line number of another definition of line index's label,
    // or 0
    const unsigned char *bytes = lines[index].bytes;
    size_t len = lines[index].label.len;
    for (size_t i = name_lower_bound(names, count, bytes, len);
         i < count && name_matches(&names[i], bytes, len); i++) {
        if (names[i].index != index) {
            return lines[names[i].index].input;
        }
    }
    return 0;
}

static bool starts_block(const struct shckit_line *line, bool first)
{
    // whether the line opens one of asmsh's blocks of ?labels: a .SECTION
    // line, or a label other than a ?label, alone or on an instruction or
    // data, that is its name's first definition, as first says. A label on a
    // .POOL, an .EQU or an .ASSIGN defines its name but opens no block.
    if (line->directive == SHCKIT_SECTION) {
        return true;
    }
    return first && line->label.len != 0 &&
           line->bytes[line->label.at] != '?' &&
           (line->kind != SHCKIT_DIRECTIVE || writes_data(line));
}

// Control through sections, as check and the patches follow it. A block
// runs from a .SECTION line to the next, and a logical section joins every
// block of one name: asmsh resumes a section where its last block left it.

// Control at a section's next statement: it doesn't get there, it falls
// into it, or the statement is a delay slot after which control stops or
// falls through.
enum { FLOW_STOPPED, FLOW_FALLS, FLOW_SLOT, FLOW_SLOT_FALLS };

// asmsh assembles the lines above any .SECTION into code section P.
static const unsigned char default_section[] = "P";

// A logical section. last is the index of its last instruction, which is the
// branch when flow is a slot. doubt is the index of the line after which shckit
// can't tell whether a slot is due, or SIZE_MAX.
struct section {
    const unsigned char *name;
    size_t len;
    int flow;
    size_t last;
    size_t doubt;
};

static void begin(struct section *section, const unsigned char *name,
                  size_t len)
{
    section->name = name;
    section->len = len;
    section->flow = FLOW_STOPPED;
    section->last = 0;
    section->doubt = SIZE_MAX;
}

static size_t enter(const struct shckit_line *line, struct section **sections,
                    size_t *count, size_t *cap)
{
    // the index of the section a .SECTION line starts or resumes, or *count
    // when out of memory
    struct shckit_span name = {0, 0};
    size_t at = 0;
    (void)shckit_operand(line, &at, &name);
    for (size_t i = 0; i < *count; i++) {
        if ((*sections)[i].len == name.len &&
            memcmp((*sections)[i].name, line->bytes + name.at, name.len) == 0) {
            return i;
        }
    }
    if (*count == *cap) {
        struct section *grown = grow(*sections, cap, sizeof **sections, 4);
        if (grown == NULL) {
            return *count;
        }
        *sections = grown;
    }
    begin(&(*sections)[*count], line->bytes + name.at, name.len);
    return (*count)++;
}

static bool slot(int flow)
{
    // the section's next statement is a delay slot
    return flow == FLOW_SLOT || flow == FLOW_SLOT_FALLS;
}

static int next_flow(int flow, int bits)
{
    // control after an instruction with these bits, or after data when
    // bits is -1. A slot is the one statement after its branch, and its own
    // bits don't matter.
    if (flow == FLOW_SLOT) {
        return FLOW_STOPPED;
    }
    if (flow == FLOW_SLOT_FALLS) {
        return FLOW_FALLS;
    }
    if (bits < 0) {
        return flow;
    }
    if ((bits & SHCKIT_DELAYED) == 0) {
        return FLOW_FALLS;
    }
    return (bits & SHCKIT_NO_FALL_THROUGH) != 0 ? FLOW_SLOT : FLOW_SLOT_FALLS;
}

static bool may_fill(const struct shckit_line *line)
{
    // .ALIGN, .POOL and .NOPOOL may write a NOP into a slot, depending on the
    // address, so line order alone doesn't place the slot
    return line->kind == SHCKIT_DIRECTIVE &&
           (line->directive == SHCKIT_ALIGN || line->directive == SHCKIT_POOL ||
            line->directive == SHCKIT_NOPOOL);
}

// check (docs/check.md). The rules table names lint's rules too.

// The rules, in the order findings for one line come.
static const struct {
    const char *name;
    const char *definition;
} rules[SHCKIT_RULES] = {
    [SHCKIT_UNKNOWN_LINE] = {"unknown-line", "a line shckit can't read"},
    [SHCKIT_UNKNOWN_INSTRUCTION] = {"unknown-instruction",
                                    "an instruction whose mnemonic, size or "
                                    "operands the opcode table doesn't hold"},
    [SHCKIT_DATA_IN_FLOW] = {"data-in-flow",
                             "data in a code section that control falls "
                             "into"},
    [SHCKIT_DELAY_SLOT] = {"delay-slot",
                           "an instruction illegal in the delay slot it "
                           "stands in"},
    [SHCKIT_NO_SLOT] = {"no-slot", "a delayed branch with no instruction or "
                                   "data after it in its section"},
    [SHCKIT_AFTER_END] = {"after-end",
                          "a line after .END that isn't blank or a comment"},
    [SHCKIT_COLUMNS] = {"columns",
                        "a field off the column the file keeps for it"},
    [SHCKIT_TAB] = {"tab", "a tab"},
    [SHCKIT_TRAILING_BLANK] = {"trailing-blank",
                               "a blank at the end of the line"},
    [SHCKIT_LINE_ENDS] = {"line-ends", "a line end unlike most of the file's"},
    [SHCKIT_EXPORT_COMMENT] = {"export-comment",
                               "an exported label without a comment line "
                               "right above it"},
};

const char *shckit_rule_name(int rule)
{
    return rule >= 0 && rule < SHCKIT_RULES ? rules[rule].name : NULL;
}

const char *shckit_rule_definition(int rule)
{
    return rule >= 0 && rule < SHCKIT_RULES ? rules[rule].definition : NULL;
}

// Findings as the rules collect them, unsorted.
struct findings {
    struct shckit_finding *at;
    size_t count;
    size_t cap;
    bool failed; // allocation failed, so the findings are incomplete
};

static void refuse(struct findings *found, size_t index, int rule)
{
    if (found->failed) {
        return;
    }
    if (found->count == found->cap) {
        struct shckit_finding *grown =
            grow(found->at, &found->cap, sizeof *found->at, 64);
        if (grown == NULL) {
            found->failed = true;
            return;
        }
        found->at = grown;
    }
    found->at[found->count].index = index;
    found->at[found->count].rule = rule;
    found->count++;
}

static int by_line(const void *left, const void *right)
{
    const struct shckit_finding *one = left;
    const struct shckit_finding *two = right;
    if (one->index != two->index) {
        return one->index < two->index ? -1 : 1;
    }
    if (one->rule != two->rule) {
        return one->rule < two->rule ? -1 : 1;
    }
    return 0;
}

static int hand_over(struct findings *found, struct shckit_finding **out,
                     size_t *count)
{
    // sort the findings, drop repeats, and hand the array to the caller
    size_t kept = 0;
    *out = NULL;
    *count = 0;
    if (found->failed) {
        free(found->at);
        return SHCKIT_NOMEM;
    }
    if (found->count != 0) {
        qsort(found->at, found->count, sizeof *found->at, by_line);
    }
    for (size_t i = 0; i < found->count; i++) {
        if (kept == 0 || by_line(&found->at[kept - 1], &found->at[i]) != 0) {
            found->at[kept++] = found->at[i];
        }
    }
    *out = found->at;
    *count = kept;
    return SHCKIT_OK;
}

static void check_flow(const struct shckit_text *text, struct findings *found)
{
    // follow control through each logical section to the first .END, past
    // which asmsh reads nothing. Lines that write nothing leave control as
    // it is, .ALIGN, .POOL and .NOPOOL included: whether one writes a NOP
    // into a slot depends on the address, and asmsh branches around a pool
    // that control falls into. A slot still open at the end is whatever
    // follows the section in memory.
    struct section *sections = malloc(4 * sizeof *sections);
    size_t count = 1;
    size_t cap = 4;
    size_t here = 0;
    bool ended = false;
    if (sections == NULL) {
        found->failed = true;
        return;
    }
    begin(&sections[0], default_section, 1);
    for (size_t i = 0; i < text->count && !found->failed; i++) {
        const struct shckit_line *line = &text->lines[i];
        struct section *in = &sections[here];
        if (ended) {
            if (line->kind != SHCKIT_BLANK && line->kind != SHCKIT_COMMENT) {
                refuse(found, i, SHCKIT_AFTER_END);
            }
        } else if (line->kind == SHCKIT_INSTRUCTION) {
            int bits = shckit_flow(text, i);
            if (slot(in->flow) && (bits & SHCKIT_SLOT_ILLEGAL) != 0) {
                refuse(found, i, SHCKIT_DELAY_SLOT);
            }
            // a trap's handler may not come back: SHC ends an interrupt
            // function with TRAPA under tn= and writes its pool right after
            in->flow = one_of(line, "TRAPA") ? FLOW_STOPPED
                                             : next_flow(in->flow, bits);
            in->last = i;
        } else if (writes_data(line)) {
            if (line->code && in->flow != FLOW_STOPPED) {
                refuse(found, i, SHCKIT_DATA_IN_FLOW);
            }
            in->flow = next_flow(in->flow, -1);
        } else if (line->directive == SHCKIT_SECTION) {
            here = enter(line, &sections, &count, &cap);
            found->failed = here == count;
        } else if (line->directive == SHCKIT_END) {
            ended = true;
        }
    }
    for (size_t i = 0; i < count; i++) {
        if (slot(sections[i].flow)) {
            refuse(found, sections[i].last, SHCKIT_NO_SLOT);
        }
    }
    free(sections);
}

int shckit_check(const struct shckit_text *text, struct shckit_finding **found,
                 size_t *count)
{
    // the flow rules need every line up to the first .END read and every
    // instruction there in the table. Otherwise those lines are the only
    // findings.
    struct findings list = {NULL, 0, 0, false};
    size_t head = SIZE_MAX;
    bool ended = false;
    for (size_t i = 0; i < text->count && !ended; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (unreadable(text, i, &head)) {
            refuse(&list, i, SHCKIT_UNKNOWN_LINE);
        } else if (line->kind == SHCKIT_INSTRUCTION &&
                   shckit_flow(text, i) < 0) {
            refuse(&list, i, SHCKIT_UNKNOWN_INSTRUCTION);
        }
        ended = line->directive == SHCKIT_END;
    }
    if (list.count == 0) {
        check_flow(text, &list);
    }
    return hand_over(&list, found, count);
}

// lint (docs/lint.md).

// A column a field starts at, and how many lines start it there.
struct column {
    size_t at;
    size_t count;
};

// The columns one field starts at on one kind of statement line, in the
// order of their first lines.
struct tally {
    struct column *at;
    size_t count;
    size_t cap;
};

// A statement's fields, and the kinds of statement line: alone on its line,
// or after a label, which pushes its fields right.
enum { FIELD_NAME, FIELD_OPERANDS, FIELD_COMMENT, FIELDS };
enum { ALONE, LABELED, PLACES };

static bool count_column(struct tally *tally, size_t at)
{
    for (size_t i = 0; i < tally->count; i++) {
        if (tally->at[i].at == at) {
            tally->at[i].count++;
            return true;
        }
    }
    if (tally->count == tally->cap) {
        struct column *grown =
            grow(tally->at, &tally->cap, sizeof *tally->at, 8);
        if (grown == NULL) {
            return false;
        }
        tally->at = grown;
    }
    tally->at[tally->count].at = at;
    tally->at[tally->count].count = 1;
    tally->count++;
    return true;
}

static size_t file_column(const struct tally *tally)
{
    // the column most lines start the field at, or on a tie the first's
    size_t best = 0;
    for (size_t i = 1; i < tally->count; i++) {
        if (tally->at[i].count > tally->at[best].count) {
            best = i;
        }
    }
    return tally->count != 0 ? tally->at[best].at : 0;
}

static bool statement_field(const struct shckit_line *line, int which,
                            struct shckit_span *part, size_t *before)
{
    // a statement's field, and where the text before it ends. False for a
    // field the statement doesn't have.
    size_t name_end = line->name.at + line->name.len;
    size_t operands_end = line->operands.at + line->operands.len;
    switch (which) {
    case FIELD_NAME:
        *part = line->name;
        *before = line->label.len != 0 ? line->label.len + 1 : 0;
        return true;
    case FIELD_OPERANDS:
        *part = line->operands;
        *before = name_end;
        return line->operands.len != 0;
    default:
        *part = line->comment;
        *before = line->operands.len != 0 ? operands_end : name_end;
        return line->comment.len != 0;
    }
}

static bool statement_line(const struct shckit_line *line)
{
    return line->kind == SHCKIT_INSTRUCTION || line->kind == SHCKIT_DIRECTIVE;
}

static void lint_columns(const struct shckit_text *text, struct findings *found)
{
    // refuse a line with a field off the file's column for it. A field whose
    // preceding text reaches that column must start after a blank instead.
    // The columns are the file's own: where most of its lines of the kind
    // start the field.
    struct tally tallies[PLACES][FIELDS];
    size_t columns[PLACES][FIELDS];
    memset(tallies, 0, sizeof tallies);
    for (size_t i = 0; i < text->count && !found->failed; i++) {
        const struct shckit_line *line = &text->lines[i];
        int place = line->label.len != 0 ? LABELED : ALONE;
        for (int which = 0; statement_line(line) && which < FIELDS; which++) {
            struct shckit_span part;
            size_t before = 0;
            if (statement_field(line, which, &part, &before) &&
                !count_column(&tallies[place][which], part.at)) {
                found->failed = true;
            }
        }
    }
    for (int place = 0; place < PLACES; place++) {
        for (int which = 0; which < FIELDS; which++) {
            columns[place][which] = file_column(&tallies[place][which]);
            free(tallies[place][which].at);
        }
    }
    for (size_t i = 0; i < text->count && !found->failed; i++) {
        const struct shckit_line *line = &text->lines[i];
        int place = line->label.len != 0 ? LABELED : ALONE;
        bool off = false;
        for (int which = 0; statement_line(line) && which < FIELDS; which++) {
            struct shckit_span part;
            size_t before = 0;
            size_t column = columns[place][which];
            if (statement_field(line, which, &part, &before)) {
                off = off ||
                      (before < column ? part.at != column : part.at <= before);
            }
        }
        if (off) {
            refuse(found, i, SHCKIT_COLUMNS);
        }
    }
}

static void lint_exports(const struct shckit_text *text, struct findings *found)
{
    // refuse each label an .EXPORT names, on its line or a continuation
    // joined to it, without a comment line right above its definition
    struct name *names = NULL;
    size_t named = 0;
    if (sorted_labels(text, &names, &named) != SHCKIT_OK) {
        found->failed = true;
        return;
    }
    for (size_t i = 0; i < text->count; i++) {
        bool exports = text->lines[i].kind == SHCKIT_DIRECTIVE &&
                       text->lines[i].directive == SHCKIT_EXPORT;
        for (size_t j = i; exports; j++) {
            const struct shckit_line *line = &text->lines[j];
            struct shckit_span symbol;
            size_t at = 0;
            while (shckit_operand(line, &at, &symbol)) {
                size_t defined =
                    first_definition(names, named, text->lines,
                                     line->bytes + symbol.at, symbol.len);
                if (defined != SIZE_MAX &&
                    (defined == 0 ||
                     text->lines[defined - 1].kind != SHCKIT_COMMENT)) {
                    refuse(found, defined, SHCKIT_EXPORT_COMMENT);
                }
            }
            exports = continued(text, j) && joins(text, i, j + 1);
        }
    }
    free(names);
}

int shckit_lint(const struct shckit_text *text, struct shckit_finding **found,
                size_t *count)
{
    struct findings list = {NULL, 0, 0, false};
    size_t crlf = 0;
    size_t head = SIZE_MAX;
    bool most_crlf = false;
    for (size_t i = 0; i < text->count; i++) {
        crlf += text->lines[i].crlf ? 1 : 0;
    }
    most_crlf =
        text->count != 0 && (crlf * 2 > text->count ||
                             (crlf * 2 == text->count && text->lines[0].crlf));
    for (size_t i = 0; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (unreadable(text, i, &head)) {
            refuse(&list, i, SHCKIT_UNKNOWN_LINE);
        }
        if (memchr(line->bytes, '\t', line->len) != NULL) {
            refuse(&list, i, SHCKIT_TAB);
        }
        if (line->len != 0 && blank(line->bytes[line->len - 1])) {
            refuse(&list, i, SHCKIT_TRAILING_BLANK);
        }
        if (line->crlf != most_crlf) {
            refuse(&list, i, SHCKIT_LINE_ENDS);
        }
    }
    lint_columns(text, &list);
    if (!list.failed) {
        lint_exports(text, &list);
    }
    return hand_over(&list, found, count);
}

// What the patches share (docs/patches.md). Each patch pushes the lines it
// leaves onto a list, which adopt makes the text's lines, or on a refusal
// discards.

// SHCASM's .ALIGN 4 after inline text and .RES.W 1 before a pool's longs,
// which align-pool writes too, and .NOPOOL and .POOL in the same columns for
// use-assembler-pools.
static const unsigned char align_line[] = "          .ALIGN      4";
static const unsigned char reserve_line[] = "          .RES.W      1";
static const unsigned char nopool_line[] = "          .NOPOOL";
static const unsigned char pool_line[] = "          .POOL";

static const unsigned char *const written_lines[] = {
    [SHCKIT_ALIGN] = align_line,
    [SHCKIT_RES_W] = reserve_line,
    [SHCKIT_POOL] = pool_line,
    [SHCKIT_NOPOOL] = nopool_line,
};

// SHCASM's NOP, which SHC writes after a copy into R0 under -extra=a=400,
// and in a delay slot it fills with nothing else.
static const unsigned char nop_line[] = "          NOP";

// SHCASM's mnemonic or directive column, and its operand column.
#define NAME_COLUMN 10
#define OPERAND_COLUMN 22

// The length of SHCASM's pool label line: the label, then spaces through
// column 33, where the comment field starts.
#define POOL_LABEL_LEN 34

static bool shcasm_columns(const struct shckit_line *line)
{
    // a statement exactly in SHCASM's columns: no label, spaces up to the
    // name, spaces from it up to the operands, and nothing after them. The
    // reader puts a blank between a name and its operands, so operands at
    // OPERAND_COLUMN means the name ends before it and the span below is
    // never reversed.
    const unsigned char *bytes = line->bytes;
    size_t name_end = line->name.at + line->name.len;
    return line->label.len == 0 && line->name.at == NAME_COLUMN &&
           all(bytes, span(0, NAME_COLUMN), space) &&
           line->operands.at == OPERAND_COLUMN &&
           all(bytes, span(name_end, OPERAND_COLUMN), space) &&
           line->comment.len == 0 &&
           line->operands.at + line->operands.len == line->len;
}

static int write_line(struct list *out, const struct shckit_line *anchor,
                      int directive, int patch)
{
    // push the line patch writes for directive: SHCASM's .ALIGN 4 or
    // .RES.W 1, or .NOPOOL or .POOL in its columns. It takes anchor's
    // section and line end, and is typed as the reader would type its bytes.
    const unsigned char *bytes = written_lines[directive];
    size_t len = strlen((const char *)bytes);
    size_t name_end = NAME_COLUMN + strlen(directives[directive]);
    struct shckit_line line;
    memset(&line, 0, sizeof line);
    line.bytes = bytes;
    line.len = len;
    line.crlf = anchor->crlf;
    line.code = anchor->code;
    line.kind = SHCKIT_DIRECTIVE;
    line.directive = directive;
    line.name = span(NAME_COLUMN, name_end);
    line.operands = name_end < len ? span(OPERAND_COLUMN, len) : span(len, len);
    line.comment = span(len, len);
    line.patch = patch;
    return push(out, &line) ? SHCKIT_OK : SHCKIT_NOMEM;
}

static int adopt(struct shckit_text *text, struct list *list, int error)
{
    // commit a patch's lines as the text's and number the pools again, or on
    // error discard them and keep the text's lines
    if (error != SHCKIT_OK) {
        free(list->at);
        return error;
    }
    free(text->lines);
    text->lines = list->at;
    text->count = list->count;
    find_pools(text);
    return SHCKIT_OK;
}

static int fail_at(size_t *refused, int error, size_t line)
{
    // refuse with error, naming the line's original number in *refused
    *refused = line;
    return error;
}

static int fail(struct shckit_text *text, int error, size_t line)
{
    return fail_at(&text->refused, error, line);
}

static unsigned char *own(struct shckit_text *text, size_t len)
{
    // allocate len bytes for a rewritten line. The text owns them until
    // shckit_free, unless disown frees them first. len is never 0: a
    // rewritten load holds at least its #.
    unsigned char *bytes = NULL;
    if (text->owned_count == text->owned_cap) {
        struct owned *grown =
            grow(text->owned, &text->owned_cap, sizeof *text->owned, 64);
        if (grown == NULL) {
            return NULL;
        }
        text->owned = grown;
    }
    bytes = malloc(len);
    if (bytes != NULL) {
        text->owned[text->owned_count++].bytes = bytes;
    }
    return bytes;
}

static void disown(struct shckit_text *text, size_t kept)
{
    // free the owned buffers after the first kept. When an apply fails,
    // shckit_apply frees the buffers its patches owned.
    while (text->owned_count > kept) {
        free(text->owned[--text->owned_count].bytes);
    }
}

// Where a line stands, from place_slots. section numbers the logical sections
// from 0 for P, in the order the text names them. section_line is the index of
// the .SECTION line that opens the line's block. branch is the index of the
// delayed branch whose slot the line fills, or, for an .ALIGN, .POOL or .NOPOOL
// where a slot is due, may fill. doubt is the index of the line after which
// shckit can't tell whether a slot is due: an instruction outside the opcode
// table, or a branch whose slot an .ALIGN, .POOL or .NOPOOL may fill.
// section_line, branch and doubt are SIZE_MAX for none. A .SECTION line stands
// in the section it opens.
struct place {
    size_t section;
    size_t section_line;
    size_t branch;
    size_t doubt;
};

// What place_slots does with a slot it can't place: record the doubt, or
// refuse the text.
enum { SLOTS_MAY_DOUBT, SLOTS_MUST_PLACE };

static int place_slots(const struct shckit_text *text, struct place *places,
                       int slots, size_t *refused)
{
    // set each line's place, following control through each section to the
    // first .END as check_flow does. SLOTS_MAY_DOUBT records doubt where it
    // can't place a slot. SLOTS_MUST_PLACE, for a command that writes lines
    // by the slots, refuses instead: an instruction outside the opcode table,
    // and a branch whose slot is in doubt, is data or an instruction
    // illegal there, waits for its section to resume, or never comes.
    const struct shckit_line *lines = text->lines;
    bool strict = slots == SLOTS_MUST_PLACE;
    struct section *sections = malloc(4 * sizeof *sections);
    size_t count = 1;
    size_t cap = 4;
    size_t here = 0;
    size_t section_line = SIZE_MAX;
    bool ended = false;
    int error = SHCKIT_OK;
    if (sections == NULL) {
        return SHCKIT_NOMEM;
    }
    begin(&sections[0], default_section, 1);
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *line = &lines[i];
        struct section *in = &sections[here];
        bool due = !ended && slot(in->flow) && in->doubt == SIZE_MAX;
        places[i].section = here;
        places[i].section_line = section_line;
        places[i].branch = SIZE_MAX;
        places[i].doubt = ended ? SIZE_MAX : in->doubt;
        if (ended) {
            continue;
        }
        if (line->kind == SHCKIT_INSTRUCTION) {
            int bits = shckit_flow(text, i);
            places[i].branch = due ? in->last : SIZE_MAX;
            if (strict && bits < 0) {
                error = fail_at(refused, SHCKIT_NOT_HELD, line->input);
            } else if (strict && due && (bits & SHCKIT_SLOT_ILLEGAL) != 0) {
                error = fail_at(refused, SHCKIT_SLOT, lines[in->last].input);
            }
            if (due) {
                // a branch in a slot opens no slot of its own
                in->flow = next_flow(in->flow, 0);
            } else if (in->doubt != SIZE_MAX) {
                // only an instruction that can't be a delayed branch settles
                // the doubt: no slot is due after it, slot or not
                if (bits >= 0 && (bits & SHCKIT_DELAYED) == 0) {
                    in->doubt = SIZE_MAX;
                    in->flow = FLOW_FALLS;
                }
            } else if (bits < 0) {
                in->doubt = i; // it may be a delayed branch
            } else {
                in->flow = next_flow(in->flow, bits);
            }
            in->last = i;
        } else if (writes_data(line)) {
            places[i].branch = due ? in->last : SIZE_MAX;
            if (strict && due) {
                error = fail_at(refused, SHCKIT_SLOT, lines[in->last].input);
            }
            // data opens no slot, so it settles any doubt
            in->flow =
                in->doubt == SIZE_MAX ? next_flow(in->flow, -1) : FLOW_FALLS;
            in->doubt = SIZE_MAX;
        } else if (may_fill(line) && due) {
            places[i].branch = in->last;
            if (strict) {
                error = fail_at(refused, SHCKIT_SLOT, lines[in->last].input);
            }
            // whether this fills the slot depends on the address
            in->doubt = in->last;
        } else if (line->directive == SHCKIT_SECTION) {
            // the slot due here waits for its section to resume
            if (strict && due) {
                error = fail_at(refused, SHCKIT_SLOT, lines[in->last].input);
            }
            here = enter(line, &sections, &count, &cap);
            error = here == count ? SHCKIT_NOMEM : error;
            section_line = i;
            places[i].section = here;
        } else if (line->directive == SHCKIT_END) {
            ended = true;
        }
    }
    // a slot still due at the end never comes
    for (size_t i = 0; strict && error == SHCKIT_OK && i < count; i++) {
        if (slot(sections[i].flow)) {
            error =
                fail_at(refused, SHCKIT_SLOT, lines[sections[i].last].input);
        }
    }
    free(sections);
    return error;
}

static bool pool_label_line(const struct shckit_line *line)
{
    // for a pool's label, exactly the line SHCASM's pool writer prints: L, a
    // number without a leading zero, a colon, and spaces through column 33
    const unsigned char *bytes = line->bytes;
    size_t colon = line->label.len;
    return line->len == POOL_LABEL_LEN && colon < POOL_LABEL_LEN &&
           bytes[1] != '0' && bytes[colon] == ':' &&
           all(bytes, span(colon + 1, line->len), space);
}

// Pool item kinds, in the order SHCASM's pool writer prints them.
enum { POOL_WORD, POOL_PAD, POOL_LONG };

static int pool_item_kind(const struct shckit_line *item)
{
    // POOL_WORD for .DATA.W x, POOL_PAD for .RES.W 1 and POOL_LONG for
    // .DATA.L x, each exactly in SHCASM's columns with one operand and no
    // string, or -1
    struct shckit_span operand;
    struct shckit_span more;
    size_t at = 0;
    if (!shcasm_columns(item) || !shckit_operand(item, &at, &operand) ||
        shckit_operand(item, &at, &more) ||
        memchr(item->bytes + operand.at, '"', operand.len) != NULL) {
        return -1;
    }
    switch (item->directive) {
    case SHCKIT_DATA_W:
        return POOL_WORD;
    case SHCKIT_RES_W:
        return is(item->bytes, operand, "1") ? POOL_PAD : -1;
    case SHCKIT_DATA_L:
        return POOL_LONG;
    default:
        return -1;
    }
}

static size_t pool_end(const struct shckit_text *text, size_t label)
{
    // the index after the last line of the pool whose label is at label
    size_t end = label + 1;
    while (end < text->count &&
           text->lines[end].pool == text->lines[label].pool) {
        end++;
    }
    return end;
}

static size_t top_of_blanks(const struct shckit_text *text, size_t index)
{
    // the index of the topmost line of the blank and comment lines right
    // above index, or index when the line above is neither. The nearest
    // other line above is then the one before the result.
    while (index > 0 && (text->lines[index - 1].kind == SHCKIT_BLANK ||
                         text->lines[index - 1].kind == SHCKIT_COMMENT)) {
        index--;
    }
    return index;
}

static bool label_difference(const struct shckit_line *line)
{
    // the operand field is exactly Lnn-Lnn, as SHCASM's table writer prints a
    // switch table's entries
    const unsigned char *bytes = line->bytes + line->operands.at;
    size_t len = line->operands.len;
    size_t at = 1;
    if (len < 5 || bytes[0] != 'L') {
        return false;
    }
    while (at < len && digit(bytes[at])) {
        at++;
    }
    if (at == 1 || at + 2 >= len || bytes[at] != '-' || bytes[at + 1] != 'L') {
        return false;
    }
    return all(bytes, span(at + 2, len), digit);
}

static int find_switch(struct shckit_text *text)
{
    // refuse a switch table entry on a pool's line. This runs before the
    // pool checks: the table writer's pad can stand before a table as a pool
    // of its own, which they would refuse for another reason.
    for (size_t i = 0; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (line->pool != 0 && line->kind == SHCKIT_DIRECTIVE &&
            label_difference(line)) {
            return fail(text, SHCKIT_SWITCH, line->input);
        }
    }
    return SHCKIT_OK;
}

// What an operand's words refer to: a code label; a word that isn't a label
// here, such as a number or a symbol defined elsewhere; or the position of
// the code itself, as $ and the PC of @(disp,PC) do.
enum { NAMES_CODE = 1, NAMES_OTHER = 2, NAMES_HERE = 4 };

static unsigned operand_names(const struct shckit_text *text,
                              const struct name *names, size_t named,
                              const bool *code, const struct shckit_line *line,
                              struct shckit_span operand)
{
    // code[i] says whether line i defines a code label. A data label or a
    // pool's label sets no bit, and an .EQU symbol that names no code label
    // counts as a number, as a symbol .ASSIGN defines does. A ?label counts
    // as a code label wherever it is defined: asmsh takes the definition in
    // the use's block, which nothing here follows.
    const unsigned char *bytes = line->bytes;
    unsigned found =
        operand_kind(bytes, operand) == KIND_PC_DISP ? NAMES_HERE : 0;
    struct token token;
    size_t at = operand.at;
    while (next_token(bytes, &at, operand.at + operand.len, &token)) {
        size_t label = SIZE_MAX;
        if (!token.word) {
            continue;
        }
        if (token.len == 1 && bytes[token.at] == '$') {
            found |= NAMES_HERE;
            continue;
        }
        if (bytes[token.at] == '?') {
            found |= NAMES_CODE;
            continue;
        }
        label = first_definition(names, named, text->lines, bytes + token.at,
                                 token.len);
        if (label == SIZE_MAX ||
            (equates(&text->lines[label]) && !code[label])) {
            found |= NAMES_OTHER;
        } else if (code[label]) {
            found |= NAMES_CODE;
        }
    }
    return found;
}

static bool equ_names_code(const struct shckit_text *text,
                           const struct name *names, size_t named,
                           const bool *code, size_t index)
{
    const struct shckit_line *line = &text->lines[index];
    struct shckit_span value;
    size_t at = 0;
    return line->directive == SHCKIT_EQU && shckit_operand(line, &at, &value) &&
           (operand_names(text, names, named, code, line, value) &
            NAMES_CODE) != 0;
}

static int find_offsets(const struct shckit_text *text, size_t *refused,
                        const struct name *names, size_t named)
{
    // refuse, up to the first .END, an operand that offsets a place in code:
    // a code label combined with a word that isn't a label, as in _f+6, or
    // $ or @(disp,PC). A patch that writes or drops lines moves code, and an
    // offset taken before the move may then point elsewhere. A label alone,
    // or the difference of two, moves with its code. Code labels are the
    // labels of code-section lines other than pool labels, and .EQU symbols
    // whose value names one. An .EQU can name another, so the second loop
    // runs to a fixed point. A symbol .ASSIGN defines can hold another value
    // at each use, and nothing here follows which, so an .ASSIGN whose value
    // names a code label is refused too, and its symbol counts as a number.
    bool *code = calloc(text->count != 0 ? text->count : 1, sizeof *code);
    bool more = true;
    int error = code != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        code[i] = line->label.len != 0 && line->code && !equates(line) &&
                  !(line->pool != 0 && line->kind == SHCKIT_LABEL);
    }
    while (error == SHCKIT_OK && more) {
        more = false;
        for (size_t i = 0; i < text->count; i++) {
            if (!code[i] && equ_names_code(text, names, named, code, i)) {
                code[i] = true;
                more = true;
            }
        }
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        struct shckit_span operand;
        size_t at = 0;
        while (error == SHCKIT_OK && shckit_operand(line, &at, &operand)) {
            unsigned found =
                operand_names(text, names, named, code, line, operand);
            if ((found & NAMES_HERE) != 0 ||
                (found & (NAMES_CODE | NAMES_OTHER)) ==
                    (NAMES_CODE | NAMES_OTHER) ||
                (line->directive == SHCKIT_ASSIGN &&
                 (found & NAMES_CODE) != 0)) {
                error = fail_at(refused, SHCKIT_OFFSET, line->input);
            }
        }
        if (line->directive == SHCKIT_END) {
            break;
        }
    }
    free(code);
    return error;
}

// drop-inline-align: drop each code-section line that is exactly SHCASM's
// .ALIGN 4, which SHCASM prints after every piece of inline text, and refuse
// any other code-section .ALIGN 4, which may be the programmer's. The line's
// exact bytes are the only sign of where inline text ended. No continuation
// can follow SHCASM's line, whose field doesn't end with a comma:
// preflight has refused one already.
static int drop_inline_align(struct shckit_text *text)
{
    struct list kept = {NULL, 0, 0};
    int error = SHCKIT_OK;
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *each = &text->lines[i];
        struct shckit_span value = each->operands;
        // .ALIGN 04 aligns to 4 too, and isn't SHCASM's line
        while (value.len > 1 && each->bytes[value.at] == '0') {
            value.at++;
            value.len--;
        }
        if (!each->code || each->directive != SHCKIT_ALIGN ||
            !is(each->bytes, value, "4")) {
            error = push(&kept, each) ? SHCKIT_OK : SHCKIT_NOMEM;
        } else if (!is_line(each, align_line, sizeof align_line - 1)) {
            error = fail(text, SHCKIT_OTHER_ALIGN, each->input);
        }
    }
    return adopt(text, &kept, error);
}

// align-pool: write SHCASM's .ALIGN 4 before each pool's label, then .RES.W 1
// where SHCASM counted the label at a half word. The pool's own items record
// that count, so no address is computed.

static int check_aligned_pool(struct shckit_text *text, size_t label,
                              const struct name *names, size_t named, bool *pad)
{
    // *pad is whether the items before the first .DATA.L take 2 bytes mod 4,
    // which is when SHCASM counted the label at a half word
    const struct shckit_line *lines = text->lines;
    size_t end = pool_end(text, label);
    size_t above = top_of_blanks(text, label);
    size_t ahead = 0;
    size_t again = 0;
    int last = POOL_WORD;

    if (!pool_label_line(&lines[label])) {
        return fail(text, SHCKIT_POOL_LABEL, lines[label].input);
    }
    // SHCASM prints a pool's label right after code, so the nearest line
    // above must be an instruction. A label there, alone or on a line that
    // takes no bytes, would name the pool's address too, and the lines
    // written before the pool's label would separate the two.
    if (above == 0 || lines[above - 1].kind != SHCKIT_INSTRUCTION) {
        return fail(text, SHCKIT_ABOVE, lines[label].input);
    }
    for (size_t i = label + 1; i < end; i++) {
        int kind = pool_item_kind(&lines[i]);
        if (kind < 0) {
            return fail(text, SHCKIT_ITEM, lines[i].input);
        }
        // words, at most one pad, then longs
        if (kind < last || (kind == POOL_PAD && last == POOL_PAD)) {
            return fail(text, SHCKIT_SHAPE, lines[i].input);
        }
        ahead += kind != POOL_LONG ? 2 : 0;
        last = kind;
    }
    if (last != POOL_LONG) {
        return fail(text, SHCKIT_NO_LONG, lines[label].input);
    }
    again = other_definition(names, named, lines, label);
    if (again != 0) {
        return fail(text, SHCKIT_DUPLICATE, again);
    }
    *pad = ahead % 4 == 2;
    return SHCKIT_OK;
}

static int align_pool(struct shckit_text *text)
{
    struct list out = {NULL, 0, 0};
    struct name *names = NULL;
    size_t named = 0;
    int error = find_switch(text);
    if (error == SHCKIT_OK) {
        error = sorted_labels(text, &names, &named);
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        bool pad = false;
        if (line->pool != 0 && line->kind == SHCKIT_LABEL) {
            error = check_aligned_pool(text, i, names, named, &pad);
            if (error == SHCKIT_OK) {
                error = write_line(&out, line, SHCKIT_ALIGN, SHCKIT_ALIGN_POOL);
            }
            if (error == SHCKIT_OK && pad) {
                error = write_line(&out, line, SHCKIT_RES_W, SHCKIT_ALIGN_POOL);
            }
        }
        if (error == SHCKIT_OK && !push(&out, line)) {
            error = SHCKIT_NOMEM;
        }
    }
    free(names);
    return adopt(text, &out, error);
}

// use-assembler-pools: replace SHCASM's literal pools with literals asmsh
// manages. Each load from a pool becomes a # load of the item it reads, a
// MOVA of a pool's long becomes MOVA of # and the long's value, and each
// pool becomes .ALIGN 4 and .POOL, with .NOPOOL after the slot of each
// branch that never falls through while a literal waits. No address is
// computed. The checks
// refuse the uses and layout dependencies the patch can't account for; they
// don't prove the result equivalent, and callers choose the patch for output
// validated against its target. A text without pools stays as it is.

static bool literal_load(const struct shckit_text *text, size_t index)
{
    // MOV.W #x, MOV.L #x or MOVA #x, whose x asmsh writes into a pool itself
    const struct shckit_line *line = &text->lines[index];
    int kinds[MOST_OPERANDS];
    size_t count = 0;
    return (is_instruction(line, "MOV", "WL") ||
            is_instruction(line, "MOVA", "-L")) &&
           operand_kinds(text, index, kinds, &count) && count != 0 &&
           kinds[0] == KIND_IMM;
}

static int find_pooled(struct shckit_text *text)
{
    // refuse, up to the first .END, a line that already hands asmsh a pool:
    // the .NOPOOL lines this patch writes could carry its literals past the
    // pool meant for them
    for (size_t i = 0; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (line->directive == SHCKIT_POOL ||
            line->directive == SHCKIT_NOPOOL || literal_load(text, i)) {
            return fail(text, SHCKIT_POOLED, line->input);
        }
        if (line->directive == SHCKIT_END) {
            break;
        }
    }
    return SHCKIT_OK;
}

static bool is_literal(const struct shckit_text *text, const struct name *names,
                       size_t named, const struct shckit_line *item)
{
    // whether the item's value can be a # immediate: an expression without
    // $, which asmsh would read as the literal's own address, and without a
    // symbol .ASSIGN defines, which can hold another value at the load
    const unsigned char *bytes = item->bytes;
    struct shckit_span value;
    struct token token;
    size_t at = 0;
    size_t end = 0;
    if (!shckit_operand(item, &at, &value)) {
        return false;
    }
    at = value.at;
    end = value.at + value.len;
    if (!expression(bytes, at, end)) {
        return false;
    }
    while (next_token(bytes, &at, end, &token)) {
        size_t defined = token.word
                             ? first_definition(names, named, text->lines,
                                                bytes + token.at, token.len)
                             : SIZE_MAX;
        if ((token.word && token.len == 1 && bytes[token.at] == '$') ||
            (defined != SIZE_MAX &&
             text->lines[defined].directive == SHCKIT_ASSIGN)) {
            return false;
        }
    }
    return true;
}

static bool stops(const struct shckit_text *text, const struct place *places,
                  size_t index)
{
    // whether the instruction at index is the slot of RTS, RTE, BRA, BRAF or
    // JMP, the branches that never fall through. asmsh writes waiting
    // literals right after such a slot and no other.
    size_t branch = places[index].branch;
    return text->lines[index].kind == SHCKIT_INSTRUCTION &&
           branch != SIZE_MAX &&
           (shckit_flow(text, branch) & SHCKIT_NO_FALL_THROUGH) != 0;
}

static size_t slot_label(const struct shckit_text *text,
                         const struct place *places, size_t index)
{
    // the first line with a label below the branch whose slot is at index,
    // up to the slot, or SIZE_MAX. asmsh writes no pool after a slot so
    // labelled, and refuses a .NOPOOL there.
    for (size_t i = places[index].branch + 1; i <= index; i++) {
        if (text->lines[i].label.len != 0) {
            return i;
        }
    }
    return SIZE_MAX;
}

static bool jumps_over(const struct shckit_text *text, size_t branch,
                       size_t end)
{
    // whether branch is a BRA to a label right below the pool that ends at
    // end, past blank, comment and label lines: SHC's own guard
    const struct shckit_line *bra = &text->lines[branch];
    struct shckit_span target;
    size_t at = 0;
    if (!is_instruction(bra, "BRA", "-") ||
        !shckit_operand(bra, &at, &target)) {
        return false;
    }
    for (size_t i = end; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (line->label.len == target.len &&
            memcmp(line->bytes, bra->bytes + target.at, target.len) == 0) {
            return true;
        }
        if (line->kind != SHCKIT_BLANK && line->kind != SHCKIT_COMMENT &&
            line->kind != SHCKIT_LABEL) {
            return false;
        }
    }
    return false;
}

static int check_handed_pool(struct shckit_text *text, size_t label,
                             const struct place *places,
                             const struct name *names, size_t named)
{
    // unlike align-pool, take items in any order. The label must be right
    // below the slot of a branch that never falls through, and that branch
    // mustn't be SHC's guard around the pool, which asmsh's would duplicate.
    const struct shckit_line *lines = text->lines;
    size_t end = pool_end(text, label);
    size_t above = top_of_blanks(text, label);
    size_t again = 0;
    if (!pool_label_line(&lines[label])) {
        return fail(text, SHCKIT_POOL_LABEL, lines[label].input);
    }
    for (size_t i = label + 1; i < end; i++) {
        int kind = pool_item_kind(&lines[i]);
        if (kind < 0 ||
            (kind != POOL_PAD && !is_literal(text, names, named, &lines[i]))) {
            return fail(text, SHCKIT_ITEM, lines[i].input);
        }
    }
    again = other_definition(names, named, lines, label);
    if (again != 0) {
        return fail(text, SHCKIT_DUPLICATE, again);
    }
    while (above > 0 && lines[above - 1].kind == SHCKIT_CONTINUATION) {
        above--;
    }
    if (above == 0 || !stops(text, places, above - 1)) {
        return fail(text, SHCKIT_REACHED, lines[label].input);
    }
    if (jumps_over(text, places[above - 1].branch, end)) {
        return fail(text, SHCKIT_GUARDED, lines[label].input);
    }
    return SHCKIT_OK;
}

static bool pool_load(const struct shckit_text *text, size_t index,
                      const struct place *places, size_t pool, size_t *offset)
{
    // whether the instruction at index, whose first operand starts with the
    // label of the pool at line pool, loads from it or takes the address of
    // one of its items: MOV.W or MOV.L of label or label+n to a register, or
    // MOVA of it to R0, n in decimal without a leading zero, above the pool
    // in the same .SECTION block. *offset is n, saturated at SIZE_MAX.
    const struct shckit_line *line = &text->lines[index];
    struct shckit_span operand;
    const unsigned char *bytes = NULL;
    size_t len = text->lines[pool].label.len;
    size_t at = 0;
    *offset = 0;
    if ((!is_instruction(line, "MOV", "WL") &&
         !is_instruction(line, "MOVA", "-L")) ||
        shckit_flow(text, index) < 0 || pool < index ||
        places[pool].section_line != places[index].section_line ||
        !shckit_operand(line, &at, &operand)) {
        return false;
    }
    bytes = line->bytes + operand.at;
    if (operand.len == len) {
        return true;
    }
    if (operand.len < len + 2 || bytes[len] != '+' || bytes[len + 1] == '0' ||
        !all(bytes, span(len + 1, operand.len), digit)) {
        return false;
    }
    for (size_t i = len + 1; i < operand.len; i++) {
        size_t value = bytes[i] - (unsigned)'0';
        *offset = *offset <= (SIZE_MAX - value) / 10 ? (*offset * 10) + value
                                                     : SIZE_MAX;
    }
    return true;
}

static size_t pool_item_at(const struct shckit_text *text, size_t pool,
                           size_t offset, int kind)
{
    // the index of the item of kind that starts offset bytes past the label
    // at line pool, pads included, or SIZE_MAX
    size_t at = 0;
    size_t end = pool_end(text, pool);
    for (size_t i = pool + 1; i < end && at <= offset; i++) {
        int each = pool_item_kind(&text->lines[i]);
        if (at == offset) {
            return each == kind ? i : SIZE_MAX;
        }
        at += each == POOL_LONG ? 4 : 2;
    }
    return SIZE_MAX;
}

// What use-assembler-pools learns of each line, in an array indexed by line.
// On a load, item is the index of the pool item it reads, and on a MOVA, of
// the item whose address it takes; it is 0 on every other line, which can't
// be an item's index, since an item is below its pool's label. On an item,
// read marks that a load reads it or a MOVA takes its address, and on a
// pool's label, that one of the pool's items is so.
struct use {
    size_t item;
    bool read;
};

static int record_pool_uses(const struct shckit_text *text, size_t *refused,
                            size_t index, const struct place *places,
                            const struct name *names, size_t named,
                            struct use *uses)
{
    // check every pool label the line at index names: each must start the
    // first operand of a load of one of its pool's whole items, or of a MOVA
    // of a whole long. Record the item, and mark it and its pool's label
    // read.
    const struct shckit_line *line = &text->lines[index];
    struct shckit_span operand;
    size_t at = 0;
    while (shckit_operand(line, &at, &operand)) {
        struct token token;
        size_t from = operand.at;
        size_t end = operand.at + operand.len;
        while (next_token(line->bytes, &from, end, &token)) {
            size_t pool =
                token.word ? first_definition(names, named, text->lines,
                                              line->bytes + token.at, token.len)
                           : SIZE_MAX;
            size_t offset = 0;
            size_t item = 0;
            if (pool == SIZE_MAX || text->lines[pool].pool == 0 ||
                text->lines[pool].kind != SHCKIT_LABEL) {
                continue;
            }
            if (token.at != line->operands.at ||
                line->kind != SHCKIT_INSTRUCTION ||
                !pool_load(text, index, places, pool, &offset)) {
                return fail_at(refused, SHCKIT_USE, line->input);
            }
            item = pool_item_at(text, pool, offset,
                                is_instruction(line, "MOV", "W") ? POOL_WORD
                                                                 : POOL_LONG);
            if (item == SIZE_MAX) {
                return fail_at(refused, SHCKIT_NO_ITEM, line->input);
            }
            uses[index].item = item;
            uses[item].read = true;
            uses[pool].read = true;
        }
    }
    return SHCKIT_OK;
}

// The MOVA check of use-assembler-pools. follow_addresses refuses a MOVA of
// a pool item unless each path from it reads its address only as the item's
// four bytes, at @R0, until R0 is written. Control is followed from each
// SHCASM function's entry, where SHC's calling interface sets SZ to 0,
// through branches to the text's own labels. A call, a return or a jump
// leaves the address unread, since no C function passes or returns one.
// Where control can't be followed, the path loses R0 and SZ, so whatever R0
// holds, follow_addresses refuses the line:
// - data, a .SECTION or .END;
// - an entry or a branch that runs a slot other than as its branch's;
// - a branch to a name an .EQU or .ASSIGN defines, which is a value, or to
//   an expression that names a label of the text;
// - a BSR to any label of the text but a function's entry.

// What follow_addresses knows before a line, from each path that reaches it
// so far: FPSCR.SZ, or -1 for unknown, and whether R0 may hold the address
// a MOVA of a pool item took. lost marks a line past which follow_addresses
// can't follow control.
struct address_state {
    int sz;
    bool address;
    bool reached;
    bool queued;
    bool lost;
};

// The lines follow_addresses has reached, and those it has yet to follow.
// slots[i] is the slot of the delayed branch at line i.
struct follow {
    const struct shckit_text *text;
    const struct place *places;
    const struct use *uses;
    const struct name *names;
    size_t named;
    size_t *slots;
    struct address_state *states;
    size_t *queue;
    size_t queued;
};

// How an instruction uses R0: not at all; to read the four bytes at @R0
// into another register, or into R0 itself; to write R0 without reading
// it; or any other way, such as an address it moves or offsets, a copy or
// a store.
enum { R0_UNUSED, R0_ITEM, R0_ITEM_INTO_R0, R0_WRITTEN, R0_OTHER };

static bool function_header(const struct shckit_line *line)
{
    // SHCASM's label line for a function, with "; function: " and the name
    // as its comment
    static const char header[] = "; function: ";
    size_t len = sizeof header - 1;
    return line->kind == SHCKIT_LABEL && line->code &&
           line->comment.len > len &&
           memcmp(line->bytes + line->comment.at, header, len) == 0;
}

static bool passes(const struct shckit_line *line)
{
    // a line control passes on to the next: any but an instruction, data, a
    // .SECTION and .END
    return line->kind != SHCKIT_INSTRUCTION && !writes_data(line) &&
           line->directive != SHCKIT_SECTION && line->directive != SHCKIT_END;
}

static int r0_use(const struct shckit_line *line, bool *fmov)
{
    // *fmov says a read of the item is an FMOV, whose width SZ sets
    struct shckit_span operands[MOST_OPERANDS];
    int kinds[MOST_OPERANDS];
    size_t count = 0;
    size_t at = 0;
    size_t r0 = 0;
    *fmov = false;
    while (count < MOST_OPERANDS &&
           shckit_operand(line, &at, &operands[count])) {
        struct token token;
        size_t from = operands[count].at;
        kinds[count] = operand_kind(line->bytes, operands[count]);
        while (next_token(line->bytes, &from,
                          operands[count].at + operands[count].len, &token)) {
            r0 += register_kind(line->bytes, token) == KIND_R0 ? 1 : 0;
        }
        count++;
    }
    if (r0 == 0) {
        return R0_UNUSED;
    }
    if (count == 2 && kinds[0] == KIND_AT &&
        register_of(line, operands[0]) == 0) {
        if (one_of(line, "FMOV") &&
            (kinds[1] == KIND_FR0 || kinds[1] == KIND_FRN)) {
            *fmov = true;
            return R0_ITEM;
        }
        if (is_instruction(line, "MOV", "L") && general(kinds[1])) {
            return kinds[1] == KIND_R0 ? R0_ITEM_INTO_R0 : R0_ITEM;
        }
    }
    if (r0 == 1 && kinds[count - 1] == KIND_R0 &&
        one_of(line, "MOV MOVA MOVT STS STC EXTS EXTU NEG NEGC NOT SWAP")) {
        return R0_WRITTEN;
    }
    return R0_OTHER;
}

static bool sets(const struct shckit_text *text, size_t index,
                 const char *names, int kind)
{
    // an instruction of names whose last operand is of kind, as LDC to SR
    int kinds[MOST_OPERANDS];
    size_t count = 0;
    return one_of(&text->lines[index], names) &&
           operand_kinds(text, index, kinds, &count) && count != 0 &&
           kinds[count - 1] == kind;
}

static void flow_into(struct follow *follow, size_t to, int sz, bool address)
{
    // join a path's state into line to's, and queue the line if that
    // changes it
    struct address_state *state = &follow->states[to];
    bool changed = !state->reached;
    if (!state->reached) {
        state->sz = sz;
        state->address = address;
        state->reached = true;
    }
    if (state->sz != sz && state->sz != -1) {
        state->sz = -1;
        changed = true;
    }
    if (address && !state->address) {
        state->address = true;
        changed = true;
    }
    if (changed && !state->queued) {
        state->queued = true;
        follow->queue[follow->queued] = to;
        follow->queued++;
    }
}

static bool enters_slot(const struct follow *follow, size_t index)
{
    // whether the first instruction control reaches from line index, past
    // the lines it passes on, is a delayed branch's slot
    const struct shckit_text *text = follow->text;
    while (index < text->count && passes(&text->lines[index])) {
        index++;
    }
    return index < text->count && follow->places[index].branch != SIZE_MAX;
}

static void flow_to_target(struct follow *follow, size_t branch, int sz,
                           bool address)
{
    // flow into each line that defines the label a direct branch names, or
    // mark the branch lost, in the cases the part's opening comment lists. A
    // label the text doesn't define is elsewhere, and control leaves for it.
    // A BSR's callee is a function's entry, which follow_addresses seeds
    // itself.
    const struct shckit_text *text = follow->text;
    const struct shckit_line *line = &text->lines[branch];
    bool call = one_of(line, "BSR");
    struct shckit_span label;
    struct token token;
    size_t at = 0;
    size_t from = 0;
    if (!shckit_operand(line, &at, &label)) {
        return;
    }
    from = label.at;
    while (next_token(line->bytes, &from, label.at + label.len, &token)) {
        if (token.word && token.len != label.len &&
            first_definition(follow->names, follow->named, text->lines,
                             line->bytes + token.at, token.len) != SIZE_MAX) {
            follow->states[branch].lost = true;
        }
    }
    for (size_t i = name_lower_bound(follow->names, follow->named,
                                     line->bytes + label.at, label.len);
         i < follow->named &&
         name_matches(&follow->names[i], line->bytes + label.at, label.len);
         i++) {
        size_t to = follow->names[i].index;
        if (equates(&text->lines[to]) || enters_slot(follow, to) ||
            (call && !function_header(&text->lines[to]))) {
            follow->states[branch].lost = true;
        } else if (!call) {
            flow_into(follow, to, sz, address);
        }
    }
}

static void follow_line(struct follow *follow, size_t index)
{
    // pass line index's state on to the lines control reaches from it. A
    // slot passes it to its branch's targets; after a call, R0 holds what
    // the callee returns.
    const struct shckit_text *text = follow->text;
    const struct shckit_line *line = &text->lines[index];
    struct address_state out = follow->states[index];
    size_t branch = follow->places[index].branch;
    const struct shckit_line *leaving = NULL;
    bool fmov = false;
    int use = R0_UNUSED;
    if (line->kind != SHCKIT_INSTRUCTION) {
        if (!passes(line)) {
            follow->states[index].lost = true;
        } else if (index + 1 < text->count) {
            flow_into(follow, index + 1, out.sz, out.address);
        }
        return;
    }
    use = r0_use(line, &fmov);
    if (follow->uses[index].item != 0 && one_of(line, "MOVA")) {
        out.address = true;
    } else if (use == R0_WRITTEN || use == R0_ITEM_INTO_R0) {
        out.address = false;
    }
    if (one_of(line, "FSCHG") && out.sz != -1) {
        out.sz = 1 - out.sz;
    } else if (sets(text, index, "LDS LDC", KIND_FPSCR)) {
        out.sz = -1;
    }
    leaving = branch != SIZE_MAX ? &text->lines[branch] : line;
    if (branch == SIZE_MAX && (shckit_flow(text, index) & SHCKIT_DELAYED)) {
        flow_into(follow, follow->slots[index], out.sz, out.address);
        return;
    }
    if (one_of(leaving, "BSR BSRF JSR")) {
        out.address = false;
    }
    if (one_of(leaving, "BT BF BT/S BF/S BRA BSR")) {
        flow_to_target(follow, branch != SIZE_MAX ? branch : index, out.sz,
                       out.address);
    }
    if (!one_of(leaving, "BRA BRAF JMP RTS RTE") && index + 1 < text->count) {
        flow_into(follow, index + 1, out.sz, out.address);
    }
}

static bool address_misused(const struct follow *follow, size_t index)
{
    // whether line index, reached with R0 maybe holding a pool item's
    // address, may use it other than to read the item's four bytes: an
    // FMOV where SZ may be 1 moves eight, a continuation may name R0, a
    // write to SR may switch R0's bank, and an exception or its return may
    // read R0
    const struct shckit_line *line = &follow->text->lines[index];
    bool fmov = false;
    if (line->kind != SHCKIT_INSTRUCTION) {
        return false;
    }
    return r0_use(line, &fmov) == R0_OTHER ||
           (fmov && follow->states[index].sz != 0) ||
           continued(follow->text, index) || one_of(line, "RTE TRAPA") ||
           sets(follow->text, index, "LDC", KIND_SR);
}

static int follow_addresses(const struct shckit_text *text, size_t *refused,
                            const struct place *places,
                            const struct name *names, size_t named,
                            const struct use *uses)
{
    // the MOVA check, as the part's opening comment says. places must be
    // from SLOTS_MUST_PLACE, which refuses an instruction outside the opcode
    // table and a branch whose slot it can't place.
    struct follow follow = {text, places, uses, names, named,
                            NULL, NULL,   NULL, 0};
    bool any = false;
    int error = SHCKIT_OK;
    for (size_t i = 0; i < text->count && !any; i++) {
        any = uses[i].item != 0 && one_of(&text->lines[i], "MOVA");
    }
    if (!any) {
        return SHCKIT_OK;
    }
    follow.slots = malloc(text->count * sizeof *follow.slots);
    follow.states = calloc(text->count, sizeof *follow.states);
    follow.queue = malloc(text->count * sizeof *follow.queue);
    if (follow.slots == NULL || follow.states == NULL || follow.queue == NULL) {
        error = SHCKIT_NOMEM;
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        if (places[i].branch != SIZE_MAX) {
            follow.slots[places[i].branch] = i;
        }
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        if (function_header(&text->lines[i])) {
            follow.states[i].lost = enters_slot(&follow, i);
            flow_into(&follow, i, 0, false);
        }
    }
    while (error == SHCKIT_OK && follow.queued != 0) {
        size_t at = follow.queue[follow.queued - 1];
        follow.queued--;
        follow.states[at].queued = false;
        follow_line(&follow, at);
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct address_state *state = &follow.states[i];
        bool mova = uses[i].item != 0 && one_of(&text->lines[i], "MOVA");
        if (state->lost || (mova && !state->reached) ||
            (state->reached && state->address && address_misused(&follow, i))) {
            error = fail_at(refused, SHCKIT_ADDRESS, text->lines[i].input);
        }
    }
    free(follow.queue);
    free(follow.states);
    free(follow.slots);
    return error;
}

static int write_load(struct shckit_text *text, struct list *out, size_t index,
                      size_t item)
{
    // push the load or MOVA at index with # and the item's value as its
    // first operand, and every other byte kept. The comment keeps its column
    // while the operands end before it, and otherwise follows them directly,
    // as SHCASM writes a comment its operands reach.
    const struct shckit_line *line = &text->lines[index];
    const struct shckit_line *source = &text->lines[item];
    size_t field_end = line->operands.at + line->operands.len;
    struct shckit_span operand;
    struct shckit_span value;
    struct shckit_span tail;
    struct shckit_line written;
    size_t at = 0;
    size_t rest = 0;
    size_t end = 0;
    size_t gap = 0;
    unsigned char *bytes = NULL;
    (void)shckit_operand(line, &at, &operand);
    at = 0;
    (void)shckit_operand(source, &at, &value);
    rest = field_end - (operand.at + operand.len);
    end = operand.at + 1 + value.len + rest;
    tail = line->comment.len != 0 ? line->comment : span(field_end, line->len);
    if (line->comment.len != 0 && end < line->comment.at) {
        gap = line->comment.at - end;
    }
    bytes = own(text, end + gap + tail.len);
    if (bytes == NULL) {
        return SHCKIT_NOMEM;
    }
    memcpy(bytes, line->bytes, operand.at);
    bytes[operand.at] = '#';
    memcpy(bytes + operand.at + 1, source->bytes + value.at, value.len);
    memcpy(bytes + operand.at + 1 + value.len,
           line->bytes + operand.at + operand.len, rest);
    memset(bytes + end, ' ', gap);
    memcpy(bytes + end + gap, line->bytes + tail.at, tail.len);
    memset(&written, 0, sizeof written);
    written.bytes = bytes;
    written.len = end + gap + tail.len;
    written.crlf = line->crlf;
    written.code = line->code;
    type_line(&written);
    written.input = line->input;
    written.patch = SHCKIT_USE_ASSEMBLER_POOLS;
    return push(out, &written) ? SHCKIT_OK : SHCKIT_NOMEM;
}

static int write_pools(struct shckit_text *text, struct list *out,
                       const struct place *places, const struct use *uses)
{
    // write each load and MOVA of an item with # and the item's value,
    // .ALIGN 4 and .POOL in place of each pool, and .NOPOOL right after each
    // slot of a branch that never falls through, past its continuations,
    // while a rewritten line's literal waits. A literal stops waiting at the
    // first .POOL below its line, even when that .POOL stands for another
    // pool. Each rewritten line is above its own pool in the same .SECTION
    // block, so no literal waits past a block. asmsh refuses that .NOPOOL,
    // with error 521, when a label stands below the branch, on the slot or
    // above it, so the patch refuses such a label.
    const struct shckit_line *lines = text->lines;
    size_t slot = SIZE_MAX;
    bool waits = false;
    int error = SHCKIT_OK;
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        if (lines[i].pool != 0 && lines[i].kind == SHCKIT_LABEL) {
            error = write_line(out, &lines[i], SHCKIT_ALIGN,
                               SHCKIT_USE_ASSEMBLER_POOLS);
            if (error == SHCKIT_OK) {
                error = write_line(out, &lines[i], SHCKIT_POOL,
                                   SHCKIT_USE_ASSEMBLER_POOLS);
            }
            waits = false;
        }
        if (lines[i].pool != 0) {
            continue;
        }
        if (uses[i].item != 0) {
            error = write_load(text, out, i, uses[i].item);
            waits = true;
        } else if (!push(out, &lines[i])) {
            error = SHCKIT_NOMEM;
        }
        if (error == SHCKIT_OK && waits && stops(text, places, i)) {
            size_t label = slot_label(text, places, i);
            if (label != SIZE_MAX) {
                error = fail(text, SHCKIT_SLOT_LABEL, lines[label].input);
            }
            slot = i;
        }
        if (error == SHCKIT_OK && slot != SIZE_MAX && !continued(text, i)) {
            error = write_line(out, &lines[slot], SHCKIT_NOPOOL,
                               SHCKIT_USE_ASSEMBLER_POOLS);
            slot = SIZE_MAX;
        }
    }
    return error;
}

static bool names_local_label(const struct shckit_line *line)
{
    // whether the line defines a ?label or names one in its operands
    struct token token;
    size_t at = line->operands.at;
    if (line->label.len != 0 && line->bytes[line->label.at] == '?') {
        return true;
    }
    while (next_token(line->bytes, &at, line->operands.at + line->operands.len,
                      &token)) {
        if (token.word && line->bytes[token.at] == '?') {
            return true;
        }
    }
    return false;
}

static int check_pool_block(struct shckit_text *text, size_t label,
                            const struct name *names, size_t named)
{
    // refuse a ?label defined or named in the block a pool's label opens.
    // The patch drops the label, so the block above takes in the rest of
    // it, where the ?label could take another definition.
    for (size_t i = label + 1; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        bool first = line->label.len != 0 &&
                     first_definition(names, named, text->lines,
                                      line->bytes + line->label.at,
                                      line->label.len) == i;
        if (starts_block(line, first) || line->directive == SHCKIT_END) {
            break;
        }
        if (names_local_label(line)) {
            return fail(text, SHCKIT_LOCAL_LABEL, line->input);
        }
    }
    return SHCKIT_OK;
}

static bool any_pool(const struct shckit_text *text)
{
    for (size_t i = 0; i < text->count; i++) {
        if (text->lines[i].pool != 0) {
            return true;
        }
    }
    return false;
}

static int use_assembler_pools(struct shckit_text *text)
{
    struct list out = {NULL, 0, 0};
    struct place *places = NULL;
    struct use *uses = NULL;
    struct name *names = NULL;
    size_t named = 0;
    size_t head = 0; // index of the label of the pool line i is in
    int error = SHCKIT_OK;
    if (!any_pool(text)) {
        return SHCKIT_OK;
    }
    places = malloc(text->count * sizeof *places);
    uses = calloc(text->count, sizeof *uses);
    error = places != NULL && uses != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    if (error == SHCKIT_OK) {
        error = find_pooled(text);
    }
    if (error == SHCKIT_OK) {
        error = place_slots(text, places, SLOTS_MUST_PLACE, &text->refused);
    }
    if (error == SHCKIT_OK) {
        error = find_switch(text);
    }
    if (error == SHCKIT_OK) {
        error = sorted_labels(text, &names, &named);
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        if (text->lines[i].pool != 0 && text->lines[i].kind == SHCKIT_LABEL) {
            error = check_handed_pool(text, i, places, names, named);
            if (error == SHCKIT_OK) {
                error = check_pool_block(text, i, names, named);
            }
        }
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        error = record_pool_uses(text, &text->refused, i, places, names, named,
                                 uses);
    }
    if (error == SHCKIT_OK) {
        error =
            follow_addresses(text, &text->refused, places, names, named, uses);
    }
    if (error == SHCKIT_OK) {
        error = find_offsets(text, &text->refused, names, named);
    }
    // every value must be read. A pad only aligns the items after it, so it
    // needs only some item of its pool read.
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        head = line->pool != 0 && line->kind == SHCKIT_LABEL ? i : head;
        if (line->pool != 0 && line->kind == SHCKIT_DIRECTIVE &&
            !uses[i].read &&
            (pool_item_kind(line) != POOL_PAD || !uses[head].read)) {
            error = fail(text, SHCKIT_UNUSED, line->input);
        }
    }
    if (error == SHCKIT_OK) {
        error = write_pools(text, &out, places, uses);
    }
    free(names);
    free(uses);
    free(places);
    return adopt(text, &out, error);
}

// drop-r0-move-nop: drop SHCASM's NOP right below SHCASM's MOV Rm,R0 in a
// code section, where the copy is in no delay slot, and refuse a copy that
// may be one. SHC writes that NOP under -extra=a=400, and the pair's exact
// form is the only sign of it in the text. The two must have been adjacent
// in the original text, input numbers n and n + 1, so an earlier patch
// can't make a pair by dropping a line between them. No address is
// computed. Dropping an instruction can change the addresses of the
// instructions and pools after it, so the patch refuses an .ALIGN, .POOL
// or .NOPOOL below a dropped NOP where a slot is or may be due, and an
// operand that offsets a code label, $ or PC.

static bool r0_copy(const struct shckit_line *line)
{
    // exactly SHCASM's MOV Rm,R0, m from 1 to 15 without a leading zero
    const unsigned char *bytes = line->bytes + line->operands.at;
    size_t len = line->operands.len;
    unsigned number = 0;
    if (line->kind != SHCKIT_INSTRUCTION || !shcasm_columns(line) ||
        !is(line->bytes, line->name, "MOV") || len < 5 || len > 6 ||
        bytes[0] != 'R' || bytes[1] == '0' ||
        memcmp(bytes + len - 3, ",R0", 3) != 0) {
        return false;
    }
    for (size_t i = 1; i < len - 3; i++) {
        if (!digit(bytes[i])) {
            return false;
        }
        number = (number * 10) + (bytes[i] - (unsigned)'0');
    }
    return number <= 15;
}

static int in_doubt(struct shckit_text *text, size_t doubt)
{
    // refuse at a doubt line from place_slots: SHCKIT_NOT_HELD for an
    // instruction outside the opcode table, SHCKIT_SLOT for a branch whose
    // slot may be filled
    const struct shckit_line *line = &text->lines[doubt];
    return fail(text,
                line->kind == SHCKIT_INSTRUCTION && shckit_flow(text, doubt) < 0
                    ? SHCKIT_NOT_HELD
                    : SHCKIT_SLOT,
                line->input);
}

static int drop_nops(struct shckit_text *text, const struct place *places,
                     const bool *gone, bool any, int error)
{
    // drop-r0-move-nop's and fill-call-slot's last steps: drop the NOPs gone
    // marks, refusing an .ALIGN, .POOL or .NOPOOL below one in its section
    // where a slot is or may be due, and an operand that offsets a place in
    // code
    const struct shckit_line *lines = text->lines;
    size_t count = text->count;
    struct list kept = {NULL, 0, 0};
    bool *moved = calloc(count != 0 ? count : 1, sizeof *moved);
    struct name *names = NULL;
    size_t named = 0;
    if (error == SHCKIT_OK && moved == NULL) {
        error = SHCKIT_NOMEM;
    }
    // moved[s] is set once a dropped NOP is above line i in logical section
    // s, so code there has moved. places[i].section is at most the number of
    // .SECTION lines at or above line i, and the dropped NOP is a line that
    // isn't one, so a flag per line is enough.
    for (size_t i = 0; error == SHCKIT_OK && any && i < count; i++) {
        size_t doubt =
            places[i].branch != SIZE_MAX ? places[i].branch : places[i].doubt;
        moved[places[i].section] = moved[places[i].section] || gone[i];
        if (may_fill(&lines[i]) && moved[places[i].section] &&
            doubt != SIZE_MAX) {
            error = in_doubt(text, doubt);
        }
    }
    if (error == SHCKIT_OK && any) {
        error = sorted_labels(text, &names, &named);
    }
    if (error == SHCKIT_OK && any) {
        error = find_offsets(text, &text->refused, names, named);
    }
    for (size_t i = 0; error == SHCKIT_OK && i < count; i++) {
        if (!gone[i] && !push(&kept, &lines[i])) {
            error = SHCKIT_NOMEM;
        }
    }
    free(names);
    free(moved);
    return adopt(text, &kept, error);
}

static int drop_r0_move_nop(struct shckit_text *text)
{
    const struct shckit_line *lines = text->lines;
    size_t count = text->count;
    struct place *places = malloc((count != 0 ? count : 1) * sizeof *places);
    bool *gone = calloc(count != 0 ? count : 1, sizeof *gone);
    bool any = false;
    int error = places != NULL && gone != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    if (error == SHCKIT_OK) {
        error = place_slots(text, places, SLOTS_MAY_DOUBT, &text->refused);
    }
    for (size_t i = 0; error == SHCKIT_OK && i + 1 < count; i++) {
        if (!r0_copy(&lines[i]) || !lines[i].code || !lines[i + 1].code ||
            !is_line(&lines[i + 1], nop_line, sizeof nop_line - 1) ||
            lines[i].input == 0 || lines[i + 1].input != lines[i].input + 1) {
            continue;
        }
        if (places[i].doubt != SIZE_MAX) {
            error = in_doubt(text, places[i].doubt);
        }
        gone[i + 1] = places[i].branch == SIZE_MAX;
        any = any || gone[i + 1];
    }
    error = drop_nops(text, places, gone, any, error);
    free(gone);
    free(places);
    return error;
}

// fill-call-slot: drop SHCASM's NOP in the delay slot of SHCASM's JSR @Rm in
// a code section, where SHCASM's ADD #imm,Rn comes right below the NOP, n
// from 8 to 14 and other than m, so the add becomes the slot. The call must
// be in no delay slot, and the patch refuses one that may be. The three must
// have been adjacent in the original text, input numbers n to n + 2, so an
// earlier patch can't make a triple by dropping a line between them. The add
// then runs before the callee rather than after it returns: a callee keeps
// R8 to R14 and takes no argument in them, ADD #imm leaves T as it is, and
// the add doesn't write Rm, which holds the call's target. JSR and ADD #imm
// are the subset the one member it is for holds, not every call or every
// instruction that could fill the slot. No address is computed. Dropping
// the NOP can change the addresses of the instructions and pools after it,
// so the patch refuses what drop-r0-move-nop refuses for a dropped NOP: an
// .ALIGN, .POOL or .NOPOOL below it where a slot is or may be due, and an
// operand that offsets a code label, $ or PC.

static bool register_number(const unsigned char *bytes, size_t len,
                            unsigned *number)
{
    // R0 to R15 as SHCASM writes them: R, then the number without a leading
    // zero
    *number = 0;
    if (len < 2 || len > 3 || bytes[0] != 'R' ||
        (len == 3 && bytes[1] == '0')) {
        return false;
    }
    for (size_t i = 1; i < len; i++) {
        if (!digit(bytes[i])) {
            return false;
        }
        *number = (*number * 10) + (bytes[i] - (unsigned)'0');
    }
    return *number <= 15;
}

static bool register_call(const struct shckit_line *line, unsigned *target)
{
    // exactly SHCASM's JSR @Rm, m from 0 to 15
    const unsigned char *bytes = line->bytes + line->operands.at;
    size_t len = line->operands.len;
    return line->kind == SHCKIT_INSTRUCTION && shcasm_columns(line) &&
           is(line->bytes, line->name, "JSR") && len > 1 && bytes[0] == '@' &&
           register_number(bytes + 1, len - 1, target);
}

static bool kept_add(const struct shckit_line *line, unsigned *added)
{
    // exactly SHCASM's ADD #imm,Rn: imm in decimal from -128 to 127 without
    // a leading zero, and not -0, and n from 8 to 14, a register a callee
    // keeps
    const unsigned char *bytes = line->bytes + line->operands.at;
    size_t len = line->operands.len;
    size_t first = len > 1 && bytes[1] == '-' ? 2 : 1; // the first digit
    size_t end = first;
    unsigned value = 0;
    if (line->kind != SHCKIT_INSTRUCTION || !shcasm_columns(line) ||
        !is(line->bytes, line->name, "ADD") || len < 4 || bytes[0] != '#') {
        return false;
    }
    while (end < len && end - first < 4 && digit(bytes[end])) {
        value = (value * 10) + (bytes[end] - (unsigned)'0');
        end++;
    }
    return end > first && end < len && bytes[end] == ',' &&
           (bytes[first] != '0' || (end - first == 1 && first == 1)) &&
           value <= (first == 2 ? 128U : 127U) &&
           register_number(bytes + end + 1, len - end - 1, added) &&
           *added >= 8 && *added <= 14;
}

static int fill_call_slot(struct shckit_text *text)
{
    const struct shckit_line *lines = text->lines;
    size_t count = text->count;
    struct place *places = malloc((count != 0 ? count : 1) * sizeof *places);
    bool *gone = calloc(count != 0 ? count : 1, sizeof *gone);
    bool any = false;
    int error = places != NULL && gone != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    if (error == SHCKIT_OK) {
        error = place_slots(text, places, SLOTS_MAY_DOUBT, &text->refused);
    }
    for (size_t i = 0; error == SHCKIT_OK && i + 2 < count; i++) {
        unsigned target = 0;
        unsigned added = 0;
        if (!register_call(&lines[i], &target) || !lines[i].code ||
            !lines[i + 1].code || !lines[i + 2].code ||
            !is_line(&lines[i + 1], nop_line, sizeof nop_line - 1) ||
            !kept_add(&lines[i + 2], &added) || added == target ||
            continued(text, i + 2) || lines[i].input == 0 ||
            lines[i + 1].input != lines[i].input + 1 ||
            lines[i + 2].input != lines[i].input + 2) {
            continue;
        }
        if (places[i].doubt != SIZE_MAX) {
            error = in_doubt(text, places[i].doubt);
        }
        gone[i + 1] = places[i + 1].branch == i;
        any = any || gone[i + 1];
    }
    error = drop_nops(text, places, gone, any, error);
    free(gone);
    free(places);
    return error;
}

// The patches, in the order shckit_apply runs them.
static const struct {
    const char *name;
    int (*apply)(struct shckit_text *text);
} patch_table[SHCKIT_PATCHES] = {
    [SHCKIT_DROP_INLINE_ALIGN] = {"drop-inline-align", drop_inline_align},
    [SHCKIT_ALIGN_POOL] = {"align-pool", align_pool},
    [SHCKIT_USE_ASSEMBLER_POOLS] = {"use-assembler-pools", use_assembler_pools},
    [SHCKIT_DROP_R0_MOVE_NOP] = {"drop-r0-move-nop", drop_r0_move_nop},
    [SHCKIT_FILL_CALL_SLOT] = {"fill-call-slot", fill_call_slot},
};

static int preflight(const struct shckit_text *text, size_t *refused)
{
    // the preflight of a command that rewrites the text. Up to the first
    // .END, refuse every line shckit can't read: an unknown line could start
    // a section, align code or hold a pool, and a continuation that joins
    // nothing could join a statement once a patch drops the line above it.
    // Past .END, a rewrite would change lines asmsh never reads, so only
    // blanks and comments may stand there.
    size_t head = SIZE_MAX;
    bool ended = false;
    for (size_t i = 0; i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        if (ended && line->kind != SHCKIT_BLANK &&
            line->kind != SHCKIT_COMMENT) {
            return fail_at(refused, SHCKIT_PAST_END, line->input);
        }
        if (!ended && unreadable(text, i, &head)) {
            return fail_at(refused, SHCKIT_UNREADABLE, line->input);
        }
        ended = ended || line->directive == SHCKIT_END;
    }
    return SHCKIT_OK;
}

int shckit_apply(struct shckit_text *text, unsigned patches, size_t *line,
                 int *patch)
{
    // a failed apply leaves the text as it was and free for another. Each
    // patch commits its lines as the text's, so apply keeps a copy of the
    // lines from before the first, and on a refusal puts it back and frees
    // the bytes the patches owned.
    unsigned pools = 1U << SHCKIT_ALIGN_POOL | 1U << SHCKIT_USE_ASSEMBLER_POOLS;
    struct shckit_line *before = NULL;
    size_t count = text->count;
    size_t owned = text->owned_count;
    int error = SHCKIT_OK;
    int by = -1;

    text->refused = 0;
    if ((patches & ~((1U << SHCKIT_PATCHES) - 1)) != 0) {
        error = SHCKIT_NO_SUCH_PATCH;
    } else if ((patches & pools) == pools) {
        error = SHCKIT_CONFLICT;
    } else if (patches != 0 && text->patched) {
        error = SHCKIT_TWICE;
    } else if (patches != 0) {
        error = preflight(text, &text->refused);
    }
    if (error == SHCKIT_OK && patches != 0) {
        before = malloc(count * sizeof *before);
        error = before != NULL ? SHCKIT_OK : SHCKIT_NOMEM;
    }
    if (before != NULL) {
        memcpy(before, text->lines, count * sizeof *before);
    }
    for (int i = 0; error == SHCKIT_OK && i < SHCKIT_PATCHES; i++) {
        if ((patches & 1U << i) != 0) {
            error = patch_table[i].apply(text);
            by = error != SHCKIT_OK ? i : by;
        }
    }
    if (error != SHCKIT_OK && before != NULL) {
        free(text->lines);
        text->lines = before;
        text->count = count;
        disown(text, owned);
        before = NULL;
    }
    free(before);
    text->patched = text->patched || (patches != 0 && error == SHCKIT_OK);
    if (line != NULL) {
        *line = text->refused;
    }
    if (patch != NULL) {
        *patch = by;
    }
    return error;
}

const char *shckit_patch_name(int patch)
{
    return patch >= 0 && patch < SHCKIT_PATCHES ? patch_table[patch].name
                                                : NULL;
}

int shckit_patch(const char *name)
{
    for (int i = 0; i < SHCKIT_PATCHES; i++) {
        if (strcmp(patch_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// translate (docs/translate.md): write the text as GNU as text that
// binutils' sh-elf-as assembles to asmsh's bytes. Each line comes out as a
// line, or as a few, in its order: an instruction keeps its spelling but for
// the size GNU as takes, each number and ?label is respelled, and each
// directive becomes GNU as's. No address is computed: GNU as lays the text out
// as asmsh does, and checks every reach. translate writes asmsh's literal
// pools where asmsh writes them, which the statements alone decide, and
// refuses what GNU as would read otherwise and what it doesn't write.

// Where translate writes: each byte to at, unless at is NULL, where it only
// counts them. size is the count, which stops at SIZE_MAX.
struct sink {
    unsigned char *at;
    size_t size;
};

static void put(struct sink *sink, const void *bytes, size_t len)
{
    if (sink->at != NULL) {
        memcpy(sink->at + sink->size, bytes, len);
    }
    sink->size = len <= SIZE_MAX - sink->size ? sink->size + len : SIZE_MAX;
}

static void put_text(struct sink *sink, const char *text)
{
    put(sink, text, strlen(text));
}

static void put_number(struct sink *sink, size_t number)
{
    // number in decimal
    unsigned char digits[24];
    size_t at = sizeof digits;
    do {
        digits[--at] = (unsigned char)('0' + (number % 10));
        number /= 10;
    } while (number != 0);
    put(sink, digits + at, sizeof digits - at);
}

static void move_to(struct sink *sink, size_t start, size_t column)
{
    // blanks to column of the line that starts at start in sink, or one
    // blank where the line already reaches column
    size_t at = sink->size - start;
    size_t blanks = at < column ? column - at : at != 0;
    while (blanks-- > 0) {
        put(sink, " ", 1);
    }
}

// The block of ?labels a line is in, as asmsh reads it: the line of the
// label that opens it, or SIZE_MAX where a .SECTION line or the text's start
// does, and how many .SECTION lines are at or above it.
struct block {
    size_t opener;
    size_t sections;
};

// A literal that waits for its pool: the load that first names it, its
// value there, that load's block, the number of its label, whether it is a
// word, and whether its value is a plain number, and that number's 32 bits.
struct literal {
    size_t load;
    struct shckit_span value;
    struct block block;
    size_t number;
    bool word;
    bool plain;
    uint32_t bits;
};

// A logical section as translate writes it: whether a line of it writes a
// byte, and whether translate has written its first line.
struct gnu_section {
    bool writes;
    bool started;
};

// What translate reads and where it stands: the text's places and labels,
// the literals that wait, how many literals it has numbered, each logical
// section, and the block of the line it writes.
struct translator {
    const struct shckit_text *text;
    const struct place *places;
    struct name *names;
    size_t named;
    struct literal *literals;
    size_t waiting;
    size_t numbered;
    struct gnu_section *sections;
    struct block block;
};

static void write_symbol(struct sink *sink, const struct translator *tr,
                         const unsigned char *bytes, size_t len,
                         struct block block)
{
    // a name as GNU as reads it. A ?label becomes .L, then the label that
    // opens its block, or a dot and the number of the .SECTION line that
    // does, then a dot and the name without its ?. asmsh's names hold no
    // dot, so none of these can be one of them.
    if (len < 2 || bytes[0] != '?') {
        put(sink, bytes, len);
        return;
    }
    put(sink, ".L", 2);
    if (block.opener != SIZE_MAX) {
        const struct shckit_line *opener = &tr->text->lines[block.opener];
        put(sink, opener->bytes, opener->label.len);
    } else {
        put(sink, ".", 1);
        put_number(sink, block.sections);
    }
    put(sink, ".", 1);
    put(sink, bytes + 1, len - 1);
}

static void write_label(struct sink *sink, const struct translator *tr,
                        const struct shckit_line *line)
{
    if (line->label.len != 0) {
        write_symbol(sink, tr, line->bytes, line->label.len, tr->block);
        put(sink, ":", 1);
    }
}

// A line translate writes for a line of the text keeps its fields' columns:
// begin_line writes the label and goes to the name's column, to_operands
// goes to the operands', and end_line writes the comment and the line's end.
// A field starts at its column, or one blank past the field before where
// that reaches the column.

static size_t begin_line(struct sink *sink, const struct translator *tr,
                         const struct shckit_line *line)
{
    // write the line's label, unless it defines an .EQU, and go to the
    // name's column; the result is where the line starts in sink
    size_t start = sink->size;
    if (!equates(line)) {
        write_label(sink, tr, line);
    }
    move_to(sink, start, line->name.at);
    return start;
}

static void to_operands(struct sink *sink, size_t start,
                        const struct shckit_line *line)
{
    if (line->operands.len != 0) {
        move_to(sink, start, line->operands.at);
    }
}

static void end_line(struct sink *sink, size_t start,
                     const struct shckit_line *line)
{
    // the comment, its ; as GNU as's !, and the line's end
    if (line->comment.len != 0) {
        move_to(sink, start, line->comment.at);
        put(sink, "!", 1);
        put(sink, line->bytes + line->comment.at + 1, line->comment.len - 1);
    }
    put(sink, "\n", 1);
}

static void write_number(struct sink *sink, const unsigned char *bytes,
                         struct token token)
{
    // a number in GNU as's radixes: H', B' and Q' as 0x, 0b and 0 before
    // the digits, and decimal without D' or the leading zeros GNU as reads as
    // octal
    size_t at = token.at;
    size_t end = token.at + token.len;
    unsigned radix = 'D';
    if (token.len > 2 && bytes[at + 1] == '\'') {
        radix = upper(bytes[at]);
        at += 2;
    }
    if (radix == 'H' || radix == 'B') {
        put_text(sink, radix == 'H' ? "0x" : "0b");
    } else if (radix == 'Q') {
        put(sink, "0", 1);
    }
    while (radix == 'D' && at + 1 < end && bytes[at] == '0') {
        at++;
    }
    put(sink, bytes + at, end - at);
}

static bool section_end(const unsigned char *bytes, struct shckit_span span,
                        struct token *section)
{
    // whether span is (STARTOF X)+(SIZEOF X), with X in *section
    static const char *const shape[] = {"(", "STARTOF", NULL, ")", "+",
                                        "(", "SIZEOF",  NULL, ")"};
    size_t at = span.at;
    struct token token;
    for (size_t i = 0; i < sizeof shape / sizeof shape[0]; i++) {
        if (!next_token(bytes, &at, span.at + span.len, &token) ||
            (shape[i] != NULL
                 ? !same_name(bytes + token.at, token.len, shape[i])
                 : !token.word ||
                       (i > 2 && (token.len != section->len ||
                                  memcmp(bytes + token.at, bytes + section->at,
                                         token.len) != 0)))) {
            return false;
        }
        if (i == 2) {
            *section = token;
        }
    }
    return !next_token(bytes, &at, span.at + span.len, &token);
}

// The binary operators whose ranks asmsh and GNU as order apart. asmsh ranks
// * and /, then + and -, then << and >>, then &, then |; GNU as ranks *, /,
// << and >> together, then | and & together, then + and -.
enum { BY_MUL = 1, BY_ADD = 2, BY_SHIFT = 4, BY_AND = 8, BY_OR = 16 };

// The levels of parentheses whose operators write_expression tells apart.
// Deeper ones share the last level, which can only refuse more.
#define MOST_LEVELS 16

static bool grouped_apart(unsigned ops)
{
    // whether operators of these kinds, at one level, group apart
    return ((ops & BY_SHIFT) != 0 && (ops & (BY_MUL | BY_ADD)) != 0) ||
           ((ops & BY_ADD) != 0 && (ops & (BY_AND | BY_OR)) != 0) ||
           (ops & (BY_AND | BY_OR)) == (BY_AND | BY_OR);
}

static unsigned binary_kind(unsigned byte)
{
    // the kind of the binary operator that starts with byte, or 0
    static const char operators[] = "*/+-<>&|";
    static const unsigned kinds[] = {BY_MUL,   BY_MUL,   BY_ADD, BY_ADD,
                                     BY_SHIFT, BY_SHIFT, BY_AND, BY_OR};
    const char *at = memchr(operators, (int)byte, sizeof operators - 1);
    return at != NULL ? kinds[at - operators] : 0;
}

static bool gnu_name(const unsigned char *bytes, struct token token)
{
    // whether GNU as reads the name otherwise than asmsh: $, asmsh's
    // location, which is a symbol to GNU as, and _GLOBAL_OFFSET_TABLE_,
    // which GNU as relocates as the GOT's address
    static const char got[] = "_GLOBAL_OFFSET_TABLE_";
    return (token.len == 1 && bytes[token.at] == '$') ||
           (token.len == sizeof got - 1 &&
            memcmp(bytes + token.at, got, token.len) == 0);
}

static bool reserved_name(const unsigned char *bytes, size_t len)
{
    // whether the name can be one translate writes for the link to define:
    // __startof_, __sizeof_ or __endof_ and a section's name. asmsh and GNU
    // as keep a name's case, and so does translate.
    static const char *const prefixes[] = {"__startof_", "__sizeof_",
                                           "__endof_"};
    for (size_t i = 0; i < sizeof prefixes / sizeof prefixes[0]; i++) {
        size_t prefix = strlen(prefixes[i]);
        if (len > prefix && memcmp(bytes, prefixes[i], prefix) == 0) {
            return true;
        }
    }
    return false;
}

static int write_expression(struct sink *sink, const struct translator *tr,
                            size_t index, struct shckit_span span,
                            struct block block)
{
    // span of the line at index, written so GNU as reads it as asmsh does:
    // each number and ?label respelled, STARTOF X, SIZEOF X and their sum as
    // the names the link defines, a unary ^ as the ~ asmsh reads it as, and
    // every other byte as it is. Refuse a string, a name gnu_name or
    // reserved_name gives, any other word operator, ~ or ^ between operands,
    // and operators the two assemblers group apart.
    const unsigned char *bytes = tr->text->lines[index].bytes;
    size_t at = span.at;
    size_t end = span.at + span.len;
    unsigned levels[MOST_LEVELS] = {0};
    size_t depth = 0;
    bool operand = true; // an operand comes next, so an operator is unary
    struct token token;
    struct token section;
    uint32_t value = 0;
    if (section_end(bytes, span, &section)) {
        put_text(sink, "__endof_");
        put(sink, bytes + section.at, section.len);
        return SHCKIT_OK;
    }
    for (size_t from = at; next_token(bytes, &at, end, &token); from = at) {
        unsigned *ops = &levels[depth < MOST_LEVELS ? depth : MOST_LEVELS - 1];
        unsigned byte = bytes[token.at];
        unsigned kind = token.word || operand ? 0 : binary_kind(byte);
        put(sink, bytes + from, token.at - from);
        if (token.word && byte == '"') {
            return SHCKIT_STRING;
        }
        if (token.word && number(bytes, token, &value)) {
            write_number(sink, bytes, token);
        } else if (token.word && word_operator(bytes, token)) {
            bool start = same_name(bytes + token.at, token.len, "STARTOF");
            if ((!start && !same_name(bytes + token.at, token.len, "SIZEOF")) ||
                !next_token(bytes, &at, end, &section)) {
                return SHCKIT_WORD;
            }
            put_text(sink, start ? "__startof_" : "__sizeof_");
            put(sink, bytes + section.at, section.len);
        } else if (token.word) {
            if (gnu_name(bytes, token)) {
                return SHCKIT_WORD;
            }
            if (reserved_name(bytes + token.at, token.len)) {
                return SHCKIT_RESERVED;
            }
            write_symbol(sink, tr, bytes + token.at, token.len, block);
        } else if ((byte == '^' || byte == '~') && !operand) {
            return SHCKIT_GROUPING;
        } else if (byte == '^') {
            put(sink, "~", 1);
        } else {
            put(sink, bytes + token.at, token.len);
        }
        if (byte == '(' && depth + 1 < MOST_LEVELS) {
            levels[depth + 1] = 0;
        }
        depth += byte == '(' ? 1 : 0;
        depth -= byte == ')' && depth != 0 ? 1 : 0;
        *ops |= kind;
        if (grouped_apart(*ops)) {
            return SHCKIT_GROUPING;
        }
        operand = !token.word && byte != ')';
    }
    return SHCKIT_OK;
}

static bool imports(const struct shckit_line *line, const unsigned char *bytes,
                    struct token token)
{
    // whether the line is an .IMPORT of the name token spells in bytes
    struct shckit_span name;
    size_t at = 0;
    while (line->directive == SHCKIT_IMPORT &&
           shckit_operand(line, &at, &name)) {
        if (name.len == token.len &&
            memcmp(line->bytes + name.at, bytes + token.at, token.len) == 0) {
            return true;
        }
    }
    return false;
}

static bool same_literal(const struct translator *tr,
                         const struct literal *literal,
                         const struct literal *load)
{
    // whether a load reads the literal: one of its size whose value is the
    // same number, as 32 bits, or is spelled alike, with no line between the
    // two loads that defines or imports a name the value holds, or for a
    // ?label opens a block. asmsh keys a literal by the names defined or
    // imported above its load.
    const struct shckit_line *lines = tr->text->lines;
    const unsigned char *one = lines[literal->load].bytes;
    const unsigned char *two = lines[load->load].bytes;
    struct shckit_span value = load->value;
    size_t at = value.at;
    struct token token;
    if (literal->word != load->word || literal->plain != load->plain) {
        return false;
    }
    if (literal->plain) {
        return literal->bits == load->bits;
    }
    if (value.len != literal->value.len ||
        memcmp(one + literal->value.at, two + value.at, value.len) != 0) {
        return false;
    }
    while (next_token(two, &at, value.at + value.len, &token)) {
        for (size_t i = literal->load + 1; token.word && i <= load->load; i++) {
            const struct shckit_line *line = &lines[i];
            if ((two[token.at] == '?' && starts_block(line, true)) ||
                (line->label.len == token.len &&
                 memcmp(line->bytes, two + token.at, token.len) == 0) ||
                imports(line, two, token)) {
                return false;
            }
        }
    }
    return true;
}

static int take_literal(struct translator *tr, size_t index,
                        struct shckit_span value, bool word, size_t *number)
{
    // *number gets the number of the waiting literal the load at index reads,
    // or of a new one, whose value must translate
    const unsigned char *bytes = tr->text->lines[index].bytes;
    struct literal *load = &tr->literals[tr->waiting];
    struct sink none = {NULL, 0};
    int64_t plain = 0;
    int error = SHCKIT_OK;
    load->load = index;
    load->value = value;
    load->block = tr->block;
    load->word = word;
    load->plain = plain_number(bytes, value.at, value.at + value.len, &plain);
    load->bits = (uint32_t)plain;
    for (size_t i = 0; i < tr->waiting; i++) {
        if (same_literal(tr, &tr->literals[i], load)) {
            *number = tr->literals[i].number;
            return SHCKIT_OK;
        }
    }
    error = write_expression(&none, tr, index, value, tr->block);
    if (error != SHCKIT_OK) {
        return error;
    }
    load->number = tr->numbered++;
    tr->waiting++;
    *number = load->number;
    return SHCKIT_OK;
}

static void write_pool(struct sink *sink, struct translator *tr)
{
    // the waiting literals as asmsh writes its pool: the words in the order
    // of their first loads, then zeros to a long's boundary where longs
    // follow, and the longs. No literal waits after it.
    bool longs = false;
    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = 0; i < tr->waiting; i++) {
            const struct literal *literal = &tr->literals[i];
            size_t start = 0;
            if (literal->word != (pass == 0)) {
                continue;
            }
            if (!literal->word && !longs) {
                put_text(sink, "          .balign     4,0\n");
                longs = true;
            }
            start = sink->size;
            put_text(sink, ".Llit");
            put_number(sink, literal->number);
            put(sink, ":", 1);
            move_to(sink, start, NAME_COLUMN);
            put_text(sink, literal->word ? ".short" : ".long");
            move_to(sink, start, OPERAND_COLUMN);
            // take_literal has refused a value that doesn't translate
            (void)write_expression(sink, tr, literal->load, literal->value,
                                   literal->block);
            put(sink, "\n", 1);
        }
    }
    tr->waiting = 0;
}

static int check_branch(const struct translator *tr, size_t index,
                        struct shckit_span target, bool *imported)
{
    // whether GNU as relocates the branch at index to target as asmsh does:
    // to a ?label, or to a label of its own section, alone or plus or minus
    // a number written in the text. BRA and BSR to a name the text doesn't
    // define, an import, take the relocation translate writes, which
    // *imported says. Any other branch is refused: to a number or an .EQU's
    // name, to another section's label, BT, BF, BT/S and BF/S to an import,
    // and any other target expression, since GNU as builds a branch to one
    // that holds a name defined below it to another place, with no message.
    const struct shckit_line *line = &tr->text->lines[index];
    size_t at = target.at;
    size_t end = target.at + target.len;
    size_t defined = SIZE_MAX;
    uint32_t value = 0;
    struct token token;
    struct token sign;
    *imported = false;
    if (!next_token(line->bytes, &at, end, &token) || !token.word ||
        !starts_symbol(line->bytes[token.at]) ||
        number(line->bytes, token, &value)) {
        return SHCKIT_BRANCH;
    }
    if (next_token(line->bytes, &at, end, &sign) &&
        ((!is_punctuation(line->bytes, sign, '+') &&
          !is_punctuation(line->bytes, sign, '-')) ||
         !next_token(line->bytes, &at, end, &sign) ||
         !number(line->bytes, sign, &value) ||
         next_token(line->bytes, &at, end, &sign))) {
        return SHCKIT_BRANCH;
    }
    if (line->bytes[token.at] == '?') {
        return SHCKIT_OK;
    }
    defined = first_definition(tr->names, tr->named, tr->text->lines,
                               line->bytes + token.at, token.len);
    if (defined == SIZE_MAX) {
        *imported = true;
        return one_of(line, "BRA BSR") ? SHCKIT_OK : SHCKIT_BRANCH;
    }
    return !equates(&tr->text->lines[defined]) &&
                   tr->places[defined].section == tr->places[index].section
               ? SHCKIT_OK
               : SHCKIT_BRANCH;
}

static void write_mnemonic(struct sink *sink, const struct shckit_line *line,
                           unsigned size)
{
    // the mnemonic with the size GNU as writes, in the mnemonic's case
    const unsigned char *name = line->bytes + line->name.at;
    size_t len = 0;
    unsigned had = '-';
    unsigned char suffix[2] = {'.', 0};
    (void)mnemonic(line, &len, &had);
    if (size == had) {
        put(sink, name, line->name.len);
        return;
    }
    put(sink, name, len);
    if (size != '-') {
        suffix[1] = (unsigned char)(name[len - 1] >= 'a' && name[len - 1] <= 'z'
                                        ? size - 'A' + 'a'
                                        : size);
        put(sink, suffix, 2);
    }
}

static bool pc_relative(const struct shckit_line *line)
{
    // whether an operand is @(disp,PC)
    struct shckit_span operand;
    size_t at = 0;
    while (shckit_operand(line, &at, &operand)) {
        if (operand_kind(line->bytes, operand) == KIND_PC_DISP) {
            return true;
        }
    }
    return false;
}

static int write_instruction(struct sink *sink, struct translator *tr,
                             size_t index)
{
    // the instruction at index with the size GNU as writes. A load of a #
    // literal loads its label instead, and a BRA or BSR to an import is its
    // opcode and the relocation of its displacement, which counts from 4
    // bytes past the branch. GNU as reads the displacement of @(disp,PC) as
    // a place, and translate refuses it.
    const struct shckit_line *line = &tr->text->lines[index];
    bool literal = literal_load(tr->text, index);
    struct shckit_span first = span(line->operands.at, line->operands.at);
    size_t start = 0;
    size_t number = 0;
    size_t at = 0;
    size_t end = 0;
    bool imported = false;
    int error = SHCKIT_OK;
    if (pc_relative(line)) {
        return SHCKIT_UNTRANSLATED;
    }
    (void)shckit_operand(line, &at, &first);
    end = first.at + first.len;
    if (one_of(line, "BT BF BT/S BF/S BRA BSR")) {
        error = check_branch(tr, index, first, &imported);
    }
    if (error == SHCKIT_OK && imported) {
        put_text(sink, "          .reloc      .,R_SH_IND12W,");
        error = write_expression(sink, tr, index, first, tr->block);
        put_text(sink, "-4\n");
        start = begin_line(sink, tr, line);
        put_text(sink, ".short");
        to_operands(sink, start, line);
        put_text(sink, one_of(line, "BRA") ? "0xA000" : "0xB000");
        end_line(sink, start, line);
        return error;
    }
    if (error == SHCKIT_OK && literal) {
        // the value, past the # and any blanks
        error = take_literal(
            tr, index, span(skip_blanks(line->bytes, end, first.at + 1), end),
            is_instruction(line, "MOV", "W"), &number);
    }
    start = begin_line(sink, tr, line);
    // place_slots has refused an instruction the opcode table doesn't hold
    write_mnemonic(sink, line, gnu_size(line, opcode_row(tr->text, index)));
    to_operands(sink, start, line);
    if (error == SHCKIT_OK && literal) {
        put_text(sink, ".Llit");
        put_number(sink, number);
        error = write_expression(
            sink, tr, index, span(end, line->operands.at + line->operands.len),
            tr->block);
    } else if (error == SHCKIT_OK) {
        error = write_expression(sink, tr, index, line->operands, tr->block);
    }
    end_line(sink, start, line);
    return error;
}

static bool writes_byte(const struct shckit_line *line)
{
    // whether the line writes a byte: an instruction, or data other than
    // .RES, which asmsh leaves unwritten, or an .SDATA of empty strings
    struct shckit_span operand;
    size_t at = 0;
    while (line->directive == SHCKIT_SDATA &&
           shckit_operand(line, &at, &operand)) {
        if (operand.len > 2) {
            return true; // more than ""
        }
    }
    return line->kind == SHCKIT_INSTRUCTION ||
           (writes_data(line) && line->directive != SHCKIT_SDATA &&
            (line->directive < SHCKIT_RES_B || line->directive > SHCKIT_RES_L));
}

static void write_section(struct sink *sink, struct translator *tr,
                          size_t index, const struct shckit_span *parts)
{
    // .section and the section's name, parts[0]. Where the text first names
    // the section, its flags and kind follow, @nobits for data that writes
    // no byte, then a .balign of parts[2]'s n, ALIGN=n; where the text
    // returns to it, the name alone
    const struct shckit_line *line = &tr->text->lines[index];
    struct gnu_section *section = &tr->sections[tr->places[index].section];
    size_t start = begin_line(sink, tr, line);
    put_text(sink, ".section");
    to_operands(sink, start, line);
    put(sink, line->bytes + parts[0].at, parts[0].len);
    if (!section->started && line->code) {
        put_text(sink, ",\"ax\",@progbits");
    } else if (!section->started) {
        put_text(sink,
                 section->writes ? ",\"aw\",@progbits" : ",\"aw\",@nobits");
    }
    end_line(sink, start, line);
    if (!section->started) {
        put_text(sink, "          .balign     ");
        put(sink, line->bytes + parts[2].at + 6, parts[2].len - 6);
        put(sink, "\n", 1);
    }
    section->started = true;
}

// GNU as's directive for each one the reader knows, or NULL where translate
// writes none. .ALIGN in a code section is .balignw, which fills with NOPs.
static const char *const gnu_directives[SHCKIT_ASSIGN + 1] = {
    [SHCKIT_ALIGN] = ".balign",  [SHCKIT_DATA_B] = ".byte",
    [SHCKIT_DATA_W] = ".short",  [SHCKIT_DATA_L] = ".long",
    [SHCKIT_RES_B] = ".space",   [SHCKIT_RES_W] = ".space",
    [SHCKIT_RES_L] = ".space",   [SHCKIT_SDATA] = ".ascii",
    [SHCKIT_FDATA_S] = ".float", [SHCKIT_DATAB_B] = ".fill",
    [SHCKIT_IMPORT] = ".extern", [SHCKIT_EXPORT] = ".global",
    [SHCKIT_END] = ".end",       [SHCKIT_EQU] = ".equ",
};

static int write_string(struct sink *sink, const struct shckit_line *line,
                        struct shckit_span operand)
{
    // operand, a string of printable ASCII but " and \, which .ascii writes
    // as asmsh does
    size_t end = operand.at;
    struct token token;
    if (!next_token(line->bytes, &end, operand.at + operand.len, &token) ||
        line->bytes[token.at] != '"' || end != operand.at + operand.len) {
        return SHCKIT_STRING;
    }
    for (size_t i = token.at + 1; i + 1 < end; i++) {
        if (line->bytes[i] < ' ' || line->bytes[i] > '~' ||
            line->bytes[i] == '"' || line->bytes[i] == '\\') {
            return SHCKIT_STRING;
        }
    }
    put(sink, line->bytes + operand.at, operand.len);
    return SHCKIT_OK;
}

static int write_float(struct sink *sink, const struct shckit_line *line,
                       struct shckit_span operand)
{
    // operand, F' and a constant, which .float takes with e for its S.
    // asmsh refuses any other F', and translate any other operand, such as
    // a float's bit pattern.
    const unsigned char *bytes = line->bytes;
    if (operand.len < 3 || upper(bytes[operand.at]) != 'F' ||
        bytes[operand.at + 1] != '\'') {
        return SHCKIT_UNTRANSLATED;
    }
    for (size_t i = operand.at + 2; i < operand.at + operand.len; i++) {
        unsigned char byte = upper(bytes[i]) == 'S' ? 'e' : bytes[i];
        // GNU as reads a lone 0 and an e as a radix's prefix, and 0.e as 0
        if (byte == 'e' && bytes[i - 1] == '0' && !digit(bytes[i - 2]) &&
            bytes[i - 2] != '.') {
            put(sink, ".", 1);
        }
        put(sink, &byte, 1);
    }
    return SHCKIT_OK;
}

static int write_operands(struct sink *sink, const struct translator *tr,
                          size_t index)
{
    // each operand of the line at index, as its directive takes it, and the
    // bytes between them as they are
    const struct shckit_line *line = &tr->text->lines[index];
    struct shckit_span operand;
    size_t at = 0;
    size_t from = line->operands.at;
    int error = SHCKIT_OK;
    while (error == SHCKIT_OK && shckit_operand(line, &at, &operand)) {
        put(sink, line->bytes + from, operand.at - from);
        if (line->directive == SHCKIT_SDATA) {
            error = write_string(sink, line, operand);
        } else if (line->directive == SHCKIT_FDATA_S) {
            error = write_float(sink, line, operand);
        } else {
            error = write_expression(sink, tr, index, operand, tr->block);
        }
        from = operand.at + operand.len;
    }
    return error;
}

static int write_directive(struct sink *sink, struct translator *tr,
                           size_t index)
{
    // the directive at index as GNU as's. Refuse one translate doesn't
    // write: .POOL, .NOPOOL, .ASSIGN, .DATAB.W and .DATAB.L, and .END with
    // an entry point; a .SECTION or .END where a literal waits; and a
    // .SECTION of a name reserved_name gives, since GNU as makes each
    // section's name a symbol
    const struct shckit_line *line = &tr->text->lines[index];
    const char *name = gnu_directives[line->directive];
    struct shckit_span parts[3] = {{0, 0}, {0, 0}, {0, 0}};
    size_t start = 0;
    size_t count = 0;
    size_t at = 0;
    int error = SHCKIT_OK;
    while (count < 3 && shckit_operand(line, &at, &parts[count])) {
        count++;
    }
    if ((line->directive == SHCKIT_SECTION || line->directive == SHCKIT_END) &&
        tr->waiting != 0) {
        return SHCKIT_WAITING;
    }
    if (line->directive == SHCKIT_SECTION &&
        reserved_name(line->bytes + parts[0].at, parts[0].len)) {
        return SHCKIT_RESERVED;
    }
    if (line->directive == SHCKIT_SECTION) {
        write_section(sink, tr, index, parts);
        return SHCKIT_OK;
    }
    if (name == NULL || (line->directive == SHCKIT_END && count != 0)) {
        return SHCKIT_UNTRANSLATED;
    }
    start = begin_line(sink, tr, line);
    put_text(sink, name);
    put_text(sink, line->directive == SHCKIT_ALIGN && line->code ? "w" : "");
    to_operands(sink, start, line);
    switch (line->directive) {
    case SHCKIT_RES_W:
    case SHCKIT_RES_L:
        put(sink, "(", 1);
        error = write_operands(sink, tr, index);
        put_text(sink, line->directive == SHCKIT_RES_W ? ")*2" : ")*4");
        break;
    case SHCKIT_DATAB_B:
        error = write_expression(sink, tr, index, parts[0], tr->block);
        put_text(sink, ",1,");
        if (error == SHCKIT_OK) {
            error = write_expression(sink, tr, index, parts[1], tr->block);
        }
        break;
    case SHCKIT_EQU:
        put(sink, line->bytes + line->label.at, line->label.len);
        put(sink, ",", 1);
        error = write_operands(sink, tr, index);
        break;
    default:
        error = write_operands(sink, tr, index);
        put_text(sink, line->directive == SHCKIT_ALIGN && line->code ? ",0x0009"
                                                                     : "");
    }
    end_line(sink, start, line);
    return error;
}

static bool in_section(const struct shckit_line *line)
{
    // whether the line takes a place in a section: a label, an instruction,
    // data or .ALIGN
    return line->kind == SHCKIT_LABEL || line->kind == SHCKIT_INSTRUCTION ||
           writes_data(line) || line->directive == SHCKIT_ALIGN;
}

static bool pool_place(const struct translator *tr, size_t index)
{
    // whether asmsh writes the waiting literals after the line at index: the
    // slot of a branch that never falls through, with no label on it or
    // between the branch and it. use-assembler-pools writes .NOPOOL there.
    return stops(tr->text, tr->places, index) &&
           slot_label(tr->text, tr->places, index) == SIZE_MAX;
}

static int translate_line(struct sink *sink, struct translator *tr,
                          size_t index)
{
    // the line at index, and after a pool place the waiting literals.
    // Refuse a label reserved_name gives, an .EQU's and an .ASSIGN's too.
    const struct shckit_line *line = &tr->text->lines[index];
    size_t start = sink->size;
    int error = SHCKIT_OK;
    if (reserved_name(line->bytes + line->label.at, line->label.len)) {
        return SHCKIT_RESERVED;
    }
    if (line->kind == SHCKIT_BLANK || line->kind == SHCKIT_COMMENT ||
        line->kind == SHCKIT_LABEL) {
        write_label(sink, tr, line);
        end_line(sink, start, line);
        return SHCKIT_OK;
    }
    if (line->kind == SHCKIT_DIRECTIVE) {
        return write_directive(sink, tr, index);
    }
    if (line->kind != SHCKIT_INSTRUCTION) {
        return SHCKIT_UNTRANSLATED; // a continuation
    }
    error = write_instruction(sink, tr, index);
    if (error == SHCKIT_OK && tr->waiting != 0 && pool_place(tr, index)) {
        write_pool(sink, tr);
    }
    return error;
}

static int write_text(struct sink *sink, struct translator *tr, size_t *refused)
{
    // write each line in order, the lines above any .SECTION in P, and
    // refuse at the first line translate refuses
    const struct shckit_text *text = tr->text;
    int error = SHCKIT_OK;
    tr->waiting = 0;
    tr->numbered = 0;
    tr->block.opener = SIZE_MAX;
    tr->block.sections = 0;
    for (size_t i = 0; i <= text->count; i++) {
        tr->sections[i].started = false;
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        bool first = line->label.len != 0 &&
                     first_definition(tr->names, tr->named, text->lines,
                                      line->bytes + line->label.at,
                                      line->label.len) == i;
        if (starts_block(line, first)) {
            tr->block.opener = line->directive == SHCKIT_SECTION ? SIZE_MAX : i;
            tr->block.sections += line->directive == SHCKIT_SECTION ? 1 : 0;
        }
        if (in_section(line) && !tr->sections[tr->places[i].section].started) {
            put_text(sink, "          .section    P,\"ax\",@progbits\n"
                           "          .balign     4\n");
            tr->sections[tr->places[i].section].started = true;
        }
        error = translate_line(sink, tr, i);
        if (error != SHCKIT_OK) {
            *refused = line->input;
        }
    }
    if (error == SHCKIT_OK && tr->waiting != 0) {
        error = fail_at(refused, SHCKIT_WAITING,
                        text->lines[tr->literals[0].load].input);
    }
    return error;
}

int shckit_translate(const struct shckit_text *text, void *dst, size_t cap,
                     size_t *out, size_t *line)
{
    // walk the text twice, measuring and then writing, so that a text
    // translate refuses, or one that doesn't fit, writes nothing
    size_t count = text->count != 0 ? text->count : 1;
    struct place *places = malloc(count * sizeof *places);
    struct translator tr;
    struct sink sink = {NULL, 0};
    size_t refused = 0;
    size_t size = 0;
    int error = SHCKIT_OK;
    memset(&tr, 0, sizeof tr);
    tr.text = text;
    tr.places = places;
    tr.literals = malloc(count * sizeof *tr.literals);
    // a text whose every line names a new section has count + 1 of them
    tr.sections = calloc(count + 1, sizeof *tr.sections);
    if (places == NULL || tr.literals == NULL || tr.sections == NULL) {
        error = SHCKIT_NOMEM;
    }
    if (error == SHCKIT_OK) {
        error = preflight(text, &refused);
    }
    if (error == SHCKIT_OK) {
        error = place_slots(text, places, SLOTS_MUST_PLACE, &refused);
    }
    for (size_t i = 0; error == SHCKIT_OK && i < text->count; i++) {
        struct gnu_section *section = &tr.sections[places[i].section];
        section->writes = section->writes || writes_byte(&text->lines[i]);
    }
    if (error == SHCKIT_OK) {
        error = sorted_labels(text, &tr.names, &tr.named);
    }
    if (error == SHCKIT_OK) {
        error = write_text(&sink, &tr, &refused);
    }
    size = sink.size;
    if (error == SHCKIT_OK &&
        (size == SIZE_MAX || (dst != NULL && size > cap))) {
        error = SHCKIT_FULL;
    }
    if (error == SHCKIT_OK && dst != NULL) {
        sink.at = dst;
        sink.size = 0;
        error = write_text(&sink, &tr, &refused);
    }
    if (out != NULL) {
        *out = error == SHCKIT_OK || error == SHCKIT_FULL ? size : 0;
    }
    if (line != NULL) {
        *line =
            error == SHCKIT_OK || error == SHCKIT_FULL || error == SHCKIT_NOMEM
                ? 0
                : refused;
    }
    free(tr.names);
    free(tr.sections);
    free(tr.literals);
    free(places);
    return error;
}

// Inspection. An instruction rule reads each instruction of the function in
// a code section, reached or not. The stack rule follows control from the
// entry, through branches to the function's own labels, until it meets what
// it can't follow: a limit. It judges fschg-pairs too, on the lines it
// reaches. docs/inspect.md has the rules' scopes and evidence.

// The releases a rule holds for, as bits: 5.0 R10, 5.0 R32, each 5.0 and
// 5.1 release read, and 6.0C.001.
enum {
    IN_R10 = 1,
    IN_R32 = 2,
    IN_FIVES = 4,
    IN_SIX = 8,
};

enum { BOTH_MODES = SHCKIT_MACHINECODE | SHCKIT_ASMCODE };

static unsigned releases_of(const char *release)
{
    // 0 for a release no rule was read in
    static const struct {
        const char *name;
        unsigned bits;
    } releases[] = {
        {"5.0R10", IN_R10 | IN_FIVES}, {"5.0R26", IN_FIVES},
        {"5.0R27", IN_FIVES},          {"5.0R28", IN_FIVES},
        {"5.0R31", IN_FIVES},          {"5.0R32", IN_R32 | IN_FIVES},
        {"5.1R01", IN_FIVES},          {"5.1R03", IN_FIVES},
        {"5.1R04", IN_FIVES},          {"5.1R08", IN_FIVES},
        {"5.1R11", IN_FIVES},          {"5.1R13", IN_FIVES},
        {"6.0C.001", IN_SIX},
    };
    for (size_t i = 0; i < sizeof releases / sizeof releases[0]; i++) {
        if (strcmp(release, releases[i].name) == 0) {
            return releases[i].bits;
        }
    }
    return 0;
}

// The instruction rules, with the releases each holds for under
// machinecode and under asmcode, and the releases that write the
// instruction for C, where it shows nothing. A rule with operands is a
// template's: SHC writes the instruction with those alone.
static const struct {
    const char *rule;
    const char *name;
    const char *sizes;
    const char *first;
    const char *second;
    unsigned machinecode;
    unsigned asmcode;
    unsigned written;
} instruction_rules[] = {
    {"rotcl", "ROTCL", "-L", NULL, NULL, IN_FIVES | IN_SIX, IN_R10 | IN_R32, 0},
    {"rotcr", "ROTCR", "-L", NULL, NULL, IN_FIVES | IN_SIX, IN_R10 | IN_R32, 0},
    {"swap", "SWAP", "-BW", NULL, NULL, IN_FIVES | IN_SIX, IN_R10 | IN_R32, 0},
    {"dmuls", "DMULS", "-L", NULL, NULL, IN_FIVES, IN_FIVES, 0},
    {"dmulu", "DMULU", "-L", NULL, NULL, IN_FIVES, IN_FIVES, 0},
    {"movca", "MOVCA", "-L", NULL, NULL, IN_R10 | IN_R32, IN_R10 | IN_R32, 0},
    {"fschg", "FSCHG", "-", NULL, NULL, IN_R32, IN_R32, IN_R10},
    {"cache-line", "OCBI", "-", NULL, NULL, IN_R10 | IN_R32, IN_R10 | IN_R32,
     0},
    {"cache-line", "OCBP", "-", NULL, NULL, IN_R10 | IN_R32, IN_R10 | IN_R32,
     0},
    {"cache-line", "OCBWB", "-", NULL, NULL, IN_R10 | IN_R32, IN_R10 | IN_R32,
     0},
    {"sett", "SETT", "-", NULL, NULL, IN_R10 | IN_R32, IN_R10 | IN_R32, 0},
    {"fipr-registers", "FIPR", "-S", "FV4", "FV0", IN_R10 | IN_R32,
     IN_R10 | IN_R32, 0},
    {"ftrv-registers", "FTRV", "-S", "XMTRX", "FV0", IN_R10 | IN_R32,
     IN_R10 | IN_R32, 0},
    {"fsca-registers", "FSCA", "-S", "FPUL", "DR0", IN_R10, IN_R10, 0},
};

// The witnesses the rules find, for shckit_inspect_all: the instruction
// rules', then from split the stack rule's. Each run is sorted by line and
// kind, and at one line and kind holds them in the rules table's order.
struct witnesses {
    struct shckit_witness *at;
    size_t count;
    size_t cap;
    size_t split;
    bool failed; // allocation failed, so the witnesses are incomplete
};

// What the rules share: the text, the function's lines from first up to
// end, the labels asmsh defines sorted by by_name, the producer's releases
// and modes, what the rules found, and unless list is NULL each witness
// they find. blocks[i] is the line that opens line i's block of ?labels, or
// 0 above the first; exposed[i] is whether the text names a label of line
// i's run of data, up to its last data line, other than as the source of a
// PC-relative load. returns is whether the function holds an RTS in a code
// section, reached or not.
struct inspector {
    const struct shckit_text *text;
    size_t first;
    size_t end;
    const struct name *labels;
    size_t labelled;
    const size_t *blocks;
    const bool *exposed;
    bool returns;
    unsigned releases;
    unsigned modes;
    struct shckit_inspection *found;
    struct witnesses *list;
};

static void limit(struct shckit_inspection *result, const char *name,
                  size_t index)
{
    // keep the first, with a limit at no line, SIZE_MAX, after any other
    if (result->incomplete == NULL || index < result->incomplete_index) {
        result->incomplete = name;
        result->incomplete_index = index;
    }
}

static void witness(const struct inspector *in, int kind, const char *rule,
                    size_t index, int32_t value)
{
    // keep the lowest kind, and the first of it, and add each to list unless
    // it is NULL
    const struct shckit_witness seen = {kind, rule, index, value};
    struct shckit_witness *kept = &in->found->witness;
    struct witnesses *list = in->list;
    if (kept->kind == SHCKIT_NO_WITNESS || kind < kept->kind ||
        (kind == kept->kind && index < kept->index)) {
        *kept = seen;
    }
    if (list == NULL || list->failed) {
        return;
    }
    if (list->count == list->cap) {
        struct shckit_witness *grown =
            grow(list->at, &list->cap, sizeof *list->at, 64);
        if (grown == NULL) {
            list->failed = true;
            return;
        }
        list->at = grown;
    }
    list->at[list->count++] = seen;
}

static bool holds(const struct inspector *in, unsigned machinecode,
                  unsigned asmcode, size_t index)
{
    // whether a rule that holds for those releases under each mode holds for
    // the producer's, in every mode asked. Where it doesn't, the line it
    // names is rule-scope.
    if (((in->modes & SHCKIT_MACHINECODE) == 0 ||
         (machinecode & in->releases) != 0) &&
        ((in->modes & SHCKIT_ASMCODE) == 0 || (asmcode & in->releases) != 0)) {
        return true;
    }
    limit(in->found, "rule-scope", index);
    return false;
}

// An instruction's operands, on its own line: each one's span, kind, and
// the general register it names first, or -1.
struct operands {
    struct shckit_span spans[MOST_OPERANDS];
    int kinds[MOST_OPERANDS];
    int regs[MOST_OPERANDS];
    size_t count;
};

static void read_operands(const struct shckit_line *line, struct operands *ops)
{
    size_t at = 0;
    ops->count = 0;
    while (ops->count < MOST_OPERANDS &&
           shckit_operand(line, &at, &ops->spans[ops->count])) {
        ops->kinds[ops->count] =
            operand_kind(line->bytes, ops->spans[ops->count]);
        ops->regs[ops->count] = register_of(line, ops->spans[ops->count]);
        ops->count++;
    }
}

static bool spelled(const struct shckit_line *line, struct shckit_span operand,
                    const char *name)
{
    return same_name(line->bytes + operand.at, operand.len, name);
}

static size_t definitions(const struct shckit_text *text, struct name *labels,
                          size_t count)
{
    // keep, in order, the definitions definition() chooses from, and return
    // how many: every one of a ?label, and of any other name the first, and
    // where that is an .ASSIGN, each later .ASSIGN
    size_t kept = 0;
    size_t first = 0;
    for (size_t i = 0; i < count; i++) {
        const struct name *each = &labels[i];
        const struct shckit_line *line = &text->lines[each->index];
        if (!defines(line)) {
            continue;
        }
        if (kept != 0 && name_matches(&labels[first], each->bytes, each->len)) {
            bool redefines =
                line->directive == SHCKIT_ASSIGN &&
                text->lines[labels[first].index].directive == SHCKIT_ASSIGN;
            if (each->bytes[0] != '?' && !redefines) {
                continue;
            }
        } else {
            first = kept;
        }
        labels[kept++] = *each;
    }
    return kept;
}

static bool opens_block(const struct inspector *in, size_t index)
{
    // starts_block, with the definitions asmsh takes from in->labels
    const struct shckit_line *line = &in->text->lines[index];
    const unsigned char *bytes = line->bytes + line->label.at;
    size_t pos =
        name_lower_bound(in->labels, in->labelled, bytes, line->label.len);
    return starts_block(
        line, line->label.len != 0 && pos < in->labelled &&
                  name_matches(&in->labels[pos], bytes, line->label.len) &&
                  in->labels[pos].index == index);
}

static size_t definition(const struct inspector *in, size_t at,
                         const unsigned char *bytes, size_t len)
{
    // the line of the definition asmsh takes for the name where the line at
    // names it, or SIZE_MAX for none: the name's first definition in at's
    // block for a ?label, and in the whole text for any other name, but
    // where that is an .ASSIGN, the last .ASSIGN at or above at, or the
    // first where at lies above them all. asmsh ignores the others, with
    // warning 801.
    size_t low = name_lower_bound(in->labels, in->labelled, bytes, len);
    size_t high = in->labelled;
    size_t found = 0;
    if (len != 0 && bytes[0] == '?') {
        // the first in at's block or past it: a name's definitions are in
        // line order, so their blocks only rise
        while (low < high) {
            size_t middle = low + ((high - low) / 2);
            const struct name *each = &in->labels[middle];
            if (name_matches(each, bytes, len) &&
                in->blocks[each->index] < in->blocks[at]) {
                low = middle + 1;
            } else {
                high = middle;
            }
        }
    }
    if (low == in->labelled || !name_matches(&in->labels[low], bytes, len)) {
        return SIZE_MAX;
    }
    found = in->labels[low].index;
    if (bytes[0] == '?') {
        return in->blocks[found] == in->blocks[at] ? found : SIZE_MAX;
    }
    if (in->text->lines[found].directive != SHCKIT_ASSIGN) {
        return found;
    }
    // definitions() kept only .ASSIGNs of the name past its first, in line
    // order
    low++;
    while (low < high) {
        size_t middle = low + ((high - low) / 2);
        const struct name *each = &in->labels[middle];
        if (name_matches(each, bytes, len) && each->index <= at) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return in->labels[low - 1].index;
}

static bool constant(const struct inspector *in, size_t line, bool literal,
                     const unsigned char *bytes, size_t at, size_t end,
                     int32_t *value)
{
    // the 32-bit word at..end of the line at index line gives: a plain
    // number, or one symbol whose definition there is an .EQU or .ASSIGN of
    // one, taken mod 2^32 as asmsh takes it. H'FFFFFFFF and -1 are both -1,
    // and -H'FFFFFFFF is 1. Above the first .ASSIGN of a name, asmsh reads
    // it as 0, but in a literal, the x of MOV.L #x, as its value where asmsh
    // writes the pool, which inspect doesn't place.
    int64_t number = 0;
    uint32_t word = 0;
    struct token token;
    struct token more;
    size_t at_token = at;
    bool plain = plain_number(bytes, at, end, &number);
    if (!plain && next_token(bytes, &at_token, end, &token) && token.word &&
        !next_token(bytes, &at_token, end, &more)) {
        const struct shckit_line *def = NULL;
        struct shckit_span operand;
        size_t from = 0;
        size_t defined = definition(in, line, bytes + token.at, token.len);
        if (defined == SIZE_MAX) {
            return false;
        }
        def = &in->text->lines[defined];
        if (def->directive == SHCKIT_ASSIGN && line < defined) {
            number = 0;
            plain = !literal;
        } else {
            plain = equates(def) && shckit_operand(def, &from, &operand) &&
                    plain_number(def->bytes, operand.at,
                                 operand.at + operand.len, &number);
        }
    }
    if (!plain) {
        return false;
    }
    word = (uint32_t)number;
    *value = word > INT32_MAX ? (int32_t)((int64_t)word - 0x100000000LL)
                              : (int32_t)word;
    return true;
}

static bool in_run(const struct shckit_line *line)
{
    // whether a run of data may hold the line: any but an instruction, a
    // .SECTION and an .END. The data on both sides of any other line lies
    // in one stretch of its section, so a label+n reaches across it.
    return line->kind == SHCKIT_BLANK || line->kind == SHCKIT_COMMENT ||
           line->kind == SHCKIT_LABEL || line->kind == SHCKIT_CONTINUATION ||
           (line->kind == SHCKIT_DIRECTIVE &&
            line->directive != SHCKIT_SECTION && line->directive != SHCKIT_END);
}

// What a PC-relative load's word is to small-constant: a constant, a word
// that is no constant, as a symbol, or a word inspect can't read.
enum { WORD_CONSTANT, WORD_OTHER, WORD_UNREAD };

static int pool_word(const struct inspector *in, size_t index,
                     struct shckit_span operand, int32_t *value, size_t *item)
{
    // the word a PC-relative MOV.L at index of label or label+n loads, n in
    // decimal: the .DATA.L item n bytes past the label's definition, counted
    // through the data and the .RES pads below it. *item is the line of any
    // item that starts there.
    const unsigned char *bytes = in->text->lines[index].bytes + operand.at;
    size_t name = 0;
    size_t offset = 0;
    size_t at = 0;
    size_t defined = 0;
    while (name < operand.len && in_symbol(bytes[name])) {
        name++;
    }
    if (name == 0 || (name < operand.len && bytes[name] != '+') ||
        name + 1 == operand.len) {
        return WORD_UNREAD;
    }
    for (size_t i = name + 1; i < operand.len; i++) {
        if (!digit(bytes[i]) || offset > (SIZE_MAX - 9) / 10) {
            return WORD_UNREAD;
        }
        offset = (offset * 10) + (bytes[i] - (unsigned)'0');
    }
    defined = definition(in, index, bytes, name);
    for (size_t i = defined; i < in->text->count; i++) {
        const struct shckit_line *data = &in->text->lines[i];
        static const unsigned widths[] = {
            [SHCKIT_DATA_B] = 1, [SHCKIT_DATA_W] = 2, [SHCKIT_DATA_L] = 4,
            [SHCKIT_RES_B] = 1,  [SHCKIT_RES_W] = 2,  [SHCKIT_RES_L] = 4};
        struct shckit_span each;
        size_t from = 0;
        int64_t pads = 0;
        if (data->kind == SHCKIT_BLANK || data->kind == SHCKIT_COMMENT ||
            data->kind == SHCKIT_LABEL) {
            continue;
        }
        if (data->kind != SHCKIT_DIRECTIVE || data->directive < SHCKIT_DATA_B ||
            data->directive > SHCKIT_RES_L || continued(in->text, i)) {
            return WORD_UNREAD;
        }
        if (data->directive >= SHCKIT_RES_B) {
            // a pad holds no word, so the item lies past it
            if (!shckit_operand(data, &from, &each) ||
                !plain_number(data->bytes, each.at, each.at + each.len,
                              &pads) ||
                pads < 0 ||
                (uint64_t)pads * widths[data->directive] > offset - at) {
                return WORD_UNREAD;
            }
            at += (size_t)pads * widths[data->directive];
            continue;
        }
        while (shckit_operand(data, &from, &each)) {
            if (at == offset) {
                *item = i;
                if (data->directive != SHCKIT_DATA_L) {
                    return WORD_UNREAD;
                }
                return constant(in, i, false, data->bytes, each.at,
                                each.at + each.len, value)
                           ? WORD_CONSTANT
                           : WORD_OTHER;
            }
            at += widths[data->directive];
            if (at > offset) {
                return WORD_UNREAD;
            }
        }
    }
    return WORD_UNREAD;
}

static bool loads_from(const struct shckit_line *line, size_t operand,
                       struct shckit_span source, size_t len)
{
    // whether the operand, which names a label len bytes long first, is
    // the source of a PC-relative MOV.W or MOV.L load: the label, or the
    // label+n in decimal, into a general register
    struct operands ops;
    if (operand != 0 || !is_instruction(line, "MOV", "-WL")) {
        return false;
    }
    read_operands(line, &ops);
    if (ops.count != 2 || ops.kinds[0] != KIND_EXPRESSION ||
        !general(ops.kinds[1])) {
        return false;
    }
    if (source.len == len) {
        return true;
    }
    if (source.len < len + 2 || line->bytes[source.at + len] != '+') {
        return false;
    }
    return all(line->bytes, span(source.at + len + 1, source.at + source.len),
               digit);
}

static void mark_named(const struct inspector *in, bool *flags)
{
    // set flags[d] where the text names the label asmsh defines at line d
    // other than as the source of a PC-relative load, as an .EXPORT, MOVA,
    // a .DATA item or #label does
    for (size_t i = 0; i < in->text->count; i++) {
        const struct shckit_line *line = &in->text->lines[i];
        struct shckit_span operand;
        size_t at = 0;
        for (size_t count = 0; shckit_operand(line, &at, &operand); count++) {
            size_t from = operand.at;
            struct token token;
            while (next_token(line->bytes, &from, operand.at + operand.len,
                              &token)) {
                size_t defined = 0;
                if (!token.word ||
                    (line->kind == SHCKIT_INSTRUCTION &&
                     token.at == operand.at &&
                     loads_from(line, count, operand, token.len))) {
                    continue;
                }
                defined = definition(in, i, line->bytes + token.at, token.len);
                if (defined != SIZE_MAX) {
                    flags[defined] = true;
                }
            }
        }
    }
}

static void mark_exposed(const struct shckit_text *text, bool *flags)
{
    // turn flags from mark_named into exposed's: whether each line's run of
    // data, a stretch of lines in_run takes, has a flagged label up to its
    // last data line
    size_t from = 0;
    while (from < text->count) {
        size_t to = from;
        size_t data = from;
        bool named = false;
        while (to < text->count && in_run(&text->lines[to])) {
            data = writes_data(&text->lines[to]) ? to + 1 : data;
            to++;
        }
        // a line no run holds takes one step, and its flag clears
        to += to == from ? 1 : 0;
        for (size_t i = from; i < data; i++) {
            named = named || flags[i];
        }
        for (size_t i = from; i < to; i++) {
            flags[i] = named;
        }
        from = to;
    }
}

static bool small_constant(const struct inspector *in, size_t index,
                           int32_t *value, const char **doubt)
{
    // a MOV.L of #x, or a MOV or MOV.L of a pool's .DATA.L item, whose
    // constant is from -128 to 127. A word inspect can't read, or one whose
    // run of data the text names, which may be a variable, is the limit
    // pool-word. MOV of #x is the 8-bit immediate, which reads no pool.
    const struct shckit_line *line = &in->text->lines[index];
    struct operands ops;
    size_t item = SIZE_MAX;
    int word = WORD_OTHER;
    if (!is_instruction(line, "MOV", "-L")) {
        return false;
    }
    read_operands(line, &ops);
    if (ops.count != 2 || !general(ops.kinds[1])) {
        return false;
    }
    if (ops.kinds[0] == KIND_IMM && is_instruction(line, "MOV", "L")) {
        word = constant(in, index, true, line->bytes, ops.spans[0].at + 1,
                        ops.spans[0].at + ops.spans[0].len, value)
                   ? WORD_CONSTANT
                   : WORD_OTHER;
    } else if (ops.kinds[0] == KIND_EXPRESSION) {
        word = pool_word(in, index, ops.spans[0], value, &item);
    } else if (ops.kinds[0] == KIND_PC_DISP) {
        // the word lies at an address inspect doesn't compute
        word = WORD_UNREAD;
    }
    if (word == WORD_CONSTANT && (*value < -128 || *value > 127)) {
        return false;
    }
    if (word == WORD_UNREAD ||
        (word == WORD_CONSTANT && item != SIZE_MAX && in->exposed[item])) {
        *doubt = "pool-word";
        return false;
    }
    return word == WORD_CONSTANT;
}

static bool zero_displacement(const struct inspector *in, size_t index,
                              int32_t *value, const char **doubt)
{
    // a MOV.B, MOV.W or MOV.L through @(0,Rn), its 0 a constant. A MOV
    // with no size is asmsh's MOV.L.
    const struct shckit_line *line = &in->text->lines[index];
    struct operands ops;
    (void)doubt;
    if (!is_instruction(line, "MOV", "-BWL")) {
        return false;
    }
    read_operands(line, &ops);
    for (size_t i = 0; i < ops.count; i++) {
        // parenthesized() made the operand @( displacement , Rn )
        const unsigned char *bytes = line->bytes;
        size_t open = 0;
        size_t comma = 0;
        if (ops.kinds[i] != KIND_DISP) {
            continue;
        }
        open = ops.spans[i].at;
        while (bytes[open] != '(') {
            open++;
        }
        comma = ops.spans[i].at + ops.spans[i].len - 1;
        while (bytes[comma] != ',') {
            comma--;
        }
        if (constant(in, index, false, bytes, open + 1, comma, value) &&
            *value == 0) {
            return true;
        }
    }
    return false;
}

// Instructions whose last operand is the one register they write, and
// instructions that write none, with no @Rn+ or @-Rn form among them. With
// those frame_step and connect take by name, they hold every mnemonic of the
// opcode table; one added to the table and not here stops the stack rule as
// instruction-effect.
static const char writes_last[] =
    "ADDC ADDV AND DIV1 OR XOR SUB SUBC SUBV NEG NEGC NOT SHAD SHLD XTRCT "
    "EXTS EXTU SWAP MOVT ROTCL ROTCR ROTL ROTR SHAL SHAR SHLL SHLL2 SHLL8 "
    "SHLL16 SHLR SHLR2 SHLR8 SHLR16 DT MOVA";
static const char writes_none[] =
    "CMP/EQ CMP/GE CMP/GT CMP/HI CMP/HS CMP/STR CMP/PL CMP/PZ TST TAS DIV0S "
    "DIV0U MUL MULS MULU DMULS DMULU CLRMAC CLRS CLRT SETS SETT LDTLB NOP "
    "FRCHG FABS FNEG FSQRT FLDI0 FLDI1 FSRRA FADD FCMP/EQ FCMP/GT FDIV FMUL "
    "FSUB FCNVDS FCNVSD FTRC FLOAT FLDS FSTS FMAC FIPR FTRV FSCA MOVCA "
    "PREF OCBI OCBP OCBWB";

static bool inert(const struct shckit_text *text, size_t index)
{
    // a line control passes without running anything: a blank, a comment, a
    // label, an .EXPORT, .IMPORT, .EQU or .ASSIGN, or a continuation of one
    const struct shckit_line *line = &text->lines[index];
    while (line->kind == SHCKIT_CONTINUATION && index > 0) {
        index--;
        line = &text->lines[index];
    }
    return line->kind == SHCKIT_BLANK || line->kind == SHCKIT_COMMENT ||
           line->kind == SHCKIT_LABEL ||
           (line->kind == SHCKIT_DIRECTIVE &&
            (line->directive == SHCKIT_IMPORT ||
             line->directive == SHCKIT_EXPORT || equates(line)));
}

static size_t next_instruction(const struct inspector *in, size_t at)
{
    // the first line from at that holds an instruction, past lines control
    // passes, or SIZE_MAX
    for (; at < in->end; at++) {
        if (in->text->lines[at].kind == SHCKIT_INSTRUCTION) {
            return at;
        }
        if (!inert(in->text, at)) {
            break;
        }
    }
    return SIZE_MAX;
}

static bool moves(const struct inspector *in, size_t index, int from_kind,
                  int from, int to_kind, int to)
{
    // whether the line at index is MOV or MOV.L between those operands:
    // KIND_RN for a general register, or an address of that kind through it
    struct operands ops;
    if (index == SIZE_MAX ||
        !is_instruction(&in->text->lines[index], "MOV", "-L") ||
        continued(in->text, index)) {
        return false;
    }
    read_operands(&in->text->lines[index], &ops);
    return ops.count == 2 && ops.regs[0] == from && ops.regs[1] == to &&
           (from_kind == KIND_RN ? general(ops.kinds[0])
                                 : ops.kinds[0] == from_kind) &&
           (to_kind == KIND_RN ? general(ops.kinds[1])
                               : ops.kinds[1] == to_kind);
}

static bool loads_stack(const struct inspector *in, size_t index, bool loaded)
{
    // whether the line at index writes R0 alone, from no general register
    // but R0 once loaded, as SHC loads the stack sp=x names: a MOV of #imm,
    // a label, @(disp,PC) or @(disp,GBR), STC GBR,R0, or once loaded a MOV
    // of @R0 or ADD #imm,R0
    const struct shckit_line *line = NULL;
    struct operands ops;
    int from = 0;
    if (index == SIZE_MAX || continued(in->text, index)) {
        return false;
    }
    line = &in->text->lines[index];
    read_operands(line, &ops);
    if (ops.count != 2 || ops.kinds[1] != KIND_R0) {
        return false;
    }
    from = ops.kinds[0];
    if (one_of(line, "MOV")) {
        return from == KIND_IMM || from == KIND_EXPRESSION ||
               from == KIND_PC_DISP || from == KIND_GBR_DISP ||
               (loaded && from == KIND_AT && ops.regs[0] == 0);
    }
    return (one_of(line, "STC") && from == KIND_GBR) ||
           (loaded && one_of(line, "ADD") && from == KIND_IMM);
}

static bool switches_stack(const struct inspector *in, size_t index)
{
    // whether index is a write to R15 in the stack switch SHC writes for
    // #pragma interrupt(f(sp=x)), and nothing else. The function starts
    //
    //       MOV.L       R0,@-R15
    //       ...                    ; one or more loads_stack lines
    //       MOV.L       R15,@-R0
    //       MOV         R0,R15
    //
    // and each MOV.L @R15+,R15 and MOV.L @R15+,R0 that RTE or TRAPA follows
    // switches back. Lines control passes may stand between them, and MOV
    // may go without its size.
    const struct shckit_line *lines = in->text->lines;
    struct operands ops;
    size_t at = next_instruction(in, in->first);
    bool loaded = false;
    if (!moves(in, at, KIND_RN, 0, KIND_PRE, 15)) {
        return false;
    }
    at = next_instruction(in, at + 1);
    while (loads_stack(in, at, loaded)) {
        loaded = true;
        at = next_instruction(in, at + 1);
    }
    if (!loaded || !moves(in, at, KIND_RN, 15, KIND_PRE, 0)) {
        return false;
    }
    at = next_instruction(in, at + 1);
    if (at == SIZE_MAX || !is_instruction(&lines[at], "MOV", "-L") ||
        continued(in->text, at)) {
        return false;
    }
    read_operands(&lines[at], &ops);
    if (ops.count != 2 || ops.kinds[0] != KIND_R0 || !general(ops.kinds[1]) ||
        ops.regs[1] != 15) {
        return false;
    }
    if (index == at) {
        return true;
    }
    at = next_instruction(in, index + 1);
    if (!moves(in, index, KIND_POST, 15, KIND_RN, 15) ||
        !moves(in, at, KIND_POST, 15, KIND_RN, 0)) {
        return false;
    }
    at = next_instruction(in, at + 1);
    return at != SIZE_MAX && one_of(&lines[at], "RTE TRAPA");
}

static bool stack_written(const struct inspector *in, size_t index,
                          int32_t *value, const char **doubt)
{
    // a write to R15 other than as a push's or pop's address, by an ADD or
    // SUB, by MOV R14,R15, or in the stack switch
    const struct shckit_line *line = &in->text->lines[index];
    struct operands ops;
    size_t last = 0;
    (void)doubt;
    *value = 0;
    read_operands(line, &ops);
    if (ops.count == 0 || one_of(line, "ADD SUB")) {
        return false;
    }
    last = ops.count - 1;
    if (!general(ops.kinds[last]) || ops.regs[last] != 15 ||
        (!one_of(line, "MOV STC STS") && !one_of(line, writes_last))) {
        return false;
    }
    if (one_of(line, "MOV") && ops.count == 2 && general(ops.kinds[0]) &&
        ops.regs[0] == 14) {
        return false;
    }
    return !switches_stack(in, index);
}

static bool fpscr_save(const struct inspector *in, size_t index, int32_t *value,
                       const char **doubt)
{
    // FPSCR's push, STS.L FPSCR,@-R15, or its pop, LDS.L @R15+,FPSCR, in a
    // function that holds an RTS. R10 saves FPSCR so only for an interrupt
    // function, which returns by RTE or ends in TRAPA.
    const struct shckit_line *line = &in->text->lines[index];
    struct operands ops;
    (void)doubt;
    *value = 0;
    if (!in->returns || !one_of(line, "STS LDS")) {
        return false;
    }
    read_operands(line, &ops);
    if (ops.count != 2) {
        return false;
    }
    if (one_of(line, "STS")) {
        return ops.kinds[0] == KIND_FPSCR && ops.kinds[1] == KIND_PRE &&
               ops.regs[1] == 15;
    }
    return ops.kinds[0] == KIND_POST && ops.regs[0] == 15 &&
           ops.kinds[1] == KIND_FPSCR;
}

// The rules that read an instruction's operands whole, with the kind of
// witness each finds and the releases it holds for under each mode. Each sets
// the value it names, or 0; one that can't read the line sets *doubt to the
// limit that is and returns false.
static const struct {
    const char *rule;
    int kind;
    unsigned machinecode;
    unsigned asmcode;
    bool (*shows)(const struct inspector *in, size_t index, int32_t *value,
                  const char **doubt);
} operand_rules[] = {
    {"small-constant", SHCKIT_OPERAND, IN_R10, IN_R10, small_constant},
    {"zero-displacement", SHCKIT_OPERAND, IN_R10 | IN_R32, IN_R10 | IN_R32,
     zero_displacement},
    {"stack-written", SHCKIT_ABI, IN_R10 | IN_R32, IN_R10 | IN_R32,
     stack_written},
    {"fpscr-save", SHCKIT_ABI, IN_R10, IN_R10, fpscr_save},
};

static void read_instruction(const struct inspector *in, size_t index)
{
    // each rule's witness at the instruction, or the limit the rule meets
    // there, or rule-scope where the rule doesn't hold. A rule that reads
    // operands can't read an instruction with a continuation below it,
    // which is the limit continued. check made sure a template's
    // instruction has two operands.
    const struct shckit_line *line = &in->text->lines[index];
    bool whole = !continued(in->text, index);
    if (!whole) {
        limit(in->found, "continued", index);
    }
    for (size_t i = 0;
         i < sizeof instruction_rules / sizeof instruction_rules[0]; i++) {
        struct shckit_span one;
        struct shckit_span two;
        size_t at = 0;
        bool fixed = instruction_rules[i].first != NULL;
        if (!is_instruction(line, instruction_rules[i].name,
                            instruction_rules[i].sizes)) {
            continue;
        }
        if ((instruction_rules[i].written & in->releases) != 0 ||
            (fixed && (!whole || !shckit_operand(line, &at, &one) ||
                       !shckit_operand(line, &at, &two) ||
                       (spelled(line, one, instruction_rules[i].first) &&
                        spelled(line, two, instruction_rules[i].second))))) {
            break;
        }
        if (holds(in, instruction_rules[i].machinecode,
                  instruction_rules[i].asmcode, index)) {
            witness(in, fixed ? SHCKIT_OPERAND : SHCKIT_OPCODE,
                    instruction_rules[i].rule, index, 0);
        }
        break;
    }
    for (size_t i = 0;
         whole && i < sizeof operand_rules / sizeof operand_rules[0]; i++) {
        int32_t value = 0;
        const char *doubt = NULL;
        bool shown = operand_rules[i].shows(in, index, &value, &doubt);
        if ((!shown && doubt == NULL) ||
            !holds(in, operand_rules[i].machinecode, operand_rules[i].asmcode,
                   index)) {
            continue;
        }
        if (doubt != NULL) {
            limit(in->found, doubt, index);
        } else {
            witness(in, operand_rules[i].kind, operand_rules[i].rule, index,
                    value);
        }
    }
}

// The stack rule (docs/inspect.md). It follows control from the entry and
// joins each path's frame into the lines it reaches until none changes.
// Then, where the runs under SZ 0 and SZ 1 agree, or in the one run --sz
// gives, it finds three witnesses:
// - fschg-pairs: a pair's move after an odd number of FSCHGs on every path;
// - stack-above-entry: R15 above its entry value after any step control
//   reaches;
// - return-below-entry: R15 below its entry value after an RTS's slot.
// A step or a join that turns R15's offset unknown is stack-unknown.

// A register's value less R15's at the entry, in bytes, if known.
struct frame_offset {
    int32_t bytes;
    bool known;
};

// What the stack rule knows at a line: R15's and R14's offsets, FPSCR.SZ,
// and whether an odd number of FSCHGs has run since the entry, each of the
// last two 0, 1 or -1 for unknown.
struct frame {
    struct frame_offset sp;
    struct frame_offset fp;
    int sz;
    int fschgs;
};

static const struct frame_offset unknown_offset = {0, false};

static struct frame_offset offset_add(struct frame_offset offset, int bytes)
{
    // an offset past int32_t's range is unknown
    int64_t sum = (int64_t)offset.bytes + bytes;
    if (!offset.known) {
        return offset;
    }
    if (sum < INT32_MIN || sum > INT32_MAX) {
        return unknown_offset;
    }
    offset.bytes = (int32_t)sum;
    return offset;
}

static struct frame_offset *tracked(struct frame *frame, int reg)
{
    // R15's offset or R14's, or NULL for any other register
    if (reg == 15) {
        return &frame->sp;
    }
    return reg == 14 ? &frame->fp : NULL;
}

static bool signed_byte(const struct shckit_line *line,
                        struct shckit_span operand, int *value)
{
    // #n, n from -128 to 127 as written: no H'80 to H'FF, which the SH-4
    // sign-extends
    int64_t number = 0;
    if (operand.len == 0 || line->bytes[operand.at] != '#' ||
        !plain_number(line->bytes, operand.at + 1, operand.at + operand.len,
                      &number) ||
        number < -128 || number > 127) {
        return false;
    }
    *value = (int)number;
    return true;
}

static void step_addresses(struct frame *frame, const struct operands *ops,
                           int width)
{
    // each @Rn+ adds the width, and each @-Rn takes it away. A width of 0 is
    // unknown.
    for (size_t i = 0; i < ops->count; i++) {
        struct frame_offset *offset = tracked(frame, ops->regs[i]);
        if (offset == NULL ||
            (ops->kinds[i] != KIND_PRE && ops->kinds[i] != KIND_POST)) {
            continue;
        }
        if (width == 0) {
            *offset = unknown_offset;
        } else {
            *offset =
                offset_add(*offset, ops->kinds[i] == KIND_PRE ? -width : width);
        }
    }
}

static void lose(struct frame *frame, int reg, int kind)
{
    // a write to R15 or R14 that leaves no offset
    struct frame_offset *offset = tracked(frame, reg);
    if (offset != NULL && general(kind)) {
        *offset = unknown_offset;
    }
}

static int width_of(const struct shckit_line *line)
{
    // the bytes a MOV or MAC moves through memory, by its size, or without
    // one by asmsh's: .L for MOV and .W for MAC
    size_t len = 0;
    unsigned size = '-';
    (void)mnemonic(line, &len, &size);
    if (size == '-') {
        size = one_of(line, "MAC") ? 'W' : 'L';
    }
    if (size == 'B') {
        return 1;
    }
    return size == 'W' ? 2 : 4;
}

static const char *frame_step(const struct shckit_text *text, size_t index,
                              struct frame *frame)
{
    // run the instruction at index on frame, or return the limit it is. An
    // expansion writes its register alone, wherever asmsh puts its literal.
    const struct shckit_line *line = &text->lines[index];
    struct operands ops;
    if (continued(text, index)) {
        return "continued";
    }
    read_operands(line, &ops);
    if (one_of(line, "BSR BSRF JSR")) {
        return "call";
    }
    if (one_of(line, "JMP BRAF TRAPA SLEEP")) {
        return "opaque-transfer";
    }
    if (one_of(line, "FSCHG")) {
        frame->sz = frame->sz < 0 ? -1 : 1 - frame->sz;
        frame->fschgs = frame->fschgs < 0 ? -1 : 1 - frame->fschgs;
    } else if (one_of(line, "MOV") && ops.count == 2) {
        // a copy takes its source's offset; a load into the register it
        // increments takes the load's
        struct frame_offset *source = tracked(frame, ops.regs[0]);
        struct frame_offset *dest = tracked(frame, ops.regs[1]);
        struct frame_offset copied = unknown_offset;
        if (general(ops.kinds[0]) && source != NULL) {
            copied = *source;
        }
        step_addresses(frame, &ops, width_of(line));
        if (general(ops.kinds[1]) && dest != NULL) {
            *dest = copied;
        }
    } else if (one_of(line, "ADD") && ops.count == 2) {
        struct frame_offset *dest = tracked(frame, ops.regs[1]);
        int bytes = 0;
        if (dest != NULL) {
            *dest = signed_byte(line, ops.spans[0], &bytes)
                        ? offset_add(*dest, bytes)
                        : unknown_offset;
        }
    } else if (one_of(line, "FMOV") && ops.count == 2) {
        // SZ, not the size, sets the width
        int width = 0;
        if (frame->sz >= 0) {
            width = frame->sz == 0 ? 4 : 8;
        }
        step_addresses(frame, &ops, width);
    } else if (one_of(line, "LDC LDS STC STS") && ops.count == 2) {
        step_addresses(frame, &ops, 4);
        lose(frame, ops.regs[1], ops.kinds[1]);
        if (ops.kinds[1] == KIND_FPSCR) {
            frame->sz = -1;
            frame->fschgs = -1;
        }
    } else if (one_of(line, "MAC") && ops.count == 2) {
        step_addresses(frame, &ops, width_of(line));
    } else if (one_of(line, writes_last) && ops.count != 0) {
        lose(frame, ops.regs[ops.count - 1], ops.kinds[ops.count - 1]);
    } else if (!one_of(line, writes_none)) {
        return "instruction-effect";
    }
    return NULL;
}

// How control leaves a line: to the next, to the branch's target, to
// either, or out of the function by its return.
enum { TO_NEXT, TO_TARGET, TO_EITHER, TO_RETURN };

// A line of the function as the stack rule's graph holds it, indexed from
// the function's first line. next is where control passes, past a delayed
// branch's slot, or the function's line count past its last line. target,
// delay and owner are a branch's target and slot, and the branch whose slot
// the line is, or SIZE_MAX; a branch out of the function's lines has no
// target either. block counts the .SECTION lines down to the line. A line
// that is a limit stops the rule where control reaches it. in is the frame
// joined from each path that reached the line so far, and queued says the
// line waits to run again.
struct inspect_node {
    struct frame in;
    size_t next;
    size_t target;
    size_t delay;
    size_t owner;
    size_t block;
    const char *limit;
    int leaves;
    bool reached;
    bool queued;
};

// The stack rule's walk: the graph of the function's count lines, and the
// lines queued to run.
struct walk {
    const struct inspector *in;
    struct inspect_node *nodes;
    size_t count;
    size_t *queue;
    size_t queued;
};

static size_t next_line(const struct walk *walk, size_t at)
{
    // the first line from at that control doesn't pass, or count
    while (at < walk->count && inert(walk->in->text, walk->in->first + at)) {
        at++;
    }
    return at;
}

static const char *branch_out(const struct inspector *in, size_t branch,
                              size_t label)
{
    // the limit of the branch at line branch to the label at line label,
    // outside the function's lines, or NULL for a branch out: section-change
    // across a .SECTION line, and branch-target unless the label marks an
    // instruction outside the lines too, the first line from it that control
    // doesn't pass
    const struct shckit_text *text = in->text;
    size_t low = label < branch ? label : branch;
    size_t high = label < branch ? branch : label;
    size_t at = label;
    for (size_t i = low; i < high; i++) {
        if (text->lines[i].directive == SHCKIT_SECTION) {
            return "section-change";
        }
    }
    while (at < text->count && inert(text, at)) {
        at++;
    }
    if (at == text->count || text->lines[at].kind != SHCKIT_INSTRUCTION ||
        (at >= in->first && at < in->end)) {
        return "branch-target";
    }
    return NULL;
}

static const char *branch_target(struct walk *walk, size_t at)
{
    // set a direct branch's target, or return the limit it is: a target that
    // is no label asmsh defines, as the branch reads it, one past a .SECTION
    // line, or one outside the function's lines but a branch out's, whose
    // target stays SIZE_MAX. $ alone, which asmsh reads as the branch's own
    // place, is no label asmsh defines.
    const struct inspector *in = walk->in;
    const struct shckit_line *line = &in->text->lines[in->first + at];
    struct inspect_node *node = &walk->nodes[at];
    struct shckit_span label;
    size_t operand = 0;
    size_t target = 0;
    if (!shckit_operand(line, &operand, &label)) {
        return "branch-target";
    }
    target = definition(in, in->first + at, line->bytes + label.at, label.len);
    if (target == SIZE_MAX || equates(&in->text->lines[target])) {
        return "branch-target";
    }
    if (target < in->first || target - in->first >= walk->count) {
        return branch_out(in, in->first + at, target);
    }
    if (walk->nodes[target - in->first].block != node->block) {
        return "section-change";
    }
    node->target = next_line(walk, target - in->first);
    return NULL;
}

static void connect(struct walk *walk, size_t at)
{
    // where control leaves the instruction at, its slot included. RTE ends
    // an interrupt's run as RTS ends a call's.
    const struct shckit_text *text = walk->in->text;
    size_t first = walk->in->first;
    const struct shckit_line *line = &text->lines[first + at];
    struct inspect_node *node = &walk->nodes[at];
    size_t slot = node->next;
    if (one_of(line, "BRA BT BF BT/S BF/S")) {
        node->leaves = one_of(line, "BRA") ? TO_TARGET : TO_EITHER;
        node->limit = branch_target(walk, at);
    } else if (one_of(line, "RTS RTE")) {
        node->leaves = TO_RETURN;
    }
    if ((shckit_flow(text, first + at) & SHCKIT_DELAYED) == 0) {
        return;
    }
    if (slot == walk->count ||
        text->lines[first + slot].kind != SHCKIT_INSTRUCTION ||
        (shckit_flow(text, first + slot) & SHCKIT_SLOT_ILLEGAL) != 0) {
        node->limit = "delay-slot";
        return;
    }
    node->delay = slot;
    node->next = next_line(walk, slot + 1);
    walk->nodes[slot].owner = at;
}

static void build(struct walk *walk)
{
    // each line's edges, or the limit it is
    const struct shckit_text *text = walk->in->text;
    size_t first = walk->in->first;
    size_t block = 0;
    for (size_t i = 0; i < walk->count; i++) {
        struct inspect_node *node = &walk->nodes[i];
        block += text->lines[first + i].directive == SHCKIT_SECTION ? 1 : 0;
        node->block = block;
        node->target = SIZE_MAX;
        node->delay = SIZE_MAX;
        node->owner = SIZE_MAX;
    }
    for (size_t i = 0; i < walk->count; i++) {
        const struct shckit_line *line = &text->lines[first + i];
        struct inspect_node *node = &walk->nodes[i];
        node->next = next_line(walk, i + 1);
        if (inert(text, first + i)) {
            continue;
        }
        if (line->directive == SHCKIT_SECTION) {
            node->limit = "section-change";
        } else if (line->directive == SHCKIT_END) {
            node->limit = "fallthrough";
        } else if (!line->code || writes_data(line)) {
            node->limit = "data-flow";
        } else if (line->kind == SHCKIT_CONTINUATION) {
            node->limit = "continued";
        } else if (line->kind != SHCKIT_INSTRUCTION) {
            node->limit = "assembler-layout";
        } else {
            connect(walk, i);
        }
    }
}

static bool offset_join(struct frame_offset *into, struct frame_offset from)
{
    // whether into changes: offsets that disagree are unknown
    if (into->known && (!from.known || into->bytes != from.bytes)) {
        *into = unknown_offset;
        return true;
    }
    return false;
}

static bool frame_join(struct inspect_node *node, const struct frame *from)
{
    // whether the line's frame changes
    bool changed = false;
    if (!node->reached) {
        node->in = *from;
        node->reached = true;
        return true;
    }
    changed = offset_join(&node->in.sp, from->sp);
    changed = offset_join(&node->in.fp, from->fp) || changed;
    if (node->in.sz != -1 && node->in.sz != from->sz) {
        node->in.sz = -1;
        changed = true;
    }
    if (node->in.fschgs != -1 && node->in.fschgs != from->fschgs) {
        node->in.fschgs = -1;
        changed = true;
    }
    return changed;
}

static void reach(struct walk *walk, size_t to, size_t from,
                  const struct frame *frame)
{
    // pass frame from line from to line to, and queue that line if its frame
    // changes. A slot run as other than its branch's would run twice, which
    // the rule doesn't follow.
    size_t first = walk->in->first;
    if (to >= walk->count) {
        limit(walk->in->found, "fallthrough", first + from);
    } else if (walk->nodes[to].owner != SIZE_MAX) {
        limit(walk->in->found, "entered-delay-slot", first + to);
    } else if (frame_join(&walk->nodes[to], frame) && !walk->nodes[to].queued) {
        walk->nodes[to].queued = true;
        // a line waits at most once, so the queue never holds more than
        // count; the analyzer can't see that through queued
        // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
        walk->queue[walk->queued] = to;
        walk->queued++;
    }
}

static size_t edges(const struct inspect_node *node, size_t to[2])
{
    // the lines control can leave node for
    size_t count = 0;
    if (node->leaves == TO_NEXT || node->leaves == TO_EITHER) {
        to[count++] = node->next;
    }
    if ((node->leaves == TO_TARGET || node->leaves == TO_EITHER) &&
        node->target != SIZE_MAX) {
        to[count++] = node->target;
    }
    return count;
}

static bool moves_pair(const struct shckit_line *line)
{
    // an FMOV between memory and FR8, FR10, FR12, FR14 or DR8 to DR14, whose
    // code word's register field is 8, 10, 12 or 14 however it is spelled:
    // under SZ 1, a move of DR8 to DR14 as a pair
    struct operands ops;
    if (!one_of(line, "FMOV")) {
        return false;
    }
    read_operands(line, &ops);
    for (size_t i = 0; ops.count == 2 && i < 2; i++) {
        int other = ops.kinds[1 - i];
        if (listed("FR8 FR10 FR12 FR14 DR8 DR10 DR12 DR14",
                   line->bytes + ops.spans[i].at, ops.spans[i].len) &&
            (other == KIND_AT || other == KIND_POST || other == KIND_PRE ||
             other == KIND_INDEX)) {
            return true;
        }
    }
    return false;
}

// What the stack rule finds at a line of its graph and the line's slot, in
// the rules table's order: at each of the two steps, a pair's move and R15
// above the entry, and past a return's slot R15 below it.
#define MOST_FOUND 5

struct verdict {
    struct shckit_witness at[MOST_FOUND];
    size_t count;
};

static void note(struct verdict *verdict, int kind, const char *rule,
                 size_t index, int32_t value)
{
    const struct shckit_witness seen = {kind, rule, index, value};
    verdict->at[verdict->count++] = seen;
}

static const char *pass_through(const struct walk *walk, size_t at,
                                struct frame *frame, struct verdict *verdict,
                                size_t *where)
{
    // run the line at and its slot on frame, but of a branch or a return
    // only the slot, or return the limit that stops them at line *where.
    // Unless verdict is NULL, put in it the witnesses the steps find, and
    // name R15's offset turned unknown as stack-unknown.
    const struct inspector *in = walk->in;
    const struct inspect_node *node = &walk->nodes[at];
    size_t steps[2];
    size_t count = 0;
    *where = in->first + at;
    if (node->limit != NULL) {
        return node->limit;
    }
    if (node->leaves == TO_NEXT) {
        steps[count++] = at;
    }
    if (node->delay != SIZE_MAX) {
        steps[count++] = node->delay;
    }
    for (size_t i = 0; i < count; i++) {
        size_t line = in->first + steps[i];
        bool known = frame->sp.known;
        const char *stop = frame_step(in->text, line, frame);
        *where = line;
        if (stop != NULL) {
            return stop;
        }
        if (verdict == NULL) {
            continue;
        }
        if (known && !frame->sp.known) {
            limit(in->found, "stack-unknown", line);
        }
        if (frame->fschgs == 1 && moves_pair(&in->text->lines[line])) {
            note(verdict, SHCKIT_OPERAND, "fschg-pairs", line, 0);
        }
        if (frame->sp.known && frame->sp.bytes > 0) {
            note(verdict, SHCKIT_ABI, "stack-above-entry", line,
                 frame->sp.bytes);
        }
    }
    if (verdict != NULL && node->leaves == TO_RETURN && frame->sp.known &&
        frame->sp.bytes < 0 &&
        one_of(&in->text->lines[in->first + at], "RTS")) {
        note(verdict, SHCKIT_ABI, "return-below-entry", *where,
             frame->sp.bytes);
    }
    return NULL;
}

static void run(struct walk *walk, const struct frame *entry)
{
    // join each path's frame into each line it reaches until none changes.
    // A loop's frame settles: each join only loses facts.
    struct inspect_node *nodes = walk->nodes;
    reach(walk, next_line(walk, 0), 0, entry);
    while (walk->queued != 0) {
        size_t at = walk->queue[walk->queued - 1];
        struct frame after = nodes[at].in;
        const char *stop = NULL;
        size_t where = 0;
        size_t to[2];
        size_t leaving = 0;
        walk->queued--;
        nodes[at].queued = false;
        stop = pass_through(walk, at, &after, NULL, &where);
        if (stop != NULL) {
            limit(walk->in->found, stop, where);
            continue;
        }
        leaving = edges(&nodes[at], to);
        for (size_t j = 0; j < leaving; j++) {
            reach(walk, to[j], at, &after);
        }
    }
}

static const struct shckit_witness *partner(const struct verdict *verdict,
                                            const struct shckit_witness *seen)
{
    // verdict's witness of seen's rule at its line, or NULL
    for (size_t i = 0; i < verdict->count; i++) {
        const struct shckit_witness *other = &verdict->at[i];
        if (other->index == seen->index &&
            strcmp(other->rule, seen->rule) == 0) {
            return other;
        }
    }
    return NULL;
}

static void agree(const struct inspector *in, const struct verdict *one,
                  const struct verdict *two)
{
    // report each witness both verdicts find, naming the offset nearer the
    // entry's value, which R15 is at least as far from under either SZ; a
    // rule's offsets at a line share a sign. A witness either finds alone
    // is stack-unknown. The count of FSCHGs starts at 0 under either SZ, so
    // both find the same pairs' moves. With one run, one and two are its
    // verdict.
    for (size_t i = 0; i < one->count; i++) {
        struct shckit_witness kept = one->at[i];
        const struct shckit_witness *other = partner(two, &kept);
        if (other == NULL) {
            limit(in->found, "stack-unknown", kept.index);
            continue;
        }
        if (kept.value > 0 ? other->value < kept.value
                           : other->value > kept.value) {
            kept.value = other->value;
        }
        witness(in, kept.kind, kept.rule, kept.index, kept.value);
    }
    for (size_t j = 0; j < two->count; j++) {
        if (partner(one, &two->at[j]) == NULL) {
            limit(in->found, "stack-unknown", two->at[j].index);
        }
    }
}

static void judge(const struct walk *walks, size_t runs)
{
    // once every path's frame has joined in each run, find the witnesses
    // both runs find, and stack-unknown at a step or a join that turns R15's
    // offset unknown in either
    const struct inspector *in = walks[0].in;
    size_t count = walks[0].count;
    size_t entry = next_line(&walks[0], 0);
    for (size_t i = 0; i < runs; i++) {
        const struct inspect_node *nodes = walks[i].nodes;
        if (entry < count && nodes[entry].reached &&
            !nodes[entry].in.sp.known) {
            limit(in->found, "stack-unknown", in->first + entry);
        }
    }
    for (size_t at = 0; at < count; at++) {
        struct verdict verdicts[2];
        for (size_t i = 0; i < runs; i++) {
            const struct inspect_node *nodes = walks[i].nodes;
            struct frame after = nodes[at].in;
            size_t where = 0;
            size_t to[2];
            size_t leaving = 0;
            verdicts[i].count = 0;
            if (!nodes[at].reached ||
                pass_through(&walks[i], at, &after, &verdicts[i], &where) !=
                    NULL) {
                continue;
            }
            leaving = edges(&nodes[at], to);
            for (size_t j = 0; j < leaving; j++) {
                if (to[j] < count && nodes[to[j]].reached && after.sp.known &&
                    !nodes[to[j]].in.sp.known) {
                    limit(in->found, "stack-unknown", in->first + to[j]);
                }
            }
        }
        agree(in, &verdicts[0], &verdicts[runs - 1]);
    }
}

static int stack_rule(const struct inspector *in, int sz)
{
    // R15's offset and the count of FSCHGs at each line control reaches, in
    // a run under the entry's SZ, or where it is unknown, in one under 0 and
    // one under 1, for the entry's SZ is one of them. SHCKIT_SZ_0 and
    // SHCKIT_SZ_1 are 1 and 2.
    size_t count = in->end - in->first;
    struct walk walks[2] = {{in, NULL, count, NULL, 0},
                            {in, NULL, count, NULL, 0}};
    size_t runs = sz == SHCKIT_SZ_UNKNOWN ? 2 : 1;
    int error = SHCKIT_OK;
    for (size_t i = 0; i < runs; i++) {
        walks[i].nodes = calloc(count, sizeof *walks[i].nodes);
        walks[i].queue = malloc(count * sizeof *walks[i].queue);
        if (walks[i].nodes == NULL || walks[i].queue == NULL) {
            error = SHCKIT_NOMEM;
        }
    }
    for (size_t i = 0; error == SHCKIT_OK && i < runs; i++) {
        struct frame entry = {{0, true},
                              unknown_offset,
                              sz == SHCKIT_SZ_UNKNOWN ? (int)i : sz - 1,
                              0};
        build(&walks[i]);
        run(&walks[i], &entry);
    }
    if (error == SHCKIT_OK) {
        judge(walks, runs);
    }
    for (size_t i = 0; i < runs; i++) {
        free(walks[i].nodes);
        free(walks[i].queue);
    }
    return error;
}

static const struct shckit_inspection no_inspection = {
    {SHCKIT_NO_WITNESS, NULL, SIZE_MAX, 0}, NULL, SIZE_MAX};

static int inspect(const struct shckit_text *text,
                   const struct shckit_producer *producer,
                   const struct shckit_function *function,
                   struct shckit_inspection *result, struct witnesses *list)
{
    // shckit_inspect, adding each witness to list unless it is NULL
    struct inspector in = {
        text, function->first, function->end, NULL, 0, NULL, NULL, false,
        0,    producer->modes, result,        list};
    struct shckit_finding *found = NULL;
    struct name *labels = NULL;
    size_t *blocks = NULL;
    bool *exposed = NULL;
    size_t count = 0;
    int error = SHCKIT_OK;

    *result = no_inspection;
    if (function->first >= function->end || function->end > text->count ||
        function->sz < SHCKIT_SZ_UNKNOWN || function->sz > SHCKIT_SZ_1 ||
        producer->modes == 0 || (producer->modes & ~BOTH_MODES) != 0 ||
        !text->lines[function->first].code ||
        (text->lines[function->first].kind != SHCKIT_LABEL &&
         text->lines[function->first].kind != SHCKIT_INSTRUCTION)) {
        return SHCKIT_INSPECT_INPUT;
    }
    error = shckit_check(text, &found, &count);
    free(found);
    if (error != SHCKIT_OK) {
        return error;
    }
    if (count != 0) {
        return SHCKIT_CHECK_FAILED;
    }
    in.releases = releases_of(producer->release);
    if (in.releases == 0) {
        limit(result, "producer-scope", SIZE_MAX);
        return SHCKIT_OK;
    }
    error = sorted_labels(text, &labels, &in.labelled);
    blocks = malloc(text->count * sizeof *blocks);
    exposed = calloc(text->count, sizeof *exposed);
    if (error == SHCKIT_OK && (blocks == NULL || exposed == NULL)) {
        error = SHCKIT_NOMEM;
    }
    if (error == SHCKIT_OK) {
        in.labelled = definitions(text, labels, in.labelled);
        in.labels = labels;
        for (size_t i = 0; i < text->count; i++) {
            blocks[i] = i != 0 && !opens_block(&in, i) ? blocks[i - 1] : i;
        }
        in.blocks = blocks;
        mark_named(&in, exposed);
        mark_exposed(text, exposed);
        in.exposed = exposed;
    }
    for (size_t i = in.first; i < in.end && !in.returns; i++) {
        in.returns =
            text->lines[i].code && is_instruction(&text->lines[i], "RTS", "-");
    }
    for (size_t i = in.first; error == SHCKIT_OK && i < in.end; i++) {
        if (text->lines[i].code && text->lines[i].kind == SHCKIT_INSTRUCTION) {
            read_instruction(&in, i);
        }
    }
    if (list != NULL) {
        list->split = list->count;
    }
    if (error == SHCKIT_OK && (in.releases & IN_R10) == 0) {
        limit(result, "abi-scope", SIZE_MAX);
    } else if (error == SHCKIT_OK) {
        error = stack_rule(&in, function->sz);
    }
    free(labels);
    free(blocks);
    free(exposed);
    if (error != SHCKIT_OK) {
        *result = no_inspection;
    }
    return error;
}

int shckit_inspect(const struct shckit_text *text,
                   const struct shckit_producer *producer,
                   const struct shckit_function *function,
                   struct shckit_inspection *result)
{
    return inspect(text, producer, function, result, NULL);
}

static bool comes_after(const struct shckit_witness *one,
                        const struct shckit_witness *two)
{
    return one->index != two->index ? one->index > two->index
                                    : one->kind > two->kind;
}

static int merge_witnesses(struct witnesses *list, struct shckit_witness **out,
                           size_t *count)
{
    // merge the instruction rules' run with the stack rule's by line and
    // kind, the instruction rules' first at a tie, which is the rules
    // table's order, and hand the array to the caller
    struct shckit_witness *merged = NULL;
    size_t rules = 0;
    size_t stack = list->split;
    *out = NULL;
    *count = 0;
    if (!list->failed && list->count != 0) {
        merged = malloc(list->count * sizeof *merged);
    }
    if (list->failed || (list->count != 0 && merged == NULL)) {
        free(list->at);
        return SHCKIT_NOMEM;
    }
    for (size_t i = 0; i < list->count; i++) {
        if (rules == list->split ||
            (stack < list->count &&
             comes_after(&list->at[rules], &list->at[stack]))) {
            merged[i] = list->at[stack++];
        } else {
            merged[i] = list->at[rules++];
        }
    }
    free(list->at);
    *out = merged;
    *count = list->count;
    return SHCKIT_OK;
}

int shckit_inspect_all(const struct shckit_text *text,
                       const struct shckit_producer *producer,
                       const struct shckit_function *function,
                       struct shckit_inspection *result,
                       struct shckit_witness **found, size_t *count)
{
    struct witnesses list = {NULL, 0, 0, 0, false};
    int error = inspect(text, producer, function, result, &list);
    if (error == SHCKIT_OK) {
        error = merge_witnesses(&list, found, count);
    } else {
        free(list.at);
        *found = NULL;
        *count = 0;
    }
    if (error != SHCKIT_OK) {
        *result = no_inspection;
    }
    return error;
}

int shckit_write(const struct shckit_text *text, void *dst, size_t cap,
                 size_t *out)
{
    // measure first, so a text that doesn't fit writes nothing
    unsigned char *at = dst;
    size_t size = 0;
    bool fits = true;
    for (size_t i = 0; fits && i < text->count; i++) {
        size_t len = text->lines[i].len + (text->lines[i].crlf ? 2 : 1);
        fits = len <= SIZE_MAX - size;
        size = fits ? size + len : SIZE_MAX;
    }
    if (out != NULL) {
        *out = size;
    }
    if (!fits || (at != NULL && size > cap)) {
        return SHCKIT_FULL;
    }
    for (size_t i = 0; at != NULL && i < text->count; i++) {
        const struct shckit_line *line = &text->lines[i];
        memcpy(at, line->bytes, line->len);
        at += line->len;
        if (line->crlf) {
            *at++ = '\r';
        }
        *at++ = '\n';
    }
    return SHCKIT_OK;
}

void shckit_free(struct shckit_text *text)
{
    if (text == NULL) {
        return;
    }
    disown(text, 0);
    free(text->owned);
    free(text->lines);
    free(text->input);
    free(text);
}

const char *shckit_strerror(int error)
{
    switch (error) {
    case SHCKIT_OK:
        return "ok";
    case SHCKIT_NOT_CP932:
        return "a byte outside CP932, or a lead byte with no trail";
    case SHCKIT_CONTROL:
        return "a control character other than a tab or a line end";
    case SHCKIT_UNENDED:
        return "the text is empty, or its last line has no line end";
    case SHCKIT_UNREADABLE:
        return "a line shckit can't read";
    case SHCKIT_PAST_END:
        return "a line after .END that isn't blank or a comment";
    case SHCKIT_OTHER_ALIGN:
        return "an .ALIGN 4 in a code section that isn't SHCASM's line";
    case SHCKIT_SWITCH:
        return "a pool item Lnn-Lnn, as a switch table's entries are";
    case SHCKIT_POOL_LABEL:
        return "a pool's label line isn't the one SHCASM's pool writer prints";
    case SHCKIT_ABOVE:
        return "a pool's label that doesn't follow an instruction";
    case SHCKIT_ITEM:
        return "a pool item SHCASM's pool writer doesn't print";
    case SHCKIT_SHAPE:
        return "a pool's items are not .DATA.W lines, at most one .RES.W 1, "
               "then .DATA.L lines";
    case SHCKIT_NO_LONG:
        return "a pool without a .DATA.L";
    case SHCKIT_DUPLICATE:
        return "a pool's label is defined twice";
    case SHCKIT_POOLED:
        return "a .POOL, a .NOPOOL or a load asmsh writes a literal for";
    case SHCKIT_NOT_HELD:
        return "an instruction the opcode table doesn't hold";
    case SHCKIT_SLOT:
        return "a delayed branch whose slot shckit can't place";
    case SHCKIT_SLOT_LABEL:
        return "a label on a slot or above it, below its branch, where a "
               ".NOPOOL is due";
    case SHCKIT_REACHED:
        return "a pool's label that isn't right below the slot of a branch "
               "that never falls through";
    case SHCKIT_GUARDED:
        return "a pool that the BRA above it branches over";
    case SHCKIT_USE:
        return "a pool's label used other than by a load or MOVA above it";
    case SHCKIT_NO_ITEM:
        return "a load or MOVA of no whole item of its size";
    case SHCKIT_UNUSED:
        return "a pool item no load or MOVA uses";
    case SHCKIT_CONFLICT:
        return "align-pool and use-assembler-pools together";
    case SHCKIT_OFFSET:
        return "an operand that offsets a code label, $ or PC";
    case SHCKIT_ADDRESS:
        return "a pool item's address used other than to read the item";
    case SHCKIT_LOCAL_LABEL:
        return "a ?label in the block a pool's label opens";
    case SHCKIT_NO_SUCH_PATCH:
        return "a bit that names no patch";
    case SHCKIT_TWICE:
        return "the text was patched already";
    case SHCKIT_FULL:
        return "the text needs more than cap bytes";
    case SHCKIT_NOMEM:
        return "out of memory";
    case SHCKIT_INSPECT_INPUT:
        return "a function or producer inspect can't take";
    case SHCKIT_CHECK_FAILED:
        return "a text check refuses";
    case SHCKIT_UNTRANSLATED:
        return "a directive, operand or continuation translate doesn't write";
    case SHCKIT_WORD:
        return "$, _GLOBAL_OFFSET_TABLE_, or a word operator other than "
               "STARTOF or SIZEOF";
    case SHCKIT_GROUPING:
        return "a ~ or ^ between operands, or operators asmsh and GNU as "
               "group apart";
    case SHCKIT_BRANCH:
        return "a branch GNU as can't relocate as asmsh does";
    case SHCKIT_STRING:
        return "a string outside .SDATA, or one with \"\", \\ or a byte "
               "not printable";
    case SHCKIT_WAITING:
        return "a literal still waiting at a .SECTION, at .END or at the "
               "text's end";
    case SHCKIT_RESERVED:
        return "a name that starts with __startof_, __sizeof_ or __endof_";
    default:
        return "unknown error";
    }
}
