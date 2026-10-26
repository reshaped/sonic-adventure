/* EV00BD.PRS's factory, drawn in three groups: eight parts, two pieces at
 * positions of their own and four machines that loop their actions. its
 * texture file, PVME101FACTORY, names it E-101's */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "ev00bd/factory.h"
#include "ev00bd/factory_part_7_object.h"
#include "ev00bd/factory_textures.h"

extern NJS_OBJECT ev00bd_factory_part_0_object;
extern NJS_OBJECT ev00bd_factory_part_1_object;
extern NJS_OBJECT ev00bd_factory_part_2_object;
extern NJS_OBJECT ev00bd_factory_part_3_object;
extern NJS_OBJECT ev00bd_factory_part_4_object;
extern NJS_OBJECT ev00bd_factory_part_5_object;
extern NJS_OBJECT ev00bd_factory_part_6_object;
extern NJS_OBJECT ev00bd_factory_piece_0_object;
extern NJS_OBJECT ev00bd_factory_piece_1_object;
extern NJS_ACTION ev00bd_factory_machine_0_action;
extern NJS_ACTION ev00bd_factory_machine_1_action;
extern NJS_ACTION ev00bd_factory_machine_2_action;
extern NJS_ACTION ev00bd_factory_machine_3_action;

struct factory_piece {
    NJS_OBJECT *object;
    NJS_POINT3 pos;
};

struct factory_machine {
    NJS_ACTION *action;
    Float frame;
    Float speed; /* motion frames per step */
};

#define FACTORY_PARTS 8
#define FACTORY_PIECES 2
#define FACTORY_MACHINES 4

static NJS_OBJECT *ev00bd_factory_parts[FACTORY_PARTS] = {
    &ev00bd_factory_part_0_object, &ev00bd_factory_part_1_object,
    &ev00bd_factory_part_2_object, &ev00bd_factory_part_3_object,
    &ev00bd_factory_part_4_object, &ev00bd_factory_part_5_object,
    &ev00bd_factory_part_6_object, &ev00bd_factory_part_7_object,
};
static struct factory_piece ev00bd_factory_pieces[FACTORY_PIECES] = {
    {&ev00bd_factory_piece_0_object, {-83.0F, 0.0F, 28.7F}},
    {&ev00bd_factory_piece_1_object, {-133.6F, 0.0F, -46.7F}},
};
static struct factory_machine ev00bd_factory_machines[FACTORY_MACHINES] = {
    {&ev00bd_factory_machine_0_action, 0.0F, 5.0F},
    {&ev00bd_factory_machine_1_action, 0.0F, 5.0F},
    {&ev00bd_factory_machine_2_action, 0.0F, 5.0F},
    {&ev00bd_factory_machine_3_action, 0.0F, 5.0F},
};

void ev00bd_factory_draw(void)
{
    /* @hack type: i unsigned, which SHC tests with CMP/HS (0x0cb80ba8,
     * 0x0cb80bf4, 0x0cb80cdc); as an int, it tests with CMP/GE and 3 of 189
     * words differ */
    Uint32 i;
    NJS_OBJECT **part;
    struct factory_piece *piece;
    struct factory_machine *machine;

    njSetTexture(&ev00bd_factory_texlist);
    for (part = ev00bd_factory_parts, i = 0; i < FACTORY_PARTS; i++, part++) {
        draw_tree(*part);
    }
    for (piece = ev00bd_factory_pieces, i = 0; i < FACTORY_PIECES;
         i++, piece++) {
        njPushMatrix(NULL);
        njTranslate(NULL, piece->pos.x, piece->pos.y, piece->pos.z);
        draw_tree(piece->object);
        njPopMatrix(1);
    }
    for (machine = ev00bd_factory_machines, i = 0; i < FACTORY_MACHINES;
         i++, machine++) {
        draw_action(machine->action, machine->frame);
        machine->frame += machine->speed;
        if (machine->frame >= machine->action->motion->nbFrame) {
            machine->frame = 0.0F;
        }
    }
}
