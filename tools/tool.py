"""How a tool fails or stops, runs a child, and reads, writes and removes files.

A tool under tools/ reports each failure it handles through fail(), as
TOOLCHAIN.md#tools gives. stoppable() has a hangup, ^\\ or TERM stop a tool
as ^C does, and run_child() runs a child that such a stop or ^C kills. content()
reads a file whole, save() writes one, and publish() replaces one whole, so
that a reader finds its old contents or its new. remove() removes files.
"""

import contextlib
import os
import signal
import subprocess
import sys
from pathlib import Path

# the stops and ^Cs that land while run_child() starts a child, which it
# raises once it holds the child. None while it starts none, when stop()
# raises a stop as it lands
held = None


def fail(*problems):
    """Exit with status 1, writing each line of each problem to stderr after the tool's name."""
    tool = Path(sys.argv[0]).stem
    raise SystemExit('\n'.join(f'{tool}: {line}' for problem in problems for line in str(problem).splitlines()))


def stoppable():
    """Have a hangup, ^\\ or TERM raise SystemExit with status 128 plus the
    signal's number, as ^C raises KeyboardInterrupt. A signal ignored at the
    start, as under nohup, stays ignored."""
    for number in (signal.SIGHUP, signal.SIGQUIT, signal.SIGTERM):
        if signal.getsignal(number) != signal.SIG_IGN:
            signal.signal(number, stop)


def stop(number, _):
    """stoppable()'s handler, and ^C's while run_child() starts a child: raise
    SystemExit with status 128 plus number, or hold number while run_child()
    starts a child."""
    if held is None:
        sys.exit(128 + number)
    held.append(number)


def raise_held():
    """Stop holding stops, as run_child() has started a child or failed to,
    and raise the first that landed: KeyboardInterrupt for ^C, else
    SystemExit with status 128 plus its number."""
    global held
    landed, held = held, None
    if landed and landed[0] == signal.SIGINT:
        raise KeyboardInterrupt
    if landed:
        sys.exit(128 + landed[0])


def run_child(argv, input=None, **options):
    """As subprocess.run(argv, input=input, **options), where options are
    Popen's, except that ^C, or a stop stoppable() raises, kills the child and
    waits for it wherever it lands. subprocess.run() kills the child only once
    Popen has returned it, so a stop that lands while Popen starts the child
    leaves it running. run_child() holds such a stop in stop() until Popen
    returns, then raises it, and ^C too where ^C raises KeyboardInterrupt, as
    Python sets it. It blocks no signal to hold one, as the child would keep
    the signal blocked through exec."""
    global held
    interrupt = signal.getsignal(signal.SIGINT) is signal.default_int_handler
    held = []
    try:
        if interrupt:
            signal.signal(signal.SIGINT, stop)
        child = subprocess.Popen(argv, stdin=None if input is None else subprocess.PIPE, **options)
    except BaseException:
        if interrupt:
            signal.signal(signal.SIGINT, signal.default_int_handler)
        raise_held()
        raise
    with child:
        try:
            # ^C raises as it lands from here, and a stop held while Popen
            # started the child raises in raise_held()
            if interrupt:
                signal.signal(signal.SIGINT, signal.default_int_handler)
            raise_held()
            output, errors = child.communicate(input)
        except BaseException:
            child.kill()
            raise
    return subprocess.CompletedProcess(argv, child.returncode, output, errors)


def content(path):
    """The bytes of the file at path. One that can't be read fails, naming it."""
    try:
        return Path(path).read_bytes()
    except OSError as error:
        fail(f'{path}: {error.strerror}')


def save(path, data):
    """Write data, bytes, to the file at path. One that can't be written
    fails, naming it. A write that fails partway can leave part of data in
    the file."""
    try:
        Path(path).write_bytes(data)
    except OSError as error:
        fail(f'{path}: {error.strerror}')


def publish(path, data):
    """Replace the file at path with data, bytes or a string it writes as
    UTF-8. It writes data to a temporary file beside path and renames that
    to path, so that a reader finds the old contents or the new. The
    temporary file's name holds the process's ID, and publish() fails where
    a file of that name exists, so it writes through no other file and
    removes no other. Missing directories are made. A file it can't write
    fails, naming it, and is left as it was."""
    path = Path(path)
    if isinstance(data, str):
        data = data.encode()
    temporary = path.with_name(f'{path.name}.{os.getpid()}.tmp')
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        file = open(temporary, 'xb')
    except OSError as error:
        fail(f'{error.filename}: {error.strerror}')
    try:
        with file:
            file.write(data)
        os.replace(temporary, path)
    except OSError as error:
        fail(f'{path}: {error.strerror}')
    finally:
        with contextlib.suppress(OSError):
            temporary.unlink()


def remove(paths):
    """Remove each file of paths that exists. Once it has tried each, it
    fails, naming each it can't remove."""
    problems = []
    for path in paths:
        try:
            path.unlink(missing_ok=True)
        except OSError as error:
            problems.append(f'{path}: {error.strerror}')
    if problems:
        fail(*problems)
