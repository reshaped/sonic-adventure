"""Build a focused Dreamcast test ELF after make dc, or check the stick test's output.

Boot the ELF in Flycast with its HLE BIOS or load it on a console. The mesh, land and readout tests set result to 1
on success and to the failed check plus one otherwise. When the stick test sets done, changed and worst give the
summary. Dump its 131072-byte differences array and pass the file to "headings" with any extracted K_*.BIN files.
"""
import argparse
import os
import struct
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory


DC = Path('build/dc')
SDK = DC / 'sdk'
CASES = {
    'mesh': ('dc_mesh', ('src/collision/mesh', 'src/collision/polygon', 'src/player/sonic', 'src/player/amy', 'src/player/big', 'src/system/util',
                        'src/system/vector', 'src/system/matrix_push', 'src/system/task', 'src/collision/collision', 'src/path/path'),
             ('njPushMatrix', 'njPopMatrix', 'njRotateX', 'njRotateY', 'njRotateZ', 'njRotate', 'njScaleV', 'njGetMatrix', 'njSetMatrix', 'njInvertMatrix', 'njDetMatrix',
              'njIsParalellPL2PL', 'njCalcVector', 'njDistanceP2PL', 'njDistancePL2PL',
              'njUnitMatrix', 'njScale', 'njScalor', 'njDistanceP2P', 'njDistanceP2L', 'njAddVector', 'njSubVector', 'njOuterProduct', 'njTranslateV'),
             ('memcpy', 'memset', 'acosf', 'asinf', 'atan2f', '_i_acoss', '_i_asins', '_i_atn2s', '_i_atans',
              '_in_logs', '_in_exps', '_a_atans', '_i_fmds', '_i_ldes', 'consts')),
    'land': ('dc_land', ('src/collision/mesh', 'src/collision/polygon', 'src/collision/ground', 'src/player/e102_effects'),
             ('njPushMatrix', 'njPopMatrix', 'njRotateX', 'njRotateY', 'njRotateZ', 'njScaleV', 'njUnitMatrix', 'njScalor'),
             ('memcpy', 'memset', 'atan2', 'asinf', '_i_asins', '_i_atn2s', '_i_atans',
              '_a_atans', '_i_fmds', '_i_ldes', 'consts')),
    'yaw': ('dc_yaw', (), (), ('memcpy', 'memset')),
    'readout': ('dc_readout', ('engine/dc/overlay',), ('njDebug', 'kmvertex_kamui_lib'),
                ('memcpy', 'memset', 'strlen', 'strcpy', 'strcmp', 'memcmp', 'r_strcmp', 'r_s_scpy', 'r_q_scpy')),
    'stick': ('stick', (), (), ('atan2f', '_i_atn2s', '_i_atans', '_a_atans', '_i_fmds', '_i_ldes', 'consts')),
}


