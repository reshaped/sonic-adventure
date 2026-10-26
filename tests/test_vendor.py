import contextlib
import hashlib
import io
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import vendor
from support import tree

OLD, NEW = '1' * 40, '2' * 40
DOWNLOAD = f'{"0" * 64}  tools/wibo-i686'


def sha256(text):
    return hashlib.sha256(text.encode()).hexdigest()


def copied(tool):
    # a tool's files at OLD, as vendor.copied names them, each holding its own name
    return {path: Path(path).name for path in vendor.copied(tool)}


# a download, then two vendored tools at OLD, which Check patches in as the tools the Makefile builds
SUMS = DOWNLOAD + '\n' + ''.join(f'# https://example.com/{tool} {OLD}\n'
                                 + ''.join(f'{sha256(text)}  {path}\n' for path, text in copied(tool).items())
                                 for tool in ('codec', 'patcher'))
FILES = {'tools/SHA256SUMS': SUMS, **copied('codec'), **copied('patcher')}


def read(sums):
    with tree({'tools/SHA256SUMS': sums}):
        return vendor.read()


class Record(unittest.TestCase):
    def test_reads_the_downloads_then_each_tool_and_writes_them_back(self):
        with tree(FILES):
            downloads, tools = vendor.read()
            self.assertEqual(downloads, [DOWNLOAD])
            self.assertEqual(tools['codec'], ['https://example.com/codec', OLD,
                                              {'tools/codec/LICENSE': sha256('LICENSE'),
                                               'tools/codec/codec.c': sha256('codec.c'),
                                               'tools/codec/codec.h': sha256('codec.h'),
                                               'tools/codec/main.c': sha256('main.c')}])
            vendor.write(downloads, tools)
            self.assertEqual(Path('tools/SHA256SUMS').read_text(), SUMS)

    def test_rejects_a_header_naming_a_branch(self):
        with self.assertRaisesRegex(SystemExit, r'tools/SHA256SUMS:7: not a hash, two spaces and a path, or # REPOSITORY COMMIT'):
            read(SUMS.replace(f'patcher {OLD}', 'patcher main'))

    def test_rejects_a_download_after_a_tool(self):
        with self.assertRaisesRegex(SystemExit, r'tools/SHA256SUMS:12: tools/shc\.tar\.gz lies outside tools/patcher/'):
            read(f'{SUMS}{"0" * 64}  tools/shc.tar.gz\n')

    def test_rejects_a_second_header_for_a_tool(self):
        with self.assertRaisesRegex(SystemExit, r'tools/SHA256SUMS:12: a second header for codec'):
            read(f'{SUMS}# https://example.com/codec {NEW}\n')

    def test_names_a_line_that_isnt_utf_8(self):
        with self.assertRaisesRegex(SystemExit, r"tools/SHA256SUMS:12: a byte that isn't UTF-8"):
            read(f'{SUMS}{"0" * 64}  tools/patcher/caf\xe9.c\n'.encode('latin-1'))


