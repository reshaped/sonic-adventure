import re
import unittest
from pathlib import Path
from unittest import mock

import csource
import memory
from config import Config, configs
from images import FILES, images
from support import GAME_TREE, cwd, tree
from tables import aligned
from tool import content

ROOT = Path(__file__).resolve().parents[1]


def table_extents(terms):
    """Each image's Extent at the bounds its tables give, by name. Its end, its file's end and its sectors' end are all
    the end Config.span() gives it. Where its tables give CFISH, the first section of the fish models, as stg08's do,
    its body ends where CFISH starts. Its packed is None. Where terms holds work past the image, the work starts at the
    image's end rounded up to WORK_ALIGNMENT. The work is as long as terms holds it."""
    found = {}
    for config in configs():
        _, high = config.span()
        body = next((start for name, start, _ in config.layout() if name == 'CFISH'), high)
        start = aligned(high, memory.WORK_ALIGNMENT)
        work = (start, start + terms.work[config.name]) if config.name in terms.work else None
        found[config.name] = memory.Extent(high, body, high, high, None, work)
    return found


# memory.py's names for Icecap, with stga for its image and a.c for both of its sources
ICECAP = {'FISH_IMAGE': 'stga', 'FISH_MODELS': 'modules/stga/a.c', 'ICECAP_C': Path('modules/stga/a.c')}


