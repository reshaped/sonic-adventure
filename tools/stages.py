"""The images loader_read_stage_files reads for each stage, and the uses of
1ST_READ.BIN's imports that those stages don't hold.

stages() reads the loader's source (TOOLCHAIN.md#stage-loading), and uses()
config/1st_read/uses.txt (GCC.md#imports-and-the-census). The import check,
tools/imports.py, reads both, and tools/content.py reads stages() to name
what 1ST_READ.BIN's path_lists point at.
"""

from pathlib import Path

from csource import macro_directives, parse
from images import CONFIG, FIRST, images
from tables import PLACE, rows
from tool import content, fail

# the uses of 1ST_READ.BIN's imports that the uses' stages don't hold, each
# with the image it means and the evidence for it
USES = FIRST / 'uses.txt'
# the source whose loader_read_stage_files reads each stage's images, and the
# header that numbers the stages
STAGE_LOADER = Path('src/system/loader.c')
STAGE_HEADER = Path('src/stage/stage.h')
# the head of the definition stages() reads, as its tokens
STAGE_READER = ['void', 'loader_read_stage_files', '(', 'void', ')']


def uses():
    """config/1st_read/uses.txt's rows, as a list of (source, place, row,
    image, reason, where) in table order. source is a C source of
    1ST_READ.BIN's, and place names a table or a function in it. row is the
    index of one of the table's rows, or None for all of place. reason gives
    the evidence for the row, and where gives its path:line. A tree without
    the table has none.

    A use is where source names a definition config/1st_read/imports.txt
    takes from image. tools/imports.py gives each use the stages of its
    place. The stages hold the use where it has at least one stage and
    stages() gives image for each. A row covers each use at place, or at
    the table's row it gives, that the use's stages don't hold.
    tools/imports.py fails a row that covers no use.

    A row is SOURCE PLACE IMAGE REASON, where PLACE is a name or NAME[N].

    A row fails where it:
    - has fewer than four fields;
    - gives a place in neither form;
    - names no image, a directory in config/;
    - gives the source, place and image of an earlier row.
    """
    if not USES.exists():
        return []
    known, found, seen = set(images()), [], {}
    for where, row in rows(USES):
        if len(row) < 4:
            fail(f'{where}: not a row: SOURCE PLACE IMAGE REASON')
        source, place, image, *reason = row
        match = PLACE.fullmatch(place)
        if match is None:
            fail(f'{where}: {place}: not a place, a name or NAME[N]')
        if image not in known:
            fail(f'{where}: {image}: no such image, a directory in {CONFIG}/')
        if (source, place, image) in seen:
            fail(f'{where}: {source} {place} {image} has a row already, at {seen[source, place, image]}')
        seen[source, place, image] = where
        name, index = match.groups()
        found.append((source, name, None if index is None else int(index), image, ' '.join(reason), where))
    return found


def stages():
    """The images loader_read_stage_files reads for each stage on every
    pass, as {number: (name, images)}. A stage's name is its case's label,
    which src/stage/stage.h numbers. Its images are those its case reads
    with loader_read_prs, each once, in the order the case reads them. A
    file is an image where config/ has a directory of its name, less its
    suffix and in lowercase. A stage no case names has no entry. A tree
    without the loader's source has no stages.

    A label is case, a name that opens with STAGE_, and a :. A read is
    loader_read_prs with a string first. A jump is a return, break or goto.
    Labels with no read, jump or default between them share their reads. A
    case ends at a jump that no read follows before the next label. A read
    between default and the next label is no stage's. stages() reads the
    loader's tokens, so a comment or a string holds no label, read or jump.
    A stage's number is a #define of its name to one decimal number.

    stages() takes a read to run on every pass where it lies at its label's
    depth of braces, right after a label, a ; or a }. No jump of the case
    may come before it. STAGE_SUNABOARD's case reads the board motions
    inside an if, for Sonic and Tails alone, so they are not its images.

    Fails on these, naming the line of each but the first:
    - a source with no definition of loader_read_stage_files(void);
    - a label the header doesn't number;
    - a case that reads a file and doesn't end before the next label or
      default;
    - a directive in the body, such as a guard's #ifdef GCC, since stages()
      would read both of a guard's sides as one.
    """
    if not STAGE_LOADER.exists():
        return {}
    known = set(images())
    numbers = {name: int(macro[1][0]) for _, name, macro in macro_directives(content(STAGE_HEADER).decode())
               if name.startswith('STAGE_') and macro is not None and macro[0] is None and len(macro[1]) == 1
               and macro[1][0].isdigit()}
    loader = parse(content(STAGE_LOADER).decode())
    body = next((block for block in loader.blocks if block.closing is not None
                 and [token.text for token in block.head if token.kind != 'comment'] == STAGE_READER), None)
    if body is None:
        fail(f'{STAGE_LOADER}: no definition of loader_read_stage_files(void)')
    for directive in loader.directives:
        if body.opening.start < directive.start < body.closing.start:
            spelled = ' '.join(['#' + directive.name, *(word.text for word in directive.words)])
            fail(f"{STAGE_LOADER}:{directive.line}: loader_read_stage_files holds {spelled}, whose sides stages() "
                 "doesn't tell apart")
    run = [token for token in loader.parsed if body.opening.start < token.start < body.closing.start]
    found, labels = {}, []
    # the case's last read or jump, as 'read' or 'jump', or None before either
    last_step = None
    # whether the case has jumped yet: a read after a jump may not run
    jumped = False
    # the depth of braces at each step, and at the case's last label
    depth, case_depth = 0, 0
    for at, token in enumerate(run):
        after = [each.text for each in run[at + 1:at + 3]]
        label = run[at + 1].text if token.text == 'case' and len(after) == 2 and after[0].startswith('STAGE_') and \
            after[1] == ':' else None
        file = run[at + 2].text[1:-1] if token.text == 'loader_read_prs' and after[:1] == ['('] and \
            run[at + 2].kind == 'string' else None
        where = f'{STAGE_LOADER}:{token.line}'
        if token.text in ('{', '}'):
            depth += 1 if token.text == '{' else -1
        elif label is not None or token.text == 'default':
            case_depth = depth
            if last_step == 'read':
                fail(f'{where}: {f"case {label}" if label else "default"}: the case before it reads a file and falls '
                     'into it')
            if label is not None and label not in numbers:
                fail(f'{where}: case {label}, which {STAGE_HEADER} gives no number')
            if last_step == 'jump' or label is None:
                labels, last_step, jumped = [], None, False
            if label is not None:
                labels.append(label)
                found[numbers[label]] = (label, [])
        elif file is not None:
            last_step = 'read'
            image = Path(file).stem.lower()
            if image not in known:
                continue
            if jumped or depth != case_depth or at == 0 or run[at - 1].text not in (':', ';', '}'):
                continue
            for each in labels:
                if image not in found[numbers[each]][1]:
                    found[numbers[each]][1].append(image)
        elif token.text in ('return', 'break', 'goto'):
            last_step, jumped = 'jump', True
    return found
