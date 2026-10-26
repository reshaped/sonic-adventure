/* E GACHA robots that walk toward player 0 and burst, and COLLI GACHA walls
 * that stop them. gacha_robot_list_step ranks the robots and stops them at
 * the walls */
#ifndef STG10_GACHA_ROBOT_H
#define STG10_GACHA_ROBOT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the robot and its textures, which O GACHA also draws inside its machine */
/* @hack type: the robot declared as an array, and read through ->.
 * gacha_machine_draw_at then loads the robot's address and reads its model 4
 * bytes past it (MOV.L @(4,R4),R4 at 0x0c90fee0). as one NJS_OBJECT read
 * through ., 2 of gacha_machine_draw_at's 78 words differ, and
 * gacha_robot_draw_at builds the same */
extern NJS_OBJECT gacha_robot_object[];
extern NJS_TEXLIST gacha_robot_texlist;

/* the kinds of launch. each starts the robot falling at pos, heading ang_y,
 * onto the floor at ground. LAUNCH_WITH_SPEED adds speed on its first move */
enum gacha_launch_kind { LAUNCH_DROP, LAUNCH_WITH_SPEED };

/* how gacha_robot_launch starts a robot */
struct gacha_launch {
    Sint32 kind; /* LAUNCH_DROP or LAUNCH_WITH_SPEED */
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Angle ang_y;
    Float ground; /* the height of the floor the robot lands on */
};

/* the object list's steps */
void gacha_robot_step(struct task *task);
void gacha_wall_step(struct task *task);
/* the step of the task final_act2_start creates. with robots enlisted, it
 * stops them at the walls and ranks them by distance from player 0. it then
 * empties the robot and wall lists for the next step */
void gacha_robot_list_step(struct task *task);
/* create a robot as a child of machine, as launch says. the call keeps no
 * pointer to launch */
void gacha_robot_launch(struct task *machine, struct gacha_launch *launch);

#endif
