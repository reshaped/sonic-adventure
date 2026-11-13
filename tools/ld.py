"""Link a set of images with GNU ld, each to an ELF of its own, and bind
each name one image takes from another to its definition there.

ld.py reads SPEC, the spec tools/spec.py writes, in the rows
tools/gcclink.py gives, and writes each image's passes under DIR/IMAGE/.
SH_ELF in the environment names the toolchain's directory, tools/sh-elf by
default, and ld.py runs its bin/sh-elf-ld.

The script lays GCC's sections and SHC's out in four output sections. An
SHC section's kind is the first letter of its name.

- .text takes .text, .text.*, .text_last.* and SHC's P sections, its code.
- .rodata takes .rodata, .rodata.* and SHC's C sections.
- .data takes .data, .data.*, .data_last.*, SHC's D sections and its R
  sections. An R section is a RAM copy of data that SHC's startup makes.
- .bss takes .bss, .bss.*, COMMON and SHC's B sections.

The script lays each output section out in runs. A run is a stretch of an
output section that can take bounds: __startof_X at its start, __endof_X
at its end and __sizeof_X, its size, where X is the run's name.

An output section's first run is named for its first kind: P, C, D or B.
That run takes GCC's sections, the SHC sections of its name, and each other
SHC section of the output's kinds that no input bounds. An input bounds a
section X where it takes __startof_X, __endof_X or __sizeof_X from outside.
Each other name an input bounds, where an input holds an SHC section of the
name, takes a run of its own, after the first, in name order. That run
holds those sections. A first run has bounds whether or not an input holds
a section of its name. The link fails on a bound no run or input defines.

ld lays out the sections of each run of .text, .rodata, .data and .bss
file by file, in link order. So an object's code stays together, but for
what kernel rows take, and a branch from its inline assembly reaches its
C. .bss's first run then takes COMMON, after every file's sections.
Sections GCC marks mergeable, as .rodata.str1.4 and .rodata.cst16, are the
exception. ld gathers each such name's sections where the first lies, and
keeps one copy of a string or constant that repeats.

The last code, the sections whose names start .text_last., is no run's.
Each holds one function, as port.h's LINKED_LAST names it. .text closes
with them, after its last run: the last code of each last row's PATH, in
row order, then the rest file by file in link order. So code a change adds
there moves no code linked before it, and no run's bounds, and a last row
keeps its object's last code ahead of what a change adds later. Where the
image has windows, .rodata starts on the operand cache's 16 KB, so the data
stays too while .text ends short of that start.

The last data is the sections whose names start .data_last., each one
datum's, as port.h's LINKED_LAST_DATA names it. It is no run's either, and
closes .data, file by file in link order, so data a change adds there
moves no data linked before it. Where the image has windows, .bss starts
on the operand cache's 16 KB, so .bss stays too while .data ends short of
that start.

A starts row starts each OUTPUT it names, of .rodata, .data and .bss, on
the operand cache's 16 KB, whether or not the image has windows. An
address's set of that cache is its bits 5 to 13, so whatever the script
lays ahead of such a start weighs, each datum after it keeps its set: the
start moves by 16 KB or not at all. An OUTPUT that no input the link takes
gives a byte, as gcclink.py's selected() takes them, has no datum to hold,
and starts where it lies: a start would end the image up to 16 KB past its
last byte. ld.py reckons which those are before ld runs, and holds each
pass to the reckoning: a pass fails where ld gives a byte to an OUTPUT the
script didn't start, or none to one it started. Where data rows open an
OUTPUT the row names, and it holds a byte past them, the script starts on
16 KB again after the last of them, so a pad ahead of a data row's section
moves that row's data and the rows' after it, and no other.

Where the row names .rodata, the sections ld merges, those whose flags
have SHF_MERGE, are no run's. They close .rodata, after its last run: the
constants first, and then the strings, whose flags have SHF_STRINGS too.
So a string a change adds moves no other read-only datum, and a constant
moves the strings alone.

A cold row's PATH is an object whose code runs too seldom for the sets it
takes to matter, and it takes no run either. Its code closes .text, after
the last code. Its read-only data, its data and its .bss, in that order,
close .data, after the last data. The cold rows' objects lie in row order.
So a change to a cold object moves no other code but a later cold row's,
and none of the last code. Where the image has windows, it moves no datum
but the cold rows' objects' either, and none of the last data, while .text
and .data end short of .rodata's and .bss's starts. Of the cold rows'
objects' data, it moves no other but a later row's where it leaves the
object's strings and constants as they were. Those ld gathers as it does
in any output section: the sections it merges with one another, of every
cold row's object, lie where the first of them does, in the first row's
object that holds one. So a change to a cold object's strings or
constants moves the data after that section: the rest of that object's,
though its row comes earlier, and each later row's object's.
ld merges strings and constants only within an output section, so a cold
object's stay out of .rodata, where a change to them would move the data
after them. Its .bss lies in the image as zeros, as the last data's does.
A common symbol lies in no section of its object, so no cold row places
one, and .bss would take it with every other object's: the link fails on
a cold row whose object holds a common symbol.

A head row puts INPUT, an input section of PATH, in .head, which opens the
image. A tail row puts it in .tail, which closes the image. PATH is one of
the image's objects, spelled as its input row spells it. .head and .tail
take their rows' sections in row order. So a record a loader reads at a
fixed offset, or one that ends the image, lies where the rows put it. The
rows of one SECTION make one run, and must stand together, all in the head
or all in the tail.

A kernel row puts INPUT, an input section of PATH, at the start of .text's
first run, ahead of the image's other code. The kernels' sections lie there
in row order, and the rows of one KERNEL, which must stand together, make
that kernel. Each section must be code: .text, .text.*, .text_last.* or an
SHC code section. An object whose code is SHC's sections and GCC's both, as
inline assembly's is, gives the kernels and the warm rows all of its code
or none of it. A
kernel runs from its first section's start to its last's end, or over its
window where the image has window rows. Each kernel must touch at most 256
lines of 32 bytes, the 8 KB of the SH-4's instruction cache, so that no two
of its lines take one set of the cache. A line of one kernel and another
line of a second may take one set only where an over row names the two
kernels. Where one image imports from another, a line of a kernel of either
may take the set of a line of a kernel of the other only where an over row
of either names its own kernel, then the other's, and then the other image
as OTHER. The kernels change the order of no other section: their objects'
other sections, and every other file's, keep the order they take without
them.

Where an image has window rows, each of its kernels has one. A window is
LINES lines of 32 bytes. .text starts on a line, and the windows follow
each other from its start in the kernels' row order. A kernel's code starts
OFFSET lines into its window. The script moves to that line before the
kernel's sections, and to the window's end after them, so code that grows
within its window moves nothing outside it. ld.py lays each kernel's
sections out as ld does, each on its alignment, and fails before ld runs
where a kernel's code passes its window's end. The script starts .rodata
and .bss on the 16 KB of the SH-4's operand cache, so that no code that
grows moves their data to another set of it.

A warm row puts INPUT, an input section of PATH that is code, after the
kernels, after the last window where the image has windows, and ahead of
the image's other code. The warm rows' sections lie there in row order.
So code that runs often between the kernels' calls takes the sets of both
caches that its place gives it, and no change to the code linked after it
moves it. ld.py checks none of their sets: they lie over the kernels where
the windows end.

The image's record, config/IMAGE/order.txt, which tools/order.py writes,
gives each warm row's section its place. A heat row gives the PATH of the
heat, a file of how often a frame enters each stretch of code, and its
SHA-256 when the record was written. A text row gives the ADDRESS .text
started at then, and a searched row what the record holds of a kernel
row's section or a warm row's: its PATH and INPUT, its PLACE, in bytes
into .text, and its SIZE. order.py costs the places against the
kernels and the code the heat gives an address of its own, so these rows
are all it read.

The script moves to a warm row's PLACE before its section, so the record
is the layout: a warm row's section lies at its PLACE whatever the size
of the row before it, and a row that grows within the bytes up to the
next row's PLACE moves nothing. Before ld runs, ld.py fails where the
record gives a warm row's section no place, or a PLACE off the section's
alignment or under the end of the windows, or of the kernels where the
image has no window. It fails where a warm row's PLACE lies under that
of the row before it, and where a row's section runs past the next row's
PLACE, and names the row that outgrew its place. Without a record the
warm rows' sections follow each other in row order, each on its
alignment, as order.py links them to read their sizes.

A window row whose KERNEL is warm gives the warm rows a window of their
own, LINES lines from where they start. The script moves to its end after
the last warm row's section, where the image has a record, so the image's
other code starts there whatever the warm rows take: a row that grows, or
that takes another place under the window's end, moves no code after the
window, and leaves the image as long as it was. ld.py fails before ld
runs where the last warm row's section runs past the window's end. A
wider window moves the code after it, and one wider by a multiple of the
cache's 256 lines leaves each of that code's lines on its set. Without
the row the image's other code follows the last warm row's section, and
moves when that row grows or another takes a place past it.

ld.py then holds each layout pass to the record. An image with warm rows
must have one, and a record a heat row and a text row. The heat at PATH
must have the heat row's hash. .text must start at ADDRESS, so a head
that grows by a line, or an origin that moves, and so moves each kernel
and warm line to another set than the code outside the image meets it on,
fails the link. Each kernel row's section must lie PLACE bytes into .text and
take SIZE bytes, since the kernels' lines are what the warm rows' places
were costed against. Each warm row's section must lie at its PLACE. Its
SIZE is what it took when the record was written, and holds the link to
nothing. The record must hold no other section. Each failure names the
record's file and make dc-order, which writes the record again and moves
no warm row that fits where it lies.

A warm row that takes another place leaves the bytes of its own free, so
the rows' moves lengthen the image. Where ld fails an image that runs
past its load area, and the record leaves bytes free among the warm rows,
ld.py says how many, and names make dc-order PACK=1, which lays the rows
with none free.

A data row puts INPUT, an input section of PATH, at the start of the first
run of the output section that takes it, in row order. INPUT must be no
code, and must lie in that first run.

A kernel's pool is the lines of its sections that hold a word of an
R_SH_DIR32 relocation, or the target of a MOV.W @(disp,PC), MOV.L
@(disp,PC) or MOVA @(disp,PC),R0 there, as a decode of its code from its
start finds them. A word of the pool that holds an address in a data row's
section or in the last data names the lines of the 64 bytes from that
address, within the object there. The object runs from the name nearest at
or below the address, over its symbol's size or else up to the next name.
Where the object is over 256 bytes, the word names the address's line
alone. A line of a kernel's pool may not take the operand cache's set of a
line that a word of the kernel's own pool names, or a word of the pool of
another kernel that no over row names with it, since a load from the pool
and a load from the address would evict each other's line. These lines are
all the check holds apart: a datum a kernel reaches through a pointer it
loads at run time, or through code it calls, can still take its sets.

Where a pool line takes a named line's set, ld.py links the pass again with
pads. Each data row's section moves on by the fewest whole lines that clear
it, once the pads of the rows ahead of it in its output section have moved
it. The last data then moves on the same way, once the pads of .data's
rows have moved it, which they don't where a starts row names .data. A pad
moves every datum the script places after it, up to the next start on
16 KB: an output section's, or the one after the rows of an output section
a starts row names. The pads are reckoned
from the pass without them, so a move that alignment swallows or rounds
up, or a pad's move of a later output section's data, isn't in them. The
padded pass's check then fails on the pair left, though other pads might
part it.

Every run has bounds but a head or tail run whose name another run takes.
SHC's assembly writes them as STARTOF X, (STARTOF X)+(SIZEOF X) and SIZEOF
X (PORTS.md#coordinated-link). __startof_X and __endof_X lie in the
run's output section. The script assigns __sizeof_X outside every output
section, so it is an absolute value. image_start lies at the image's start,
and image_end at its end. The runs' bounds, image_start and image_end are
the boundaries of the link, which the script defines. No import takes one.

A run with bounds starts on the largest alignment of its sections, so no
padding lies between __startof_X and its first section but a data row's
pad. ld starts each output section on the largest alignment of its
sections, so the first run of .text, .rodata, .data and .bss starts on its
own. The script aligns each other run that has bounds.

The script places .comment, the compiler's name, at address 0, where it
loads nowhere. It places each DWARF section DEBUG lists there too, under
its own name. ld fails on any other DWARF section, as one the script
doesn't place.

ld.py fails on SPEC as gcclink.py's read() does.

Given --lto=FLAGS, ld.py first compiles each image's intermediate code
whole, as whole() says, where make dc's objects carry it, and links each
object's link-time object in its place. Its head and tail rows then place
the link-time objects' sections, as gcclink.py's link_time() gives them,
and its checks hold the link it makes. The object of a kernel, warm, data,
last or cold row keeps its own code, which the row places, and the link
fails on one that carries the intermediate code, before any image's
compile runs. The compile keeps each definition that an input it doesn't
compile takes, so a change to such an input that takes another name of
the compiled objects', or one fewer, can resize their code, and then
moves what lies after it, the last code and the cold rows' with it. The
compile roots each datum its objects define by a global name, and once
the layout passes have run, ld.py fails on one that an image doesn't
define. FLAGS is one word of the command line, its flags split by spaces:
written --lto=FLAGS, a FLAGS of one flag, which starts with a dash, is no
option of ld.py's own.

ld.py links each image twice with ld -q --no-relax, from the same inputs,
under DIR/IMAGE/. -q keeps the relocations in the ELF, for the pools it
checks and tools/verify.py's binding check. Every image's layout pass runs,
then the check of kernels across images and of each image's searched rows,
then resolve, then every image's final pass.

- The layout pass writes each import's provisional value, its provider's
  origin in its view, to provisional.ld. It links by layout.ld, which
  includes provisional.ld, into layout.map and layout.elf. The script gives
  the load area as a MEMORY region. It places .head, .text, .rodata, .data,
  .bss and .tail there, in that order.
- resolve binds each import to its provider's export of SYMBOL, in the
  provider's layout.elf and in the import's view. tools/gcclink.py says
  what an export is. The absolute values the script assigns lie in no
  section, so none is an export.
- The final pass writes resolve's values to imports.ld. It links by
  final.ld into final.map and final.elf. final.ld includes imports.ld where
  layout.ld includes provisional.ld, and lays the image out alike.

ld collects no section in either pass, so each section of an input the
link takes stays. The script roots, by EXTERN, each name another image
imports from the image, and each SYMBOL of its aliases. A root pulls in the
archive member that defines it.

A pass fails where ld fails or prints anything. ld fails on a warning, on a
name the image takes that nothing binds, and on a section the script
doesn't place. It fails on an image that starts past ORIGIN or runs past
its load area, and on a dynamic relocation. It fails on an input that
defines a name the image binds or a boundary of the link, and on an alias
whose SYMBOL nothing defines.

Where an image with a starts row runs past its load area, ld.py links the
pass once more without the row, to packed.ld's script, in a load area that
runs to the end of RAM, and reads where the image then ends. Where it then
fits its own load area, the failure says how many bytes its starts take
and how many its load area has for them, and names what gives the image
room: each starts row that names fewer of the output sections and fits, as
a link of each shows, and a packed row, in its kernel table,
config/IMAGE/kernel.txt. Where it doesn't fit, the failure says by how
many bytes, so that no one packs it for nothing.

An input section whose own alignment is the operand cache's 16 KB starts
there under any row, as the game's heap does, and no row packs it. Where
an image that holds one runs past its load area with none of its starts,
the failure names each such section and the bytes ld skips to reach it,
as the map of that link gives them.

ld.py fails a pass on a weak reference that nothing defines, for which it
has no policy, on a kernel that touches more than 256 lines, on two kernels
whose lines take one set though no over row names them, and on a line of a
kernel's pool that takes the operand cache's set of a line in a data row's
section that a word of the pool of a kernel it runs with names, where the
pads leave it. It fails on a head, tail, kernel, warm or data row whose PATH
is none of the image's objects, or whose INPUT is no allocated section of
PATH. It fails on two rows that place one input section, on a SECTION or
KERNEL whose rows stand apart, on a kernel or warm row whose INPUT is no
code, on a data row whose INPUT is code or lies past its output section's
first run, on an over row that names one kernel twice or one no kernel row
names, and on kernel and warm rows that take part of an object's code that
is SHC's sections and GCC's both. Where the image has window rows, it fails on a window row that
names no kernel and isn't the warm rows', on a kernel that no window row
or two name, on a kernel whose code passes its window's end, on a second
window of the warm rows, and on one that gives an OFFSET. Once every layout pass has run,
ld.py fails on a kernel of one image and a kernel of another whose lines
take one set, where either image imports from the other and no over row
names the two. It fails too on an over row whose OTHER has no kernel of its
second KERNEL's name. It fails on an image with warm rows and no searched
row, on searched rows without a heat row, on a heat it can't read or whose
SHA-256 isn't its heat row's, on searched rows without a text row, on a
.text that starts elsewhere than the text row has it, on the first kernel
row's section that the searched rows give another place or size, or the
first warm row's that they give another place, on the first of either
that they don't give, on the first searched row of a section no kernel
or warm row places, and on a last warm row that runs past the end of the
warm rows' window. Each of these names make dc-order, which
writes the rows again.
resolve fails on an import whose provider's layout.elf doesn't export
SYMBOL, and on an alias whose SYMBOL the image's own layout.elf doesn't
export.

The layout check then runs, and ld.py fails on every problem it finds,
each naming its image and the section, name or input section at fault. It
holds each image's signature in its final pass to its signature in its
layout pass. The signature holds each allocated section's place, size and
flags, and whether it is zeroed. It holds the place, size and section of
each name defined in an allocated section, local or not, and of each
boundary of the link, wherever it lies. It holds the place and size of
each input section the map places in an allocated section, so it holds
each archive member the link takes. So each import's value, which resolve
read from its provider's layout pass, holds in the provider's final pass.
tools/verify.py then checks the final passes.

A final.elf ld.py leaves has passed ld.py's checks alone. tools/verify.py
checks the rest, and make has tools/publish.py write the manifest, which
marks a checked link, only once verify passes. So that the final passes in
DIR are one run's, ld.py removes the final.elf of each directory in DIR as
it starts, before it reads SPEC. It removes them again where the link
fails, or where ^C, ^\\, a hangup or TERM stops it. A run that stops part
way then leaves no final.elf of an earlier run beside its own. KILL, which
no process can catch, can leave a run's own final.elf files behind.

ld.py fails, naming each final.elf it can't remove, once it has tried the
rest. It fails too on a symlink in DIR to a directory, through which it
removes nothing, and on a DIR it can't list, which it names: it can't tell
which final.elf DIR holds. A DIR that isn't there holds none. Where the
run has failed or been stopped already, ld.py names them, and then exits
as that failure or stop would.
"""

