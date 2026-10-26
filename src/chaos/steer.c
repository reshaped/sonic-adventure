/* each Chaos form's parameters, and steering a Chaos */
#include <mathf.h>
#include <ninja.h>

#include "chaos/chaos.h"
#include "chaos/steer.h"
#include "draw/motion.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* clang-format off */
struct chaos_form chaos_forms[CHAOS_FORMS] = {
    /* Chaos 0 */
    {3, 25.0F, 9.0F, 0.6F, NJM_DEG_ANG(2.5), {270.0F, 3.0F, 267.0F},
     0.39607842F, 5.0F, 32.0F, 10.0F, 0.0F, 0.0F, 0, 60, 0.0F, 0.0F,
     {271.0F, 3.0F, 337.0F}, 1.6F, 0, 0, 0.0F, 0.0F},
    /* Chaos 2 */
    {4, 30.0F, 9.0F, 0.2F, NJM_DEG_ANG(2.5), {0.0F, 1.0F, 0.0F},
     0.6F, 10.0F, 32.0F, 10.0F, 0.0F, 2.5F, 30, 60, 1.0F, 20.0F,
     {0.0F, 0.0F, 0.0F}, 3.0F, 180, 180, 25.0F, 25.0F},
    /* Chaos 4 */
    {5, 30.0F, 0.0F, 0.2F, NJM_DEG_ANG(2.5), {50.0F, -15.0F, 50.0F},
     0.39607842F, 10.0F, 32.0F, 50.0F, 1.0F, 2.5F, 30, 60, 1.0F, 20.0F,
     {0.0F, 0.0F, 0.0F}, 0.0F, 0, 0, 50.0F, 50.0F},
    /* Chaos 6 */
    {4, 90.0F, 10.0F, 0.3F, NJM_DEG_ANG(2.5), {0.0F, 748.0F, -257.0F},
     0.39607842F, 10.0F, 36.0F, 70.0F, 0.25F, 2.5F, 30, 60, 1.0F, 20.0F,
     {0.0F, 744.0F, -385.0F}, 10.0F, 180, 180, 75.0F, 75.0F},
    /* Perfect Chaos: Chaos 6's row, but for hits, speed, center and
     * attack_wait. his boss sets chaos_current_form to CHAOS_7, but not
     * chaos_params, which only chaos_setup points at a row */
    {6, 90.0F, 10.0F, 0.2F, NJM_DEG_ANG(2.5), {0.0F, 748.0F, -257.0F},
     0.39607842F, 10.0F, 36.0F, 70.0F, 0.25F, 2.5F, 30, 60, 1.0F, 20.0F,
     {0.0F, 770.0F, -385.0F}, 10.0F, 300, 180, 75.0F, 75.0F},
    /* Chaos 0's row again */
    {3, 25.0F, 9.0F, 0.6F, NJM_DEG_ANG(2.5), {270.0F, 3.0F, 267.0F},
     0.39607842F, 5.0F, 32.0F, 10.0F, 0.0F, 0.0F, 0, 60, 0.0F, 0.0F,
     {271.0F, 3.0F, 337.0F}, 1.6F, 0, 0, 0.0F, 0.0F},
};
/* clang-format on */

/* @hack type: the record taken as a void pointer, which SHC keeps in R13
 * (MOV R6,R13 at 0x8c0fa3c8); typed, it spills to the stack, the function
 * runs 6 bytes long and 71 of 72 words differ */
void chaos_steer(struct task_work *work, void *unused, void *record)
{
    struct chaos *chaos = record;
    Angle ang = (Angle)(atan2f(chaos->target.z - work->pos.z,
                               chaos->target.x - work->pos.x) *
                        ANGLE_PER_RADIAN);

    work->ang.y = angle_approach(work->ang.y, ang, chaos_params->turn);
    work->pos.x += chaos_params->speed * njCos(work->ang.y);
    work->pos.z += chaos_params->speed * njSin(work->ang.y);
    chaos_travel += chaos_params->speed;
}

/* @hack type: the record taken as a void pointer, which SHC stores to a
 * frame slot at @(4,R15) (0x8c0fa462); typed, it pushes it and 7 of 41 words
 * differ */
void chaos_face_player(struct task_work *work, void *record)
{
    Angle ang;
    struct task_work *player = player_work[0];
    struct chaos *chaos = record;

    ang = (Angle)(atan2f(player->pos.z - work->pos.z,
                         player->pos.x - work->pos.x) *
                  ANGLE_PER_RADIAN);
    work->ang.y =
        angle_approach(work->ang.y, ang - chaos->facing, chaos_params->turn);
}

Angle chaos_angle_to_player(struct task_work *work, int player)
{
    return (Angle)(atan2f(player_work[player]->pos.z - work->pos.z,
                          player_work[player]->pos.x - work->pos.x) *
                   ANGLE_PER_RADIAN);
}