class Check(unittest.TestCase):
    def setUp(self):
        patch = mock.patch.object(vendor, 'TOOLS', ('codec', 'patcher'))
        patch.start()
        self.addCleanup(patch.stop)

    def check(self, files, *problems):
        # vendor.py --check on files, which reports problems, each a pattern, in their order, or none
        with tree(files):
            if not problems:
                vendor.main(['--check'])
                return
            with self.assertRaisesRegex(SystemExit, r'(?s)' + r'.*'.join(problems)):
                vendor.main(['--check'])

    def test_accepts_files_with_their_recorded_hashes(self):
        self.check(FILES)

    def test_reports_every_changed_and_missing_file_at_once(self):
        files = dict(FILES, **{'tools/codec/codec.c': 'codec.c, edited'})
        del files['tools/patcher/patcher.c']
        self.check(files, rf'tools/codec/codec\.c: not the file tools/SHA256SUMS records; python3 tools/vendor\.py codec {OLD} '
                          r'restores it\n', r'tools/patcher/patcher\.c: not the file tools/SHA256SUMS records')

    def test_fails_naming_a_directory_in_a_file_s_place(self):
        # in one line, after the tool's name, and not as a traceback
        files = {path: text for path, text in FILES.items() if path != 'tools/codec/codec.c'}
        self.check(dict(files, **{'tools/codec/codec.c/a': ''}), r'\A[^\n]*: tools/codec/codec\.c: Is a directory\Z')

    def test_wants_a_header_for_each_tool_the_makefile_builds_and_no_other(self):
        # patcher's lines gone, and its files with them
        sums = SUMS[:SUMS.index('# https://example.com/patcher')]
        files = {path: text for path, text in FILES.items() if not path.startswith('tools/patcher/')}
        self.check(dict(files, **{'tools/SHA256SUMS': sums}), r'tools/SHA256SUMS: no # REPOSITORY COMMIT line for patcher')
        self.check(dict(FILES, **{'tools/SHA256SUMS': f'{SUMS}# https://example.com/other {OLD}\n'}),
                   r'tools/SHA256SUMS: other: not a tool the Makefile builds')

    def test_wants_a_line_for_each_file_the_makefile_reads(self):
        # codec.h's line gone, and the file still there
        sums = ''.join(line for line in SUMS.splitlines(True) if not line.endswith('tools/codec/codec.h\n'))
        self.check(dict(FILES, **{'tools/SHA256SUMS': sums}),
                   rf'tools/codec/codec\.h: no line in tools/SHA256SUMS; python3 tools/vendor\.py codec {OLD} records it\n',
                   r'tools/codec/codec\.h: in tools/codec/, and tools/SHA256SUMS lists no such file')

    def test_wants_no_file_without_a_line(self):
        self.check(dict(FILES, **{'tools/codec/extra.c': 'extra'}),
                   r'tools/codec/extra\.c: in tools/codec/, and tools/SHA256SUMS lists no such file')

    def test_rejects_a_second_line_for_a_path(self):
        # codec.c's line and the download's, at their numbers in SUMS, which --check passes. Before each goes the
        # same line, or its path with a wrong hash, which a reader keeping a path's last line would drop
        for line, number in ((f'{sha256("codec.c")}  tools/codec/codec.c\n', 4), (f'{DOWNLOAD}\n', 1)):
            path = line.split()[1]
            for before in (line, f'{"f" * 64}  {path}\n'):
                with self.subTest(before=before):
                    self.check(dict(FILES, **{'tools/SHA256SUMS': SUMS.replace(line, before + line)}),
                               rf'tools/SHA256SUMS:{number + 1}: a second line for {re.escape(path)}; '
                               rf'line {number} is the first')


def refreshed(commit, copies):
    """What a refresh of codec at commit prints, fetch giving copies as the bytes of codec's four files."""
    with mock.patch('vendor.fetch', return_value=copies) as fetch, contextlib.redirect_stdout(io.StringIO()) as out:
        vendor.main(['codec', commit])
    fetch.assert_called_once_with('https://example.com/codec', commit, ['LICENSE', 'codec.c', 'codec.h', 'main.c'])
    return out.getvalue()


