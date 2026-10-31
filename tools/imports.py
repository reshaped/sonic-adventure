"""Check each of 1ST_READ.BIN's imports against the definition in the image
it names, and each use of one against the images its stages read.

    python3 tools/imports.py [--add IMAGE SOURCE ...]

An import is a row of config/1st_read/imports.txt, as tools/config.py's
imports() reads it: a name 1ST_READ.BIN's sources use for a definition in
an image that loads beside it. The matching build links the name at the
address the image's symbols.txt gives the image's own name for it. make
gcc's coordinated link binds the name to the definition of the image's name
instead. The two builds agree only where the image defines that name at
that address.

The first check holds each import to the definition at its address. The
placement in the image's splits.txt that holds the address names a source.
A gen/ range passes, since the build writes its C from the disc at the
addresses the image's tables give. Any other source's object must define
the image's name for the import there. The object is the source's under
build/, or its stub's under build/stub/ where the source has bodies. It is
the one build/gcc/deps.mk makes the check wait for. Each of its sections
starts at the source's first placement in that section, as link.py links
it. An import fails, naming its row, where:
- no placement holds its address;
- the object defines no global of that name, as for a static or a place
  inside another definition;
- the object defines the name at an address, in none of its sections, as
  an .EQU does;
- the object defines it in another section, in one its section table
  doesn't list, or at another offset.

The second check holds each use of an import in 1ST_READ.BIN's C sources to
the stages the source gives it. Images that load at one address can each
define a name there, as STG01 and STG02 each define their step at
0x0c900000. A source that names the wrong one builds the same bytes under
SHC, and links to the wrong image's definition under make gcc. Each stage a
use takes must read the import's image. A stage reads an image where
loader_read_stage_files reads it whenever it loads the stage, as
stages.py's stages() gives a stage's images. The check reads each C
source for SHC without its guards' GCC sides, and for GCC without
their shipped sides, as csource.py's code() gives them. GCC compiles the C
in gcc/ in place of the .src file it replaces, so the check reads that C
for GCC alone. It reads each SOURCE an --add gives 1st_read, which make gcc
links after the image's own sources, for GCC alone too. It reads no gen/
range. The matching build's link binds each name its generated data takes
by address, so that C may name any import at the address. make gcc's
content names imports only in path_lists, and tools/content.py picks each
of those by its row's stage.

A use's place is the table or function at file scope that names it. A table
is a definition with an initializer, and each element of the initializer is
a row. A row's index is the number of rows before it. A table's rows have
no index where one of them opens with a designator. Nor do they in a table
with a second extent where one of them opens with neither a brace nor a
string. C may then fill one element from several rows. The check reads no
types, so where a table of structs leaves a row's braces out, it
counts each element as a row. The indices after an #include, or after a
macro the check doesn't expand, shift where it writes another number of
rows than the check counts.

A row with an index takes a stage by it where its table's first extent is
STAGE_COUNT or STAGE_ROWS, as each of stage_steps' rows does. Any other row
takes a stage where it names that stage and no other, as a story section
does. A function takes no stage of its own. A static table or function
takes the stages of the places that name it, where each of those has some.
The pvm lists take theirs from stage_pvms' rows, and windy_set_fog takes
STAGE_WINDY from its row of act_hooks. Statics that name each other, as two
functions that call each other do, share their stages. They take those of
the places outside them that name one, where each of those has some. A
table takes such stages in every row. File scope outside a table and a
function has no stages.

A use takes the stages of the place that names it, not of where it runs. A
stage's row can give a task its step, and the task runs that step for as
long as the task lives. A test of the current stage gives the code under it
no stage. Bounding that code takes a parse of C the check doesn't make. So
each use in field_people_place, which switches on the current stage, takes
a row of uses.txt.

The check ignores a declaration that holds no = and opens with a keyword
for a storage class, a qualifier or a type. Each name of an import in the
code is a use, but for:
- one in a declaration the check ignores;
- a member's, which . or -> comes before;
- a parameter's in its function's body, where parameter() reads the name.
So the check misses a use that runs in a declaration it ignores, as in a
variable-length array's extent or a cleanup attribute. It ignores no
statement that opens with a name, a typedef's included: the name may be a
macro's, which can define something. Where parameter() reads no name, as
from a pointer to a function, the parameter's name in the body names a
use. The check reads no other scope, so a variable a body declares under
an import's name names one.

The check expands each macro a file defines whose body names an import, or
names a macro that does. It expands the macro where the file names it, from
its #define to the next #define or #undef of its name. It expands a
function-like macro only where ( follows its name. A parameter's name in
the body stands for the tokens a use passes, not for an import. Those
tokens stay where the file writes them, after the macro's body. The check
pastes no tokens, so it fails on a macro whose ## can make an import's
name, as pastes() reads one.

The check reads each source without the files it includes, so it holds no
use in one of those to a stage. csource.py's included() finds those files,
headers and others. The check fails on each macro of such a file that names
an import, or names a macro that does. It fails on each use uses_in() finds
in the file too. Each such line names the first source that includes the
file. A file included() doesn't find, as a body make writes under build/,
goes unread.

A use with no stage, or with a stage that doesn't read the import's image,
needs a row of config/1st_read/uses.txt. The row gives the use's place and
image, and the evidence, as sonic_actions' row for STG02 does. A row's
place is a table, a function, or TABLE[N], the row of index N. A row for a
table covers each of its rows. A use at file scope outside a table and a
function has no place, so no row covers it. A row's evidence is a
reviewer's reading, and the check takes it as given.

The tool writes nothing, and fails with a line for each problem the checks
find. An import's line names its row. A use's line names its source line,
its place, and the import's row and image. A use no row covers gets a line
for each of its stages that doesn't read the image. That line names the
images loader_read_stage_files reads for the stage. A use with no stage
that no row covers gets one line. A line for a use only GCC's reading finds
names its place as GCC's. A row of uses.txt fails where it covers no use
its stages don't hold. A row for TABLE[N] fails where a row for TABLE gives
the same source and image. The tool fails at once, naming the file, on:
- a file of config/, or src/system/loader.c, that tools/config.py or
  tools/stages.py refuses;
- a file it can't read;
- a source with neither object built;
- an object whose records, sections or definitions don't parse;
- a bracket at file scope that nothing closes, in a source or a file it
  includes, with its line;
- a macro of a source or a file it includes whose ## can make an import's
  name, with its line.
"""