import argparse
import errno
import fnmatch
import hashlib
import os
import re
import shutil
import stat
import subprocess
import sys
from bisect import bisect_left, bisect_right
from collections import namedtuple
from itertools import combinations, product
from pathlib import Path

import elf
import gcclink
from images import CONFIG
from tables import hexa
from tool import content, fail, remove, run_child, save, stoppable

# the SH-4's instruction cache: its lines, and the bytes of each. It holds
# one line a set, and takes an address's set from its bits 5 to 12, so 256
# lines in a row take each set once
ICACHE_LINES, ICACHE_LINE = 256, 32
# the SH-4's operand cache: its lines, and the bytes of each. It holds one
# line a set, and takes an address's set from its bits 5 to 13
OCACHE_LINES, OCACHE_LINE = 512, 32
# a word of a kernel's pool that holds an address in a data row's section
# or the last data names the lines of the POOL_REACH bytes from that
# address, within the object there, or the address's line alone where the
# object is over POOL_OBJECT bytes, as pools() gives them
POOL_REACH, POOL_OBJECT = 64, 256
# the top four bits of a MOV.W @(disp,PC),Rn and a MOV.L @(disp,PC),Rn, and
# the top eight of a MOVA @(disp,PC),R0
MOV_W, MOV_L, MOVA = 0x9, 0xD, 0xC7
# what ends each problem of an image's record that the goal settles by
# writing the record again, with each warm row where it lies, and what ends
# one of a warm row that it settles by giving the row another place
RECORD, PLACE = 'make dc-order records the rows again', 'make dc-order gives it another place'
# the KERNEL of the window row that gives the warm rows their window, which
# is no kernel's name: a table's row that starts with it is a warm row
WARM = 'warm'
# what the script's assertion says of an image too long for its load area
OVERRUN = 'it runs past its load area'
# what ld prints of an image that runs past its load area, the script's
# MEMORY region: by how many bytes
PAST = re.compile(r"region `area' overflowed by (\d+) bytes")
# a line of ld's map that gives the bytes it skips to the alignment of the
# input section the next line places: their address and how many
FILL = re.compile(r' \*fill\* +0x([0-9a-f]+) +0x([0-9a-f]+) *\S*')
# the routines GCC's code may call where the C doesn't, as config/gcc.mk's
# GCC_TARGET says. The link-time compile roots each, since GCC writes those
# calls only once it has dropped what nothing called
CALLED_BY_GCC = ('memcpy', 'memmove', 'memset', 'memcmp')
# a warning in what the link-time compile prints, as GCC, its wrapper and
# the linker spell one, and as GNU as does, with a capital
WARNING = re.compile(r'\b[Ww]arning: ')
# the section index of a common symbol, SHN_COMMON
COMMON = 0xFFF2
# the file of each pass's import values, by the pass's name, which is also
# the stem of its script's, map's and ELF's names
PASSES = {'layout': 'provisional.ld', 'final': 'imports.ld'}
# the output sections between .head and .tail, in order. Each gives its
# name, the patterns of GCC's sections it takes, and the kinds of SHC's. Its
# first kind names its first run
OUTPUTS = (('.text', '.text .text.*', 'P'), ('.rodata', '.rodata .rodata.*', 'C'),
           ('.data', '.data .data.*', 'DR'), ('.bss', '.bss .bss.*', 'B'))
# the DWARF sections the script places at 0: those of make dc's objects and
# of libgcc.a's members. ld fails on any other, as an orphan
DEBUG = ('.debug_aranges', '.debug_info', '.debug_abbrev', '.debug_line', '.debug_frame', '.debug_str',
         '.debug_line_str', '.debug_loclists', '.debug_rnglists')
SCRIPT = '''\
/* {name}'s {stage} pass, as tools/ld.py links it */
MEMORY
{{
  area (rwx) : ORIGIN = {origin}, LENGTH = {length}
}}
INCLUDE "{assigned}"
{named}{roots}SECTIONS
{{
{sections}  ASSERT(_image_start == ORIGIN(area), "it starts past its origin, {origin}")
  ASSERT(_image_end <= ORIGIN(area) + LENGTH(area), "{overrun}")
{sizes}  /* where SH's backend would put a dynamic relocation against a name the
     script assigns, as an import: a static link writes none. An SHC
     section's relocations take its name after .rela, and none of those
     lands here either */
  .rela.dyn : {{ *(.rela.*) *(.rela[A-Z]*) }}
  ASSERT(SIZEOF(.rela.dyn) == 0, "it holds a dynamic relocation")
  .comment 0 : {{ *(.comment) }}
{debug}}}
'''

# a run of the script: its name where it takes bounds, or else None; the
# alignment the script gives its start, or 1 for none; and the input
# section statements it holds
Run = namedtuple('Run', 'name align statements')


def provisional(images, image):
    """The value each of image's imports takes in its layout pass, by ALIAS:
    its provider's origin, in the import's view. It fails as
    gcclink.in_view() does."""
    return {alias: gcclink.in_view(images[wanted.provider].origin, wanted.view, f'{image.name}: {alias}')
            for alias, wanted in image.imports.items()}


