/* Chaos 6's debris: pieces thrown out and up from round the boss */
#ifndef B_CHAOS6_DEBRIS_H
#define B_CHAOS6_DEBRIS_H

/* throw count pieces out from the boss, at headings evenly spaced round
 * him. kind 0 picks each piece at random from the last four of debris.c's
 * models, and any other kind from the first three. a piece whose
 * task_create fails is left out */
void chaos6_debris_scatter(int kind, int count);

#endif
