"""Write make pc's per-object SHC flags from config/compilers.mk.

Usage: pcflags.py COMPILERS OUT

config/compilers.mk gives each GCC object, under build/gcc/, the SHC flags
make gcc's float pass reads, beside its SHC objects. OUT gets the same
flags for each object's make pc counterpart, under build/pc/, and for
nothing else:

- each rule whose targets include a build/gcc/ word, with only those
  words, each moved to build/pc/;
- the own macro, redefined for build/pc/ objects alone, and each call of
  it, as config/compilers.mk spells it.

So pc.mk's include adds no flag to an SHC or GCC object. The tool fails on
an own macro it can't redefine, since its calls would then add flags to the
SHC objects a second time.
"""

import re
import sys

OWN = 'own = $(foreach s,$1,$(eval build/$s.obj build/stub/$s.obj build/gcc/$s.o: private GAME += $2))'
PC_OWN = 'own = $(foreach s,$1,$(eval build/pc/$s.o: private GAME += $2))'
# a rule's targets end at the first ': ', so a target may be a substitution
# reference, as $(LOADER_C:%.c=build/gcc/%.o)
RULE = re.compile(r'(?P<targets>[^#\s][^#]*?): (?P<rest>.*)')


def convert(lines):
    """OUT's lines for config/compilers.mk's lines."""
    out = []
    calling = False
    for line in lines:
        if line.startswith('own ='):
            if line != OWN:
                raise SystemExit(f'pcflags.py: own is no longer {OWN!r}; update the tool')
            out.append(PC_OWN)
            continue
        if line.startswith('$(call own,'):
            calling = True
        if calling:
            out.append(line)
            calling = line.endswith('\\')
            continue
        match = RULE.fullmatch(line)
        if not match or 'build/gcc/' not in match['targets']:
            continue
        targets = [t.replace('build/gcc/', 'build/pc/', 1) for t in match['targets'].split() if 'build/gcc/' in t]
        out.append(f'{" ".join(targets)}: {match["rest"]}')
    return out


def main():
    """Read COMPILERS, write OUT."""
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.split('\n\n')[1])
    with open(sys.argv[1], encoding='utf-8') as f:
        lines = f.read().splitlines()
    with open(sys.argv[2], 'w', encoding='utf-8') as f:
        f.write('# written by tools/pcflags.py from config/compilers.mk\n')
        f.writelines(line + '\n' for line in convert(lines))


if __name__ == '__main__':
    main()
