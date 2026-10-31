"""make gcc's manifest, the record of a link's files that tools/publish.py
writes last and tools/disc.py builds a disc from: text() writes one, and
recorded() reads one and checks it.

Each line of a manifest is a comment, which # opens, or a row:

    tool     DIGEST  PATH
    spec     DIGEST  PATH
    image    NAME  ORIGIN  LENGTH  END
    input    NAME  DIGEST  PATH
    bind     NAME  ALIAS  PROVIDER  SYMBOL  VALUE
    alias    NAME  ALIAS  SYMBOL  VALUE
    constant NAME  SYMBOL  VALUE
    file     NAME  DIGEST  PATH

The tool rows give the hashes of the tools TOOLS names, in its order, and
the spec row that of tools/ld.py's spec. An image row gives an image the
spec links, its ORIGIN and LENGTH, and the END its final link gives it. The
rows after it, up to the next image row, are that image's. Its input rows
give the hash of each input, in link order. Its bind rows give each import,
bound to SYMBOL as PROVIDER defines it. Its alias rows give each alias of
SYMBOL, and its constant rows each constant. Each such row gives the VALUE
the image's final link gives its name. Its file rows give the hash of each
of its files: its final link, NAME/final.elf, first, then each file publish
writes for it.

DIGEST is a file's SHA-256 in 64 lowercase hex digits, PATH a path as the
spec spells one, NAME and PROVIDER images, ALIAS and SYMBOL C names, and
ORIGIN, LENGTH, END and VALUE 0x and eight lowercase hex digits. Spaces pad
a row's kind to nine columns, and two part its fields.

recorded() fails, naming its line, on:
- a line that is neither a comment nor a row so spelled;
- a second spec row, or a second image row of one image;
- a row of an image that doesn't follow the image's row, before the next;
- an import, alias or constant its image binds already;
- a path whose rows give it two hashes.

It fails too, naming the manifest, on:
- tool rows that don't name TOOLS, in order;
- no spec row, or an image whose first file row isn't its final link;
- a file whose hash isn't the one the rows give;
- once the spec holds its hash, images that aren't the spec's, each with
  the spec's origin and length, its inputs in link order, its imports and
  aliases, and its constants at the spec's VALUEs.

Which files follow an image's final link is publish's: recorded() holds
each to its hash alone. Each END, and each import's and alias's VALUE, is
publish's too, as it read them from the final link: recorded() holds them
to their spelling alone.

text() fails on a field its form doesn't take, such as a path with a space,
so recorded() reads each row it writes.
"""

import hashlib
import re
from collections import namedtuple
from pathlib import Path

import gcclink
from tables import ADDRESS, hexa
from tool import content, fail

# the comment text() writes first
HEADER = "# make gcc's build, as tools/publish.py records it once the files it writes pass its checks\n"
# the file names of the tools whose hashes a manifest gives, in the order of
# its tool rows: make gcc's toolchain's, and build/prskit
TOOLS = ('sh-elf-gcc', 'cc1', 'sh-elf-as', 'sh-elf-ld', 'prskit')
# each kind of row's form, by the kind, and the spelling of each field a
# form names
FORMS = {'tool': 'tool DIGEST PATH', 'spec': 'spec DIGEST PATH', 'image': 'image NAME ORIGIN LENGTH END',
         'input': 'input NAME DIGEST PATH', 'bind': 'bind NAME ALIAS PROVIDER SYMBOL VALUE',
         'alias': 'alias NAME ALIAS SYMBOL VALUE', 'constant': 'constant NAME SYMBOL VALUE',
         'file': 'file NAME DIGEST PATH'}
FIELDS = {'DIGEST': re.compile(r'[0-9a-f]{64}'), 'PATH': gcclink.PATH, 'NAME': gcclink.IMAGE, 'PROVIDER': gcclink.IMAGE,
          'ALIAS': gcclink.C_NAME, 'SYMBOL': gcclink.C_NAME, 'ORIGIN': ADDRESS, 'LENGTH': ADDRESS, 'END': ADDRESS,
          'VALUE': ADDRESS}

# a manifest, as recorded() gives one and text() takes one: the hash of each
# file it names, by path; its tools' paths, in order; its spec's path; and
# its images, each a Built, by name, in order
Manifest = namedtuple('Manifest', 'hashes tools spec images')
# an image of a manifest: its origin, length and end; its inputs' paths, in
# link order; its imports, each (PROVIDER, SYMBOL, VALUE), by ALIAS; its
# aliases, each (SYMBOL, VALUE), by ALIAS; its constants' VALUEs, by SYMBOL;
# and its files' paths, in order
Built = namedtuple('Built', 'origin length end inputs binds aliases constants files')


def digest(path):
    """The SHA-256 of the file at path, in hex."""
    return hashlib.sha256(content(path)).hexdigest()


def line(fields):
    """The line, its newline included, that text() writes for the row of
    fields."""
    return f'{fields[0]:<8} ' + '  '.join(map(str, fields[1:])) + '\n'


def formed(fields):
    """Whether fields, a row's, take the form FORMS gives their kind, each
    field spelled as FIELDS gives it."""
    form = FORMS[fields[0]].split()[1:] if fields and fields[0] in FORMS else []
    return bool(form) and len(fields) == len(form) + 1 and all(
        FIELDS[name].fullmatch(str(field)) for name, field in zip(form, fields[1:]))


