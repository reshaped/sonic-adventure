/* shuffles: an array's records in a random order */
#ifndef SYSTEM_SHUFFLE_H
#define SYSTEM_SHUFFLE_H

/* put the count records of size bytes at array in a random order, in
 * place. it draws a number from rand for each record past the first */
void array_shuffle(void *array, int count, int size);

#endif