import argparse
import os
import re
import sys
from pathlib import Path

from bodies import has_bodies
from config import Config, imports
from csource import GCC, SHC, Token, code, definition, included, macro_directives, tokens
from layout import KEYWORDS, identifier
from shc_object import definitions, section_table
from stages import USES, stages, uses
from tables import GENERATED, hexa
from tool import fail

# the first extent, as file_scope() gives one, of a table with a row for
# each stage: src/stage/stage.h's STAGE_COUNT or STAGE_ROWS
BY_STAGE = (('STAGE_COUNT',), ('STAGE_ROWS',))
BRACKETS = {'(': ')', '[': ']', '{': '}'}
# an #include of any file. csource.py's INCLUDED matches only a header's
INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)
# the keywords that open a declaration and never an expression: layout.py's
# KEYWORDS but sizeof
SPECIFIERS = KEYWORDS - {'sizeof'}


def definition_problems():
    """The first check's lines, one for each import it fails, in the table's order."""
    lines, configs, objects = [], {}, {}
    for name, image, symbol, address, where in imports():
        if image not in configs:
            configs[image] = Config(image)
        config = configs[image]
        source, section = next(((source, section) for source, section, start, end in config.placements
                                if start <= address < end), (None, None))
        if source is None:
            lines.append(f'{where}: {name}: no row of {config.splits} holds {hexa(address)}')
            continue
        if GENERATED.fullmatch(source):
            continue
        obj = Path('build/stub' if has_bodies(source) else 'build', source).with_suffix('.obj')
        if not obj.exists():
            fail(f'{source}: no {obj}, so build it first')
        if obj not in objects:
            objects[obj] = list(section_table(obj)), definitions(obj)
        sections, defined = objects[obj]
        if f'_{symbol}' not in defined:
            lines.append(f'{where}: {name}: {obj} defines no {symbol}, which {config.name} puts at {hexa(address)}')
            continue
        base = next(start for label, start, _ in config.ranges(source) if label == section)
        number, offset = defined[f'_{symbol}']
        if number is None:
            lines.append(f'{where}: {name}: {obj} defines {symbol} at the address {hexa(offset)}, in none of its '
                         f'sections, and {config.name} puts it at {hexa(address)}, in {section}')
            continue
        if number >= len(sections) or (sections[number], base + offset) != (section, address):
            at = f'{sections[number]} + {offset:#x}' if number < len(sections) else f'section {number}'
            lines.append(f'{where}: {name}: {obj} defines {symbol} at {at}, and {config.name} puts it at '
                         f'{hexa(address)}, in {section}')
    return lines