class Refresh(unittest.TestCase):
    def test_writes_and_records_the_named_tools_files_at_the_commit(self):
        later = [f'{name}, later'.encode() for name in ('LICENSE', 'codec.c', 'codec.h', 'main.c')]
        with tree(FILES):
            refreshed(NEW, later)
            self.assertEqual(Path('tools/codec/codec.h').read_text(), 'codec.h, later')
            text = SUMS.replace(f'codec {OLD}', f'codec {NEW}')
            for name in ('LICENSE', 'codec.c', 'codec.h', 'main.c'):
                text = text.replace(f'{sha256(name)}  tools/codec/', f'{sha256(f"{name}, later")}  tools/codec/', 1)
            self.assertEqual(Path('tools/SHA256SUMS').read_text(), text)

    def test_names_each_file_it_changes(self):
        # at NEW, codec.c and main.c differ and LICENSE and codec.h don't, so the refresh leaves those two untouched
        copies = [b'LICENSE', b'codec.c, later', b'codec.h', b'main.c, later']
        with tree(FILES):
            os.utime('tools/codec/codec.h', ns=(1, 1))
            self.assertEqual(refreshed(NEW, copies), 'tools/codec/codec.c\ntools/codec/main.c\ntools/SHA256SUMS\n')
            self.assertEqual(Path('tools/codec/codec.h').stat().st_mtime_ns, 1)
        # and at the commit it records, none, not even tools/SHA256SUMS
        unchanged = [name.encode() for name in ('LICENSE', 'codec.c', 'codec.h', 'main.c')]
        with tree(FILES):
            os.utime('tools/SHA256SUMS', ns=(1, 1))
            self.assertEqual(refreshed(OLD, unchanged), '')
            self.assertEqual(Path('tools/SHA256SUMS').stat().st_mtime_ns, 1)

    def test_records_a_file_the_makefile_reads_that_had_no_line(self):
        sums = ''.join(line for line in SUMS.splitlines(True) if not line.endswith('tools/codec/main.c\n'))
        copies = [name.encode() for name in ('LICENSE', 'codec.c', 'codec.h', 'main.c')]
        with tree(dict(FILES, **{'tools/SHA256SUMS': sums})):
            self.assertEqual(refreshed(OLD, copies), 'tools/SHA256SUMS\n')
            self.assertEqual(Path('tools/SHA256SUMS').read_text(), SUMS)

    def test_restores_a_tool_whose_directory_is_gone(self):
        copies = [name.encode() for name in ('LICENSE', 'codec.c', 'codec.h', 'main.c')]
        with tree({path: text for path, text in FILES.items() if not path.startswith('tools/codec/')}):
            self.assertEqual(refreshed(OLD, copies), ''.join(f'tools/codec/{name}\n' for name in
                                                            ('LICENSE', 'codec.c', 'codec.h', 'main.c')))
            self.assertEqual({path: Path(path).read_text() for path in copied('codec')}, copied('codec'))
            self.assertEqual(Path('tools/SHA256SUMS').read_text(), SUMS)

    def test_takes_only_a_full_commit(self):
        for commit in ('main', OLD[:8]):
            with self.subTest(commit=commit), tree(FILES), mock.patch('vendor.fetch') as fetch, \
                    self.assertRaisesRegex(SystemExit, rf'{commit}: not a commit; give its 40 hex digits'):
                vendor.main(['codec', commit])
            fetch.assert_not_called()

    def test_takes_only_a_recorded_tool(self):
        with tree(FILES), mock.patch('vendor.fetch') as fetch, \
                self.assertRaisesRegex(SystemExit, r'patch: not a vendored tool; tools/SHA256SUMS records codec, patcher'):
            vendor.main(['patch', NEW])
        fetch.assert_not_called()



def commit(repository, files):
    """A commit of files, path to text, in a new repository at repository, and its 40 hex digits."""
    def git(*args):
        return subprocess.run(['git', '-C', repository, *args], check=True, capture_output=True, text=True,
                              env={**{k: v for k, v in os.environ.items() if not k.startswith('GIT_')},
                                   'GIT_AUTHOR_NAME': 't', 'GIT_AUTHOR_EMAIL': 't@t',
                                   'GIT_COMMITTER_NAME': 't', 'GIT_COMMITTER_EMAIL': 't@t'}).stdout
    git('init', '-q')
    for path, text in files.items():
        (Path(repository) / path).write_text(text)
    git('add', '.')
    git('commit', '-qm', 'files')
    return git('rev-parse', 'HEAD').strip()


class Fetch(unittest.TestCase):
    def test_leaves_the_repository_a_hook_names_as_it_was(self):
        # a hook runs with GIT_DIR and GIT_COMMON_DIR naming its own repository, which --git-dir alone doesn't override
        with tempfile.TemporaryDirectory() as upstream, tempfile.TemporaryDirectory() as caller:
            at = commit(upstream, {'gdi.c': 'upstream'})
            commit(caller, {'x': 'caller'})
            meta = Path(caller, '.git')
            before = {path: path.read_bytes() for path in meta.rglob('*') if path.is_file()}
            with mock.patch.dict(os.environ, {'GIT_DIR': str(meta), 'GIT_COMMON_DIR': str(meta),
                                              'GIT_OBJECT_DIRECTORY': str(meta / 'objects')}):
                self.assertEqual(vendor.fetch(upstream, at, ['gdi.c']), [b'upstream'])
            self.assertEqual({path: path.read_bytes() for path in meta.rglob('*') if path.is_file()}, before)


if __name__ == '__main__':
    unittest.main()
