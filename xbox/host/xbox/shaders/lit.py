"""Write shaders/lit_NAME.ps from shaders/NAME.ps: the same combiners for a
model the NV2A lights (video.c's lit runs). Stages 2 and 3 sample the run's
palette, its base colors' row and its offset colors', at the place the
vertex program (shaders/lit.vp) gives from the normal's dot with the light;
a first combiner puts them where the program's others read the vertex's
colors: the base color's RGB in col0, its alpha kept, the vertex's, and the
offset color in col1, times the vertex's col1, which is 1 where the
offset counts and 0 where it doesn't.

    python3 shaders/lit.py shaders/NAME.ps > shaders/lit_NAME.ps
"""

import sys

FIRST = '''{
  rgb
  {
    col0 = tex2.rgb * unsigned_invert(zero.rgb);
    col1 = tex3.rgb * col1.rgb;
  }
}
'''


def lit(text, name):
    head, rest = text.split('!!TS1.0\n', 1)
    stages, rest = rest.split('!!RC1.0\n', 1)
    stages = [line for line in stages.splitlines() if line.strip()]
    if len(stages) != 2:
        raise SystemExit(f'lit.py: {name}: {len(stages)} texture stages, not 2')
    stages += ['texture_2d();', 'texture_2d();']
    return (f'// lit_{name}: {name}.ps for a model the NV2A lights (shaders/lit.py), made from it\n'
            + head + '!!TS1.0\n' + '\n'.join(stages) + '\n!!RC1.0\n' + FIRST + rest)


if __name__ == '__main__':
    path = sys.argv[1]
    name = path.rsplit('/', 1)[-1][:-3]
    sys.stdout.write(lit(open(path).read(), name))