def closing(found, at, path):
    """The index of the bracket in found that closes the one at at. found is
    the (token, line) pairs of the C at path. A bracket nothing closes
    fails, naming its path:line."""
    depth = 0
    for index in range(at, len(found)):
        depth += found[index][0] == found[at][0]
        depth -= found[index][0] == BRACKETS[found[at][0]]
        if not depth:
            return index
    fail(f'{path}:{found[at][1]}: an unclosed {found[at][0]}')


def without_parameters(macro):
    """The tokens of macro's body, as macro_directives() gives a macro, without
    its parameters' names, which stand for the tokens a use passes."""
    parameters, words = macro
    return [word for word in words if word not in (parameters or ())]


def macros_naming(directives, names):
    """The names of the macros directives define whose body names one of names,
    or names a macro that does, in the order found. directives is
    macro_directives()'s list. A macro defined more than once counts where any
    of its bodies does."""
    bodies, found, more = {}, {}, True
    for _, name, macro in directives:
        if macro is not None:
            bodies.setdefault(name, set()).update(without_parameters(macro))
    while more:
        more = False
        for name, words in bodies.items():
            if name not in found and any(word in names or word in found for word in words):
                found[name] = None
                more = True
    return list(found)


def pastes(macro, names):
    """Whether the body of macro, as macro_directives() gives a macro, can make
    one of names with ##. A run of tokens that ## joins makes one where it
    spells the name with any word in each parameter's place. A run that . or ->
    comes before makes a member's name, never an import's."""
    parameters, words = macro
    at = 0
    while at < len(words):
        end = at
        while words[end + 1:end + 3] == ['#', '#'] and end + 3 < len(words):
            end += 3
        if end > at and (not at or words[at - 1] not in ('.', '->')):
            spelled = re.compile(''.join(r'\w*' if part in (parameters or ()) else re.escape(part)
                                         for part in words[at:end + 1:3]))
            if any(spelled.fullmatch(name) for name in names):
                return True
        at = end + 1
    return False


def expansion(word, after, live, names, inside=()):
    """The tokens word gives where after follows it. live maps the name of
    each macro scan() expands to its definition where word stands, or to
    None past an #undef. A definition is (body, function): the body as
    without_parameters() gives it, and whether the macro takes parameters. A
    macro of live gives its body with each token expanded in turn, but only
    where that names one of names. A function-like macro expands only where
    ( follows it. No macro expands inside its own body, and inside holds the
    macros whose bodies word lies in. Any other word gives [word]."""
    if live.get(word) is None or word in inside:
        return [word]
    body, function = live[word]
    if function and after != '(':
        return [word]
    found = [each for at, part in enumerate(body)
             for each in expansion(part, body[at + 1] if at + 1 < len(body) else after, live, names, (*inside, word))]
    return found if any(each in names for each in found) else [word]


def scan(path, names, compiler):
    """The (token, line) pairs of the C at path, as code() reads it for
    compiler, with each macro expanded whose tokens name one of names. A
    comment or a directive gives no token. A macro's tokens take its name's
    line. The tokens a use passes a macro follow its body's, where the file
    writes them. A macro whose ## can make one of names fails, naming its
    path:line."""
    text = code(path, compiler=compiler)
    directives = macro_directives(text)
    for line, name, macro in directives:
        if macro is not None and pastes(macro, names):
            fail(f"{path}:{line}: {name}'s ## can make an import's name, and the tool pastes no tokens")
    expanded, live, found, at = set(macros_naming(directives, names)), {}, [], 0
    words = tokens(text, directives=False)
    for index, (word, line) in enumerate(words):
        # no token lies on a directive's lines, so a directive is above a word where its first line is
        while at < len(directives) and directives[at][0] < line:
            _, name, macro = directives[at]
            if name in expanded:
                live[name] = None if macro is None else (without_parameters(macro), macro[0] is not None)
            at += 1
        after = words[index + 1][0] if index + 1 < len(words) else None
        found += [(each, line) for each in expansion(word, after, live, names)]
    return found


def ignored_declaration(found):
    """Whether the check ignores found, a declaration's or a statement's
    (token, line) pairs, as a declaration: it opens with one of SPECIFIERS
    and holds no =."""
    return found[0][0] in SPECIFIERS and all(word != '=' for word, _ in found)