def build(case, output):
    tools = Path(os.environ.get('SH_ELF', 'tools/sh-elf')) / 'bin'
    source, objects, ninja, runtime = CASES[case]
    with TemporaryDirectory() as directory:
        objects = [*(DC / (n + '.o') for n in objects), *(SDK / 'ninja' / (n + '.o') for n in ninja),
                   *(SDK / 'runtime' / (n + '.o') for n in runtime)]
        flags = []
        if case in ('mesh', 'yaw', 'land'):
            flags = ['-DDREAMCAST', '-DNDEBUG', '-ffreestanding', '-fno-math-errno', '-mfsca', '-mfsrra', '-Itools/sh4zam/include']
        if case == 'land':
            world = Path(directory) / 'world.o'
            subprocess.run([tools / 'sh-elf-objcopy', '--globalize-symbol=_world_land',
                            '--globalize-symbol=_world_clear_land_cache',
                            DC / 'src/collision/world.o', world], check=True)
            objects.append(world)
        if case == 'mesh':
            corner = Path(directory) / 'corner.o'
            subprocess.run([tools / 'sh-elf-objcopy', '--globalize-symbol',
                            '_character_constants', '--globalize-symbol', '_player_collide_world',
                            '--globalize-symbol', '_player_sweep_world', '--globalize-symbol', '_player_prepare_world',
                            DC / 'src/player/player.o', corner], check=True)
            grip = Path(directory) / 'grip.o'
            subprocess.run([tools / 'sh-elf-objcopy', '--remove-section=.gnu.lto_*',
                            '--globalize-symbol=_knuckles_glide_collide', '--globalize-symbol=_knuckles_start_climb',
                            '--globalize-symbol=_knuckles_wall_near',
                            DC / 'src/player/knuckles.o', grip], check=True)
            tails = Path(directory) / 'tails.o'
            subprocess.run([tools / 'sh-elf-objcopy', '--remove-section=.gnu.lto_*',
                            '--globalize-symbol=_tails_try_jump', DC / 'src/player/tails.o', tails], check=True)
            gamma = Path(directory) / 'e102.o'
            subprocess.run([tools / 'sh-elf-objcopy', '--remove-section=.gnu.lto_*',
                            '--globalize-symbol=_e102_hover', '--globalize-symbol=_e102_skate',
                            DC / 'src/player/e102.o', gamma], check=True)
            hammer = Path(directory) / 'hammer.o'
            subprocess.run([tools / 'sh-elf-objcopy', '--remove-section=.gnu.lto_*',
                            '--globalize-symbol=_hammer_head_follow',
                            DC / 'modules/stg05/hammer.o', hammer], check=True)
            objects.extend((corner, grip, tails, gamma, hammer))
            for module, name, entry in (('adv00', 'traffic', 'car_carry'),
                                        ('stg04', 'traffic', 'traffic_car_carry'),
                                        ('adv0130', 'egg_lift', 'egg_lift_move'),
                                        ('stg05', 'propeller', 'mountain_propeller_run')):
                actor = Path(directory) / (module + '-' + name + '.o')
                subprocess.run([tools / 'sh-elf-objcopy', '--remove-section=.gnu.lto_*',
                                '--globalize-symbol=_' + entry,
                                DC / 'modules' / module / (name + '.o'), actor], check=True)
                objects.append(actor)
        if case == 'stick':
            angle = Path(directory) / 'angle.o'
            subprocess.run([tools / 'sh-elf-objcopy', '-j', '.text.shz_atan2f', '--redefine-sym',
                            '_shz_atan2f=_stick_atan2f', '--globalize-symbol=_stick_atan2f',
                            DC / 'src/system/pad.o', angle], check=True)
            objects.append(angle)
        if case == 'readout':
            formatter = Path(directory) / 'format.o'
            subprocess.run([tools / 'sh-elf-ld', '-EL', '-r', '--gc-sections', '-u', '_string_vformat',
                            DC / 'src/system/util.o', '-o', formatter], check=True)
            objects.append(formatter)
        script = Path(directory) / 'test.ld'
        script.write_text('SECTIONS { . = 0x8c010000; .text : { *(.text.boot) *(.text*) } '
                          '.rodata : { *(.rodata*) } .data : { *(.data*) } .bss : { *(.bss*) *(COMMON) } }')
        subprocess.run([tools / 'sh-elf-gcc', '-std=gnu17', '-ml', '-m4-single-only', '-Os', '-fpack-struct=4',
                        '-ffunction-sections', '-fdata-sections', '-fno-builtin', '-nostdlib', '-DNONMATCHING', *flags,
                        '-Isdk/include', '-Isrc', '-I.', '-Wl,-e,_start,--gc-sections', '-T', script, '-o', output,
                        f'engine/tests/{source}.c', *(['engine/tests/player_terrain.c'] if case == 'mesh' else []),
                        *objects, DC / 'sh4zam/libsh4zam.a', '-lgcc'], check=True)


def check(data, recordings):
    differences = struct.unpack('<65536h', data)
    active = [differences[(y + 128) * 256 + x + 128] for y in range(-128, 128) for x in range(-128, 128)
              if abs(x) > 12 or abs(y) > 12]
    worst = max(map(abs, active))
    print(f'{sum(bool(d) for d in active)} of {len(active)} active stick positions changed; max {worst} angle units')
    if worst > 1:
        raise SystemExit('stick heading exceeds the one-unit bound')
    for path in recordings:
        changed = count = 0
        for buttons, _, _, x, y in struct.iter_unpack('<IHHhh', path.read_bytes()):
            if buttons == 0xffffffff:
                break
            if abs(x) > 12 or abs(y) > 12:
                count += 1
                changed += differences[(y + 128) * 256 + x + 128] != 0
        print(f'{path.name}: {changed} of {count} active samples changed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    elf = commands.add_parser('build')
    elf.add_argument('case', choices=CASES)
    elf.add_argument('output', type=Path)
    headings = commands.add_parser('headings')
    headings.add_argument('data', type=Path)
    headings.add_argument('recordings', nargs='*', type=Path)
    args = parser.parse_args()
    if args.action == 'build':
        build(args.case, args.output)
    else:
        check(args.data.read_bytes(), args.recordings)
