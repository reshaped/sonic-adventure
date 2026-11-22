/* native/threads.c calls this on the normal game stack. */
#include <stdio.h>
#include <stdlib.h>

#include "native.h"

int pc_terrain_cases(const void *constants);

void __wrap_sa1_main(void)
{
    const void *constants = native_symbol("character_constants");
    if (!constants) {
        fputs("player terrain: missing character_constants\n", stderr);
        exit(2);
    }
    int failure = pc_terrain_cases(constants);
    if (failure) {
        fprintf(stderr, "player terrain: case %d failed\n", failure);
        exit(1);
    }
    puts("player terrain: all cases passed");
    exit(0);
}
