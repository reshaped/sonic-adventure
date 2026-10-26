"""Check the tags in C and assembly sources: @asm, @hack and @bug.

Each inline body, #pragma asm block of assembly and .src function takes
one @asm tag, directly above it. A body or block opens at a directive: a
comment or a literal that mentions one opens neither. A run fails, naming
each problem, on:

- a tag of the wrong form or kind;
- one of those without its @asm tag or with two;
- a #pragma inline_asm spelled other than (NAME) or (NAME(size=N));
- an @asm tag in C not directly above a body or block, or in a .src file
  not ending the comment above a label that starts a function;
- an @asm tag whose text doesn't open as its kind requires, or cites no
  address where its kind needs one;
- an address an @asm tag cites outside its allowed ranges: its
  function's, those of the functions of its source its text names, and for
  an inline body those of its callers, direct or through macros; or for a
  block, its source's rows;
- an if, or a while loop whose body opens with {, whose condition is a
  literal 0 or FALSE in any parentheses, or opens with one ahead of &&,
  without an @hack redundant tag above it.

--review lists each open tag as FILE:LINE, what it tags and its text, then
counts the bodies, blocks and functions, and those with a tag by its kind.
It fails on the same problems. TAGS.md#tags, TAGS.md#assembly,
TAGS.md#hacks-and-bugs and TAGS.md#what-lint-checks give the rules.

--inline checks no tag. It names each C source that holds a #pragma
inline_asm or #pragma asm, a directory standing for each .c file under it,
for the Makefile's INLINE_C and UNSETTLED.
"""

import argparse
import os
import re
import sys

from config import configs, content, fail, hexa, named

KINDS = ('opcode', 'operand', 'abi', 'epilogue', 'name', 'open')
# each kind's rules, one of which an opcode, operand or abi tag's text opens with: shckit inspect's, and for abi also
# three contracts with the places a function is entered from, which inspect, reading one function, can't see
RULES = {'opcode': ('rotcl', 'rotcr', 'swap', 'dmuls', 'dmulu', 'movca', 'fschg', 'cache-line', 'sett'),
         'operand': ('fipr-registers', 'ftrv-registers', 'fsca-registers', 'small-constant', 'zero-displacement',
                     'fschg-pairs'),
         'abi': ('stack-written', 'fpscr-save', 'stack-above-entry', 'return-below-entry', 'register-contract',
                 'exception-entry', 'exception-continuation')}
# the evidence an epilogue or name tag's text opens with: for epilogue, SHC's dead RTS after an inline body's return,
# or its alignment and RTS after a body that ends in neither a return nor a tail jump; for name, the member the
# shipped library names as assembly
EVIDENCE = {'epilogue': ('dead-return', 'fill'),
            'name': ('vmsasm_', 'lcdasm_', 'mpasm_', 'kmasm_workaround_for_sh_lnk_', 'kmasm_kamui_lib_')}
