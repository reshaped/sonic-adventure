"""Write a source of the game with its console guards at their matching side.

Usage: pcconsole.py SOURCE OUT

engine/tools/marks.py's CONSOLE names the guards whose non-matching side
rewrites the shipped C in the SH-4's own instructions, and keeps a matching
side, which upstream's native game takes. make xbox-pc-test takes it too: OUT gets
SOURCE with each of those guards' #ifdef NONMATCHING made an #if 0, as
marks.py's console() makes it, and a line marker, so the compiler names
SOURCE in its messages and debug information. marks.py fails where a guard
stands another number of times than CONSOLE gives.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'engine' / 'tools'))
import marks  # noqa: E402


def main():
    """Read SOURCE, write OUT."""
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.split('\n\n')[1])
    source, out = sys.argv[1:]
    if source not in marks.CONSOLE:
        raise SystemExit(f'pcconsole.py: {source}: marks.py names no console guard of it')
    text = Path(source).read_text(encoding='utf-8')
    Path(out).write_text(f'# 1 "{source}"\n' + marks.console(source, text), encoding='utf-8')


if __name__ == '__main__':
    main()
