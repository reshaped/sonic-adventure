/* curves: records sampled round a closed curve through keys, and runs along
 * one axis */
#ifndef PATH_CURVE_H
#define PATH_CURVE_H
#include <sg_xpt.h>

/* how a curve follows its keys. PATH_CURVE_LAGRANGE fits a cubic to each
 * four keys in turn, and passes through every key. PATH_CURVE_BSPLINE is a
 * uniform cubic b-spline, which passes near them. any other kind samples as
 * PATH_CURVE_BSPLINE */
enum path_curve_kind { PATH_CURVE_LAGRANGE, PATH_CURVE_BSPLINE };

/* a closed curve through a loop of keys, and how to sample it. keys and out
 * hold records that format lays out. each field is aligned to its size, and
 * a record's size is a multiple of its largest field's */
struct path_curve {
    void *keys;
    int count; /* of keys: 0, or 3 or more */
    /* a letter for each field, 19 at most. c, s and l are an unsigned byte,
     * short and long. C, S and L are the signed ones. I and i are an int, A
     * and a an Angle, and F and f a Float. any other letter faults, and a 20th
     * field overruns an array in path_curve_sample's frame */
    const char *format;
    Float step; /* the share of a span from one record to the next */
    void *out;
    enum path_curve_kind kind;
};

/* sample the closed curve through the keys into out, and return the count
 * of records written. each key starts a span. a span takes a record at part
 * 0, and at each step on while part <= 1 - step, with part summed in Float.
 * how many records that is rests on how the sums round. the game rounds
 * floating-point results toward zero. a step of 0.2F then gives 5 records a
 * span, a step of 0.05F gives 20, and a step of 1.0F / 14 gives 14.
 * rounding to nearest, a step of 0.05F gives 19.
 *
 * a count of 0 writes nothing. with a step of 0 or less, or one too small
 * to move part, the loop never ends. out must hold every record, and
 * nothing checks it. keys and out must be aligned to their records' largest
 * field. format is laid out before count is read, so even with a count of
 * 0, any other letter faults and a 20th field overruns */
int path_curve_sample(struct path_curve *curve);
/* a run along one axis visits stops in turn: one coordinate or more, none
 * negative, then a negative one that ends the list. return the coordinate
 * the run is at once it has gone distance, or the last stop past the run's
 * end. distance must not be negative: a negative one reads stops[-1] and an
 * unset local */
Float path_axis_position(const Float *stops, Float distance);
/* the run's length: the distances between its stops in turn, summed */
Float path_axis_length(const Float *stops);

#endif
