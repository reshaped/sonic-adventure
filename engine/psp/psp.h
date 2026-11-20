/* what the PSP slice's C shares, the game's and the host's. the PSP has no
 * memory at the console's addresses. console_ram holds the console's RAM
 * from CONSOLE_RAM_FIRST, where the loader's areas and the game's fixed
 * buffers lie, and NATIVE_RAM names in it the byte an address of any of
 * RAM's three views names. marks.py's narrow copy of the game's C spells
 * each such address through it. RAM's last megabyte is Shinobi's heap,
 * which the slice gives nothing, so console_ram ends before it */
#ifndef PSP_H
#define PSP_H

#define CONSOLE_RAM_FIRST 0x00900000UL
#define CONSOLE_RAM_SIZE 0x00600000UL
extern char console_ram[CONSOLE_RAM_SIZE];
#define NATIVE_RAM(address)                                                    \
    (console_ram + (((address) & 0x00ffffffUL) - CONSOLE_RAM_FIRST))

/* zeros that take writes, in place of the console's devices. the game's C
 * and the SDK's reach a register by its address, and console_registers holds
 * each by that address's low 16 bits: the PVR's lie from CONSOLE_PVR on. they
 * send vertices through the store queues, which nothing here reads:
 * console_queues stands at a multiple of its 64 KB, which a list's pointer
 * wraps in, with as much again past it for what one send writes. marks.py's
 * narrow pass spells both */
#define CONSOLE_PVR 0x8000UL
extern char console_registers[0x10000];
extern char console_queues[0x20000];

#endif