def text(found):
    """The manifest of found, a Manifest, as text, HEADER first. A row with a
    field its form doesn't take fails, naming the row."""
    rows = [*(('tool', found.hashes[path], path) for path in found.tools),
            ('spec', found.hashes[found.spec], found.spec)]
    for name, image in found.images.items():
        rows.append(('image', name, hexa(image.origin), hexa(image.length), hexa(image.end)))
        rows += [('input', name, found.hashes[path], path) for path in image.inputs]
        rows += [('bind', name, alias, provider, symbol, hexa(value))
                 for alias, (provider, symbol, value) in image.binds.items()]
        rows += [('alias', name, alias, symbol, hexa(value)) for alias, (symbol, value) in image.aliases.items()]
        rows += [('constant', name, symbol, hexa(value)) for symbol, value in image.constants.items()]
        rows += [('file', name, found.hashes[path], path) for path in image.files]
    problems = [f"{' '.join(map(str, row))}: not a row of the form {FORMS[row[0]]}" for row in rows if not formed(row)]
    if problems:
        fail(*problems)
    return HEADER + ''.join(map(line, rows))


def recorded(data, where):
    """The Manifest data records, the bytes of the manifest that where
    names, once it passes the checks the module docstring gives. Each path
    a row gives is read from the working directory."""
    try:
        lines = data.decode().splitlines(True)
    except UnicodeDecodeError:
        fail(f"{where}: a byte that isn't UTF-8")
    hashes, tools, spec, images, name = {}, [], None, {}, None
    for number, each in enumerate(lines, 1):
        at, fields = f'{where}:{number}', each.split()
        if each.startswith('#'):
            continue
        if not formed(fields) or each != line(fields):
            fail(f'{at}: not a row: ' + ', '.join(list(FORMS.values())[:-1])
                 + f', or {FORMS["file"]}, spelled as tools/manifest.py writes it')
        kind, image = fields[0], images.get(fields[1])
        if kind == 'spec' and spec is not None:
            fail(f'{at}: a second spec row')
        if kind == 'image' and image:
            fail(f'{at}: {fields[1]} has an image row already')
        if kind not in ('tool', 'spec', 'image') and fields[1] != name:
            fail(f'{at}: {fields[1]}: not the image of the image row above it')
        if kind in ('bind', 'alias', 'constant') and fields[2] in (*image.binds, *image.aliases, *image.constants):
            fail(f'{at}: {name} binds {fields[2]} already')
        if kind in ('tool', 'spec', 'input', 'file'):
            path = fields[-1]
            if hashes.setdefault(path, fields[-2]) != fields[-2]:
                fail(f'{at}: {path}: a row above gives it another hash')
        if kind == 'tool':
            tools.append(path)
        elif kind == 'spec':
            spec = path
        elif kind == 'image':
            name = fields[1]
            images[name] = Built(*(int(field, 16) for field in fields[2:]), [], {}, {}, {}, [])
        elif kind == 'input':
            image.inputs.append(path)
        elif kind == 'bind':
            image.binds[fields[2]] = (fields[3], fields[4], int(fields[5], 16))
        elif kind == 'alias':
            image.aliases[fields[2]] = (fields[3], int(fields[4], 16))
        elif kind == 'constant':
            image.constants[fields[2]] = int(fields[3], 16)
        else:
            image.files.append(path)
    named, problems = tuple(Path(path).name for path in tools), []
    if named != TOOLS:
        problems.append(f"{where}: its tool rows name {', '.join(named) or 'none'}, not {', '.join(TOOLS)}, in "
                        'order')
    if spec is None:
        problems.append(f'{where}: holds no spec row')
    problems += [f'{where}: gives {each} no final link, {each}/final.elf, as its first file row'
                 for each, image in images.items()
                 if not image.files or Path(image.files[0]).parts[-2:] != (each, 'final.elf')]
    if problems:
        fail(*problems)
    problems = [f'{where}: {path}: no file with the hash the manifest gives it' for path, hashed in hashes.items()
                if not Path(path).is_file() or digest(path) != hashed]
    if problems:
        fail(*problems)
    linked = gcclink.read(spec)
    problems = [f'{where}: names no image {each}, which its spec links' for each in linked if each not in images]
    problems += [f"{where}: names image {each}, which its spec doesn't link" for each in images if each not in linked]
    for each, image in linked.items():
        given = images.get(each)
        if given is None:
            continue
        for what, got, wanted in (
                ('another origin', given.origin, image.origin), ('another length', given.length, image.length),
                ('other inputs', given.inputs, image.inputs),
                ('other imports', {alias: bound[:2] for alias, bound in given.binds.items()},
                 {alias: (taken.provider, taken.symbol) for alias, taken in image.imports.items()}),
                ('other aliases', {alias: bound[0] for alias, bound in given.aliases.items()}, image.aliases),
                ('other constants', given.constants, image.constants)):
            if got != wanted:
                problems.append(f'{where}: gives {each} {what} than its spec')
    if problems:
        fail(*problems)
    return Manifest(hashes, tools, spec, images)