class Contract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with cwd(ROOT):
            cls.terms = memory.contract(images())

    def test_holds_the_images_it_takes_each_once(self):
        # an iterator of the images gives the same contract as their list. Without ADVERTISE, the menus' state holds
        # neither it nor its work. An image config/ doesn't describe fails, and so does one named twice.
        with cwd(ROOT):
            self.assertEqual(memory.contract(iter(images())), self.terms)
            terms = memory.contract([image for image in images() if image != 'advertise'])
            for named, problem in (([*images(), 'stgz'], 'stgz: no image config/ describes it'),
                                   ([*images(), 'stg01'], 'stg01: images names it twice, and the memory contract takes '
                                                          'each image once')):
                with self.subTest(problem=problem), self.assertRaisesRegex(SystemExit, f': {problem}$'):
                    memory.contract(named)
        menus = {state.name: state for state in terms.states}['the menus']
        self.assertEqual((menus.images, menus.areas), ({'1st_read': 'whole'}, ['stack', 'heap', 'work', 'stream work']))

    def test_reads_each_area_from_the_source_that_states_it(self):
        areas = {name: (owner.start, owner.end) for name, owner in self.terms.areas.items()}
        # each area as its source gives it: loader.h's work area; strt2.c's stack; sbinit.c's heap, RAM's last
        # megabyte; play.c's zeroing; and loader.h's first camera area, of 16 KB
        self.assertEqual(areas['work'], (0x0CD00000, 0x0CF00000))
        self.assertEqual(areas['stack'], (0x0C00C000, 0x0C00F400))
        self.assertEqual(areas['heap'], (0x0CF00000, 0x0D000000))
        self.assertEqual(areas['event zeroing'], (0x0CB80000, 0x0CBC8000))
        self.assertEqual(areas['CAMERA_AREA_0'], (0x0CB50000, 0x0CB54000))
        # Station Square's messages start at field_messages, below the camera areas, and take SS_MES_S.BIN's 29,580
        # bytes in 15 whole sectors. draw_lights, a name the link places past 1ST_READ.BIN's image, is no anchor.
        self.assertEqual(areas['adv00 field_messages'], (0x0CB48000, 0x0CB4F800))
        self.assertEqual(self.terms.anchored['adv00'], {'field_messages': 0x0CB48000})
        self.assertNotIn('1st_read draw_lights', areas)
        # download.c's two model buffers of 8 KB each, then stream.c's music and voice work
        self.assertEqual(areas['download models'], (0x0CCA4000, 0x0CCA8000))
        self.assertEqual(areas['stream work'], (0x0CCA8000, 0x0CCE2948))
        # the gardens' SAMP_LCD.PRS, read to SET_AREA_1, has room up to EXTRA_AREA, where each garden lies. Sky Chase's
        # path files, read to EXTRA_AREA, have room up to MOTION_AREA.
        self.assertEqual(areas['SET_AREA_1 file'], (0x0CB70000, 0x0CB80000))
        self.assertEqual(areas['EXTRA_AREA file'], (0x0CB80000, 0x0CC00000))
        # the movie's work is the 3 MB from MODULE_AREA. Its cast's read takes EV_TR2_BIG.PRS's 248,920 bytes in 122
        # whole sectors, up to WORK_AREA.
        self.assertEqual(areas['movie work'], (0x0C900000, 0x0CC00000))
        self.assertEqual(areas['cast room'], (0x0CCC3000, 0x0CD00000))

    def test_fails_on_a_macro_its_source_doesn_t_state(self):
        files = {str(path): '' for path in memory.MACROS}
        with tree(files), self.assertRaisesRegex(SystemExit, 'src/system/loader.h: gives no number for SECTOR_SIZE, '
                                                             'which the memory contract reads'):
            memory.numbers()

    def test_gives_each_image_the_interval_to_the_first_owner_past_it(self):
        # ADVERTISE's interval ends at DEMO_RECORDING, where demo_advance can read a recording as a demo starts from
        # the menus. SUMMARY's ends at the stream work, which its music plays from. b_chaos0's ends at CAMERA_AREA_0,
        # which its case sets dest to for its camera read.
        ends = {image: self.terms.origins[image] + self.terms.lengths[image] for image in self.terms.origins}
        self.assertEqual({image: ends[image] for image in ('1st_read', 'stg01', 'adv00', 'adv01obj', 'adv03', 'sboard',
                                                           'movie', 'loader', 'ev0001', 'advertise', 'summary',
                                                           'b_chaos0')},
                         {'1st_read': 0x8C900000, 'stg01': 0x0CB50000, 'adv00': 0x0CB48000, 'adv01obj': 0x0C920000,
                          'adv03': 0x0CB00000, 'sboard': 0x0CADC000, 'movie': 0x8CF00000, 'loader': 0x8CF00000,
                          'ev0001': 0x0CC00000, 'advertise': 0x8CBE0000, 'summary': 0x8CCA8000, 'b_chaos0': 0x0CB50000})
        # a motion file's interval ends at download.c's model buffers, and each board motion file's at the next area
        self.assertEqual({image: ends[image] for image in ('s_mot', 'b_mot', 's_sbmot', 'm_sbmot')},
                         {'s_mot': 0x0CCA4000, 'b_mot': 0x0CCA4000, 's_sbmot': 0x0CB50000, 'm_sbmot': 0x0CB08000})

    def test_takes_the_states_and_who_is_live_with_whom_from_the_loader_s_transitions(self):
        states = {state.name: state for state in self.terms.states}
        self.assertEqual(set(states['stage_chaos7'].images), {'1st_read', 'b_chaos7', 'tikal_prog'})
        # an event loads where a garden does, so no event is live with one
        self.assertFalse(states['stage_alife_ss'].event)
        self.assertIn('stg01', self.terms.colive['ev0001'])
        self.assertNotIn('al_garden00', self.terms.colive['ev0001'])
        self.assertNotIn('ev0002', self.terms.colive['ev0001'])
        # in the hand-off, LOADER.BIN is read and not yet started, and at a demo's start, ADVERTISE has stopped. So the
        # two states hold their images apart, and LOADER.BIN is live with no image, though main takes the hand-off's
        # two names from it.
        self.assertEqual([state.name for state in self.terms.states if state.apart], ["a demo's start", 'the hand-off'])
        self.assertEqual(states['the hand-off'].images, {'1st_read': 'whole', 'loader': 'file'})
        self.assertNotIn('loader', self.terms.colive['1st_read'])
        self.assertEqual(self.terms.colive['loader'], set())
        # Icecap's board motions load over its tail, so there its body is live
        self.assertEqual(states['stage_icecap with the board motions'].images,
                         {'1st_read': 'whole', 'stg08': 'body', 's_sbmot': 'whole', 'm_sbmot': 'whole'})
        self.assertEqual(set(states['stage_sunaboard'].images), {'1st_read', 'sboard', 's_sbmot', 'm_sbmot'})

    def test_holds_the_movie_s_work_and_its_cast_s_read_beside_movie_bin(self):
        # 1ST_READ.BIN running into the movie's work fails
        extents = table_extents(self.terms)
        extents['1st_read'] = extents['1st_read']._replace(end=0x8C900004)
        self.assertIn('in the movie: 1st_read, 0x0c010000 to 0x0c900004, overlaps movie work, 0x0c900000 to 0x0cc00000',
                      memory.check(self.terms, extents))
        # the cast's read ends at WORK_AREA, so MOVIE.BIN placed there passes, and placed a sector lower fails. A room
        # rounded to 256 bytes, as the shipped texture.c gives a file, would run the read up to 1792 bytes past
        # WORK_AREA.
        for origin, problems in ((0x8CD00000, []), (0x8CCFF800, ['in the movie: cast room, 0x0ccc3000 to 0x0cd00000, '
                                                                 "overlaps movie's read, 0x0ccff800 to 0x0cd0f800"])):
            with self.subTest(origin=hex(origin)):
                terms = self.terms._replace(origins={**self.terms.origins, 'movie': origin})
                extents = table_extents(self.terms)
                extents['movie'] = memory.Extent(*[origin + 0x10000] * 4, None, None)
                self.assertEqual(memory.check(terms, extents), problems)

    def test_holds_movie_bin_to_the_end_of_the_sectors_its_read_writes(self):
        # movie_play reads MOVIE.BIN in whole sectors, then runs it. A MOVIE.BIN whose last sector ends where the heap
        # starts passes. One whose last sector runs over the heap fails, in either view, and so does one whose end
        # does, which runs past its interval too.
        overlap = ["in the movie: movie's read, 0x0ceb0000 to 0x0cf00800, overlaps heap, 0x0cf00000 to 0x0d000000"]
        past = 'movie: it runs to 0x8cf00800, past its interval, which ends at 0x8cf00000'
        for end, sectors, problems in ((0x8CEFF000, 0x8CF00000, []), (0x8CEFF000, 0x8CF00800, overlap),
                                       (0x8CEFF000, 0x0CF00800, overlap), (0x8CF00800, 0x8CF00000, [past, *overlap])):
            with self.subTest(end=hex(end), sectors=hex(sectors)):
                extents = table_extents(self.terms)
                extents['movie'] = extents['movie']._replace(end=end, sectors=sectors)
                self.assertEqual(memory.check(self.terms, extents), problems)

    def test_holds_a_character_s_motion_file_any_one_in_each_stage_s_state(self):
        # loader_read_character_motions reads each to MOTION_AREA
        self.assertEqual(self.terms.motions, ['a_mot', 'b_mot', 'e_mot', 'k_mot', 's_mot'])
        # a demo's start holds one too: demo_advance has set_character read it as a demo starts from the menus
        states = {state.name: state for state in self.terms.states}
        self.assertTrue(states['stage_beach'].motions)
        self.assertFalse(states['the menus'].motions)
        self.assertTrue(states["a demo's start"].motions)
        # each is live with each stage's images and each event. None is live with another, as one character plays. None
        # is live with ADVERTISE either: a demo's start holds both, but ADVERTISE has stopped there.
        self.assertIn('s_mot', self.terms.colive['b_chaos7'])
        self.assertIn('ev0001', self.terms.colive['k_mot'])
        self.assertIn('k_mot', self.terms.colive['ev0001'])
        self.assertNotIn('s_mot', self.terms.colive['k_mot'])
        self.assertNotIn('advertise', self.terms.colive['s_mot'])

    def test_holds_a_motion_file_that_is_no_image_as_one_more_character_s(self):
        # with Tails' motions in t_mot.PRS, which is no image, its room becomes one more of the motion files a stage's
        # state may hold one of
        loader = content(ROOT / memory.LOADER_C).decode('latin-1').replace(
            '    case CHARACTER_SONIC:\n    case CHARACTER_TAILS:\n'
            '        loader_read_prs("s_mot.PRS", MOTION_AREA);\n',
            '    case CHARACTER_SONIC:\n        loader_read_prs("s_mot.PRS", MOTION_AREA);\n        break;\n'
            '    case CHARACTER_TAILS:\n        loader_read_prs("t_mot.PRS", MOTION_AREA);\n')
        with cwd(ROOT), mock.patch.object(csource, 'content', lambda path: loader.encode('latin-1')
                                            if Path(path) == memory.LOADER_C else content(path)):
            terms = memory.contract(images())
            self.assertEqual(terms.motions, ['MOTION_AREA file', 'a_mot', 'b_mot', 'e_mot', 'k_mot', 's_mot'])
            self.assertEqual(memory.check(terms, table_extents(terms)), [])

    def test_fails_on_a_motion_file_past_its_interval_or_over_the_areas_past_it(self):
        # Amy's or Big's file one byte into the model buffers runs past its interval. It overlaps them in a stage's
        # state, and in Icecap's with the board motions too.
        for motion in ('a_mot', 'b_mot'):
            with self.subTest(motion=motion):
                extents = table_extents(self.terms)
                extents[motion] = memory.Extent(0x0CCA4001, 0x0CCA4001, 0x0CCA4001, 0x0CCA4001, None, None)
                found = memory.check(self.terms, extents)
                self.assertIn(f'{motion}: it runs to 0x0cca4001, past its interval, which ends at 0x0cca4000', found)
                for state in ('stage_beach', 'stage_icecap with the board motions'):
                    self.assertIn(f'in {state}: {motion}, 0x0cc00000 to 0x0cca4001, overlaps download models, '
                                  '0x0cca4000 to 0x0cca8000', found)

    def test_anchors_each_record_where_the_original_holds_it(self):
        # in the original, each record's source has a range that starts its offset past the image's base. The tables
        # give the record's symbol that address, but for a _startof_ symbol, which the link defines.
        with cwd(ROOT):
            for image in sorted(set(memory.RECORDS) | set(self.terms.events)):
                config = Config(image)
                table = {name: address for address, name in memory.named(config.path / 'symbols.txt')}
                for section, source, _, symbol, offset, _ in memory.records(image, self.terms.events):
                    with self.subTest(image=image, source=source, section=section):
                        self.assertIn((section, config.base + offset), [(name, start) for name, start, _
                                                                        in config.ranges(source)])
                        if symbol != f'_startof_{section}':
                            self.assertEqual(table[symbol], config.base + offset)

    def test_holds_for_extents_at_the_tables_bounds(self):
        with cwd(ROOT):
            self.assertEqual(memory.check(self.terms, table_extents(self.terms)), [])

    def test_fails_on_an_image_past_its_interval(self):
        extents = table_extents(self.terms)
        for end, problems in ((0x0CB50000, []), (0x0CB50001, ['stg01: it runs to 0x0cb50001, past its interval, which '
                                                               'ends at 0x0cb50000'])):
            with self.subTest(end=hex(end)):
                extents['stg01'] = memory.Extent(end, end, end, end, None, None)
                found = memory.check(self.terms, extents)
                self.assertEqual([problem for problem in found if not problem.startswith('in ')], problems)

    def test_fails_on_owners_a_state_holds_that_overlap(self):
        # each owner one byte into its neighbor fails, and each that meets its neighbor passes. stg08's body meets
        # Tails' board motions in Icecap's state with the board motions, and in its recorded demo's. LOADER.BIN's file
        # in whole sectors meets main's heap in the hand-off. ADVERTISE's backup work meets ADVERTISE in the menus'
        # state and at a demo's start.
        cases = (('stg08', lambda end, short: memory.Extent(0x0CB00000, 0x0CADC000 + short, 0x0CB00000, 0x0CB00000,
                                                                 None, None),
                  [f"in {state}: stg08's body, 0x0c900000 to 0x0cadc001, overlaps m_sbmot, 0x0cadc000 to 0x0cb04574"
                   for state in ('stage_icecap with the board motions',
                                 'a recorded demo of stage_icecap with the board motions')]),
                 ('loader', lambda end, short: memory.Extent(end, end, end, 0x8CF00000 + short, None, None),
                  ["in the hand-off: loader's file, 0x0ce00000 to 0x0cf00001, overlaps heap, 0x0cf00000 to "
                   '0x0d000000']),
                 ('advertise', lambda end, short: memory.Extent(end, end, end, end, None,
                                                                    (end - short, end - short + 0x7A100)),
                  [f'in {state}: advertise, 0x0c900000 to 0x0c9c5b30, overlaps advertise work, 0x0c9c5b2f to '
                   '0x0ca3fc2f' for state in ('the menus', "a demo's start")]))
        for image, extent, problems in cases:
            for short in (0, 1):
                with self.subTest(image=image, past=short):
                    extents = table_extents(self.terms)
                    extents[image] = extent(extents[image].end, short)
                    found = memory.check(self.terms, extents)
                    self.assertEqual(found, problems if short else [])

    def test_fails_on_an_extent_outside_ram_or_out_of_order(self):
        # each of these fails alone, as no owner a state holds lies there: stg01's end below its origin, its sectors
        # past RAM's end, and ADVERTISE's backup work at RAM's end
        for image, change, problem in (
                ('stg01', {'end': 0x0C8F0000, 'body': 0x0C8F0000}, "stg01: its body ends at 0x0c8f0000 and it ends "
                                                                   "at 0x0c8f0000, which don't run in that order from "
                                                                   "its origin, 0x0c900000, to RAM's end, 0x0d000000"),
                ('stg01', {'sectors': 0x0D000800}, "stg01: its file ends at 0x0cadd4f4 and its sectors at 0x0d000800, "
                                                   "which don't run in that order from its origin, 0x0c900000, to "
                                                   "RAM's end, 0x0d000000"),
                ('advertise', {'work': (0x8D000000, 0x8D07A100)},
                 "advertise: its work starts at 0x8d000000 and ends at 0x8d07a100, which don't run in that order "
                 'within RAM, 0x0c000000 to 0x0d000000')):
            with self.subTest(problem=problem):
                extents = table_extents(self.terms)
                extents[image] = extents[image]._replace(**change)
                self.assertEqual(memory.check(self.terms, extents), [problem])

    def test_fails_on_extents_that_leave_out_an_image_or_give_one_the_contract_doesn_t_hold(self):
        # extents that leave out the events, or give an image the contract doesn't hold, fail on that alone. stg08's
        # body running into the board motions goes unreported.
        extents = {name: extent for name, extent in table_extents(self.terms).items() if not name.startswith('ev')}
        extents['stg08'] = extents['stg08']._replace(body=0x0CADC001)
        self.assertEqual(memory.check(self.terms, extents),
                         [f'{event}: extents give it no Extent, and the memory contract holds it'
                          for event in self.terms.events])
        extents = {**table_extents(self.terms), 'stgz': table_extents(self.terms)['stg01']}
        self.assertEqual(memory.check(self.terms, extents),
                         ["stgz: extents give it an Extent, and the memory contract doesn't hold it"])
        self.assertEqual(memory.check(self.terms, {}), [f'{image}: extents give it no Extent, and the memory contract '
                                                        'holds it' for image in self.terms.origins])

    def test_holds_each_recording_where_its_demo_plays_and_the_casino_s_palette(self):
        # demos[] plays recorded pads in six stages. Each state of each one's case comes again with DEMO_RECORDING and
        # no event. A demo's start holds the recording too, which demo_advance reads with ADVERTISE resident. Big's demo
        # of Beach holds it again as event 212 loads. Casinopolis' state holds SET_AREA_4, where casino_act_start reads
        # PL_9MB.BIN.
        states = {state.name: state for state in self.terms.states}
        recorded = [f'a recorded demo of stage_{stage}' for stage in ('beach', 'highway', 'icecap',
                                                                     'icecap with the board motions', 'mountain',
                                                                     'shelter', 'windy')]
        self.assertEqual(sorted(name for name, state in states.items() if 'DEMO_RECORDING' in state.areas),
                         ["a demo's start", *recorded[:1], 'a recorded demo of stage_beach as ev00d4 loads',
                          *recorded[1:]])
        self.assertEqual([name for name in recorded if states[name].event], [])
        self.assertEqual(self.terms.areas['DEMO_RECORDING'], memory.Owner('DEMO_RECORDING', 0x0CBE0000, 0x0CC00000))
        self.assertIn('SET_AREA_4', states['stage_casino'].areas)
        # the menus' backup work over the recording fails at a demo's start
        extents = table_extents(self.terms)
        extents['advertise'] = extents['advertise']._replace(work=(0x8CB65F01, 0x8CBE0001))
        self.assertEqual(memory.check(self.terms, extents), ["in a demo's start: advertise work, 0x0cb65f01 to "
                                                             '0x0cbe0001, overlaps DEMO_RECORDING, 0x0cbe0000 to '
                                                             '0x0cc00000'])

    def test_fails_on_work_an_extent_leaves_out_or_sizes_otherwise(self):
        # the menus and a demo's start hold the backup work past ADVERTISE. So its extent must give the work, as long as
        # the contract holds it. Work left out fails, and so does work a byte long.
        for work, problems in ((None, ['advertise: its extent gives no work, and the memory contract holds 0x7a100 '
                                       'bytes of work past it']),
                               ((0x8CBD0000, 0x8CBD0001), ['advertise: its work, 0x8cbd0000 to 0x8cbd0001, takes 0x1 '
                                                           'bytes, and the memory contract holds 0x7a100'])):
            with self.subTest(work=work):
                extents = table_extents(self.terms)
                extents['advertise'] = extents['advertise']._replace(work=work)
                self.assertEqual(memory.check(self.terms, extents), problems)

    def test_holds_a_recording_with_each_event_its_demo_can_load_as_the_pads_play(self):
        # clear_events gives Big's Beach act 2 event 212, which stage_clear_event can load during his demo if the act
        # clears. That state holds EV00D4 whole beside the recording, so the recording ends EV00D4's interval. EV00D4
        # running over the recording fails in that state alone.
        states = {state.name: state for state in self.terms.states}
        loading = states['a recorded demo of stage_beach as ev00d4 loads']
        self.assertEqual(loading, states['a recorded demo of stage_beach']._replace(
            name='a recorded demo of stage_beach as ev00d4 loads',
            images={**states['stage_beach'].images, 'ev00d4': 'whole'}))
        self.assertEqual(self.terms.origins['ev00d4'] + self.terms.lengths['ev00d4'], 0x0CBE0000)
        extents = table_extents(self.terms)
        extents['ev00d4'] = memory.Extent(*[0x0CBE0001] * 4, None, None)
        self.assertEqual(memory.check(self.terms, extents), [
            'ev00d4: it runs to 0x0cbe0001, past its interval, which ends at 0x0cbe0000',
            'in a recorded demo of stage_beach as ev00d4 loads: ev00d4, 0x0cb80000 to 0x0cbe0001, overlaps '
            'DEMO_RECORDING, 0x0cbe0000 to 0x0cc00000'])

    def test_reads_each_source_it_takes_a_fact_from_as_it_builds_the_contract_and_none_after(self):
        # contract() reads no source but those SOURCES names, and stage_states() and check() read none. The contract
        # holds loader.h's SECTOR_SIZE, which publish takes its sectors from.
        read, staging = [], []
        source, states = csource.content, memory.stage_states

        def reading(path):
            read.append((Path(path), bool(staging)))
            return source(path)

        def staged(*facts):
            staging.append(True)
            try:
                return states(*facts)
            finally:
                staging.pop()

        with cwd(ROOT), mock.patch.object(csource, 'content', reading), \
                mock.patch.object(memory, 'stage_states', staged):
            terms = memory.contract(images())
            during = list(read)
            memory.check(terms, table_extents(terms))
        self.assertLessEqual({path for path, _ in during}, set(memory.SOURCES))
        self.assertEqual([path for path, inside in during if inside], [])
        self.assertEqual(read, during)
        self.assertEqual(terms.sector, 0x800)

    def test_sizes_the_work_past_an_image_s_end(self):
        # the backup library's work takes 122 blocks of 512 bytes and a 32-byte gap for each of eight drives. Each
        # fight's banks take the longer of its two files in whole sectors: EROBO.PB's 301,560 bytes and E101R.PB's
        # 308,288.
        self.assertEqual(self.terms.work, {'advertise': 0x7A100, 'b_robo': 0x4A000, 'b_e101_r': 0x4B800})

    def test_fails_on_an_event_whose_bss_runs_past_both_its_file_and_the_bytes_event_load_zeroes(self):
        # EV0034's bss runs past the 0x48000 bytes event_load zeroes, as 16 other originals' does. So its file must hold
        # its bss, or the bss starts unzeroed.
        extents = table_extents(self.terms)
        for stored, problems in ((0x0CBFE9E4, []), (0x0CBFE6B8, ['ev0034: its bss runs to 0x0cbfe9e4, past its file, '
                                                                   'which ends at 0x0cbfe6b8, and past the bytes '
                                                                   'event_load zeroes, which end at 0x0cbc8000'])):
            with self.subTest(stored=hex(stored)):
                extents['ev0034'] = extents['ev0034']._replace(stored=stored)
                self.assertEqual(memory.check(self.terms, extents), problems)
        # EV0001's bss lies inside those bytes, as most events' does, so its file may end before it
        extents = table_extents(self.terms)
        extents['ev0001'] = extents['ev0001']._replace(stored=extents['ev0001'].end - 0x100)
        self.assertEqual(memory.check(self.terms, extents), [])

    def test_holds_the_bytes_event_load_zeroes_as_the_event_s(self):
        # EV0001 ends at 0x0cb93bd0, and the bytes event_load zeroes for it run on to 0x0cbc8000. So Zero's banks, put
        # past its end over those bytes, overlap the event.
        extents = table_extents(self.terms)
        extents['b_robo'] = extents['b_robo']._replace(work=(0x0CB96000, 0x0CBE0000))
        self.assertIn('in stage_eggmanrobo: ev0001, 0x0cb80000 to 0x0cbc8000, overlaps b_robo work, 0x0cb96000 to '
                      '0x0cbe0000', memory.check(self.terms, extents))

    def test_fails_on_a_prs_file_whose_sectors_overrun_the_work_area(self):
        extents = table_extents(self.terms)
        for packed, problems in ((0x200000, []), (0x200800, ['stg01: its PRS file takes 0x200800 bytes in whole '
                                                             'sectors, and the work area loader_read_prs reads it to '
                                                             'holds 0x200000'])):
            with self.subTest(packed=hex(packed)):
                extents['stg01'] = extents['stg01']._replace(packed=packed)
                self.assertEqual(memory.check(self.terms, extents), problems)

    def test_holds_an_image_s_bss_in_its_file_where_its_original_does(self):
        # SUMMARY's constants and STG01's fish models follow their bss, so their original files hold it. STG02's and
        # B_CHAOS0's bss ends their tables.
        with cwd(ROOT):
            held = {name: memory.holds_bss(Config(name)) for name in ('summary', 'stg01', 'stg02', 'b_chaos0')}
        self.assertEqual(held, {'summary': True, 'stg01': True, 'stg02': False, 'b_chaos0': False})

    def test_reads_a_macro_through_its_casts_its_arithmetic_and_the_file_s_other_macros(self):
        # a macro's name in parentheses is its value, not a cast, and a comment is no part of an expression
        text = ('#define AREA ((Uint8 *)0x0c900000)\n#define SIZE (1 + 8 + 113)\n#define END (AREA + SIZE * 2)\n'
                '#define TEXT "a string" /* no number */\n#define VIEW ((struct view *)0x8c000000)\n'
                '#define BELOW (AREA)-1\n#define PAST ((AREA) + 4)\n#define HELD ((Uint8 *)(AREA))\n'
                '#define HALF (SIZE /* of 2 */ + 2) // 2\n')
        with tree({'a.h': text}):
            self.assertEqual(memory.macros(Path('a.h')), {'AREA': 0x0C900000, 'SIZE': 122, 'END': 0x0C900000 + 244,
                                                          'VIEW': 0x8C000000, 'BELOW': 0x0C8FFFFF, 'PAST': 0x0C900004,
                                                          'HELD': 0x0C900000, 'HALF': 124})

    def test_reads_a_macro_s_expansion_and_no_cast_that_can_change_its_number(self):
        # B expands, as C does, to 1 + 2 * 4, and C to (1 + 2) * 4. These give none: a cast to short, arithmetic on a
        # pointer to a model, and a shift by a negative count. Nor do a cast of a value past 32 bits and a macro that
        # names itself.
        text = ('#define A 1 + 2\n#define B A * 4\n#define C (A) * 4\n#define SHORT ((short)0x10000)\n'
                '#define MODEL ((struct model *)0x0c900000)\n#define NEXT (MODEL + 1)\n'
                '#define BYTE ((Uint8 *)0x0c900000 + 1)\n#define WORD ((Uint32)0x0c900000)\n#define ODD (1 << -1)\n'
                '#define WIDE ((Uint32)0x100000000)\n#define LOOP (LOOP + 1)\n#define LESS (-1)\n')
        with tree({'a.h': text}):
            self.assertEqual(memory.macros(Path('a.h')), {'A': 3, 'B': 9, 'C': 12, 'MODEL': 0x0C900000,
                                                          'BYTE': 0x0C900001, 'WORD': 0x0C900000, 'LESS': -1})


