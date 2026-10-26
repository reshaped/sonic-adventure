"""Build temporary input trees for tools that read the working directory."""

import contextlib
import os
import tempfile
from pathlib import Path


@contextlib.contextmanager
def cwd(path):
    """path as the working directory, the old one back after."""
    old = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(old)


@contextlib.contextmanager
def tree(files):
    """A temporary working directory holding files, a {path: text or bytes} map."""
    with tempfile.TemporaryDirectory() as root:
        for name, content in files.items():
            path = Path(root, name)
            path.parent.mkdir(parents=True, exist_ok=True)
            if isinstance(content, bytes):
                path.write_bytes(content)
            else:
                path.write_text(content)
        with cwd(root):
            yield Path(root)


# a module's tables and sources, and 1ST_READ.BIN's, whose symbols.txt every module reads
MODULE = {
    'config/1st_read/splits.txt': 'src/main.c  P  0x8c0101a0  0x8c0101b0\n',
    'config/1st_read/symbols.txt': '0x8c0101a0  main\n0x0c900040  shadowed\n0x8c0101a4  shared\n',
    'config/mod/splits.txt': ('P  0x0c900000  0x0c900100\n'
                              'D  0x0c900100  0x0c900200\n'
                              'B  0x0c900200  0x0c900300\n'
                              'modules/mod/a.c  P  0x0c900000  0x0c900010\n'
                              'modules/mod/a.c  D  0x0c900100  0x0c900110\n'
                              'modules/mod/b.src  P  0x0c900020  0x0c900030\n'
                              'fill  0xff\n'),
    'config/mod/symbols.txt': '0x0c900000  a_init\n0x0c900000  a_start\n0x0c900104  shared\n',
    'config/kept.txt': '',
    'src/main.c': '',
    'modules/mod/a.c': '',
    'modules/mod/b.src': '',
}

# LOADER.BIN's section rows: each section strt1's tables bound that holds bytes needs a row, but DSG32 and BSG32
# can lie in DSG's and BSG's, as they do here
LOADER_SECTIONS = ('DSGLH  0x8ce00000  0x8ce00080', 'DSGLE  0x8ce00080  0x8ce000a0', 'P  0x8ce000a0  0x8ce064f4',
                   'C  0x8ce064f4  0x8ce06cc0', 'D  0x8ce06cc0  0x8ce06d00', 'PSG  0x8ce06d00  0x8ce1f72c',
                   'CSG  0x8ce1f72c  0x8ce20340', 'DSG  0x8ce20340  0x8ce20a00', 'B  0x8ce20a00  0x8ce50040',
                   'BSG  0x8ce50040  0x8ce7e920')
