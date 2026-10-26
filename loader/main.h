/* LOADER.BIN's main, which strt2 calls */
#ifndef LOADER_MAIN_H
#define LOADER_MAIN_H

/* read sp.bin, Dream Passport, to 1ST_READ.BIN's load address and jump to
 * it. the system starts for the read and ends before the jump. it doesn't
 * return */
void main(void);

#endif