def without_declarations(found):
    """found, a function body's (token, line) pairs, less each statement the
    check ignores as a declaration. A statement here runs from the token
    after a ;, {, } or : to the next ;."""
    kept, start = [], 0
    for at, (word, _) in enumerate(found):
        if word in (';', '{', '}', ':'):
            if word != ';' or not ignored_declaration(found[start:at + 1]):
                kept.extend(found[start:at + 1])
            start = at + 1
    return kept + found[start:]


def declarator(declaration, path):
    """The name a declaration defines, and the tokens of each extent after
    the name, or (None, []) where it names nothing. declaration is its
    (token, line) pairs up to its =. The name is the first name, other than a
    keyword, right before a [, or else the last name. A bracket nothing
    closes fails as closing() fails."""
    names = [index for index, (word, _) in enumerate(declaration) if identifier(word) and word not in KEYWORDS]
    index = next((index for index in names if index + 1 < len(declaration) and declaration[index + 1][0] == '['),
                 names[-1] if names else None)
    if index is None:
        return None, []
    extents, at = [], index + 1
    while at < len(declaration) and declaration[at][0] == '[':
        close = closing(declaration, at, path)
        extents.append(tuple(word for word, _ in declaration[at + 1:close]))
        at = close + 1
    return declaration[index][0], extents


def file_scope(found, path):
    """The definitions at file scope of the C at path, as (tables, functions,
    statics, rest). found is the C's (token, line) pairs.

    A declaration runs to its ; outside brackets, or to the brace that
    closes its body. tables maps the name declarator() reads in each
    definition with an initializer to (extents, rows). extents holds the
    tokens of each extent. rows holds the initializer's elements, as
    elements() splits them, or the whole initializer as one row where it
    doesn't open with a brace. functions maps the name of each function, as
    csource.py's definition() reads one in either form, to its body's
    tokens. Those leave out each statement the check ignores as a
    declaration, and each name of a parameter that parameter() reads.
    statics holds the names of the tables and functions defined static.
    rest holds the tokens outside the initializers and bodies, less each
    declaration the check ignores. Here a definition's part before its
    initializer or body counts as a declaration. A bracket nothing closes
    fails as closing() fails."""
    tables, functions, statics, rest = {}, {}, set(), []
    # opened is the index after the last } at file scope: definition() reads a function's head from there
    at = start = opened = 0
    while at < len(found):
        token, head = found[at][0], start
        if token == '=':
            declaration = found[start:at]
            name, extents = declarator(declaration, path)
            if at + 1 < len(found) and found[at + 1][0] == '{':
                end = opened = closing(found, at + 1, path) + 1
                rows = elements(found[at + 2:end - 1])
            else:
                end = at + 1
                while end < len(found) and found[end][0] not in ',;':
                    end = closing(found, end, path) + 1 if found[end][0] in BRACKETS else end + 1
                rows = [found[at + 1:end]]
            if name is not None:
                tables[name] = extents, rows
        elif token == '{' and (function := definition(lexed(found[opened:at]), start - opened)) is not None:
            # file scope took an old form's head with its first parameter's declaration, so declaration is empty
            head, name = opened + function[0], function[1]
            declaration, end = found[start:at], closing(found, at, path) + 1
            opened = end
            name_at = next(index for index in range(head, at)
                           if found[index][0] == name and found[index + 1][0] == '(')
            close = closing(found, name_at + 1, path)
            parameters = {parameter(each) for each in elements(found[name_at + 2:close])}
            functions[name] = [(word, line) for word, line in without_declarations(found[at + 1:end - 1])
                               if word not in parameters]
        elif token in BRACKETS:
            at = closing(found, at, path) + 1
            opened = at if token == '{' else opened
            continue
        elif token == ';':
            declaration, name, end = found[start:at + 1], None, at + 1
        else:
            at += 1
            continue
        if name is not None and 'static' in (word for word, _ in found[head:at]):
            statics.add(name)
        if declaration and not ignored_declaration(declaration):
            rest.extend(declaration)
        at = start = end
    return tables, functions, statics, rest + found[start:]


def lexed(found):
    """found's (token, line) pairs as the Tokens csource.py's definition()
    reads, each name's kind name."""
    return [Token(word, 'name' if identifier(word) else 'punctuator', None, None, line) for word, line in found]


def elements(found):
    """found's parts between the commas outside its brackets, each its
    (token, line) pairs: an initializer's elements, or a parameter list's
    declarations. A comma after the last part, as C allows in an
    initializer, makes no empty one."""
    result, current, depth = [], [], 0
    for token, line in found:
        depth += token in BRACKETS
        depth -= token in BRACKETS.values()
        if token == ',' and not depth:
            result.append(current)
            current = []
        else:
            current.append((token, line))
    if current:
        result.append(current)
    return result