class Tree(unittest.TestCase):
    def test_fails_where_an_image_s_base_isn_t_its_loader_s_destination(self):
        # loader_read_stage_files reads stga to MODULE_AREA, and stga's tables link it 64 KB on
        splits = GAME_TREE['config/stga/splits.txt'].replace('0x0c900', '0x0c910')
        files = {**GAME_TREE, 'config/stga/splits.txt': splits, 'config/stga/symbols.txt': ''}
        with tree(files), self.assertRaisesRegex(SystemExit, ": stga: its tables link it at 0x0c910000, and loader.c's "
                                                             'loader_read_stage_files, for STAGE_A, reads it to '
                                                             '0x0c900000$'):
            memory.contract(images())

    def test_holds_each_read_of_an_image_to_its_base_and_fails_on_a_read_to_an_area_no_source_gives(self):
        # a read of an image to an area other than its base fails, though another read takes it to its base: a case
        # ahead of stga's that reads it to FIELD_AREA, and ICECAP_C reading stgb there. A motion file read to
        # ODD_AREA fails, and so do two images whose files share a name once lowercased.
        earlier = GAME_TREE['src/system/loader.c'].replace('    case STAGE_A:\n', '    case STAGE_C:\n'
                                                           '        loader_read_prs("stga.PRS", FIELD_AREA);\n'
                                                           '        return;\n    case STAGE_A:\n')
        board = (GAME_TREE['modules/stga/a.c']
                 + 'void boards(void)\n{\n    loader_read_prs("stgb.PRS", FIELD_AREA);\n}\n')
        odd = GAME_TREE['src/system/loader.c'].replace('"s_mot.PRS", MOTION_AREA', '"s_mot.PRS", ODD_AREA')
        for files, patched, problem in (
                ({'src/system/loader.c': earlier}, {}, "stga: its tables link it at 0x0c900000, and loader.c's "
                                                       'loader_read_stage_files, for STAGE_C, reads it to 0x0c920000'),
                ({'modules/stga/a.c': board}, {}, 'stgb: its tables link it at 0x0c900000, and modules/stga/a.c reads '
                                                  'it to 0x0c920000'),
                ({'src/system/loader.c': odd}, {}, "s_mot.PRS: loader.c's loader_read_character_motions, for "
                                                   'CHARACTER_SONIC, reads it to ODD_AREA, an area no source the '
                                                   'memory contract reads gives'),
                ({}, {'stgb': 'SONICADV/STGB/STGA.PRS'}, 'stga and stgb: their files are both stga.prs once '
                                                         'lowercased, and a read names one image by its file')):
            with self.subTest(problem=problem), tree({**GAME_TREE, **files}), mock.patch.dict(FILES, patched), \
                    mock.patch.multiple(memory, **ICECAP), \
                    self.assertRaisesRegex(SystemExit, f': {re.escape(problem)}$'):
                memory.contract(images())

    def test_fails_on_an_image_no_loader_the_contract_reads_reads(self):
        # no case of loader_read_stage_files reads stgb
        loader = GAME_TREE['src/system/loader.c'].replace(
            '    case STAGE_B:\n        loader_read_prs("stgb.PRS", MODULE_AREA);\n        return;\n', '')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}), \
                self.assertRaisesRegex(SystemExit, ': stgb: no loader the memory contract reads reads it$'):
            memory.contract(images())

    def test_fails_where_a_case_reads_to_extra_area_and_extra_area_isn_t_event_module(self):
        # stgb's case reads path.PRS to EXTRA_AREA, which is EVENT_MODULE, so its state leaves the event out. With
        # EXTRA_AREA moved, the contract fails. With no read to EXTRA_AREA, the two may part.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        loader_read_prs("path.PRS", EXTRA_AREA);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}):
            terms = memory.contract(images())
        self.assertFalse({state.name: state for state in terms.states}['stage_b'].event)
        moving = ('EXTRA_AREA ((Uint8 *)0x0cb80000)', 'EXTRA_AREA ((Uint8 *)0x0cb90000)')
        with tree({**GAME_TREE, 'src/system/loader.c': loader.replace(*moving)}), self.assertRaisesRegex(
                SystemExit, ": src/system/loader.c: reads path.PRS to EXTRA_AREA, 0x0cb90000, and src/event/play.c's "
                            "EVENT_MODULE, where event_load reads each event, is 0x0cb80000, so the case's state can't "
                            'leave the event out$'):
            memory.contract(images())
        with tree({**GAME_TREE, 'src/system/loader.c': GAME_TREE['src/system/loader.c'].replace(*moving)}):
            memory.contract(images())

    def test_holds_a_recording_where_its_demo_plays_and_fails_on_a_stage_no_case_reads(self):
        # demos[] plays recorded pads in stga's stage, whose state comes again with DEMO_RECORDING and no event. demos[]
        # fails where it plays them in a stage no case reads.
        with tree(GAME_TREE):
            terms = memory.contract(images())
        states = {state.name: state for state in terms.states}
        self.assertEqual(states['a recorded demo of stage_a'], states['stage_a']._replace(
            name='a recorded demo of stage_a', areas=states['stage_a'].areas + ['DEMO_RECORDING'], event=False))
        self.assertNotIn('a recorded demo of stage_b', states)
        demo = GAME_TREE['src/progress/demo.c'].replace('STAGE_A', 'STAGE_C')
        with tree({**GAME_TREE, 'src/progress/demo.c': demo}), \
                self.assertRaisesRegex(SystemExit, r": src/progress/demo.c: demos\[\] plays recorded pads in STAGE_C, "
                                                   r"which no case of loader.c's loader_read_stage_files reads$"):
            memory.contract(images())

    def test_holds_a_recording_with_each_event_its_demo_can_load_and_fails_on_one_no_image_is(self):
        # demos[] plays Sonic's recorded pads in an act of stga's stage, and stage.c gives ev0001 to an entry or a
        # clear of that act. The demo's state comes again as ev0001 loads. An event no image is fails, as does a row
        # of clear_events in another form.
        entry = GAME_TREE['src/stage/stage.c'].replace(
            '    {STAGE_NONE', '    {STAGE_A, 0, CHARACTER_SONIC, 0x304, 1, NULL},\n    {STAGE_NONE', 1)
        clear = GAME_TREE['src/stage/stage.c'].replace('STAGE_B', 'STAGE_A')
        for stage in (entry, clear):
            with self.subTest(stage=stage), tree({**GAME_TREE, 'src/stage/stage.c': stage}):
                terms = memory.contract(images())
                states = {state.name: state for state in terms.states}
                self.assertEqual(states['a recorded demo of stage_a as ev0001 loads'].images,
                                 {'1st_read': 'whole', 'stga': 'whole', 'ev0001': 'whole'})
                self.assertEqual(terms.lengths['ev0001'], 0x60000)
        for stage, problem in ((clear.replace('1, 1, NULL', '1, 2, NULL'), ': src/stage/stage.c: gives ev0002 a '
                                                                          'recorded demo of stage_a can load, which no '
                                                                          'image is$'),
                               (clear.replace('1, 1, NULL', '1, 1'), ": src/stage/stage.c:6: {STAGE_A, 0, "
                                                                     "CHARACTER_SONIC, 1, 1}, a row of clear_events"
                                                                     r"\[\] in a form the memory contract doesn't "
                                                                     'read$')):
            with self.subTest(stage=stage), tree({**GAME_TREE, 'src/stage/stage.c': stage}), \
                    self.assertRaisesRegex(SystemExit, problem):
                memory.contract(images())

    def test_fails_on_a_row_of_demos_or_a_read_in_a_form_it_doesn_t_read(self):
        # stga stands in for Icecap, and its a.c reads the board motions. Each change below fails, naming the file's
        # line. The first two change a row of demos[] and the rows' list. The rest change a read: of a recording by
        # demo.c, of a texture by cast_load and of the board motions by a.c.
        boards = 'void boards(void)\n{\n    loader_read_prs("s_sbmot.PRS", SONIC_BOARD_AREA);\n}\n'
        files = {**GAME_TREE, 'modules/stga/a.c': GAME_TREE['modules/stga/a.c'] + boards}
        form = "a call of {} in a form the memory contract doesn't read".format
        with tree(files), mock.patch.multiple(memory, **ICECAP):
            memory.contract(images())
        for path, old, new, problem in (
                ('src/progress/demo.c', 'DEMO_NO_EVENT}', '-1}',
                 "2: {STAGE_A, 0, CHARACTER_SONIC, -1}, a row of demos[] in a form the memory contract doesn't read"),
                ('src/progress/demo.c', '};', '    DEMO_PAIR,\n};',
                 "3: DEMO_PAIR, a row of demos[] in a form the memory contract doesn't read"),
                ('src/progress/demo.c', '"K_SONIC.bin"', 'name',
                 f'7: loader_read_file(name, DEMO_RECORDING), {form("loader_read_file")}'),
                ('modules/movie/cast.c', '"AMY.PRS"', 'amy', f'3: texture_load_prs_file(amy, &texlist), '
                                                           f'{form("texture_load_prs_file")}'),
                ('modules/stga/a.c', '"s_sbmot.PRS"', 'board', f'10: loader_read_prs(board, SONIC_BOARD_AREA), '
                                                               f'{form("loader_read_prs")}')):
            with self.subTest(path=path, new=new), tree({**files, path: files[path].replace(old, new, 1)}), \
                    mock.patch.multiple(memory, **ICECAP), \
                    self.assertRaisesRegex(SystemExit, f': {re.escape(path)}:{re.escape(problem)}$'):
                memory.contract(images())

    def test_fails_on_a_texture_the_cast_reads_whose_size_it_doesn_t_give(self):
        # the cast's read starts at the largest texture's room below WORK_AREA, so a texture CAST doesn't size fails
        cast = GAME_TREE['modules/movie/cast.c'].replace('AMY.PRS', 'TIKAL.PRS')
        with tree({**GAME_TREE, 'modules/movie/cast.c': cast}), self.assertRaisesRegex(
                SystemExit, ': modules/movie/cast.c: cast_load reads EV_TR2_BIG.PRS, KNUCKLES.PRS, MILES.PRS, TIKAL.PRS, '
                            'where the memory contract gives the sizes of AMY.PRS, EV_TR2_BIG.PRS, KNUCKLES.PRS, '
                            'MILES.PRS$'):
            memory.contract(images())

    def test_fails_on_a_texture_room_gcc_doesn_t_take_in_whole_sectors(self):
        # the cast's read ends at WORK_AREA, and texture_load_pvm's at WORK_AREA_END, only where GCC's FILE_ROOM takes a
        # file's whole sectors. GCC's side rounding the file to 256 bytes, as the shipped side does, fails.
        texture = GAME_TREE['src/system/texture.c'].replace('(SECTORS(size) * SECTOR_SIZE)', '(((size) + 255) & ~255)',
                                                             1)
        with tree({**GAME_TREE, 'src/system/texture.c': texture}), self.assertRaisesRegex(
                SystemExit, r': src/system/texture.c: defines FILE_ROOM for GCC other than once, as #define '
                            r'FILE_ROOM\(size\) \(SECTORS\(size\) \* SECTOR_SIZE\), on the GCC side of an #ifdef GCC: '
                            r"the room the memory contract gives a texture's read$"):
            memory.contract(images())

    def test_takes_no_definition_or_read_from_a_comment_a_conditional_or_a_redefinition(self):
        # each of these fails:
        # - ROOM_DEFINITION in a comment on GCC's side, a FILE_ROOM redefined past the guard, or ROOM_DEFINITION past an
        #   #elifdef;
        # - a WORK_AREA under #if 0, under a second #ifndef in an include guard's form, or in a // comment a splice
        #   runs on to its line;
        # - a read of a recording under #ifdef GCC;
        # - a loader_read_stage_files, demos[] or clear_events[] under a conditional, ahead of the one past its #else.
        texture, loader = GAME_TREE['src/system/texture.c'], GAME_TREE['src/system/loader.h']
        function, clear = 'void loader_read_stage_files(void)\n', 'static struct stage_event clear_events[] = {\n'
        staged = GAME_TREE['src/system/loader.c'].replace(
            function, f'#if 0\n{function}{{\n}}\n#else\n{function}').replace(
            '}\n\nvoid loader_read_character_motions', '}\n#endif\n\nvoid loader_read_character_motions')
        demos = ('#if 0\nstatic struct demo demos[DEMO_COUNT] = {\n    {STAGE_A, 0, CHARACTER_SONIC, 1},\n};\n#else\n'
                 + GAME_TREE['src/progress/demo.c'].replace('};\n', '};\n#endif\n', 1))
        cleared = GAME_TREE['src/stage/stage.c'].replace(
            clear, f'#ifdef SHC_ONLY\n{clear}    {{STAGE_NONE, 0, 0, 0, 0, NULL}},\n}};\n#else\n{clear}') + '#endif\n'
        work = '#define WORK_AREA ((Uint8 *)0x0cd00000)\n'
        guarded = ('#ifndef LOADER_H\n#define LOADER_H\n'
                   + loader.replace(work, f'#ifndef OPT\n#define OPT\n{work}#endif\n') + '#endif\n')
        apart = ", whose sides the memory contract doesn't tell apart"
        room = 'defines FILE_ROOM for GCC other than once, as #define FILE_ROOM(size) (SECTORS(size) * SECTOR_SIZE)'
        for path, text, problem in (
                ('src/system/texture.c', texture.replace('#ifdef GCC\n', '#ifdef GCC\n/*\n').replace(
                    '#else\n', '*/\n#define FILE_ROOM(size) (size)\n#else\n'), room),
                ('src/system/texture.c', texture + '#undef FILE_ROOM\n#define FILE_ROOM(size) (size)\n', room),
                ('src/system/texture.c', texture.replace('#ifdef GCC\n', '#ifdef GCC\n#elifdef NEVER\n'), room),
                ('src/system/loader.h', loader + '#if 0\n#define WORK_AREA ((Uint8 *)0x0cc80000)\n#endif\n',
                 'gives no number for WORK_AREA, which the memory contract reads'),
                ('src/system/loader.h', guarded, 'gives no number for WORK_AREA, which the memory contract reads'),
                ('src/system/loader.h', loader.replace(work, f'// the area past the images\\\n{work}'),
                 'gives no number for WORK_AREA, which the memory contract reads'),
                ('src/progress/demo.c', GAME_TREE['src/progress/demo.c'].replace(
                    '    loader_read_file("K_SONIC.bin", DEMO_RECORDING);\n',
                    '#ifdef GCC\n    loader_read_file("K_SONIC.bin", DEMO_RECORDING);\n#endif\n'),
                 '8: loader_read_file("K_SONIC.bin", DEMO_RECORDING), a call of loader_read_file under #ifdef GCC, '
                 "whose sides the memory contract doesn't tell apart"),
                ('src/system/loader.c', staged, f'6: loader_read_stage_files lies under #if 0{apart}'),
                ('src/progress/demo.c', demos, f'2: demos[] lies under #if 0{apart}'),
                ('src/stage/stage.c', cleared, f'6: clear_events[] lies under #ifdef SHC_ONLY{apart}')):
            with self.subTest(text=text), tree({**GAME_TREE, path: text}), \
                    self.assertRaisesRegex(SystemExit, f': {re.escape(path)}:? ?{re.escape(problem)}'):
                memory.contract(images())

    def test_fails_on_a_read_to_a_variable_no_case_sets_to_an_area(self):
        # stgb's case reading a camera file to where, which no case sets, fails. The same read to dest, which the case
        # sets to CAMERA_AREA_0, passes.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        loader_read_camera("0100", where);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}), self.assertRaisesRegex(
                SystemExit, ": src/system/loader.c:13: loader_read_stage_files reads to where, which case STAGE_B "
                            "doesn't set to an area$"):
            memory.contract(images())
        loader = loader.replace('loader_read_camera("0100", where);\n',
                                'dest = CAMERA_AREA_0;\n        loader_read_camera("0100", dest);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}):
            memory.contract(images())

    def test_takes_a_read_past_a_label_to_the_area_each_case_that_jumps_there_sets(self):
        # stga's case jumps to stgb's camera read, which reads to dest. Each case's state holds the area it sets dest
        # to, and stga's case fails where it sets none.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stga.PRS", MODULE_AREA);\n        return;\n',
            '        loader_read_prs("stga.PRS", MODULE_AREA);\n        dest = CAMERA_AREA_1;\n        goto camera;\n'
        ).replace('        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
                  '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        dest = CAMERA_AREA_0;\n    camera:\n'
                  '        loader_read_camera("0100", dest);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}):
            states = {state.name: state for state in memory.contract(images()).states}
        self.assertIn('CAMERA_AREA_1', states['stage_a'].areas)
        self.assertIn('CAMERA_AREA_0', states['stage_b'].areas)
        with tree({**GAME_TREE, 'src/system/loader.c': loader.replace('        dest = CAMERA_AREA_1;\n', '')}), \
                self.assertRaisesRegex(SystemExit, ": src/system/loader.c:15: loader_read_stage_files reads to dest, "
                                                   "which case STAGE_A doesn't set to an area$"):
            memory.contract(images())

    def test_takes_each_area_a_case_sets_and_each_label_its_gotos_reach(self):
        # stga's case reads to dest at two areas. In the other two tries, it jumps to stgb's camera read under an if,
        # or through a label of stgb's past its return. Each time, its state holds each area it sets dest to.
        stga = '        loader_read_prs("stga.PRS", MODULE_AREA);\n        return;\n'
        stgb = '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        return;\n'
        camera = ('        loader_read_prs("stgb.PRS", MODULE_AREA);\n        dest = CAMERA_AREA_0;\n    camera:\n'
                  '        loader_read_camera("0100", dest);\n        return;\n')
        for a, b, areas in (
                ('        loader_read_prs("stga.PRS", MODULE_AREA);\n        dest = CAMERA_AREA_1;\n'
                 '        loader_read_camera("0100", dest);\n        dest = CAMERA_AREA_2;\n'
                 '        loader_read_camera("0101", dest);\n        return;\n', stgb, ['CAMERA_AREA_1', 'CAMERA_AREA_2']),
                ('        loader_read_prs("stga.PRS", MODULE_AREA);\n        dest = CAMERA_AREA_1;\n        if (x)\n'
                 '            goto camera;\n        return;\n', camera, ['CAMERA_AREA_1']),
                ('        loader_read_prs("stga.PRS", MODULE_AREA);\n        dest = CAMERA_AREA_1;\n        goto tail;\n',
                 camera + '    tail:\n        goto camera;\n', ['CAMERA_AREA_1'])):
            loader = GAME_TREE['src/system/loader.c'].replace(stga, a).replace(stgb, b)
            with self.subTest(a=a, b=b), tree({**GAME_TREE, 'src/system/loader.c': loader}):
                states = {state.name: state for state in memory.contract(images()).states}
                self.assertEqual([area for area in states['stage_a'].areas if area.startswith('CAMERA')], areas)

    def test_reads_a_read_under_an_if_as_its_case_s_and_none_in_a_comment(self):
        # stgb's case reads lcd.PRS for one character alone, which its state holds, as the case can read it. A read in a
        # comment is none.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        if (get_character() == CHARACTER_SONIC) {\n'
            '            loader_read_prs("lcd.PRS", SET_AREA_1);\n        }\n'
            '        /* loader_read_prs("odd.PRS", ODD_AREA); return; */\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}):
            states = {state.name: state for state in memory.contract(images()).states}
        self.assertIn('SET_AREA_1 file', states['stage_b'].areas)
        self.assertNotIn('ODD_AREA file', states['stage_b'].areas)

    def test_reads_a_call_of_a_whole_name_outside_literals_alone(self):
        # my_loader_read_prs is no read, and neither is a read named in a string. demo_advance's read named across two
        # strings is none. Its read to SET_AREA_5 past a // in a string counts, as the // opens no comment.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        my_loader_read_prs("lcd.PRS", SET_AREA_1);\n'
            '        puts("loader_read_set(dummy, SET_AREA_4)");\n')
        demo = GAME_TREE['src/progress/demo.c'].replace(
            '    loader_read_file("K_SONIC.bin", DEMO_RECORDING);\n',
            '    loader_read_file("K_SONIC.bin", DEMO_RECORDING);\n'
            '    puts("loader_read_file("); puts(", SET_AREA_4)");\n'
            '    puts("//"); loader_read_file("K_TAILS.bin", SET_AREA_5);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader, 'src/progress/demo.c': demo}):
            states = {state.name: state for state in memory.contract(images()).states}
        self.assertEqual(states['stage_b'].areas, ['stack', 'heap', 'work', 'download models', 'stream work'])
        self.assertEqual(states['a recorded demo of stage_a'].areas[-2:], ['DEMO_RECORDING', 'SET_AREA_5'])
        # a quote that opens no literal fails
        with tree({**GAME_TREE, 'src/system/loader.c': "char odd = 'x;\n" + GAME_TREE['src/system/loader.c']}), \
                self.assertRaisesRegex(SystemExit, ": src/system/loader.c:1: a quote that opens no literal, which the "
                                                   "memory contract doesn't read$"):
            memory.contract(images())

    def test_fails_on_a_case_or_a_read_in_a_form_it_doesn_t_read(self):
        # each change to loader_read_stage_files fails, naming loader.c's line
        read = '        loader_read_prs("stgb.PRS", MODULE_AREA);\n'
        returning = '        loader_read_prs("stga.PRS", MODULE_AREA);\n        return;\n'
        opening = '    switch (stage) {\n    case STAGE_A:\n' + returning
        for old, new, problem in (
                (read, read + '        loader_read_prs(name, EXTRA_AREA);\n',
                 "13: loader_read_stage_files reads in a form the memory contract doesn't read: "
                 'loader_read_prs(name, EXTRA_AREA)'),
                (read, read + '        loader_read_file("lcd.BIN", SET_AREA_1);\n',
                 "13: loader_read_stage_files reads in a form the memory contract doesn't read: "
                 'loader_read_file("lcd.BIN", SET_AREA_1)'),
                (read, read + '        switch (get_character()) {\n        case CHARACTER_SONIC:\n'
                              '            loader_read_prs("lcd.PRS", SET_AREA_1);\n        }\n',
                 '7: loader_read_stage_files holds other than one switch with its body in braces, which the memory '
                 'contract reads'),
                ('    switch (stage) {\n', '    loader_read_prs("lcd.PRS", SET_AREA_1);\n    switch (stage) {\n',
                 "7: loader_read_stage_files reads outside its switch's cases, which the memory contract doesn't read"),
                (returning, returning.replace('        return;\n', ''),
                 "8: loader_read_stage_files's case STAGE_A can fall into the next with a statement, which the memory "
                 "contract doesn't read"),
                (returning, returning.replace('        return;\n', '        if (x)\n            return;\n'),
                 "8: loader_read_stage_files's case STAGE_A can fall into the next with a statement, which the memory "
                 "contract doesn't read"),
                (returning, returning.replace('        return;\n', '        if (x)\n    tail:\n            return;\n'),
                 "8: loader_read_stage_files's case STAGE_A can fall into the next with a statement, which the memory "
                 "contract doesn't read"),
                (read, read + '        dest = CAMERA_AREA_0;\n        dest += 0x4000;\n'
                              '        loader_read_camera("0100", dest);\n',
                 "14: loader_read_stage_files writes dest in a form the memory contract doesn't read"),
                (read, read + '        dest = CAMERA_AREA_0;\n        pick(&dest);\n'
                              '        loader_read_camera("0100", dest);\n',
                 "14: loader_read_stage_files writes dest in a form the memory contract doesn't read"),
                (opening, '    dest = SET_AREA_5;\n' + opening.replace('        return;\n', '        if (x)\n'
                                                                       '            dest = CAMERA_AREA_2;\n'
                                                                       '        loader_read_camera("0100", dest);\n'
                                                                       '        return;\n'),
                 "7: loader_read_stage_files writes dest outside its switch's cases, which the memory contract doesn't "
                 'read'),
                (read, read + '    tail:\n        dest = CAMERA_AREA_0;\n        loader_read_camera("0100", dest);\n',
                 '14: loader_read_stage_files writes dest past a label, which a goto can reach from a case whose areas '
                 "the memory contract doesn't give the write"),
                (returning, returning.replace('        return;\n', '        if (x)\n            goto tail;\n'
                                                                '        return;\n'),
                 '11: loader_read_stage_files jumps to tail, a label no case holds'),
                (read, read + '#ifdef GCC\n        loader_read_prs("lcd.PRS", SET_AREA_1);\n#endif\n',
                 "13: loader_read_stage_files holds #ifdef GCC, whose sides the memory contract doesn't tell apart"),
                (read, read + '    tail:\n        loader_read_prs("lcd.PRS", SET_AREA_1);\n',
                 '14: loader_read_stage_files reads lcd.PRS past a label, which a goto can reach from a case whose '
                 "state the memory contract doesn't give the read"),
                (read, read + '    tail:\n        loader_read_set("0100", SET_AREA_0);\n',
                 '14: loader_read_stage_files reads to SET_AREA_0 past a label, which a goto can reach from a case '
                 "whose state the memory contract doesn't give the read")):
            with self.subTest(new=new):
                loader = GAME_TREE['src/system/loader.c'].replace(old, new)
                with tree({**GAME_TREE, 'src/system/loader.c': loader}), \
                        self.assertRaisesRegex(SystemExit, f': src/system/loader.c:{re.escape(problem)}$'):
                    memory.contract(images())

    def test_fails_on_an_area_outside_ram_or_that_ends_before_it_starts(self):
        # strt2.c's stack past RAM's end fails, and so does download.c's second model buffer below its first
        for path, old, new, problem in (
                ('sdk/shinobi/strt2.c', '0x8c00', '0x8d10', "stack: starts at 0x0d10c000 and ends at 0x0d10f400, which "
                                                            "don't run in that order within RAM, 0x0c000000 to "
                                                            '0x0d000000'),
                ('src/object/download.c', '0x8cca6000', '0x8cca2000', "download models: starts at 0x0cca4000 and ends "
                                                                      "at 0x0cca0000, which don't run in that order "
                                                                      'within RAM, 0x0c000000 to 0x0d000000')):
            with self.subTest(path=path), tree({**GAME_TREE, path: GAME_TREE[path].replace(old, new)}), \
                    self.assertRaisesRegex(SystemExit, f': {re.escape(problem)}$'):
                memory.contract(images())

    def test_holds_a_motion_file_that_is_no_image_where_a_state_holds_one(self):
        # s_mot.PRS is no image of GAME_TREE's, so its room is the motion file a stage's state and a demo's start hold.
        # It is no state's area. DEMO_RECORDING moved over it fails at a demo's start.
        recorder = GAME_TREE['src/progress/recorder.h'].replace('0x0cbe0000', '0x0cbf0000')
        with tree({**GAME_TREE, 'src/progress/recorder.h': recorder}):
            terms = memory.contract(images())
            found = memory.check(terms, table_extents(terms))
        self.assertEqual(terms.motions, ['MOTION_AREA file'])
        self.assertEqual([state.name for state in terms.states if 'MOTION_AREA file' in state.areas], [])
        self.assertIn("in a demo's start: DEMO_RECORDING, 0x0cbf0000 to 0x0cc10000, overlaps MOTION_AREA file, "
                      '0x0cc00000 to 0x0cca4000', found)

    def test_holds_an_area_once_and_an_empty_owner_over_none(self):
        # stgb's case reads two data files to SET_AREA_1, whose room its state holds once, so the room overlaps nothing.
        # An empty owner overlaps none, and two owners at one address overlap.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
            '        loader_read_prs("stgb.PRS", MODULE_AREA);\n        loader_read_prs("lcd.PRS", SET_AREA_1);\n'
            '        loader_read_prs("lcd2.PRS", SET_AREA_1);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}):
            terms = memory.contract(images())
            found = memory.check(terms, table_extents(terms))
        self.assertEqual({state.name: state for state in terms.states}['stage_b'].areas.count('SET_AREA_1 file'), 1)
        self.assertEqual([problem for problem in found if 'SET_AREA_1 file' in problem], [])
        self.assertEqual(memory.overlapping([('a', 0, 10), ('b', 5, 5)]), [])
        self.assertEqual(memory.overlapping([('a', 0, 10), ('b', 0, 10)]),
                         ['a, 0x00000000 to 0x0000000a, overlaps b, 0x00000000 to 0x0000000a'])

    def test_fails_where_two_set_areas_overlap(self):
        # SET_AREA_1 starts 8 KB past SET_AREA_0, and each holds 16 KB
        files = {**GAME_TREE, 'src/system/loader.h': GAME_TREE['src/system/loader.h'].replace(
            '#define SET_AREA_1 ((Uint8 *)0x0cb70000)', '#define SET_AREA_1 ((Uint8 *)0x0cb6a000)')}
        with tree(files), self.assertRaisesRegex(SystemExit, ': src/system/loader.h: its set and camera areas hold '
                                                             'less than 0x4000 bytes each$'):
            memory.contract(images())

    def test_runs_a_data_file_to_the_next_owner_its_state_holds_and_fails_on_an_area_no_source_gives(self):
        # stga's case reads lcd.PRS to SET_AREA_1, whose room runs to the event at EVENT_MODULE, the next owner its
        # state holds. stgb's case reading odd.PRS to ODD_AREA, which no source the contract reads gives, fails.
        loader = GAME_TREE['src/system/loader.c'].replace(
            '        loader_read_prs("stga.PRS", MODULE_AREA);\n',
            '        loader_read_prs("stga.PRS", MODULE_AREA);\n        loader_read_prs("lcd.PRS", SET_AREA_1);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': loader}):
            terms = memory.contract(images())
        self.assertEqual(terms.areas['SET_AREA_1 file'], memory.Owner('SET_AREA_1 file', 0x0CB70000, 0x0CB80000))
        # the character's motion file, which no image of GAME_TREE's is, has room up to download.c's model buffers
        self.assertEqual(terms.areas['MOTION_AREA file'], memory.Owner('MOTION_AREA file', 0x0CC00000, 0x0CCA4000))
        odd = loader.replace('        loader_read_prs("stgb.PRS", MODULE_AREA);\n',
                             '        loader_read_prs("stgb.PRS", MODULE_AREA);\n'
                             '        loader_read_prs("odd.PRS", ODD_AREA);\n')
        with tree({**GAME_TREE, 'src/system/loader.c': odd}), \
                self.assertRaisesRegex(SystemExit, ": odd.PRS: loader.c's loader_read_stage_files, for STAGE_B, reads "
                                                   'it to ODD_AREA, an area no source the memory contract reads '
                                                   'gives$'):
            memory.contract(images())


if __name__ == '__main__':
    unittest.main()
