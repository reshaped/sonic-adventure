import struct
import unittest

import elf
import test_ld

# the fields of an ELF section header, each a 32-bit word, in order
SECTION_HEADER = ('sh_name', 'sh_type', 'sh_flags', 'sh_addr', 'sh_offset', 'sh_size', 'sh_link', 'sh_info',
                  'sh_addralign', 'sh_entsize')


class Read(test_ld.Fixture):
    def test_refuses_an_object_whose_headers_name_what_it_doesn_t_hold(self):
        # main.o, changed in each of five ways. .data's header puts its bytes past the file's end. .strtab's header
        # cuts it to its first byte, so each named symbol's name runs past it. Or .strtab's header makes it NOBITS,
        # with no bytes in the file. The second list of relocations applies to the section the first does. The first
        # relocation is against the symbol past the last
        data = (self.root / 'main.o').read_bytes()
        found = elf.read(data, 'main.o')
        # the ELF header's e_shoff and e_shentsize: where the section headers lie, and the size of each
        *_, at, _, _, _, _, size, _, _ = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
        lists = [section.index for section in found.sections if section.kind == elf.RELA]
        named = {section.name: section.index for section in found.sections}

        def header(index, field):
            # the file offset of field, one of SECTION_HEADER, in section index's header
            return at + size * index + SECTION_HEADER.index(field) * 4

        def put(offset, value):
            return data[:offset] + struct.pack('<I', value) + data[offset + 4:]

        # the file offset of the first list of relocations, and the section it applies to
        start, first = (struct.unpack_from('<I', data, header(lists[0], field))[0]
                        for field in ('sh_offset', 'sh_info'))
        cases = {'.data past the end': put(header(named['.data'], 'sh_offset'), len(data) - 1),
                 '.strtab cut short': put(header(named['.strtab'], 'sh_size'), 1),
                 '.strtab with no bytes in the file': put(header(named['.strtab'], 'sh_type'), elf.NOBITS),
                 'two lists for one section': put(header(lists[1], 'sh_info'), first),
                 'a relocation against the symbol past the last':
                     put(start + 4, len(found.symbols) << 8 | elf.DIR32)}
        for case, mutated in cases.items():
            with self.subTest(case), self.assertRaisesRegex(SystemExit, "main.o: not an ELF file of GNU as's or ld's$"):
                elf.read(mutated, 'main.o')


class Symbols(unittest.TestCase):
    def test_names_a_definition_by_a_symbol_s_section_and_type(self):
        # a symbol in a section, an absolute one, an external reference, and a section's and a source file's symbols
        for section, kind, defines in ((1, 0, True), (elf.ABSOLUTE, 0, True), (elf.UNDEFINED, 0, False),
                                       (1, elf.SECTION_SYMBOL, False), (elf.ABSOLUTE, elf.FILE_SYMBOL, False)):
            with self.subTest(section=section, kind=kind):
                symbol = elf.Symbol('_f', 0, 0, section, kind, elf.GLOBAL)
                self.assertEqual(elf.names_definition(symbol), defines)


class Members(unittest.TestCase):
    def test_refuses_an_archive_that_holds_two_members_of_one_name(self):
        # a member as GNU ar writes one: a header of 60 bytes, its name at 0 and its size at 48, then its bytes
        member = b'a.o/'.ljust(48) + b'4'.ljust(10) + b'`\n' + b'\x7fELF'
        with self.assertRaisesRegex(SystemExit, 'two.a: holds two members named a.o$'):
            elf.members(elf.ARCHIVE + member * 2, 'two.a')

    def test_refuses_an_archive_cut_short_or_with_a_header_gnu_ar_doesn_t_write(self):
        # each case's member breaks what its name says. A size of -60, a header's length, would take the reader back to
        # its own header
        cases = {'a member cut short': b'a.o/'.ljust(48) + b'10'.ljust(10) + b'`\n' + b'\x7fELF',
                 'a header of another end': b'a.o/'.ljust(48) + b'4'.ljust(10) + b'XX' + b'\x7fELF',
                 'a size of -60': b'/'.ljust(48) + b'-60'.ljust(10) + b'`\n'}
        for case, member in cases.items():
            with self.subTest(case), self.assertRaisesRegex(SystemExit, "bad.a: not an archive of GNU ar's$"):
                elf.members(elf.ARCHIVE + member, 'bad.a')


if __name__ == '__main__':
    unittest.main()