def parameter(declaration):
    """The name a parameter's declaration gives the parameter, or None.
    declaration is its (token, line) pairs. The name is the token before the
    first [, or else the last token. It must be a name, not a keyword, that
    neither opens the declaration nor follows struct, union or enum. A
    declaration that holds a (, as a pointer to a function's does, gives
    None, since the parameters inside it may take names too."""
    words = [word for word, _ in declaration]
    end = words.index('[') if '[' in words else len(words)
    if '(' in words or end < 2 or not identifier(words[end - 1]) or words[end - 1] in KEYWORDS \
            or words[end - 2] in ('struct', 'union', 'enum'):
        return None
    return words[end - 1]


def mentions(found, names):
    """The (token, line) pairs of found that are one of names, but for a
    member's, which . or -> comes before."""
    return [(word, line) for (before, _), (word, line) in zip([('', 0), *found], found)
            if word in names and before not in ('.', '->')]


def uses_in(source, names, numbers, compiler):
    """Each use of one of names in the C at source, as scan() reads it for
    compiler, as (word, line, place, row, held). word is the name, on line.
    place is the table or function that names it, or None at file scope. row
    is the index of the table's row, or None for a function's use or a row
    with no index. held is the set of the numbers of the stages the source
    gives the use, or None for none. numbers maps each stage's name to its
    number. The module docstring gives a use its stages."""
    tables, functions, statics, rest = file_scope(scan(source, names, compiler), source)
    # the stages of each place that has some, keyed (table, index) for a row and (function, None) for a function
    staged = {}
    # the places that name each static table or function, by its name. A place is keyed as in staged, or is
    # (None, None) for file scope
    sites = {word: [(None, None)] for word, _ in mentions(rest, statics)}
    # the tables whose rows have no index
    unindexed = {name for name, (extents, rows) in tables.items() for row in rows
                 if row and (row[0][0] in ('[', '.') or len(extents) > 1 and row[0][0][0] not in '{"')}
    for name, (extents, rows) in tables.items():
        for index, row in enumerate(rows):
            words = {word for word, _ in row}
            named = {numbers[word] for word in words if word in numbers}
            if extents and extents[0] in BY_STAGE and name not in unindexed:
                staged[name, index] = {index}
            elif len(named) == 1:
                staged[name, index] = named
            for word in {word for word, _ in mentions(row, statics)} - {name}:
                sites.setdefault(word, []).append((name, index))
    for name, pairs in functions.items():
        for word in {word for word, _ in mentions(pairs, statics)} - {name}:
            sites.setdefault(word, []).append((name, None))
    # the statics that name each static, directly or through other statics
    namers = {}
    for name in sites:
        found, todo = set(), [name]
        while todo:
            for owner, _ in sites.get(todo.pop(), ()):
                if owner in statics and owner not in found:
                    found.add(owner)
                    todo.append(owner)
        namers[name] = found
    # each static's group: itself and each static that names it and that it names, directly or through other
    # statics. The group's statics share their stages
    groups = {frozenset({name, *(other for other in found if name in namers.get(other, ()))})
              for name, found in namers.items()}
    # until no group takes more, each takes the stages of the places that name its statics, where each of those
    # outside it has some. A place inside it counts once it has stages
    more = True
    while more:
        more = False
        for group in groups:
            given = [staged.get(place) for name in group for place in sites[name]
                     if place[0] not in group or place in staged]
            if not given or not all(given):
                continue
            held = set().union(*given)
            for name in group:
                for index in range(len(tables[name][1])) if name in tables else (None,):
                    had = staged.setdefault((name, index), set())
                    more |= not held <= had
                    had |= held
    result = [(word, line, name, None if name in unindexed else index, staged.get((name, index)))
              for name, (_, rows) in tables.items() for index, row in enumerate(rows)
              for word, line in mentions(row, names)]
    result += [(word, line, name, None, staged.get((name, None)))
               for name, pairs in functions.items() for word, line in mentions(pairs, names)]
    return result + [(word, line, None, None, None) for word, line in mentions(rest, names)]


