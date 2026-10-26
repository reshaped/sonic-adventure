/* the ribbon in Chaos 4's body. its record names its 11 nodes and 22
 * strands, and each strand names the path its beads run along. a 23rd
 * strand, of node -1 with no path, ends the strands' array */
#include <ninja.h>

#include "b_chaos4/ribbon.h"
#include "chaos/ribbon.h"

static const struct ribbon_node chaos4_ribbon_nodes[11] = {
#include "b_chaos4/ribbon/chaos4_ribbon_nodes.inc"
};
static NJS_POINT3 dat_0c908440[3] = {
#include "b_chaos4/ribbon/dat_0c908440.inc"
};
static NJS_POINT3 dat_0c908464[3] = {
#include "b_chaos4/ribbon/dat_0c908464.inc"
};
static NJS_POINT3 dat_0c908488[3] = {
#include "b_chaos4/ribbon/dat_0c908488.inc"
};
static NJS_POINT3 dat_0c9084ac[3] = {
#include "b_chaos4/ribbon/dat_0c9084ac.inc"
};
static NJS_POINT3 dat_0c9084d0[4] = {
#include "b_chaos4/ribbon/dat_0c9084d0.inc"
};
static NJS_POINT3 dat_0c908500[4] = {
#include "b_chaos4/ribbon/dat_0c908500.inc"
};
static NJS_POINT3 dat_0c908530[3] = {
#include "b_chaos4/ribbon/dat_0c908530.inc"
};
static NJS_POINT3 dat_0c908554[3] = {
#include "b_chaos4/ribbon/dat_0c908554.inc"
};
static NJS_POINT3 dat_0c908578[3] = {
#include "b_chaos4/ribbon/dat_0c908578.inc"
};
static NJS_POINT3 dat_0c90859c[3] = {
#include "b_chaos4/ribbon/dat_0c90859c.inc"
};
static NJS_POINT3 dat_0c9085c0[3] = {
#include "b_chaos4/ribbon/dat_0c9085c0.inc"
};
static NJS_POINT3 dat_0c9085e4[3] = {
#include "b_chaos4/ribbon/dat_0c9085e4.inc"
};
static NJS_POINT3 dat_0c908608[3] = {
#include "b_chaos4/ribbon/dat_0c908608.inc"
};
static NJS_POINT3 dat_0c90862c[3] = {
#include "b_chaos4/ribbon/dat_0c90862c.inc"
};
static NJS_POINT3 dat_0c908650[3] = {
#include "b_chaos4/ribbon/dat_0c908650.inc"
};
static NJS_POINT3 dat_0c908674[3] = {
#include "b_chaos4/ribbon/dat_0c908674.inc"
};
static NJS_POINT3 dat_0c908698[3] = {
#include "b_chaos4/ribbon/dat_0c908698.inc"
};
static NJS_POINT3 dat_0c9086bc[3] = {
#include "b_chaos4/ribbon/dat_0c9086bc.inc"
};
static struct ribbon_strand chaos4_ribbon_strands[23] = {
#include "b_chaos4/ribbon/chaos4_ribbon_strands.inc"
};
struct ribbon chaos4_ribbon = {
#include "b_chaos4/ribbon/chaos4_ribbon.inc"
};