def file_pattern(path):
    """path as a pattern of ld's that matches path alone: its last character
    in brackets. ld opens a file the script names outright ahead of the
    command line's, which would move the file's other sections ahead of the
    files before it."""
    return f'"{path[:-1]}[{path[-1]}]"'


def output(name):
    """The output section the script puts name, an input section's, in, by
    OUTPUTS, or None for none. The last code goes in .text, and the last
    data in .data."""
    if name.startswith(gcclink.LAST):
        return '.text'
    if name.startswith(gcclink.LAST_DATA):
        return '.data'
    for each, gcc, kinds in OUTPUTS:
        if (any(fnmatch.fnmatchcase(name, pattern) for pattern in gcc.split())
                or (gcclink.SECTION.fullmatch(name) and name[0] in kinds)):
            return each
    return None


def windows(image, objects):
    """Each of image's kernels laid in its window, as (window, start, end),
    by the kernel's name in row order. start is where the window starts and
    end where its code ends, as offsets into .text in bytes. objects maps the
    path of each of image's objects to its allocated sections, by name, as
    check_rows() takes it.

    The windows follow each other from .text's start in row order, each
    LINES lines long. A kernel's code starts OFFSET lines into its window
    and lies as ld lays its sections, each on its alignment. .text starts on
    the largest alignment of its sections, so the offsets keep each
    section's alignment."""
    given = {window.kernel: window for window in image.windows}
    laid = {}
    for piece in image.kernel:
        if piece.group not in laid:
            window = given[piece.group]
            start = sum(each.lines for each, _, _ in laid.values()) * ICACHE_LINE
            laid[piece.group] = (window, start, start + window.offset * ICACHE_LINE)
        window, start, end = laid[piece.group]
        align = max(objects[piece.path][piece.input].align, 1)
        laid[piece.group] = (window, start, -(-end // align) * align + objects[piece.path][piece.input].size)
    return laid


def check_rows(image, objects, bounded, common):
    """Fail, naming image, on every problem of its head, tail, kernel,
    window, warm, data, over, last and cold rows at once. objects maps the path of
    each of image's objects to its allocated sections, by name, bounded
    is the names of the sections an input bounds, and common maps the path
    of each cold row's object to the C names of its common symbols.

    A row fails where PATH is no object of image's inputs, or INPUT is no
    allocated section of it, a last row where PATH holds no last code, and
    a cold row where PATH holds a section that no output section's GCC
    patterns take, as SHC's sections, the last code and the last data, or
    one that another row places, or a common symbol, which lies in no
    section for the row to place. Two rows that place one input section
    fail, and so do two last rows or two cold rows that name one object,
    and a last row and a cold row that do. A SECTION whose rows don't stand
    together, all in the head or all in the tail, fails, and so does a
    KERNEL whose rows don't stand together. A kernel or warm row fails where
    INPUT is no code, and the kernel and warm rows fail where they take some
    of an object's code and not all, and the object's code is SHC's
    sections and GCC's both. Where image has windows, a window row fails where it names a
    kernel no kernel row names, a kernel fails where no window row or two
    name it, and a kernel whose code passes its window fails, as windows()
    gives them. A data row fails where INPUT is code or no section of the
    script's, or takes a run of its own. An over row fails where it names
    one kernel twice, or a kernel no kernel row names, but for OTHER's
    kernel, which across() checks. A window row that names warm gives the
    warm rows their window, and fails where another does, or where it gives
    an OFFSET. Where image has searched rows, a warm row fails as
    misplaced() says."""
    pieces = [(piece.group, piece) for piece in image.heads + image.tails]
    pieces += [(f'{piece.group} kernel', piece) for piece in image.kernel]
    pieces += [(piece.group, piece) for piece in image.warm + image.data]
    problems = [f"{image.name}: its {label} row places {piece.path}'s {piece.input}, "
                + ('and that object holds no such section' if piece.path in objects else
                   'and that is no object of its inputs')
                for label, piece in pieces if piece.input not in objects.get(piece.path, ())]
    placed = [(piece.path, piece.input) for _, piece in pieces]
    problems += [f"{image.name}: its rows place {path}'s {name} twice" for path, name in sorted(set(placed))
                 if placed.count((path, name)) > 1]
    problems += [f'{image.name}: its last row names {path}, '
                 + ('which holds no last code' if path in objects else 'which is no object of its inputs')
                 for path in image.last if not any(name.startswith(gcclink.LAST) for name in objects.get(path, ()))]
    problems += [f'{image.name}: its last rows name {path} twice' for path in sorted(set(image.last))
                 if image.last.count(path) > 1]
    rowed = {piece.path for _, piece in pieces}
    for path in dict.fromkeys(image.cold):
        if path not in objects:
            problems.append(f'{image.name}: its cold row names {path}, which is no object of its inputs')
            continue
        problems += [f"{image.name}: its cold row names {path}, whose {name} no output section's GCC patterns take"
                     for name in sorted(objects[path]) if not output(name) or not name.startswith('.')
                     or name.startswith((gcclink.LAST, gcclink.LAST_DATA))]
        problems += [f'{image.name}: its cold row names {path}, whose common symbol {name} lies in no section of it, '
                     'so .bss takes it' for name in common[path]]
        if path in rowed:
            problems.append(f'{image.name}: its cold row names {path}, a section of which another row places')
        if path in image.last:
            problems.append(f'{image.name}: its last and cold rows name {path}')
        if image.cold.count(path) > 1:
            problems.append(f'{image.name}: its cold rows name {path} twice')
    # None stands between the head's rows and the tail's, so a SECTION on
    # both sides of it stands apart
    order = [piece.group for piece in image.heads] + [None] + [piece.group for piece in image.tails]
    for name in sorted(set(order) - {None}):
        at = [index for index, section in enumerate(order) if section == name]
        if at != list(range(at[0], at[-1] + 1)):
            problems.append(f'{image.name}: its rows split {name} into runs apart')
    order = [piece.group for piece in image.kernel]
    for name in sorted(set(order)):
        at = [index for index, kernel in enumerate(order) if kernel == name]
        if at != list(range(at[0], at[-1] + 1)):
            problems.append(f'{image.name}: its rows split its {name} kernel into runs apart')
    problems += [f"{image.name}: its {label} row places {piece.path}'s {piece.input}, which is no code"
                 for label, piece in [*((f'{piece.group} kernel', piece) for piece in image.kernel),
                                      *(('warm', piece) for piece in image.warm)]
                 if piece.path in objects and not gcclink.code(piece.input)]
    for path in sorted({piece.path for piece in image.kernel + image.warm if piece.path in objects}):
        held = {name for name, section in objects[path].items() if gcclink.code(name) and section.size}
        taken = {piece.input for piece in image.kernel + image.warm if piece.path == path}
        if held - taken and any(name.startswith('.') for name in held) and any(name[0] == 'P' for name in held):
            problems.append(f"{image.name}: its kernel and warm rows take some of {path}'s code, which is SHC's "
                            "sections and GCC's both, and leave " + ', '.join(sorted(held - taken)))
    # the name of each output section's first run
    first = {each: kinds[0] for each, _, kinds in OUTPUTS}
    for piece in image.data:
        if piece.input in objects.get(piece.path, ()):
            into = output(piece.input)
            if into in (None, '.text'):
                problems.append(f"{image.name}: its data row places {piece.path}'s {piece.input}, which is no data")
            elif piece.input in bounded and piece.input != first[into]:
                problems.append(f"{image.name}: its data row places {piece.path}'s {piece.input}, which takes a run "
                                'of its own')
    kernels = list(dict.fromkeys(piece.group for piece in image.kernel))
    if image.windows:
        named = [window.kernel for window in image.windows if window.kernel != WARM]
        closing = [window for window in image.windows if window.kernel == WARM]
        problems += [f'{image.name}: its warm rows have {len(closing)} windows'] if len(closing) > 1 else []
        problems += [f"{image.name}: its warm rows' window gives an OFFSET, {window.offset}, and the warm rows start "
                     'where the last window ends' for window in closing if window.offset]
        problems += [f'{image.name}: its window row names {name}, which no kernel row names'
                     for name in sorted(set(named) - set(kernels))]
        problems += [f'{image.name}: its {name} kernel has ' + ('no window' if not named.count(name) else
                                                                 f'{named.count(name)} windows')
                     for name in kernels if named.count(name) != 1]
        # windows() takes each kernel's one window and each row's section
        if not problems:
            problems += [f"{image.name}: its {name} kernel takes {-(-(end - start) // ICACHE_LINE)} lines, more "
                         f"than its window's {window.lines}"
                         for name, (window, start, end) in windows(image, objects).items()
                         if end > start + window.lines * ICACHE_LINE]
    # misplaced() takes each kernel and warm row's section
    if image.searched and not problems:
        problems += misplaced(image, objects)
    for row in image.over:
        # a row that gives OTHER names one kernel of image's, its first
        own = row[:1] if len(row) == 3 else row
        problems += [f'{image.name}: its over row names {name}, which no kernel row names'
                     for name in sorted(set(own) - set(kernels))]
        if len(own) == 2 and own[0] == own[1]:
            problems.append(f'{image.name}: its over row names its {own[0]} kernel twice')
    if problems:
        fail(*problems)


def warm_start(image, objects):
    """Where image's warm rows start, in bytes into .text, and what ends
    there, as (start, name): the end of its kernels' windows, or of its
    kernels, laid as ld lays them, each on its alignment, where it has no
    window. objects maps the path of each of image's objects to its
    allocated sections, by name, and holds each kernel row's section."""
    if image.windows:
        return sum(window.lines for window in image.windows if window.kernel != WARM) * ICACHE_LINE, 'windows'
    start = 0
    for piece in image.kernel:
        section = objects[piece.path][piece.input]
        start = -(-start // max(section.align, 1)) * max(section.align, 1) + section.size
    return start, 'kernels'


def warm_end(image, objects):
    """Where image's warm rows' window ends, in bytes into .text, or None
    where no window row names warm: its LINES lines past the warm rows'
    start. objects is as warm_start() takes it."""
    closing = [window for window in image.windows if window.kernel == WARM]
    return warm_start(image, objects)[0] + closing[0].lines * ICACHE_LINE if closing else None


def misplaced(image, objects):
    """The problems of the places image's searched rows give its warm
    rows' sections, each naming image, the section and what make dc-order
    does for it. objects maps the path of each of image's objects to its
    allocated sections, by name, as check_rows() takes it, and holds each
    kernel and warm row's section.

    A warm row fails where no searched row gives its section a place. It
    fails where the place is off the section's alignment, or under the end
    of image's windows, or of its kernels, laid as ld lays them, each on
    its alignment, where image has no window. It fails where its place lies
    under that of the warm row before it, since ld lays the rows in row
    order and moves on, never back. A row whose section runs past the next
    row's place fails, as one that outgrew the bytes the record gives it,
    and so does the last row where its section runs past the end of the
    warm rows' window."""
    places = {(row.path, row.input): row.place for row in image.searched}
    start, under = warm_start(image, objects)
    problems, before, record = [], None, CONFIG / image.name / 'order.txt'
    for piece in image.warm:
        section, named = objects[piece.path][piece.input], f"{piece.path}'s {piece.input}"
        if (piece.path, piece.input) not in places:
            problems.append(f"{image.name}: {record} gives {named}, a warm row's section, no place; make dc-order "
                            'gives it one')
            continue
        place = places[(piece.path, piece.input)]
        lies = f'{image.name}: {record} places {named} {place:#x} bytes into .text'
        if place % max(section.align, 1):
            problems.append(f'{lies}, off the {section.align} bytes it is aligned to; {PLACE}')
        elif place < start:
            problems.append(f'{lies}, under the end of its {under}, {start:#x} bytes in; {PLACE}')
        if before and place < before[1]:
            problems.append(f"{lies}, ahead of {before[0]}, the warm row before it; make dc-order writes the warm "
                            "rows in their places' order")
        elif before and before[1] + before[2] > place:
            problems.append(f'{image.name}: {before[0]} takes {before[2]} bytes from its place, {before[1]:#x} bytes '
                            f"into .text, {before[1] + before[2] - place} past the next warm row's place in {record}; "
                            f'{PLACE}')
        before = (named, place, section.size)
    end = warm_end(image, objects)
    if before and end is not None and before[1] + before[2] > end:
        problems.append(f'{image.name}: {before[0]} takes {before[2]} bytes from its place, {before[1]:#x} bytes into '
                        f".text, {before[1] + before[2] - end} past the end of the warm rows' window, {end:#x} bytes "
                        f'in; {PLACE}')
    return problems


def roomier(image, printed):
    """What frees room in image, in a list, where printed, what ld printed
    of a pass of it, says OVERRUN, and image's record leaves bytes free
    among its warm rows' sections: how many, and what lays the rows with
    none. The bytes are those between where the rows end by the record and
    where they would, laid from the warm rows' start, each after the one
    before on its alignment. Each row that left its place for another left
    its bytes free. An image whose warm rows have a window is as long
    wherever they lie in it, so it takes none. It fails as gcclink.inputs()
    does."""
    if OVERRUN not in printed or not image.searched or not image.warm or any(
            window.kernel == WARM for window in image.windows):
        return []
    objects = {path: {section.name: section for section in obj.sections} for path, member, obj in gcclink.inputs(image)
               if member is None}
    places = {(row.path, row.input): row.place for row in image.searched}
    at, _ = warm_start(image, objects)
    ends = at
    for piece in image.warm:
        section = objects[piece.path][piece.input]
        at = -(-at // max(section.align, 1)) * max(section.align, 1) + section.size
        ends = max(ends, places[(piece.path, piece.input)] + section.size)
    if ends <= at:
        return []
    return [f'{image.name}: {CONFIG / image.name / "order.txt"} leaves {ends - at} bytes free among its warm rows; '
            'make dc-order PACK=1 lays the rows with none free, which moves each row after the first free byte']


def runs(image, found, pads):
    """Each output section of image's script as (output, runs), in order.
    Each run is a Run. found is image's inputs, as gcclink.inputs() gives
    them, and pads the bytes the script skips ahead of a data row's
    section, by the row's index, and ahead of the last data, by the index
    past the rows', as fit() gives them. In an output section image's
    starts row names, the script moves to the next OCACHE_LINES lines of
    OCACHE_LINE after the data rows, and in .rodata the sections ld merges
    close the output section, in no run, the strings last. It fails as
    check_rows() does."""
    objects = {}
    for path, member, obj in found:
        if member is None:
            objects.setdefault(path, {section.name: section for section in obj.sections if section.flags & elf.ALLOC})
    bounded = {match[2] for _, _, obj in found for symbol in obj.symbols if symbol.section == elf.UNDEFINED
               for match in [gcclink.BOUND.fullmatch(symbol.name)] if match}
    common = {path: sorted(elf.as_c_name(symbol.name) for symbol in obj.symbols if symbol.section == COMMON)
              for path, member, obj in found if member is None and path in image.cold}
    check_rows(image, objects, bounded, common)
    placed = {(piece.path, piece.input) for piece in image.heads + image.tails}
    # the sections that statements of their own place after the others that
    # could take them: each tail row's, and each of a cold row's object's
    apart = {(piece.path, piece.input) for piece in image.tails}
    apart |= {(path, name) for path in image.cold for name in objects[path]}
    # the largest alignment of each name's allocated SHC sections, by the
    # name. A section a head or tail row places counts for none
    aligned = {}
    for path, member, obj in found:
        for section in obj.sections:
            if (section.flags & elf.ALLOC and gcclink.SECTION.fullmatch(section.name)
                    and (member is not None or (path, section.name) not in placed)):
                aligned[section.name] = max(aligned.get(section.name, 1), section.align)

    def excluded(pattern):
        # the objects of the sections apart that pattern matches
        return sorted({path for path, name in apart if fnmatch.fnmatchcase(name, pattern)})

    def statement(*patterns):
        # one input section statement of patterns, which leaves each section
        # apart to its own: a tail row's to .tail, and a cold object's to
        # those that close .text and .data. ld gives an input section to the
        # first statement that matches it, and those come after. EXCLUDE_FILE
        # leaves out every section of its files that its pattern matches, so
        # the statement names each other such section of those files
        others = sorted({name for pattern in patterns for path in excluded(pattern) for name in objects[path]
                         if fnmatch.fnmatchcase(name, pattern) and (path, name) not in apart})
        parts = []
        for pattern in [*patterns, *others]:
            files = ' '.join(f'"{path}"' for path in excluded(pattern))
            parts.append(f'EXCLUDE_FILE({files}) {pattern}' if files else pattern)
        return f'*({" ".join(parts)})'

    # the statements that open each output section's first run: the
    # kernels' in .text, each in its window where image has windows, then
    # the warm rows', each at its place where image has searched rows, up
    # to their window's end where they have one, and the data rows' in the
    # output section of each, each after its pad
    opening = {each: [] for each, _, _ in OUTPUTS}
    for index, piece in enumerate(image.data):
        if index in pads:
            opening[output(piece.input)].append(f'. += {pads[index]:#x};')
        opening[output(piece.input)].append(f'{file_pattern(piece.path)}({piece.input})')
    # the rows' pads move nothing past them where the output section starts
    # again there
    for each in image.starts:
        if opening[each]:
            opening[each].append(f'. = ALIGN({OCACHE_LINES * OCACHE_LINE:#x});')
    laid = windows(image, objects) if image.windows else {}
    for name in dict.fromkeys(piece.group for piece in image.kernel):
        # ld takes . in an output section as an offset into it
        window, start, _ = laid.get(name, (None, 0, 0))
        if window and window.offset:
            opening['.text'].append(f'. = {start + window.offset * ICACHE_LINE:#x};')
        opening['.text'] += [f'{file_pattern(piece.path)}({piece.input})' for piece in image.kernel
                             if piece.group == name]
        if window:
            opening['.text'].append(f'. = {start + window.lines * ICACHE_LINE:#x};')
    # each warm row's section at the place the record gives it, which
    # check_rows() has held to lie at or past where ld stands by then
    places = {(row.path, row.input): row.place for row in image.searched}
    for piece in image.warm:
        if (piece.path, piece.input) in places:
            opening['.text'].append(f'. = {places[(piece.path, piece.input)]:#x};')
        opening['.text'].append(f'{file_pattern(piece.path)}({piece.input})')
    # the end of the warm rows' window, which check_rows() has held the last
    # row's section to end under. Rows without a record lie as they come,
    # for order.py to read, and nothing closes them
    if image.warm and image.searched and warm_end(image, objects) is not None:
        opening['.text'].append(f'. = {warm_end(image, objects):#x};')
    outputs = []
    for each, gcc, kinds in OUTPUTS:
        names = sorted(name for name in aligned if name[0] in kinds)
        own = [name for name in names if name in bounded and name != kinds[0]]
        # a started .rodata keeps the sections ld merges out of its runs:
        # INPUT_SECTION_FLAGS gives a statement only the sections whose
        # flags fit, and those it leaves fall to the statements that close
        # the output section, constants and then strings
        merged = each == '.rodata' and each in image.starts
        taking = statement(*gcc.split(), *(name for name in names if name not in own))
        # the script aligns no first run: ld starts an output section on the
        # largest alignment of its sections, which covers its first run's
        first = Run(kinds[0], 1, opening[each] + [f'INPUT_SECTION_FLAGS (!SHF_MERGE) {taking}' if merged else taking]
                    + (['*(COMMON)'] if each == '.bss' else []))
        # the last code closes .text, each last row's first, and the last
        # data .data, after its pad, in no run. Each cold row's object
        # follows: its code in .text, and its read-only data, data and .bss
        # in .data
        closing = {'.text': gcclink.LAST, '.data': gcclink.LAST_DATA}
        skip = [f'. += {pads[len(image.data)]:#x};'] if each == '.data' and len(image.data) in pads else []
        rowed = [f'{file_pattern(path)}({gcclink.LAST}*)' for path in image.last] if each == '.text' else []
        placing = [OUTPUTS[0][1]] if each == '.text' else [gcc for _, gcc, _ in OUTPUTS[1:]] if each == '.data' else []
        cold = [f'{file_pattern(path)}({patterns})' for path in image.cold for patterns in placing]
        last = [Run(None, 1, [*skip, *rowed, statement(f'{closing[each]}*'), *cold])] if each in closing else []
        if merged:
            last = [Run(None, 1, [f'INPUT_SECTION_FLAGS (!SHF_STRINGS) {statement(*gcc.split())}',
                                  statement(*gcc.split())])]
        outputs.append((each, [first, *(Run(name, aligned[name], [statement(name)]) for name in own), *last]))
    taken = {run.name for _, output_runs in outputs for run in output_runs}
    rows_aligned = {}
    for piece in image.heads + image.tails:
        if piece.group not in taken:
            rows_aligned[piece.group] = max(rows_aligned.get(piece.group, 0),
                                              objects[piece.path][piece.input].align)

    def grouped(pieces):
        # the runs of pieces, in row order. Rows next to each other whose
        # runs take no bounds share one
        made = []
        for piece in pieces:
            name = piece.group if piece.group in rows_aligned else None
            if not made or made[-1].name != name:
                made.append(Run(name, rows_aligned.get(name, 1), []))
            made[-1].statements.append(f'{file_pattern(piece.path)}({piece.input})')
        return made

    return ([('.head', grouped(image.heads))] if image.heads else []) + outputs + (
        [('.tail', grouped(image.tails))] if image.tails else [])


def filled(image, found, roots):
    """The output sections of image's script that an input the link takes
    gives a byte, and those of them it gives a byte that no data row
    places, as a pair of their names. found is image's inputs, as
    gcclink.inputs() gives them, and the script roots each name of roots.
    The link takes the inputs gcclink.selected() gives. An allocated section
    of nonzero size goes to the output section output() gives its name,
    but a head or tail row's, which goes to neither, and the read-only
    data and .bss of a cold row's object, which go to .data. A common
    symbol goes to .bss."""
    read = {(path, member): obj for path, member, obj in found}
    rowed = {(piece.path, piece.input) for piece in image.heads + image.tails}
    opening = {(piece.path, piece.input) for piece in image.data}
    taking, past = set(), set()
    for path, member in gcclink.selected(image.inputs, roots)[0]:
        for section in read[(path, member)].sections:
            if section.flags & elf.ALLOC and section.size and (member is not None or (path, section.name) not in rowed):
                cold = member is None and path in image.cold and output(section.name) in ('.rodata', '.bss')
                taking.add('.data' if cold else output(section.name))
                if member is not None or (path, section.name) not in opening:
                    past.add('.data' if cold else output(section.name))
        if any(symbol.section == COMMON and symbol.size for symbol in read[(path, member)].symbols):
            taking.add('.bss')
            past.add('.bss')
    return taking, past


def unheld(image, roots, passed):
    """The problems of image's starts row in passed, a pass of it, whose
    script roots each name of roots: each output section the row names that
    the pass's map gives a byte of an input where filled() reckoned none,
    or none where it reckoned one, and each it gives a byte off the next
    OCACHE_LINES lines of OCACHE_LINE. The script starts an output section
    by filled(), so either is ld.py's fault, and no table's."""
    if not image.starts:
        return []
    reckoned, _ = filled(image, gcclink.inputs(image), roots)
    given = {each.output for each in passed.placements if each.size}
    at = {section.name: section.address for section in passed.elf.sections}
    problems = []
    for each in image.starts:
        if each in given and each not in reckoned:
            problems.append(f'{image.name}: ld gives its {each} a byte, and ld.py reckoned that no input does, so its '
                            "script starts it on no 16 KB of the SH-4's operand cache, as its starts row has it; "
                            "tools/ld.py's filled() is at fault")
        elif each in reckoned and each not in given:
            problems.append(f'{image.name}: ld gives its {each} no byte, and ld.py reckoned that an input does, so '
                            "its script starts it on the 16 KB of the SH-4's operand cache for nothing; "
                            "tools/ld.py's filled() is at fault")
        elif each in given and at[each] % (OCACHE_LINES * OCACHE_LINE):
            problems.append(f"{image.name}: its {each} lies at {hexa(at[each])}, off the 16 KB of the SH-4's operand "
                            'cache that its starts row has it start on')
    return problems


def sections_text(image, found, pads, roots):
    """The text of the output sections of image's script, and the names of
    the runs that take bounds, in script order, as a pair. found is image's
    inputs, as gcclink.inputs() gives them, pads its data rows' pads, as
    runs() takes them, and roots the names the script roots. Each output
    section lies in area, the script's MEMORY region. image_start opens the
    first, and image_end closes the last. Where image has windows, .text
    starts on a line of ICACHE_LINE bytes, and .rodata and .bss on
    OCACHE_LINES lines of OCACHE_LINE, as does each output section image's
    starts row names that filled() gives a byte. One that it gives no byte
    past its data rows has nothing to keep clear of their pads, so runs()
    lays it as one the row doesn't name. It fails as runs() does."""
    holding, past = filled(image, found, roots) if image.starts else (set(), set())
    lines, with_bounds = [], []
    outputs = runs(image._replace(starts=[each for each in image.starts if each in past]), found, pads)
    starts = {'.text': ICACHE_LINE, '.rodata': OCACHE_LINES * OCACHE_LINE, '.bss': OCACHE_LINES * OCACHE_LINE}
    for number, (output, output_runs) in enumerate(outputs):
        start = f' ALIGN({starts[output]:#x})' if image.windows and output in starts else ''
        if output in image.starts and output in holding:
            start = f' ALIGN({OCACHE_LINES * OCACHE_LINE:#x})'
        lines += [f'  {output} :{start}', '  {']
        if number == 0:
            lines.append('    _image_start = .;')
        for run in output_runs:
            if run.name is None:
                lines += [f'    {statement}' for statement in run.statements]
                continue
            if run.align > 1:
                lines.append(f'    . = ALIGN({run.align});')
            lines += [f'    __startof_{run.name} = .;', *(f'    {statement}' for statement in run.statements),
                      f'    __endof_{run.name} = .;']
            with_bounds.append(run.name)
        if number == len(outputs) - 1:
            lines.append('    _image_end = .;')
        lines.append('  } > area')
    return ''.join(f'{line}\n' for line in lines), with_bounds


def imports_text(image, values, stage):
    """The text of the file of image's import values for its pass named
    stage. values maps each import's ALIAS to its value. Each value's
    assignment follows an assertion that no input defines ALIAS, and names
    the definition it takes in a comment."""
    lines = [f"/* {image.name}'s imports, for its {stage} pass */"]
    for alias, value in values.items():
        wanted = image.imports[alias]
        lines += [f'ASSERT(!DEFINED(_{alias}), "its inputs define {alias}, which it imports")',
                  f"_{alias} = {hexa(value)}; /* {wanted.provider}'s {wanted.symbol} */"]
    return '\n'.join([*lines, ''])


def script_text(image, roots, assigned, stage, pads):
    """The text of image's script for its pass named stage. It includes
    assigned, the file of the pass's import values, roots each name of
    roots, and pads its data rows as pads gives, as runs() takes them. It
    fails as gcclink.inputs() and sections_text() do."""
    # the aliases and constants, which the script assigns alike in both
    # passes
    named = ''.join(f'ASSERT(!DEFINED(_{alias}), "its inputs define {alias}, which it aliases")\n'
                    f'ASSERT(DEFINED(_{symbol}), "{alias} is a second name for its {symbol}, which nothing defines")\n'
                    f'_{alias} = DEFINED(_{symbol}) ? ABSOLUTE(_{symbol}) : 0; /* its {symbol} */\n'
                    for alias, symbol in image.aliases.items())
    named += ''.join(f'ASSERT(!DEFINED(_{symbol}), "its inputs define {symbol}, which it binds to {hexa(value)}")\n'
                     f'_{symbol} = {hexa(value)};\n' for symbol, value in image.constants.items())
    sections, with_bounds = sections_text(image, gcclink.inputs(image), pads, roots)
    named += ''.join(f'ASSERT(!DEFINED(_{name}), "its inputs define {name}, a boundary of its link")\n'
                     for name in (*gcclink.IMAGE_BOUNDS,
                                  *(f'_{bound}_{run}' for run in with_bounds for bound in gcclink.RUN_BOUNDS)))
    sizes = ''.join(f'  __sizeof_{name} = __endof_{name} - __startof_{name};\n' for name in with_bounds)
    debug = ''.join(f'  {name} 0 : {{ *({name}) }}\n' for name in DEBUG)
    return SCRIPT.format(name=image.name, stage=stage, origin=hexa(image.origin), length=hexa(image.length),
                         assigned=assigned, named=named, roots=''.join(f'EXTERN(_{root})\n' for root in roots),
                         sections=sections, overrun=OVERRUN, sizes=sizes, debug=debug)


def ld_command(image, script, mapped, linked):
    """ld's command to link image by script into linked, an ELF, with its
    map in mapped, as its words."""
    # --no-warn-rwx-segments: the image loads into RAM whole, so a segment
    # both writable and executable is no fault
    return list(map(str, [(gcclink.SH_ELF / 'bin/sh-elf-ld').resolve(), '-EL', '-q', '--no-relax',
                          '--orphan-handling=error', '--fatal-warnings', '--no-warn-rwx-segments', '-e',
                          f'_{image.entry}' if image.entry else hexa(image.origin), '-T', script, '-Map', mapped, '-o',
                          linked, *image.inputs]))


def lifted(image):
    """image with a load area that runs from its origin to the end of RAM,
    in the view its origin lies in. No link of it fails for want of room
    in the image's own load area, so one gives where the image ends, and
    what each of its sections takes, whatever its length: trial() links a
    pass so, and tools/order.py the pass it reads an image's rows from."""
    # RAM is a power of two, and each view of it starts on one
    return image._replace(length=(image.origin | (gcclink.RAM - 1)) + 1 - image.origin)


def trial(image, roots, out, stage, pads, name):
    """image's pass named stage linked once more, in a load area that runs
    to the end of RAM, as lifted() gives it, as a Linked, or None where ld
    can't run, fails or prints anything. The link goes by the pass's file
    of import values, to NAME.ld, NAME.map and NAME.elf under out/IMAGE/,
    and trial() removes the ELF. roots and pads are the pass's, as link()
    takes them: where pads is None, trial() pads the link's data rows as
    link() would, by fit()."""
    script, mapped, linked = (gcclink.pass_file(out, image.name, name, suffix) for suffix in ('.ld', '.map', '.elf'))
    long, fitted = lifted(image), pads or {}
    while True:
        save(script, script_text(long, roots, out / image.name / PASSES[stage], stage, fitted).encode())
        remove([linked])
        try:
            done = run_child(ld_command(long, script, mapped, linked), stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        except OSError:
            return None
        if done.returncode or done.stdout:
            remove([linked])
            return None
        passed = gcclink.linked(out, image.name, name)
        if pads is not None or fitted or not pooled(long, passed) or not fit(long, passed):
            remove([linked])
            return passed
        fitted = fit(long, passed)


def skipped(text):
    """Each input section that text, ld's map, places after bytes ld skips
    to its alignment, as (file, section, bytes), in map order."""
    found, fill = [], None
    # ld puts a name too long for its column on a line of its own
    for line in re.sub(r'^( [^\s*]\S*)\n(?= +0x)', r'\1', text, flags=re.M).splitlines():
        placed = gcclink.PLACED.fullmatch(line)
        if placed and fill and int(placed[2], 16) == fill[0] + fill[1]:
            found.append((placed[4].strip(), placed[1], fill[1]))
        at = FILL.fullmatch(line)
        fill = (int(at[1], 16), int(at[2], 16)) if at else None
    return found


def room(image, roots, out, stage, pads, printed):
    """The problems of image's room where its pass named stage runs past
    its load area, or no problem. printed is what ld printed of the pass,
    and roots and pads are the pass's, as link() takes them.

    Where printed says by how many bytes the image runs past its load area,
    and image has a starts row or an allocated input section aligned to
    OCACHE_LINES lines of OCACHE_LINE or more, room() links the pass again
    without the row, as trial() does, to packed.ld and packed.map, which it
    leaves. It gives no problem where that link fails, since room isn't
    then all the image lacks.

    Where that link ends inside the load area, the row is what takes the
    image past it. The problem gives the bytes the starts take, which is
    how far the two links' ends lie apart, and the bytes the load area has
    for them, past that link's end. It names each starts row that names
    fewer of the row's output sections and fits the load area, but one
    that names only output sections of another that fits, as a trial() of
    each to narrower.ld shows, whose files room() removes, and the packed
    row that starts none.

    Where that link ends past the load area, no row gives the image room.
    The first problem says by how many bytes it runs past with none of its
    starts, where it has a row. Then one comes for each input section
    aligned so that skipped() gives of that link's map, with the bytes ld
    skips to it."""
    past, size = PAST.search(printed), OCACHE_LINES * OCACHE_LINE
    if not past:
        return []
    wide = {(gcclink.map_name(path, member), section.name) for path, member, found in gcclink.inputs(image)
            for section in found.sections if section.flags & elf.ALLOC and section.align >= size}
    if not image.starts and not wide:
        return []
    packed = trial(image._replace(starts=[]), roots, out, stage, pads, 'packed')
    if not packed:
        return []
    ends = image.origin + image.length
    left = ends - next(symbol.value for symbol in packed.elf.symbols if symbol.name == '_image_end')
    table = f'config/{image.name}/kernel.txt'
    if left < 0:
        mapped = gcclink.pass_file(out, image.name, 'packed', '.map')
        return ([f'{image.name}: with none of its starts it runs {-left} bytes past its load area, so no packed row in '
                 f'{table} gives it room'] if image.starts else []) + [
            f"{image.name}: {file}'s {name} starts on the 16 KB of the SH-4's operand cache by its own alignment, "
            f'which no row packs, and ld skips {fill} bytes to it'
            for file, name, fill in skipped(content(mapped).decode('latin-1')) if (file, name) in wide]
    if not image.starts:
        return []
    # each narrower row, the longest first, that fits and that no row found
    # to fit holds whole
    fitting = []
    for count in range(len(image.starts) - 1, 0, -1):
        for fewer in combinations(image.starts, count):
            if any(set(fewer) <= set(found) for found in fitting):
                continue
            narrower = trial(image._replace(starts=list(fewer)), roots, out, stage, pads, 'narrower')
            if narrower and next(symbol.value for symbol in narrower.elf.symbols if symbol.name == '_image_end') <= ends:
                fitting.append(fewer)
    remove([gcclink.pass_file(out, image.name, 'narrower', suffix) for suffix in ('.ld', '.map', '.elf')])
    named = ' and '.join(filter(None, (', '.join(image.starts[:-1]), image.starts[-1])))
    rows = ' or '.join(f'starts {" ".join(fewer)}' for fewer in fitting)
    return [f"{image.name}: the starts of its {named} on the 16 KB of the SH-4's operand cache take "
            f'{left + int(past[1])} bytes, and its load area has {left} for them; in {table}, '
            + (f'a row that names fewer fits, {rows}, and a packed row starts none of them' if fitting else
               'no starts row that names fewer fits, and a packed row starts none of them'
               if len(image.starts) > 1 else 'a packed row starts none of them')]


def link(image, roots, values, out, stage, pads=None):
    """image's pass named stage, layout or final, linked under out/IMAGE/,
    as a Linked. values maps each of image's imports to its value, by ALIAS,
    and the script roots each name of roots. pads gives its data rows'
    pads, as runs() takes them.

    The pass writes imports_text() to the file PASSES names, and
    script_text() to STAGE.ld. ld writes its map, STAGE.map, and its ELF,
    STAGE.elf. The pass removes the ELF an earlier run left before ld runs,
    so that a pass ld fails leaves none.

    Where pads is None, the pass links with no pads first. Where pooled()
    then gives a pair and fit() gives pads, it links again with those pads,
    and takes that link. Both passes do so, and lay out the image alike, so
    the two take the same pads.

    It fails, naming the image, where ld fails or prints anything, and
    with room()'s problems where the image runs past its load area, and
    what roomier() says frees room among its warm rows. It fails on each
    problem unheld() gives of its starts row, on a weak reference that
    nothing defines, on a kernel that touches more
    than ICACHE_LINES lines, on two kernels crossed() gives, and on each
    pair of lines pooled() gives, and removes the ELF. It fails naming a
    directory it can't make, a file it can't write, read or remove, and ld
    where ld can't run. It fails as script_text() does, and as
    gcclink.linked() does on ld's ELF and map."""
    directory = out / image.name
    try:
        directory.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        fail(f'{error.filename}: {error.strerror}')
    assigned = directory / PASSES[stage]
    script, mapped, linked = (gcclink.pass_file(out, image.name, stage, suffix) for suffix in ('.ld', '.map', '.elf'))
    save(assigned, imports_text(image, values, stage).encode())
    save(script, script_text(image, roots, assigned, stage, pads or {}).encode())
    # ld starts each line it prints with its real path, which the failure
    # below takes off
    linker = (gcclink.SH_ELF / 'bin/sh-elf-ld').resolve()
    remove([linked])
    try:
        done = run_child(ld_command(image, script, mapped, linked), stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         text=True, errors='replace')
    except OSError as error:
        fail(f"{linker}: can't run it: {error.strerror}")
    if done.returncode or done.stdout:
        prefix = f'{linker}: '
        fail(*(f'{image.name}: {line[len(prefix):] if line.startswith(prefix) else line}'
               for line in done.stdout.splitlines()), *room(image, roots, out, stage, pads, done.stdout),
             *roomier(image, done.stdout),
             f'{image.name}: its {stage} pass failed')
    passed = gcclink.linked(out, image.name, stage)
    weak = [elf.as_c_name(symbol.name) for symbol in passed.elf.symbols
            if symbol.bind == elf.WEAK and symbol.section == elf.UNDEFINED]
    problems = [f'{image.name}: {name} is a weak reference that nothing defines, and ld.py has no policy for one'
                for name in weak]
    problems += unheld(image, roots, passed)
    lines = kernel_lines(image, passed)
    problems += [f"{image.name}: its {name} kernel touches {len(taken)} lines, more than the {ICACHE_LINES} of the "
                 "SH-4's instruction cache" for name, taken in lines.items() if len(taken) > ICACHE_LINES]
    problems += [f"{image.name}: a line of its {one} kernel and another of its {other} kernel take set {at} of the "
                 "SH-4's instruction cache, and no over row names the two" for one, other, at in crossed(image, lines)]
    clashes = pooled(image, passed)
    if clashes and not problems and pads is None:
        fitted = fit(image, passed)
        if fitted:
            return link(image, roots, values, out, stage, fitted)
    problems += [f"{image.name}: {function}'s pool line at {hexa(pool * OCACHE_LINE)} and {name}'s line at "
                 f"{hexa(line * OCACHE_LINE)}, which {namer}'s pool names, take set {line % OCACHE_LINES} of the "
                 "SH-4's operand cache, and " + (f'the {kernel} kernel runs with itself' if kernel == other else
                                                 f'no over row names {kernel} and {other}')
                 for pool, line, function, name, namer, kernel, other in clashes]
    if problems:
        remove([linked])
        fail(*problems, f'{image.name}: its {stage} pass failed')
    return passed


def kernel_lines(image, passed):
    """The lines of ICACHE_LINE bytes each of image's kernels touches in
    passed, a pass of it, as the range of their addresses divided by
    ICACHE_LINE, by the kernel's name in row order. Where image has windows,
    a kernel's lines are its window's, which follow each other from .text's
    start in row order. Otherwise a kernel runs from its first section's
    start to its last's end, as the pass's map places them."""
    if image.windows:
        text = next(each for each in elf.allocated_sections(passed.elf).values() if each.name == '.text')
        given = {window.kernel: window.lines for window in image.windows}
        lines, at = {}, text.address // ICACHE_LINE
        for name in dict.fromkeys(piece.group for piece in image.kernel):
            lines[name], at = range(at, at + given[name]), at + given[name]
        return lines
    placed = {(each.file, each.section): each for each in passed.placements if each.output == '.text'}
    spans = {}
    for piece in image.kernel:
        at = placed[(piece.path, piece.input)]
        start, end = spans.get(piece.group, (at.address, at.address))
        spans[piece.group] = (min(start, at.address), max(end, at.address + at.size))
    return {name: range(start // ICACHE_LINE, (end - 1) // ICACHE_LINE + 1) for name, (start, end) in spans.items()}


def pools(image, passed):
    """Each of image's kernels' pool lines, and the lines its pool's words
    name in image's data rows' sections and its last data, in passed, a
    pass of image, with the C name nearest at or below an address of .text,
    as (pool, named, function). pool and named give each kernel's lines by
    its name, in row order. pool gives each pool line the address it was
    found by, and named gives each named line the name of the object there,
    the address of the word that names it, and the index of the data row
    whose section holds it, or for the last data the index past the rows'.
    A line is an address divided by OCACHE_LINE.

    A kernel's pool lines hold the targets pool_targets() finds in its
    sections, or words of R_SH_DIR32 relocations. Such a word that holds an
    address in a data row's section or the last data names the lines of the
    POOL_REACH bytes from that address, up to the end of the object there.
    The object runs from the name nearest at or below the address over its
    largest symbol's size there, or else up to the next name. Where the
    object is over POOL_OBJECT bytes, the word names the address's line
    alone. Where a boundary of the link and another name lie at one
    address, each of these gives the other."""
    found = passed.elf
    sections = {section.name: section for section in elf.allocated_sections(found).values()}
    text, data = sections['.text'], [sections[name] for name in ('.rodata', '.data', '.bss') if name in sections]
    placed = {(each.file, each.section): each for each in passed.placements
              if each.output in (output for output, _, _ in OUTPUTS)}
    rows = [placed.get((piece.path, piece.input)) for piece in image.data]
    # the last data, one row past the rows, from the first's start to the
    # last's end
    last = [each for each in passed.placements if each.output == '.data' and each.section.startswith(gcclink.LAST_DATA)]
    if last:
        start, end = min(each.address for each in last), max(each.address + each.size for each in last)
        rows.append(last[0]._replace(address=start, size=end - start))
    words = sorted(relocation.offset for relocation in found.relocations.get(text.index, [])
                   if relocation.kind == elf.DIR32)
    # the C name the pass defines at each address of .text and the data,
    # where it defines one, and the size of its largest symbol there. Each
    # output section opens with its first run's start, a boundary, so a
    # name lies at or below each address. A boundary takes its address
    # first, and another name there replaces it
    names = {section.index: {} for section in (text, *data)}
    sizes = {section.index: {} for section in (text, *data)}
    defined = []
    for symbol in found.symbols:
        if symbol.section in names and elf.names_definition(symbol):
            name = elf.as_c_name(symbol.name)
            defined.append((not gcclink.boundary(name), name, symbol.section, symbol.value))
            sizes[symbol.section][symbol.value] = max(sizes[symbol.section].get(symbol.value, 0), symbol.size)
    for _, name, index, value in sorted(defined):
        names[index][value] = name
    starts = {index: sorted(each) for index, each in names.items()}

    def nearest(section, address):
        # the index into starts of the C name nearest at or below address in
        # section
        return bisect_right(starts[section.index], address) - 1

    def function(address):
        return names[text.index][starts[text.index][nearest(text, address)]]

    def word(at):
        return int.from_bytes(text.data[at - text.address:at - text.address + 4], 'little')

    pool, named = {}, {}
    for piece in image.kernel:
        span = placed[(piece.path, piece.input)]
        lines, wanted = pool.setdefault(piece.group, {}), named.setdefault(piece.group, {})
        held = words[bisect_left(words, span.address):bisect_left(words, span.address + span.size)]
        for at in [*pool_targets(text, span.address, span.address + span.size), *held]:
            lines.setdefault(at // OCACHE_LINE, at)
        for at in held:
            address = word(at)
            row = next((index for index, each in enumerate(rows)
                        if each and each.address <= address < each.address + each.size), None)
            if row is None:
                continue
            section = next(each for each in data if each.address <= address < each.address + each.size)
            index = nearest(section, address)
            opens = starts[section.index][index]
            follows = starts[section.index][index + 1:index + 2] or [section.address + section.size]
            closes = opens + sizes[section.index][opens] if sizes[section.index][opens] else follows[0]
            stop = address + 1 if closes - opens > POOL_OBJECT else max(min(address + POOL_REACH, closes), address + 1)
            for line in range(address // OCACHE_LINE, (stop - 1) // OCACHE_LINE + 1):
                wanted.setdefault(line, (names[section.index][opens], at, row))
    return pool, named, function


def runs_with(image, kernels):
    """Each of kernels with the kernels it runs with, by name, in kernels'
    order: itself, then each other that no over row of image names with
    it."""
    let = {frozenset(row) for row in image.over if len(row) == 2}
    return {kernel: [kernel, *(other for other in kernels if other != kernel and frozenset((kernel, other)) not in let)]
            for kernel in kernels}


def barred(image, pool):
    """The sets of the operand cache that a line a kernel's pool names may
    not take, by the kernel: those of the pool lines of each kernel it runs
    with, as runs_with() gives them. pool gives each kernel's pool lines, as
    pools() does."""
    running = runs_with(image, pool)
    return {other: {line % OCACHE_LINES for kernel in running[other] for line in pool[kernel]} for other in pool}


def pooled(image, passed):
    """Each pair of a line of a kernel's pool and a line in a data row's
    section or the last data that a word of the pool of a kernel it runs
    with names, where the two take one set of the operand cache, in passed,
    a pass of image, as pools() gives the lines. Each comes as (pool line,
    named line, function, name, namer, kernel, other), in order, and once,
    with the first kernel, other, function and namer to give it, other
    first where it is kernel. A kernel runs with the kernels runs_with()
    gives. kernel is the pool's kernel and other the word's. function is the
    C name the pass defines nearest at or below the pool line's first load
    target, or else its first word, namer the one nearest at or below the
    word, and name the object's."""
    pool, named, function = pools(image, passed)
    clashes = {}
    for kernel, others in runs_with(image, pool).items():
        for other in others:
            sets = {}
            for line, (name, at, _) in named[other].items():
                sets.setdefault(line % OCACHE_LINES, []).append((line, name, at))
            for line, by in pool[kernel].items():
                for each, name, at in sets.get(line % OCACHE_LINES, []):
                    clashes.setdefault((line, each), (function(by), name, function(at), kernel, other))
    return sorted((*pair, *given) for pair, given in clashes.items())


def fit(image, passed):
    """The bytes the script skips ahead of each of image's data rows'
    sections, by the row's index, and ahead of its last data, by the index
    past the rows', so that no line a kernel's pool names in them takes a
    set barred() bars for that kernel, as passed, a pass of image with no
    pads, lays them out. The rows take their pads in row order, and the last
    data after .data's, each the fewest whole lines of OCACHE_LINE bytes
    that clear it once the pads of the rows before it in its output section
    have moved it. Where image's starts row names .data, the start after
    .data's rows takes up their pads, and they move no last data. A row
    that needs none, or that no pad clears, takes none, and pooled() then
    gives what is left, as it does where alignment or a pad of another
    output section moves a row otherwise."""
    pool, named, _ = pools(image, passed)
    bars = barred(image, pool)
    lines = {}
    for other, wanted in named.items():
        for line, (_, _, row) in wanted.items():
            lines.setdefault(row, []).append((line, other))
    pads, moved, kinds = {}, {}, [output(piece.input) for piece in image.data]
    for row, into in enumerate([*kinds, '.data']):
        ahead = 0 if row == len(kinds) and into in kinds and into in image.starts else moved.get(into, 0)
        pad = next((pad for pad in range(OCACHE_LINES)
                    if not any((line + ahead + pad) % OCACHE_LINES in bars[other]
                               for line, other in lines.get(row, ()))), 0)
        if pad:
            pads[row] = pad * OCACHE_LINE
            moved[into] = moved.get(into, 0) + pad
    return pads


def pool_targets(text, start, end):
    """The address of each load target of the code from start to end in
    text, the .text of a pass, once each, in order. A target is the address
    a MOV.W @(disp,PC), MOV.L @(disp,PC) or MOVA @(disp,PC),R0 there loads
    or takes, where it lies before end. The decode runs from start, a half
    word at a time, and skips each half word of a target it has found, since
    a pool follows the code that loads it."""
    targets, known = set(), set()
    for at in range(start, end, 2):
        if at in known:
            continue
        code = int.from_bytes(text.data[at - text.address:at - text.address + 2], 'little')
        if code >> 12 == MOV_W:
            target, width = at + 4 + (code & 0xFF) * 2, 2
        elif code >> 12 == MOV_L or code >> 8 == MOVA:
            target, width = (at & ~3) + 4 + (code & 0xFF) * 4, 4
        else:
            continue
        if target < end:
            targets.add(target)
            known.update(range(target, target + width, 2))
    return sorted(targets)


def crossed(image, lines):
    """Each pair of image's kernels, in row order, where a line of the first
    and another line of the second take one set of the instruction cache,
    and no over row of image names the two, as (first, second, set), set
    the lowest such. lines gives each kernel's lines, as kernel_lines()
    does. Two kernels that meet on a line share that line, which takes its
    set once."""
    let = {frozenset(row) for row in image.over if len(row) == 2}
    found = []
    for one, other in combinations(lines, 2):
        if frozenset((one, other)) in let:
            continue
        sets = {}
        for line in lines[one]:
            sets.setdefault(line % ICACHE_LINES, set()).add(line)
        shared = [line % ICACHE_LINES for line in lines[other] if sets.get(line % ICACHE_LINES, set()) - {line}]
        if shared:
            found.append((one, other, min(shared)))
    return found


def across(images, layouts):
    """The problems of the kernels of images where one imports from the
    other. layouts gives each image's layout pass. One comes for each over
    row whose OTHER has no kernel of its second KERNEL's name. Then one
    comes for each pair of a kernel of one image and a kernel of a later
    one, in spec order, where either image imports from the other, a line
    of the first and a line of the second take one set of the instruction
    cache, and no over row of either image names the two."""
    lines = {name: kernel_lines(image, layouts[name]) for name, image in images.items()}
    # each image with each image it imports from
    takes = {(name, wanted.provider) for name, image in images.items() for wanted in image.imports.values()}
    let, problems = set(), []
    for name, image in images.items():
        for one, other, beside in (row for row in image.over if len(row) == 3):
            if other not in lines[beside]:
                problems.append(f"{name}: its over row names {beside}'s {other} kernel, which no kernel row of "
                                f'{beside} names')
            let.add(frozenset({(name, one), (beside, other)}))
    for first, second in combinations(images, 2):
        if (first, second) not in takes and (second, first) not in takes:
            continue
        for one, other in product(lines[first], lines[second]):
            taken = {line % ICACHE_LINES for line in lines[first][one]}
            shared = [line % ICACHE_LINES for line in lines[second][other] if line % ICACHE_LINES in taken]
            if frozenset({(first, one), (second, other)}) not in let and shared:
                problems.append(f"{first}: a line of its {one} kernel and another of {second}'s {other} kernel "
                                f"take set {min(shared)} of the SH-4's instruction cache, and no over row names the "
                                'two')
    return problems


def placed(image, passed):
    """Each of image's kernel rows' sections and then each of its warm
    rows', in row order, as passed, a pass of image, places it: (PATH,
    INPUT, place, size), place in bytes into .text."""
    text = next(each for each in elf.allocated_sections(passed.elf).values() if each.name == '.text')
    found = {(each.file, each.section): each for each in passed.placements if each.output == '.text'}
    return [(piece.path, piece.input, at.address - text.address, at.size)
            for piece in image.kernel + image.warm for at in [found[(piece.path, piece.input)]]]


def stale(image, passed):
    """The problem of image's heat, text and searched rows in passed, its
    layout pass, in a list, or no problem: warm rows and no searched row;
    searched rows and no heat row, or a heat whose SHA-256 isn't its heat
    row's; searched rows and no text row, or a .text that starts at another
    address than its text row's; the first of
    placed()'s sections that no searched row gives, or that passed places
    elsewhere than its searched row does, or, for a kernel row's, finds
    another size; or the first searched row of a section that is none of
    placed()'s. A warm row's SIZE holds nothing: check_rows() has held the
    row's section to end at or under the next row's place. It fails as
    content() does on the heat."""
    record = CONFIG / image.name / 'order.txt'
    if not image.searched:
        return [f'{image.name}: its warm rows have no record of their places, {record}; make dc-order writes one, '
                'and gives every row a place anew, so restore a record that was lost'] if image.warm else []
    if not image.heat:
        return [f'{image.name}: {record} names no heat; {RECORD}']
    if hashlib.sha256(content(image.heat[0])).hexdigest() != image.heat[1]:
        return [f"{image.name}: {image.heat[0]} isn't the heat {record} was written by; {RECORD}"]
    if image.text is None:
        return [f"{image.name}: {record} doesn't say where .text starts; {RECORD}"]
    text = next(each for each in elf.allocated_sections(passed.elf).values() if each.name == '.text')
    if text.address != image.text:
        return [f'{image.name}: .text starts at {hexa(text.address)}, and {record} has it at {hexa(image.text)}; '
                f'{RECORD}']
    then = {(row.path, row.input): row for row in image.searched}
    for index, (path, name, place, size) in enumerate(placed(image, passed)):
        row = then.pop((path, name), None)
        if row is None:
            return [f"{image.name}: {record} doesn't hold {path}'s {name}, which its rows place; {RECORD}"]
        if index < len(image.kernel) and (place, size) != (row.place, row.size):
            return [f"{image.name}: {path}'s {name} lies {place:#x} bytes into .text and takes {size}, and {record} "
                    f'has it {row.place:#x} bytes in, taking {row.size}; {RECORD}']
        if place != row.place:
            return [f"{image.name}: {path}'s {name} lies {place:#x} bytes into .text, and {record} places it "
                    f'{row.place:#x} bytes in; {RECORD}']
    return [f"{image.name}: {record} holds {row.path}'s {row.input}, which no kernel or warm row places; {RECORD}"
            for row in list(then.values())[:1]]


def resolve(images, layouts):
    """Each image's import values, by image and ALIAS. An import's value is
    its provider's export of SYMBOL in the provider's layout pass, as
    layouts gives it, spelled in the import's view. An export is one
    gcclink.exports() gives.

    It fails as gcclink.in_view() does. It fails, at once, on each import
    whose SYMBOL its provider's layout pass doesn't export, and each alias
    whose SYMBOL the image's own doesn't."""
    exported = {name: gcclink.exports(linked.elf) for name, linked in layouts.items()}
    bound, problems = {}, []
    for image in images.values():
        bound[image.name] = {}
        for alias, wanted in image.imports.items():
            if wanted.symbol in exported[wanted.provider]:
                bound[image.name][alias] = gcclink.in_view(exported[wanted.provider][wanted.symbol].value,
                                                           wanted.view, f'{image.name}: {alias}')
                continue
            why = (': it imports it too' if wanted.symbol in images[wanted.provider].imports else
                   ': it is a boundary of its link, not an export' if gcclink.boundary(wanted.symbol) else '')
            problems.append(f"{image.name}: {alias} takes {wanted.provider}'s {wanted.symbol}, which "
                            f"{layouts[wanted.provider].elf.where} doesn't define{why}")
        for alias, symbol in image.aliases.items():
            if symbol not in exported[image.name]:
                problems.append(f'{image.name}: {alias} is a second name for its {symbol}, which '
                                f"{layouts[image.name].elf.where} doesn't define")
    if problems:
        fail(*problems)
    return bound


def signature(linked):
    """What the layout check compares of linked, a pass, as a description of
    each item, by the item.

    - Each allocated section gives its start, end and flags, and whether it
      is zeroed. Its item is its name.
    - Each name defined in an allocated section, local or not, gives the
      value, size and section of each symbol of that name. So does each
      boundary of the link, wherever it lies. Its item is the name less one
      leading underscore.
    - Each input section the map places in an allocated section gives the
      address, size and output section of each placement. Its item is
      FILE's SECTION."""
    found = linked.elf
    allocated = elf.allocated_sections(found)
    described = {section.name: f'{hexa(section.address)} to {hexa(section.address + section.size)}, flags '
                               f'{section.flags:#x}' + (', zeroed' if section.kind == elf.NOBITS else '')
                 for section in allocated.values()}
    places = {}
    for symbol in found.symbols:
        name = elf.as_c_name(symbol.name)
        if elf.names_definition(symbol) and (symbol.section in allocated or gcclink.boundary(name)):
            section_name = allocated[symbol.section].name if symbol.section in allocated else 'no section'
            places.setdefault(name, []).append(f'{hexa(symbol.value)}, {symbol.size} bytes of {section_name}')
    outputs = {section.name for section in allocated.values()}
    for placed in linked.placements:
        if placed.output in outputs:
            places.setdefault(f"{placed.file}'s {placed.section}", []).append(
                f'{hexa(placed.address)}, {placed.size} bytes of {placed.output}')
    described.update((item, '; '.join(sorted(each))) for item, each in places.items())
    return described


def moved(layouts, finals):
    """The layout check's problems: one for each item of an image's
    signature that its two passes describe otherwise, or that one lacks.
    layouts and finals give each image's layout pass and final pass."""
    problems = []
    for name, layout in layouts.items():
        old, new = signature(layout), signature(finals[name])
        for item in sorted(old.keys() | new.keys()):
            if old.get(item) != new.get(item):
                problems.append(f"{name}: {item} is {old.get(item, 'absent')} in {layout.elf.where}, and "
                                f"{new.get(item, 'absent')} in {finals[name].elf.where}")
    return problems


def clear(out):
    """Remove the final.elf of each directory in out, whether or not the
    spec names its image. Once it has tried each, it fails, naming each
    final.elf it can't remove. It fails too on each symlink in out to a
    directory, through which it removes nothing. An out that isn't there
    holds no final.elf, and one it can't list fails, naming it."""
    problems = []
    try:
        held = list(out.iterdir())
    except FileNotFoundError:
        held = []
    except OSError as error:
        fail(f"{out}: can't list it: {error.strerror}")
    for each in held:
        if each.is_symlink() and each.is_dir():
            problems.append(f'{each}: a symlink to a directory, through which ld.py removes nothing')
            continue
        final = gcclink.final(out, each.name)
        try:
            # unlink fails on a directory with EISDIR on Linux but EPERM on
            # macOS, so name a directory the same way on both. lstat follows
            # no symlink: unlink removes one that names a directory
            if stat.S_ISDIR(final.lstat().st_mode):
                raise IsADirectoryError(errno.EISDIR, os.strerror(errno.EISDIR), str(final))
            final.unlink()
        except (FileNotFoundError, NotADirectoryError):
            pass  # it holds no final.elf, or it isn't a directory
        except OSError as error:
            problems.append(f'{error.filename}: {error.strerror}')
    if problems:
        fail(*problems)


def carries(path):
    """Whether the input at path is an object that carries GCC's
    intermediate code, as one compiled with -flto does: a .gnu.lto_
    section. It fails as elf.opened() does."""
    found = elf.opened(path)
    return (len(found) == 1 and found[0][0] is None
            and any(section.name.startswith('.gnu.lto_') for section in found[0][1].sections))


def named_data(path):
    """The C name of each datum the object at path defines by a global or
    weak name, in its symbols' order. It fails as elf.opened() does."""
    return [symbol.name[1:] for _, obj in elf.opened(path) for symbol in obj.symbols
            if symbol.kind == elf.OBJECT_SYMBOL and symbol.bind in (elf.GLOBAL, elf.WEAK)
            and symbol.section != elf.UNDEFINED and symbol.name.startswith('_')]


def whole(images, roots, out, flags):
    """images as GNU ld links them, by name, once GCC's link-time compile
    has compiled each image's intermediate code whole, as gcclink.py's
    link_time() gives them. roots gives each image's roots, as
    gcclink.rooted() does, and flags are the compile's, config/gcc.mk's
    GCC_LTO_LINK, or None for a link with no link-time compile.

    It empties each image's out/IMAGE/lto/ first, so the directory holds
    this run's compile or nothing, and a link with no compile leaves no
    record of an earlier one for tools/verify.py to read. Given flags, an
    image's compile takes each of its objects that carries the code, and
    links all of its inputs once, under out/IMAGE/lto/, by the script and
    the import values of its layout pass, so that each object's code lies
    as in the pass. The script is the pass's without the image's starts
    row, which moves no code: an image without room for its starts then
    fails in its layout pass, which names that room, as room() says, and
    not here. Its roots, its entry, each anchor's SYMBOL,
    CALLED_BY_GCC and each datum those objects define by a global name, as
    named_data() gives them, root that link: a debugger reads and writes
    the game's data by name, so GCC keeps each such datum and each store to
    it, and kept() holds the passes to that. So GCC keeps each
    definition that another image, a root or any other input takes, and
    inlines, clones or drops one that only those objects take.
    -flto-partition=1to1 has GCC write the code it keeps of each object to
    an object of its own: GCC names each as -Q has it print its partitions.
    The compile moves each to its object's own path under out/IMAGE/lto/,
    and records them in objects.txt there. The ELF it links is no pass's,
    and goes with GCC's other files. whole.log keeps what GCC and ld print.

    An object a kernel, warm, data, last or cold row names keeps its own
    code, which the row places, so make dc compiles it without the
    intermediate code. One that carries it is older than its row, and
    fails the link.

    It fails before any image's compile runs, on every such problem of
    every image at once, each naming its image:
    - on such an object, naming it and its row's kind;
    - on an object that carries the code and whose path is absolute or
      holds .., since its link-time object would lie outside
      out/IMAGE/lto/;
    - on an out that is no PATH of the record's rows, as gcclink.py's
      taken() reads them, where an image has a compile to record.

    It then fails at the first image, naming it:
    - where the compile fails, or where whole.log holds a warning, each of
      which it prints;
    - where GCC writes code of an object the compile didn't take, or two
      objects of one;
    - where GCC writes an object that whole.log names for no object, which
      would otherwise leave the link with the code it holds.

    It fails naming a directory it can't empty or make, or a file it can't
    write or remove, and as gcclink.link_time() does."""
    gcc = (gcclink.SH_ELF / 'bin/sh-elf-gcc').resolve()
    carried, problems = {}, []
    for name, image in images.items():
        directory = out / name / gcclink.LINK_TIME
        try:
            if directory.exists():
                shutil.rmtree(directory)
        except OSError as error:
            fail(f'{error.filename}: {error.strerror}')
        carried[name] = [path for path in image.inputs if carries(path)] if flags is not None else []
        # each object a row of the kernel table names, by its path, with
        # the kind of its first such row
        rowed = {}
        for kind, paths in (('kernel', [piece.path for piece in image.kernel]),
                            ('warm', [piece.path for piece in image.warm]),
                            ('data', [piece.path for piece in image.data]), ('last', image.last),
                            ('cold', image.cold)):
            for path in paths:
                rowed.setdefault(path, kind)
        problems += [f"{name}: {path} carries GCC's intermediate code, and a {rowed[path]} row names it, whose "
                     f'object keeps its own code; remove {path}, and make dc compiles it again'
                     for path in carried[name] if path in rowed]
        problems += [f"{name}: {path} carries GCC's intermediate code, and its path is absolute or holds .., so "
                     f'its link-time object has no place under {directory}/; name it by a path from the working '
                     'directory' for path in carried[name] if Path(path).is_absolute() or '..' in Path(path).parts]
    if any(carried.values()) and not gcclink.PATH.fullmatch(str(out)):
        problems.append(f'{out}: the link-time compile records each object it writes by its path under the '
                        f"directory, in each image's {gcclink.LINK_TIME}/{gcclink.OBJECTS}, and a path there holds "
                        'letters, digits and _ . / - alone; link under a directory named so')
    if problems:
        fail(*problems)
    for name, image in images.items():
        taken = carried[name]
        if not taken:
            continue
        directory = out / name / gcclink.LINK_TIME
        try:
            directory.mkdir(parents=True)
        except OSError as error:
            fail(f'{error.filename}: {error.strerror}')
        # each datum the objects define by a global name, which a debugger
        # reads and writes by that name
        data = [datum for path in taken for datum in named_data(path)]
        assigned = directory / PASSES['layout']
        save(assigned, imports_text(image, provisional(images, image), 'layout').encode())
        save(directory / 'whole.ld', script_text(image._replace(starts=[]), list(dict.fromkeys([*roots[name], *data])),
                                                 assigned, 'link-time', {}).encode())
        entry = f'_{image.entry}' if image.entry else hexa(image.origin)
        command = [gcc, *flags, '-nostdlib', '-Q', '-save-temps', '-o', directory / 'whole.elf',
                   f'-Wl,-T,{directory / "whole.ld"}', f'-Wl,-e,{entry}', '-Wl,--no-warn-rwx-segments',
                   *(f'-Wl,-u,_{symbol}' for symbol in dict.fromkeys(
                       [*CALLED_BY_GCC, *(anchor.symbol for anchor in image.anchors)])), *image.inputs]
        try:
            done = run_child(list(map(str, command)), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                             errors='replace', env={**os.environ, 'LC_ALL': 'C'})
        except OSError as error:
            fail(f"{gcc}: can't run it: {error.strerror}")
        log = directory / 'whole.log'
        save(log, done.stdout.encode())
        if done.returncode:
            fail(f'{name}: its link-time compile failed, as {log} says')
        # -Werror, which GCC_LTO_LINK gives, fails the compile on a warning
        # of GCC's own, and its -Wa,--fatal-warnings on one of GNU as's. One
        # that the linker or the driver's wrapper prints, or GCC or GNU as
        # under other flags, leaves its status 0
        warned = [line for line in done.stdout.splitlines() if WARNING.search(line)]
        if warned:
            fail(*(f'{name}: {line}' for line in warned),
                 f'{name}: its link-time compile warns, as {log} says, and a warning fails it, as it fails each compile')
        made = {}
        for written, path in re.findall(r' (\S+)\.o \((\S*) \d+ insns\)', done.stdout):
            if path not in taken:
                fail(f'{name}: its link-time compile wrote code of {path or "no object"}, which it took from no '
                     f'object of its own, as {log} says')
            if path in made:
                fail(f'{name}: its link-time compile wrote two objects of {path}, as {log} says, and a link-time '
                     "object takes its object's place alone")
            made[path] = directory / path
            try:
                made[path].parent.mkdir(parents=True, exist_ok=True)
                os.replace(f'{written}.ltrans.o', made[path])
            except OSError as error:
                fail(f'{error.filename}: {error.strerror}')
        # an object GCC wrote that no line of -Q's claims: a line the pattern
        # above misses would otherwise read as an object GCC kept nothing of
        unclaimed = sorted(directory.glob('whole.elf*.ltrans.o'))
        if unclaimed:
            fail(*(f'{name}: its link-time compile wrote {each}, which {log} names for no object, so its code '
                   "would leave the link" for each in unclaimed))
        save(directory / gcclink.OBJECTS, ''.join(f'{path} {made.get(path, "-")}\n' for path in taken).encode())
        remove(sorted(directory.glob('whole.elf*')))
    return gcclink.link_time(images, out)


def kept(out, layouts):
    """The problems of the data the link-time compile roots, once every
    layout pass has run: one for each datum that an object the compile took
    defines by a global name, as named_data() gives them, and that its
    image's layout pass, of layouts, doesn't define. The compile's script
    roots each such name, so GCC drops none while the root holds, and a
    pass that lacks one links an image whose datum no debugger finds. The
    pass reads the image as linked, so a weak datum that another input
    defines counts as defined. The layout check holds the final pass to
    the layout pass, and so to this.

    It fails as gcclink.taken() and elf.opened() do."""
    problems = []
    for name, layout in layouts.items():
        defined = gcclink.exports(layout.elf)
        script = out / name / gcclink.LINK_TIME / 'whole.ld'
        problems += [f'{name}: {path} defines {datum}, a datum of a global name, which {script} roots for its '
                     f"link-time compile, and {layout.elf.where} doesn't define it: GCC dropped a datum a debugger "
                     'reads by that name'
                     for path in gcclink.taken(out, name) for datum in dict.fromkeys(named_data(path))
                     if datum not in defined]
    return problems


def coordinate(images, out, flags=None):
    """Link images, the spec's, under out, and hold each final pass to its
    layout pass. It returns the import values, layout passes and final
    passes, each by image, as resolve() and link() give them. Every layout
    pass runs, then across()'s check, stale()'s of each image and kept()'s,
    then resolve(), then every final pass, then the layout check.

    So that the final passes in out are one run's, coordinate() removes
    the final.elf of each directory in out, as clear() does, before the
    first pass. It removes them again where anything raises, as a failure,
    ^C, or a signal that stoppable() turns into SystemExit does. A run that
    stops part way then leaves no final.elf of an earlier run beside its
    own. Where clear() fails then, coordinate() writes its problems to
    stderr. It then raises what stopped the run. whole() runs first,
    with flags, the link-time compile's, or None for no compile, and the
    passes link what it gives.

    It fails as clear(), whole(), provisional(), link() and resolve() do. It
    fails on every problem across(), stale() and kept() give, at once, and
    then on every problem the layout check finds, at once."""
    clear(out)
    try:
        roots = gcclink.rooted(images)
        images = whole(images, roots, out, flags)
        layouts = {name: link(image, roots[name], provisional(images, image), out, 'layout')
                   for name, image in images.items()}
        problems = across(images, layouts)
        problems += [problem for name, image in images.items() for problem in stale(image, layouts[name])]
        problems += kept(out, layouts)
        if problems:
            fail(*problems)
        bound = resolve(images, layouts)
        finals = {name: link(image, roots[name], bound[name], out, 'final') for name, image in images.items()}
        problems = moved(layouts, finals)
        if problems:
            fail(*problems)
    except BaseException:
        try:
            clear(out)
        except SystemExit as left:
            print(left, file=sys.stderr)
        raise
    return bound, layouts, finals


def main(argv):
    parser = argparse.ArgumentParser(prog='ld', description=__doc__.split('\n\n')[0])
    parser.add_argument('spec', metavar='SPEC', help='the images, their inputs and their imports, a row each')
    parser.add_argument('out', metavar='DIR', type=Path, help='the directory the passes write under, DIR/IMAGE/')
    parser.add_argument('--lto', metavar='FLAGS', help="the link-time compile's flags, config/gcc.mk's GCC_LTO_LINK, "
                                                       'as one word: --lto=FLAGS')
    args = parser.parse_args(argv)
    clear(args.out)
    coordinate(gcclink.read(args.spec), args.out, None if args.lto is None else args.lto.split())


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