def use_problems(added=()):
    """The second check's lines. added is the C in gcc/ that make gcc links
    into 1ST_READ.BIN after its sources. A use the check fails gets a line
    for each of its stages that doesn't read the import's image, or one
    where it has no stage. A file a source includes gets one for each macro
    or use in it that names an import. uses.txt gets one for each row that
    fails."""
    loads = stages()
    # the number of each stage loader_read_stage_files has a case for, by the stage's name
    numbers = {name: number for number, (name, _) in loads.items()}
    taken = {name: (image, where) for name, image, _, _, where in imports()}
    listed = {(source, place, row, image): where for source, place, row, image, _, where in uses()}
    sources = [source for source in Config('1st_read').sources() if not GENERATED.fullmatch(source)]
    # each C source, which both compilers compile, then the C in gcc/ that GCC compiles in place of a .src file or
    # adds
    compiled = [(source, (SHC, GCC)) for source in sources if source.endswith('.c')] + [
        (str(Path('gcc', source).with_suffix('.c')), (GCC,)) for source in sources
        if source.endswith('.src') and Path('gcc', source).with_suffix('.c').is_file()] + [
        (source, (GCC,)) for source in added]
    # the first source that includes each file, a source including itself
    texts, includer = {}, {}
    for source, _ in compiled:
        included(Path(source), texts, INCLUDE)
        for path in texts:
            includer.setdefault(path, source)
    lines, needed = [], set()
    for path in sorted(includer.keys() - {Path(os.path.relpath(Path(source).resolve())) for source, _ in compiled}):
        for name in dict.fromkeys(name for compiler in (SHC, GCC)
                                  for name in macros_naming(macro_directives(code(path, compiler=compiler)), taken)):
            lines.append(f'{path}: {name} is a macro that names an import, in a file {includer[path]} includes, so no '
                         'use of it is held to a stage')
        for word, line, place in dict.fromkeys((word, line, place) for compiler in (SHC, GCC)
                                               for word, line, place, _, _ in uses_in(path, taken, numbers, compiler)):
            lines.append(f'{path}:{line}: {place or "file scope"} names {word}, which {taken[word][1]} takes from '
                         f'{taken[word][0]}, in a file {includer[path]} includes, so no use of it is held to a stage')
    for source, compilers in compiled:
        found = {compiler: uses_in(source, taken, numbers, compiler) for compiler in compilers}
        shipped = found.get(SHC, [])
        # SHC's uses, then each of GCC's that SHC's reading doesn't find, which the line names as GCC's
        for (word, line, place, row, held), gcc in [*((use, '') for use in shipped),
                                                    *((use, "GCC's ") for use in found[GCC] if use not in shipped)]:
            image, where = taken[word]
            read = [loads.get(number, (f'stage {number}', [])) for number in sorted(held or ())]
            missing = [(label, loaded) for label, loaded in read if image not in loaded]
            if held is not None and not missing:
                continue
            covering = [key for key in ((source, place, row, image), (source, place, None, image)) if key in listed]
            if covering:
                needed.update(covering)
                continue
            if held is None:
                at = place if row is None else f'{place}[{row}]'
                lines.append(f'{source}:{line}: {gcc}{at or "file scope"} names {word}, which {where} takes from '
                             f'{image}, for no stage this source gives it, and {USES} gives it no row for {image}')
                continue
            for label, loaded in missing:
                at = f'{place}[{row}], {label}\'s row,' if row is not None else f'{place}\'s code for {label}'
                lines.append(f'{source}:{line}: {gcc}{at} names {word}, which {where} takes from {image}, and '
                             f'loader_read_stage_files reads {", ".join(loaded) or "no image"} for {label}')
    for (source, place, row, image), where in listed.items():
        at = place if row is None else f'{place}[{row}]'
        if row is not None and (source, place, None, image) in listed:
            lines.append(f'{where}: {source}\'s {at} needs no row for {image}: {listed[source, place, None, image]} '
                         f'covers all of {place}')
        elif (source, place, row, image) not in needed:
            lines.append(f'{where}: {source}\'s {at} names no import from {image} that its stages don\'t hold')
    return lines


def main(argv):
    parser = argparse.ArgumentParser(prog='imports', description=__doc__.split('\n\n')[0])
    parser.add_argument('--add', metavar=('IMAGE', 'SOURCE'), nargs=2, action='append', default=[],
                        help="a C source in gcc/ that IMAGE links after its own, which the check reads for GCC alone "
                             "where IMAGE is 1st_read; each --add gives one")
    args = parser.parse_args(argv)
    lines = definition_problems() + use_problems([source for image, source in args.add if image == '1st_read'])
    if lines:
        fail(*lines)


if __name__ == '__main__':
    main(sys.argv[1:])
