"""Sort an image's unresolved names under make pc by where each must come from.

Usage: pcunresolved.py IMAGE OUT

make dc's final link of IMAGE, build/dc/link/IMAGE/final.map, names the
image's GCC objects. Each has a counterpart under build/pc/, which make pc
compiles, or which no rule there builds yet. The tool reads each
counterpart's names with llvm-nm, and writes to OUT each name some
counterpart uses and none defines, with its kind and the source of its
definition under make dc:

- import: another image defines it, by IMAGE's imports.ld (stage 2);
- src: a .src file, which xbox/ doesn't yet replace (stage 1);
- inline: an inline body of a C source make pc compiles (stage 1);
- failing: a C source make pc fails to compile, for its SH-4 assembly;
- renderer: a source in gcc/, the PowerVR renderer (stage 3);
- undefined: no GCC object defines it: the port layer, a link alias or the
  C runtime.

config/pc.mk's PC_ADDED objects count where their sources link. It ends
with a count of each kind, which it also prints. make dc's SH-4
libraries count for none: the port runs none of them. make dc must have
linked IMAGE first.
"""

import collections
import re
import subprocess
import sys
from pathlib import Path

OBJECT = re.compile(r'build/dc/[A-Za-z0-9_/.-]+\.o')
IMPORT = re.compile(r'^(_[A-Za-z0-9_]+) = ', re.M)


def names(objects, defined):
    """The global names objects define, or those they use, by llvm-nm."""
    if not objects:
        return {}
    flags = ['--defined-only', '-g'] if defined else ['-u']
    out = subprocess.run(['llvm-nm', '-A', *flags, *map(str, objects)], capture_output=True, text=True,
                         check=False).stdout
    found = {}
    for line in out.splitlines():
        path, _, rest = line.partition(': ')
        fields = rest.split()
        if fields:
            found.setdefault(fields[-1], path)
    return found


def kind(name, gcc_definitions, pc_objects):
    """name's kind and source, as the module docstring gives them."""
    path = gcc_definitions.get(name)
    if path is None:
        return 'undefined', ''
    stem = path[len('build/dc/'):-len('.o')]
    if Path(f'gcc/{stem}.c').exists() and not Path(f'xbox/{stem}.c').exists():
        return 'renderer', f'gcc/{stem}.c'
    if Path(f'{stem}.src').exists() and not Path(f'xbox/{stem}.c').exists():
        return 'src', f'{stem}.src'
    if Path(f'build/pc/{stem}.o') in pc_objects:
        return 'inline', f'{stem}.c'
    return 'failing', f'{stem}.c'


def main():
    """Read the map and the objects, write OUT."""
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.split('\n\n')[1])
    image, out = sys.argv[1:]
    link = Path('build/dc/link') / image
    gcc_objects = sorted({p for p in OBJECT.findall((link / 'final.map').read_text()) if '/sh4zam/' not in p})
    # make dc's names, SH ELF's, take SHC's leading underscore, which make
    # pc's i386 ELF names lack
    sh_name = lambda name: name[1:] if name.startswith('_') else name
    imports = {sh_name(n) for n in IMPORT.findall((link / 'imports.ld').read_text())}
    pc_objects = {Path(p.replace('build/dc/', 'build/pc/', 1)) for p in gcc_objects}
    # config/pc.mk's PC_ADDED, SOURCE:ADDED, links ADDED where SOURCE links
    added = re.search(r'^PC_ADDED := (.*)$', Path('config/pc.mk').read_text(), re.M)
    for entry in (added[1].split() if added else []):
        source, _, extra = entry.partition(':')
        if Path('build/pc') / Path(source).with_suffix('.o') in pc_objects:
            pc_objects.add(Path('build/pc') / Path(extra).relative_to('xbox').with_suffix('.o'))
    present = {p for p in pc_objects if p.exists()}
    defined = names(sorted(present), True)
    used = names(sorted(present), False)
    gcc_definitions = {sh_name(n): path for n, path in names(gcc_objects, True).items()}
    rows = []
    for name in sorted(set(used) - set(defined)):
        rows.append(('import', name, '') if name in imports else (*kind(name, gcc_definitions, present)[:1], name,
                                                                    kind(name, gcc_definitions, present)[1]))
    counts = collections.Counter(row[0] for row in rows)
    summary = ', '.join(f'{counts[k]} {k}' for k in ('import', 'src', 'inline', 'failing', 'renderer', 'undefined'))
    with open(out, 'w', encoding='utf-8') as f:
        f.write(f"# {image}'s unresolved names under make pc, by tools/pcunresolved.py: KIND NAME SOURCE\n")
        f.writelines(' '.join(row).rstrip() + '\n' for row in sorted(rows))
        f.write(f'# {len(rows)}: {summary}\n')
    print(f'{image}: {len(rows)} unresolved: {summary}')


if __name__ == '__main__':
    main()