# a hack's kinds in the order that decides a hack fitting more than one, and a bug's
HACKS = ('implicit', 'type', 'redundant', 'spelling')
BUGS = ('memory', 'uninitialized', 'logic', 'arithmetic', 'mismatched-call')
# each tag's kinds, by the tag's name
TAGS = {'asm': KINDS, 'hack': HACKS, 'bug': BUGS}
# a tag's form: its name, its kind and a colon, then its text
GRAMMAR = re.compile(r'@(asm|hack|bug) ([a-z-]+): (\S.*)')
# what each @asm kind but open opens its text with, before a comma: a rule, or its evidence
OPENED = {kind: re.compile(rf'(?:{"|".join(words)}), \S') for kind, words in dict(RULES, **EVIDENCE).items()}
# the tag a comment's text opens, after a /** */ comment's stars
OPENING = re.compile(r'(\*+\s*)?@(asm|hack|bug)\b')
ADDRESS = re.compile(r'\b0x[0-9a-f]{8}\b', re.I)
WORD = re.compile(r'\w+')
# a C comment, or a literal that might hold one's opening
LEXEME = re.compile(r'/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
# an if or a while, to its condition's opening
TEST = re.compile(r'\b(if|while)\s*\(')
# a condition's tokens: operators that end an operand, brackets, and the rest
TOKEN = re.compile(r'&&|\|\||[()\[\]?,]|[^&|()\[\]?,\s]+|[&|]')
# a 0 literal, decimal, octal or hex, with any suffix; or the SDK's FALSE
ZERO = re.compile(r'(?:0+|0[xX]0+)[uUlL]*|FALSE')
# a while loop's body, after its condition, which InsertBraces opens with { unless it is empty or in a macro; a do's
# while has none
BODY = re.compile(r'(?:\s|\\\n)*\{')
# on a line of blanked() C, a directive, its # the line's first token: #pragma inline_asm, however it names its
# bodies; the one spelling of it tags.py reads, and the body it names, which may carry the size in bytes SHC takes for
# it; #pragma asm, which opens a block of assembly; and its #pragma endasm
DIRECTIVE = re.compile(r'\s*#\s*pragma\s+inline_asm\b')
PRAGMA = re.compile(r'\s*#\s*pragma\s+inline_asm\((\w+)(?:\(size=\d+\))?\)\s*$')
BLOCK = re.compile(r'\s*#\s*pragma\s+asm\s*$')
END = re.compile(r'\s*#\s*pragma\s+endasm\s*$')
# in assembly: a label, but for a name .EQU or .ASSIGN gives a value; a label local to its function; the name an .EQU
# or .ASSIGN gives a value; the names an .EXPORT exports; a comment on its own line, indented or not; and one that
# starts a tag
LABEL = re.compile(r'(\w+):(?!\s*\.(?:EQU|ASSIGN)\b)', re.I)
LOCAL = re.compile(r'\?\w*:')
VALUE = re.compile(r'\w+:')
EXPORT = re.compile(r'\s+\.EXPORT\s+([^;]*)', re.I)
NOTE = re.compile(r'\s*;(.*)')
TAG = re.compile(r'\s*;\s*@(asm|hack|bug)\b')


class Tables:
    """Where each source's functions lie: its rows' ranges, and the names in them."""

    def __init__(self):
        self.places = {}
        self.rows = {}
        # each source's functions: the names in its rows of code, whose sections' names start with P
        self.functions = {}
        for config in configs():
            names = sorted(named(config.path / 'symbols.txt'))
            for source, section, start, end in config.placements:
                inside = [(address, name) for address, name in names if start <= address < end]
                self.places.setdefault(source, []).append((inside, end))
                self.rows.setdefault(source, []).append((start, end))
                if section.startswith('P'):
                    self.functions.setdefault(source, set()).update(name for _, name in inside)

    def ranges(self, source, name):
        """Each [start, end) the tables give the function name in source, up to the next name or the row's end."""
        found = []
        for inside, end in self.places.get(source, []):
            for i, (address, symbol) in enumerate(inside):
                if symbol == name:
                    after = [later for later, _ in inside[i + 1:] if later > address]
                    found.append((address, after[0] if after else end))
        return found

    def mentioned(self, source, text):
        """Each [start, end) the tables give a function in source that text names."""
        names = self.functions.get(source, set())
        return [(start, end) for name in sorted(names & set(WORD.findall(text))) for start, end in self.ranges(source, name)]


def tag(where, text, problems):
    """The (tag, kind, text) of the tag a comment's text opens, or None. A malformed one and one in a /** */ comment
    are problems."""
    opening = OPENING.match(text)
    if opening is None:
        return None
    name = opening[2]
    if opening[1]:
        problems.append(f'{where}: a tag is a plain /* */ comment, not /** */')
        return None
    form = GRAMMAR.fullmatch(text)
    if not form or form[2] not in TAGS[name]:
        problems.append(f'{where}: not @{name} <kind>: <text>, with a kind of {", ".join(TAGS[name])}')
        return None
    if name == 'asm' and form[2] in RULES and not OPENED[form[2]].match(form[3]):
        problems.append(f'{where}: not @asm {form[2]}: <rule>, <text>, with a rule of {", ".join(RULES[form[2]])}')
        return None
    if name == 'asm' and form[2] in EVIDENCE and not OPENED[form[2]].match(form[3]):
        problems.append(f'{where}: not @asm {form[2]}: <evidence>, <text>, with evidence of '
                        f'{", ".join(EVIDENCE[form[2]])}')
        return None
    return form.groups()


def cited(where, kind, text, ranges, problems, scope='its function'):
    """Check the addresses a tag's text cites: at least one, but for an open tag, and each inside one of ranges, which
    are scope's."""
    addresses = [int(found, 16) for found in ADDRESS.findall(text)]
    if not addresses and kind != 'open':
        problems.append(f'{where}: @asm {kind} cites no address')
    for address in addresses:
        if not any(start <= address < end for start, end in ranges):
            problems.append(f'{where}: {hexa(address)} is outside {scope}')


def comments(text):
    """Each C comment: the line it ends on, the line it starts on, and its text
    as one line, without /* and */ or each later line's leading *, each run of
    whitespace one space and none at its ends."""
    for match in LEXEME.finditer(text):
        if match[0].startswith('/*'):
            body = re.sub(r'\n\s*\*(?!/)', ' ', match[0][2:-2])
            yield text.count('\n', 0, match.end()) + 1, text.count('\n', 0, match.start()) + 1, ' '.join(body.split())


def blanked(text):
    """C text with each comment and literal blanked, each of its characters
    but a newline or a backslash ending a line a space, so that each line
    holds its code alone, where it stood, and a macro's lines still join."""
    return LEXEME.sub(lambda lexeme: re.sub(r'(?!\\\n)[^\n]', ' ', lexeme[0]), text)


def zero(tokens):
    """The 0 literal or FALSE a condition, as its tokens, comes to, alone or ahead of &&, and whether a && follows it;
    or None."""
    depth, first = 0, len(tokens)
    for i, token in enumerate(tokens):
        depth += (token in '([') - (token in ')]')
        if depth == 0 and token in ('||', '?', ','):
            return None
        if depth == 0 and token == '&&':
            first = min(first, i)
    head, more = tokens[:first], first < len(tokens)
    if len(head) == 1 and ZERO.fullmatch(head[0]):
        return head[0], more
    if len(head) > 2 and head[0] == '(' and closing(head, 0) == len(head) - 1:
        inner = zero(head[1:-1])
        return inner and (inner[0], inner[1] or more)
    return None


def closing(tokens, at):
    """The index of the token that closes the bracket at tokens[at], or None."""
    depth = 0
    for i in range(at, len(tokens)):
        depth += (tokens[i] in '([') - (tokens[i] in ')]')
        if depth == 0:
            return i
    return None


def never_run(code):
    """Each if, and each while whose body opens with {, in blanked() C whose condition zero() reads as a 0 literal or
    FALSE, outside its #if lines: its line and how it reads."""
    for match in TEST.finditer(code):
        start = code.rfind('\n', 0, match.start()) + 1
        if re.fullmatch(r'\s*#\s*', code[start:match.start()]):
            continue
        depth, at = 1, match.end()
        while depth and at < len(code):
            depth += (code[at] == '(') - (code[at] == ')')
            at += 1
        found = zero(TOKEN.findall(code[match.end():at - 1].replace('\\\n', ' ')))
        if found and (match[1] == 'if' or BODY.match(code, at)):
            yield code.count('\n', 0, match.start()) + 1, f'{match[1]} ({found[0]}{" && ..." * found[1]})'


def definitions(code):
    """The functions and macros of blanked() C, as two {name: body} maps. A
    function counts where its { and } stand alone at the margin, as
    .clang-format's Linux braces put them. Its name is the first word before
    a ( on the nearest line above the { with a word at the margin. A { with
    no such line above it, or below one with no (, as an initializer's in a
    clang-format off region, opens no function."""
    lines = code.split('\n')
    functions = {}
    for i, line in enumerate(lines):
        if line == '{':
            head = next((lines[j] for j in range(i - 1, -1, -1) if re.match(r'[A-Za-z_]', lines[j])), '')
            if name := re.search(r'(\w+)\s*\(', head):
                end = next((j for j in range(i, len(lines)) if lines[j] == '}'), len(lines))
                functions[name[1]] = '\n'.join(lines[i:end])
    macros = {macro[1]: macro[2] for macro in re.finditer(r'^#define (\w+)((?:.*\\\n)*.*)', code, re.M)}
    return functions, macros


def calls(body, names):
    return any(re.search(rf'\b{name}\s*\(', body) for name in names)


def hosts(name, functions, macros):
    """The functions that call name, directly or through macros. A function that calls one of them is not one."""
    called = {name}
    while more := {macro for macro, body in macros.items() if macro not in called and calls(body, called)}:
        called |= more
    return [function for function, body in functions.items() if calls(body, called)]


def blocks(lines):
    """The lines that open a #pragma asm block holding more than assembler comments."""
    found = []
    for i, line in enumerate(lines):
        if BLOCK.match(line):
            end = next((j for j in range(i + 1, len(lines)) if END.match(lines[j])), len(lines))
            if any(text.strip() and not text.strip().startswith(';') for text in lines[i + 1:end]):
                found.append(i + 1)
    return found


def check_c(path, text, tables, problems):
    """Check a C source's tags, and return each body and block as (line, what, its @asm tag's [kind, text]), with
    None for no tag."""
    code = blanked(text)
    lines = code.split('\n')
    directives = [i + 1 for i, line in enumerate(lines) if DIRECTIVE.match(line)]
    sites = {line: match[1] for line in directives if (match := PRAGMA.match(lines[line - 1]))}
    problems.extend(f'{path}:{line}: not #pragma inline_asm(<function>) or #pragma inline_asm(<function>(size=<n>))'
                    for line in directives if line not in sites)
    assembly = blocks(lines)
    functions, macros = definitions(code) if sites else ({}, {})
    # each body's and block's @asm tag's kind and text, by its line
    tagged = {}
    # each @hack tag's kind and first line, by the line directly below it
    hacks = {}
    for end, start, body in comments(text):
        found = tag(f'{path}:{start}', body, problems)
        if not found:
            continue
        name, *found = found
        if name == 'hack':
            hacks[end + 1] = found[0], start
        if name != 'asm':
            continue
        if end + 1 in tagged:
            what = f'#pragma inline_asm({sites[end + 1]})' if end + 1 in sites else '#pragma asm'
            problems.append(f'{path}:{start}: a second @asm tag for {what}')
            continue
        if end + 1 in assembly:
            tagged[end + 1] = found
            cited(f'{path}:{start}', *found, tables.rows.get(path, []), problems, 'its source\'s rows')
            continue
        name = sites.get(end + 1)
        if name is None:
            problems.append(f'{path}:{start}: an @asm tag not directly above a #pragma inline_asm or a #pragma asm '
                            'block')
            continue
        tagged[end + 1] = found
        ranges = tables.ranges(path, name)
        for host in hosts(name, functions, macros):
            ranges += tables.ranges(path, host)
        cited(f'{path}:{start}', *found, ranges + tables.mentioned(path, found[1]), problems)
    for line, test in never_run(code):
        below = line
        while below in hacks and hacks[below][0] != 'redundant':
            below = hacks[below][1]
        if below not in hacks:
            problems.append(f'{path}:{line}: {test} has no @hack redundant tag above it')
    return sorted([(line, f'#pragma inline_asm({name})', tagged.get(line)) for line, name in sites.items()] +
                  [(line, '#pragma asm', tagged.get(line)) for line in assembly], key=lambda piece: piece[0])


def inline(text):
    """Whether C text holds a #pragma inline_asm or #pragma asm, as check_c
    reads them: R10 takes either only under -code=asmcode."""
    # each spells asm, so text that never does goes unlexed
    return 'asm' in text and any(DIRECTIVE.match(line) or BLOCK.match(line) for line in blanked(text).split('\n'))


def sources(path):
    """The C source at path, or for a directory each .c file under it."""
    if not os.path.isdir(path):
        yield path
        return
    try:
        entries = list(os.scandir(path))
    except OSError as error:
        fail(f'{path}: {error.strerror}')
    for entry in entries:
        if entry.is_dir(follow_symlinks=False):
            yield from sources(entry.path)
        elif entry.name.endswith('.c'):
            yield entry.path


def symbol(label):
    """The name the image tables give a label: its C spelling, without the linker's leading underscore."""
    return label[1:] if label.startswith('_') else label


def starts(lines):
    """Each run of labels that starts code, as their lines' indexes: labels
    with no statement between them, and after them an instruction, not a
    directive such as .DATA.L. A local label in between is part of the code."""
    found, run = [], []
    for i, line in enumerate(lines):
        statement = line.split(';')[0]
        if match := LABEL.match(statement):
            run.append(i)
            statement = statement[match.end():]
        statement = statement.strip()
        if match := LOCAL.match(statement):
            statement = statement[match.end():].strip()
        if statement:
            if run and not statement.startswith('.') and not VALUE.match(statement):
                found.append(run)
            run = []
    return found


def check_src(path, text, tables, problems):
    """Check an assembly source's tags, and return each function as (line, its names, its @asm tag's [kind, text]),
    with None for no tag. A function is a run of labels that starts code, as starts() gives them, with a label the
    source exports or the image tables name. Its names are those labels. A run with neither starts no function."""
    lines = text.split('\n')
    exported = {name.strip() for line in lines if (match := EXPORT.match(line)) for name in match[1].split(',')}
    # each label's line, and its function's ranges: those the tables give it
    # and the other labels of its run, which name the same code, or where they
    # give none, those of the last label above it they name
    functions, last = {}, []
    for i, line in enumerate(lines):
        if match := LABEL.match(line):
            last = functions[i] = tables.ranges(path, symbol(match[1])) or last
    runs = starts(lines)
    for run in runs:
        if own := [found for i in run for found in tables.ranges(path, symbol(LABEL.match(lines[i])[1]))]:
            functions.update(dict.fromkeys(run, own))
    # each label's @asm tag's kind and text, and where the tag starts, by the label's index
    tagged, at = {}, {}
    i = 0
    while i < len(lines):
        if not TAG.match(lines[i]):
            i += 1
            continue
        start, i = i, i + 1
        while i < len(lines) and NOTE.match(lines[i]) and not TAG.match(lines[i]):
            i += 1
        where = f'{path}:{start + 1}'
        found = tag(where, ' '.join(' '.join(NOTE.match(line)[1] for line in lines[start:i]).split()), problems)
        if TAG.match(lines[start])[1] != 'asm':
            continue
        if i not in functions:
            problems.append(f'{where}: an @asm tag not at the end of the comment above a label')
        elif found:
            tagged[i], at[i] = found[1:], where
            cited(where, *found[1:], functions[i] + tables.mentioned(path, found[2]), problems)
    found, counted = [], set()
    for run in runs:
        if len(second := [i for i in run if i in tagged]) > 1:
            problems.append(f'{at[second[1]]}: a second @asm tag for '
                            f'{", ".join(LABEL.match(lines[i])[1] for i in run)}')
        names = [(i, label) for i in run if (label := LABEL.match(lines[i])[1]) in exported
                 or tables.ranges(path, symbol(label))]
        if names:
            counted.update(run)
            found.append((names[0][0] + 1, ', '.join(label for _, label in names),
                          next((tagged[i] for i in run if i in tagged), None)))
    problems.extend(f'{at[i]}: an @asm tag above {LABEL.match(lines[i])[1]}, which starts no function'
                    for i in tagged if i not in counted)
    return found


def summary(kinds):
    """The line --review ends on: how many bodies, blocks and functions there are, and how many have an @asm tag, by
    kind, given each one's tag's kind, or None."""
    tagged = [kind for kind in kinds if kind]
    each = ', '.join(f'{tagged.count(kind):,} {kind}' for kind in KINDS)
    return (f'{len(tagged):,} of {len(kinds):,} inline bodies, #pragma asm blocks and .src functions have an @asm tag '
            f'({each})')


def main(argv):
    parser = argparse.ArgumentParser(prog='tags', description=__doc__.split('\n\n')[0])
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--review', action='store_true',
                      help="also list each open @asm tag, then count the assembly by its tag's kind")
    mode.add_argument('--inline', action='store_true',
                      help='check no tag, but name each C source that holds a #pragma inline_asm or #pragma asm')
    parser.add_argument('paths', metavar='FILE', nargs='+',
                        help='a C or assembly source, named as splits.txt names it: src/system/thread.c, '
                             'not ./src/system/thread.c; for --inline, a directory stands for each .c file under it')
    args = parser.parse_args(argv)
    if args.inline:
        for path in args.paths:
            for source in sources(path):
                if inline(content(source).decode('latin-1')):
                    print(source)
        return
    tables = Tables()
    problems, pieces = [], []
    for path in args.paths:
        check = check_src if path.endswith('.src') else check_c
        pieces += [(path, *piece) for piece in check(path, content(path).decode('latin-1'), tables, problems)]
    problems += [f'{path}:{line}: {what} has no @asm tag' for path, line, what, found in pieces if found is None]
    if args.review:
        for path, line, what, found in sorted(pieces, key=lambda piece: piece[:2]):
            if found and found[0] == 'open':
                print(f'{path}:{line}: {what}: {found[1]}')
        print(summary([found and found[0] for *_, found in pieces]))
    if problems:
        fail(*problems)


if __name__ == '__main__':
    main(sys.argv[1:])
