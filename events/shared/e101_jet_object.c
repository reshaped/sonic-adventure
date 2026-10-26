/* the jet that e101.c draws on E-101's back: one node, over the model each
 * event defines */
#include "shared/e101.h"
#include <ninja.h>

extern NJS_MODEL event_e101_jet_model;

NJS_OBJECT event_e101_jet_object = {
#include "shared/e101_jet_object/event_e101_jet_object.inc"
};
