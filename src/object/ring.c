/* rings: a ring placed to be taken, and scattered rings, which fly out
 * from a point and lie a while before they go */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/mesh.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

/* the sound for E-102 when a hit finds no rings to lose. any other
 * character plays SE_DOWN */
#define SE_E102_DOWN 1197

/* a placed ring looks for the ground only while the camera is this near */
#define RING_PROBE_RANGE 400.0F
/* a ring follows ground that moves less than this between its looks */
#define RING_FOLLOW_LIMIT 8.0F

/* a sparkle draws its frames below this, and a taken ring's three
 * sparkles are done once the third passes it */
#define SPARKLE_END 7.0F

/* a magnetic barrier's pull starts on a ring within ATTRACT_RANGE of player
 * 0, and aims ATTRACT_HEIGHT up player 0's own y axis */
#define ATTRACT_RANGE 50.0F
#define ATTRACT_HEIGHT 7.0F

#define RINGS_LOST_MAX 20

/* a scattered ring can be taken once it has flown LOST_RING_FLY_STEPS
 * steps, or once it lies. a lying ring goes LOST_RING_LIE_STEPS steps after
 * its last bounce */
#define LOST_RING_FLY_STEPS 46
#define LOST_RING_LIE_STEPS 100
/* a scattered ring lies once its speed drops below this */
#define LOST_RING_LIE_SPEED 0.2F
/* where it finds no ground, a scattered ring takes the ground to be this
 * far below it */
#define LOST_RING_DEPTH 100.0F

/* a scattered ring's work mode */
enum lost_ring_mode {
    LOST_RING_INIT,
    LOST_RING_FLY,
    LOST_RING_LIE,
    LOST_RING_SPARKLE,
    LOST_RING_KILL
};

/* a scattered ring's vars. while it flies, scl.x and scl.z are its step
 * across x and z, and scl.y how fast it rises. once it lies, scl.x and
 * scl.z offset its collision across x and z */
struct lost_ring_vars {
    /* its turn in degrees, which ring_draw_sparkle reads as RING(work)->spin */
    Float spin;
    Float slow; /* 1, then 1.08 on the step it lies, whose move it divides */
    Float speed; /* its speed, then the ground's height once it lies */
};

#define LOST_RING(work) ((struct lost_ring_vars *)&(work)->vars)

/* a scattered ring's task extra */
struct lost_ring_extra {
    /* the ground's height below it, or LOST_RING_DEPTH below it where
     * there is none */
    Float ground;
    Float probe; /* steps to its next look for the ground */
};

/* ring01 to ring08, the first eight of EFF_REGULAR's textures */
NJS_TEXLIST ring_texlist = {
#include "object/ring/ring_texlist.inc"
};
static NJS_TEXANIM ring_texanims[8] = {
#include "object/ring/ring_texanims.inc"
};

/* clang-format off */
static struct collision_shape ring_shape = {
    SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

static Sint32 dat_8c1d5454[2] = {0, 0};

Sint32 ring_stagger = 0;

static Uint8 dat_8c7a14c8[64];
/* the sprite a taken ring's sparkle draws */
static NJS_SPRITE ring_sprite;

static void ring_draw(struct task *task);
static void ring_draw_sparkle(struct task_work *work);
static void lost_ring_step(struct task *task);
static void ring_sprite_init(struct task_work *work, NJS_SPRITE *sprite);
static void ring_destroy(struct task *task);
static void lost_ring_draw(struct task *task, Float ground, Float x, Float z);
static Bool ring_attract(struct task_work *work);

void ring_reset_stagger(void)
{
    ring_stagger = 0;
}

void ring_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player;
    struct task *lost;
    struct task_work *lost_work;
    Float dx;
    Float dy;
    Float dz;
    Float height;
    Float rise;

    /* @hack spelling: gone set in the test, which builds its 1 in R0 at the
     * store (MOV #1,R0 at 0x8c08b9de); as a statement, the 1 goes to R12 in
     * the call's delay slot, and 54 of 303 words differ */
    if (placement_try_leave(task) && (RING(work)->gone = 1)) {
        return;
    }
    switch (work->mode) {
    case RING_INIT:
        collision_init(task, &ring_shape, COLLISION_RING, 1, 1);
        /* a ring placed with an scl.x of -1 follows the ground under it,
         * and its submode counts down to its next look. any other ring's
         * submode is -1 */
        if (work->scl.x == -1.0F) {
            work->submode = ring_stagger;
            ring_stagger++;
            if (ring_stagger > RING_PROBE_COUNT) {
                ring_stagger = 0;
            }
        } else {
            work->submode = -1;
        }
        /* scl.y keeps the height of the ground below the ring */
        work->scl.y =
            ground_below(work->pos.x, work->pos.y, work->pos.z, &work->ang);
        work->mode = RING_WAIT;
        RING(work)->spin = 0.0F;
        /* scl.x and scl.z become the second and third sparkles' frames, 2
         * and 4 behind the first's */
        work->scl.x = -2.0F;
        work->scl.z = -4.0F;
        work->id = RING_ID;
        task->destructor = ring_destroy;
        task->draw = ring_draw;
        ring_sprite_init(work, &ring_sprite);
        break;
    case RING_WAIT:
        if (edit_mode == 0) {
            if (work->collision->flags & COLLISION_HIT) {
                work->mode = RING_SPARKLE;
                if (!(player_get(0)->effects & PLAYER_FALLING)) {
                    player = player_get_task(0);
                    /* playing Tails, a ring counts only when player 0 took
                     * it */
                    /* @hack spelling: ring_add and sound_play called in
                     * each arm, as the image calls them twice (JSR at
                     * 0x8c08bb16 and 0x8c08bb6e); joined by ||, SHC calls
                     * them once and 184 of 303 words differ */
                    if (get_character() != CHARACTER_TAILS) {
                        ring_add(1);
                        sound_play(SE_RING, 0, 0, 0);
                    } else if (work->collision->hit->owner == player) {
                        ring_add(1);
                        sound_play(SE_RING, 0, 0, 0);
                    }
                }
            }
        }
        if (edit_mode == 0 && !ring_attract(work)) {
            collision_register(work);
            work->ang.y += NJM_DEG_ANG(2);
        }
        if (work->submode >= 0) {
            work->submode--;
            if (work->submode == -1) {
                work->submode = RING_PROBE_COUNT;
                dx = camera_work->pos.x - work->pos.x;
                dy = camera_work->pos.y - work->pos.y;
                dz = camera_work->pos.z - work->pos.z;
                if (vector_length_squared(dx, dy, dz) <
                    RING_PROBE_RANGE * RING_PROBE_RANGE) {
                    height = ground_below(work->pos.x, work->pos.y, work->pos.z,
                                          &work->ang);
                    rise = height - work->scl.y;
                    /* ground that has moved RING_FOLLOW_LIMIT or more
                     * since the last look ends the ring, and a scattered
                     * ring falls in its place */
                    if (_builtin_fabsf(rise) < RING_FOLLOW_LIMIT) {
                        work->scl.y = height;
                        work->pos.y += rise;
                    } else {
                        work->mode = RING_DONE;
                        lost = task_create(TASK_WORK | TASK_EXTRA, 2,
                                           lost_ring_step);
                        if (lost != NULL) {
                            lost_work = lost->work;
                            lost_work->pos = work->pos;
                            lost_work->pos.y -= 10.0F;
                            lost_work->ang.y =
                                (Angle)(rand() * RAND_SCALE * 360.0F *
                                        ANGLE_PER_DEGREE);
                        }
                    }
                }
            }
        }
        ring_draw(task);
        break;
    case RING_SPARKLE:
        ring_draw_sparkle(work);
        break;
    case RING_DONE:
        if (task->placement != NULL) {
            placement_kill(task);
        } else {
            task_kill(task);
        }
        break;
    }
}

static void ring_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        if (work->tick < 50) {
            if (work->scl.y != -GROUND_FAR) {
                shadow_draw_at_height(work, work->scl.y);
            }
        }
        draw_attr_save();
        draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateY, work->ang.y);
        draw_model(&ring_model);
        njPopMatrix(1);
        draw_attr_restore();
    }
}

/* draw a taken ring's three sparkles, each starting later than the last
 * and drawn off it along x and z. once the third passes SPARKLE_END, set
 * the mode to RING_DONE */
static void ring_draw_sparkle(struct task_work *work)
{
    Float size;
    Float scale;
    int frame[3];
    int i;

    RING(work)->spin += 0.8F;
    work->scl.x += 0.8F;
    work->scl.z += 0.8F;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&ring_texlist);
    frame[0] = RING(work)->spin;
    frame[1] = work->scl.x;
    frame[2] = work->scl.z;
    for (i = 0; i < 3; i++) {
        if (frame[i] < SPARKLE_END && frame[i] >= 0) {
            size = frame[i] * 0.05F;
            scale = _builtin_fabsf(frame[i] * 0.05F);
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x - (i * 2.0F),
                        work->pos.y - (size * 3.0F), work->pos.z - (i * 2.0F));
            TURN(njRotateY, camera_work->ang.y);
            TURN(njRotateX, camera_work->ang.x);
            njScale(NULL, scale, scale, scale);
            njDrawSprite3D(&ring_sprite, frame[i],
                           NJD_SPRITE_ALPHA | NJD_SPRITE_VFLIP |
                               NJD_SPRITE_ANGLE);
            njPopMatrix(1);
        }
    }
    if (frame[2] > SPARKLE_END) {
        work->mode = RING_DONE;
    }
}

void ring_lose(Sint8 player)
{
    Float base;
    int count;
    int i;
    int degrees;
    struct task *lost;
    struct task_work *lost_work;
    /* @hack spelling: sound_play called through play, which keeps its
     * address in R10 and player_work's in R9 (MOV.L at 0x8c08bf9c and
     * 0x8c08c054); called by name, the two trade and 5 of 117 words
     * differ */
    int (*play)(int se, int id, int priority, int volume);

    base = rand() * RAND_SCALE * 360.0F;
    count = (Sint16)ring_count();
    if (player == 0) {
        /* @hack spelling: play set in the test, which loads sound_play's
         * address ahead of the 20 (MOV.L at 0x8c08bf9c); set before the if,
         * the load follows it and 2 of 117 words differ */
        if (play = sound_play, count > RINGS_LOST_MAX) {
            count = RINGS_LOST_MAX;
        } else if (count == 0) {
            player_down(0);
            if (get_character() == CHARACTER_E102) {
                play(SE_E102_DOWN, 0, 0, 0);
            } else {
                play(SE_DOWN, 0, 0, 0);
            }
            return;
        }
        ring_reset();
        for (i = 0, degrees = 0; i < count; i++, degrees += 360) {
            lost = task_create(TASK_WORK | TASK_EXTRA, 2, lost_ring_step);
            if (lost != NULL) {
                lost_work = lost->work;
                lost_work->pos = player_work[0]->pos;
                /* the int division makes each heading's offset from base a
                 * whole number of degrees */
                /* NOLINTBEGIN(bugprone-integer-division) */
                lost_work->ang.y =
                    (Angle)(((degrees / count) + base) * ANGLE_PER_DEGREE);
                /* NOLINTEND(bugprone-integer-division) */
            }
        }
        play(SE_RINGS_LOST, 0, 0, 0);
    }
}

static void lost_ring_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lost_ring_extra *extra = task->extra;
    NJS_VECTOR up;
    NJS_VECTOR turned;
    NJS_POINT3 heading;
    NJS_POINT3 ahead;
    Float lean_x;
    Float lean_z;
    Float x;
    Float y;
    Float z;
    Float rest_x;
    Float rest_z;

    switch (work->mode) {
    case LOST_RING_INIT:
        extra->ground = work->pos.y - LOST_RING_DEPTH;
        extra->probe = ring_stagger;
        ring_stagger++;
        if (ring_stagger > RING_PROBE_COUNT) {
            ring_stagger = 0;
        }
        collision_init(task, &ring_shape, COLLISION_RING, 1, 1);
        task->destructor = ring_destroy;
        work->mode = LOST_RING_FLY;
        work->pos.y += 10.0F;
        work->scl.x = 0.3F;
        work->scl.z = 0.3F;
        work->scl.y = 4.6F;
        LOST_RING(work)->speed = 1.0F;
        LOST_RING(work)->spin =
            (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        work->timer = 0;
        work->submode = 0;
        LOST_RING(work)->slow = 1.0F;
        work->id = RING_ID;
        break;
    case LOST_RING_FLY:
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            LOST_RING(work)->spin = 0.0F;
            work->scl.x = -2.0F;
            work->scl.z = -4.0F;
            ring_sprite_init(work, &ring_sprite);
            work->pos.y += 3.44F;
            work->mode = LOST_RING_SPARKLE;
            if (!(player_get(0)->effects & PLAYER_FALLING)) {
                ring_add(1);
                sound_play(SE_RING, 0, 0, 0);
            }
            break;
        }
        /* look for the ground every RING_PROBE_COUNT + 1 steps */
        extra->probe -= 1.0F;
        /* @hack type: volatile on the count's read, which loads it back
         * after its store (FMOV.S @(R0,R13),FR2 at 0x8c08c2e8); plain, SHC
         * compares a copy of the FR3 it stores and 2 of 625 words differ */
        /* @hack spelling: the look as nested selects, which order the pool
         * as the image does (H'0000FFFD at 0x8c08c220); as ifs, rest_x and
         * rest_z trade FR4 and FR5, and 13 of 625 words differ */
        /* @hack spelling: the ground set in its test, which compares the FR0 it
         * stores (FCMP/EQ FR0,FR3 at 0x8c08c318); set apart, SHC compares a
         * copy and 173 of 625 words differ */
        (0.0F > ((volatile struct lost_ring_extra *)extra)->probe)
            ? (void)(extra->probe = RING_PROBE_COUNT,
                     (-GROUND_FAR == (extra->ground = ground_below(
                                          work->pos.x, work->pos.y + 6.88F,
                                          work->pos.z, &work->ang)))
                         ? (void)(extra->ground = work->pos.y - LOST_RING_DEPTH)
                         : (void)0)
            : (void)0;
        work->timer++;
        work->scl.x = njSin(work->ang.y) * 0.3F;
        work->scl.z = njCos(work->ang.y) * 0.3F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        if (LOST_RING(work)->speed >= LOST_RING_LIE_SPEED) {
            work->pos.x += work->scl.x * LOST_RING(work)->speed;
            work->pos.z += work->scl.z * LOST_RING(work)->speed;
        } else {
            LOST_RING(work)->slow += 0.08F;
            /* NOLINTBEGIN(clang-diagnostic-sometimes-uninitialized) */
            if (LOST_RING(work)->slow < 2.4F) {
                work->pos.x += work->scl.x * (0.3F / LOST_RING(work)->slow);
                work->pos.z += work->scl.z * (0.3F / LOST_RING(work)->slow);
            } else if (LOST_RING(work)->slow >= 13.0F) {
                up.x = 0.0F;
                up.y = 6.88F;
                up.z = 0.0F;
                njCalcVector(NULL, &up, &turned);
                rest_x = turned.x;
                rest_z = turned.z;
            }
            work->mode = LOST_RING_LIE;
            LOST_RING(work)->speed = extra->ground;
            /* @bug uninitialized: rest_x and rest_z are set only once slow
             * reaches 13, but the ring lies and reads them on its first slow
             * step, with slow at 1.08. scl.x and scl.z take FR4 and FR5 as
             * they stand (FMOV.S at 0x8c08c42c and 0x8c08c430), so a lying
             * ring's collision sits off its place by them */
            work->scl.x = rest_x;
            work->scl.z = rest_z;
            /* NOLINTEND(clang-diagnostic-sometimes-uninitialized) */
            njPopMatrix(1);
            break;
        }
        if (work->scl.y >= 0.0F) {
            work->pos.y += (work->scl.y - (TASK_TIMER(work) * (1.0F / 3.0F))) *
                           (1.0F / 3.0F);
        } else {
            work->pos.y = extra->ground;
        }
        up.x = 0.0F;
        up.y = 3.44F;
        up.z = 0.0F;
        njCalcVector(NULL, &up, &turned);
        lean_x = turned.x;
        lean_z = turned.z;
        if (extra->ground >= work->pos.y) {
            LOST_RING(work)->speed -= 0.04F;
            work->scl.y -= 0.28F;
            work->timer = 0;
        }
        up.x = 0.0F;
        up.y = 1.0F;
        up.z = 0.0F;
        njCalcVector(NULL, &up, &turned);
        if (turned.y < 0.7F) {
            ahead.x = work->scl.x;
            ahead.y = 1.0F;
            ahead.z = work->scl.z;
            njCalcPoint(NULL, &ahead, &heading);
            work->ang.y =
                (Angle)(atan2f(heading.x, heading.z) * ANGLE_PER_RADIAN);
        }
        njPopMatrix(1);
        work->submode++;
        if (work->submode > LOST_RING_FLY_STEPS) {
            work->submode = LOST_RING_FLY_STEPS + 1;
            y = work->pos.y;
            work->pos.y += 3.44F;
            collision_register(work);
            work->pos.y = y;
        }
        lost_ring_draw(task, extra->ground, lean_x, lean_z);
        break;
    case LOST_RING_LIE:
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            work->mode = LOST_RING_SPARKLE;
            LOST_RING(work)->spin = 0.0F;
            work->scl.x = -2.0F;
            work->scl.z = -4.0F;
            ring_sprite_init(work, &ring_sprite);
            work->pos.y += 3.44F;
            if (!(player_get(0)->effects & PLAYER_FALLING)) {
                ring_add(1);
                sound_play(SE_RING, 0, 0, 0);
            }
            break;
        }
        y = work->pos.y;
        x = work->pos.x;
        z = work->pos.z;
        work->pos.y += 3.44F;
        work->pos.x += work->scl.x;
        work->pos.z += work->scl.z;
        collision_register(work);
        work->pos.y = y;
        work->pos.x = x;
        work->pos.z = z;
        work->timer++;
        if (play_time_get() & 2) {
            lost_ring_draw(task, LOST_RING(work)->speed, work->scl.x,
                           work->scl.z);
        }
        if (TASK_TIMER(work) >= LOST_RING_LIE_STEPS) {
            work->mode = LOST_RING_KILL;
        }
        break;
    case LOST_RING_SPARKLE:
        ring_draw_sparkle(work);
        /* @bug logic: the test always holds: ring_draw_sparkle leaves the
         * mode LOST_RING_SPARKLE or sets RING_DONE, and both are 3 (CMP/EQ
         * #3,R0 at 0x8c08c706). so a scattered ring sparkles for one step
         * and goes */
        if (work->mode == LOST_RING_SPARKLE) {
            work->mode = LOST_RING_KILL;
        }
        break;
    case LOST_RING_KILL:
        task_destroy(task);
        break;
    }
}

static void ring_sprite_init(struct task_work *work, NJS_SPRITE *sprite)
{
    sprite->tlist = &ring_texlist;
    sprite->tanim = ring_texanims;
    sprite->sx = 1.0F;
    sprite->sy = 1.0F;
    sprite->p.x = sprite->p.y = sprite->p.z = 0.0F;
}

static void ring_destroy(struct task *task)
{
}

/* draw a scattered ring, spinning, with its shadow on the ground at height
 * ground. the shadow tilts with the ground's slope and shrinks as the ring
 * rises. x and z go unread */
static void lost_ring_draw(struct task *task, Float ground, Float x, Float z)
{
    struct task_work *work = task->work;
    Float scale;
    Angle ang;

    scale = 1.0F;
    scale /= (_builtin_fabsf(work->pos.y - ground) * 0.03F) + 1.0F;
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, ground + 0.4F, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, scale, scale, scale);
    draw_tree(shadow_object);
    njPopMatrix(1);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    LOST_RING(work)->spin += 5.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + 3.44F, work->pos.z);
    TURN_VIA(njRotateY, LOST_RING(work)->spin * ANGLE_PER_DEGREE, ang);
    draw_model(&ring_model);
    njPopMatrix(1);
    draw_attr_restore();
}

/* pull the ring toward player 0 while player 0 has a magnetic barrier. the
 * pull starts once the ring is within ATTRACT_RANGE, and goes on from any
 * distance, faster the faster player 0 moves. it returns TRUE while player
 * 0 has a barrier, and FALSE otherwise. on TRUE the ring has registered its
 * collision here, and its caller doesn't turn it */
static Bool ring_attract(struct task_work *work)
{
    struct task *task;
    struct task_work *player;
    struct player *record;
    Float distance;
    NJS_POINT3 target;
    NJS_VECTOR speed;

    /* @hack spelling: task and record each set in its test, which tests R0 and
     * copies it in the branch's delay slot (TST R0,R0 at 0x8c08c85a and
     * 0x8c08c868); set apart, SHC tests the copies and 6 of 183 words
     * differ */
    /* @hack redundant: the else after the return, which loads player_get's
     * address into R2 (MOV.L at 0x8c08c860); without it, R3, and 2 of 183
     * words differ */
    if ((task = player_get_task(0)) == NULL) {
        return FALSE;
    } else {
        player = task->work;
    }
    if ((record = player_get(0)) == NULL ||
        !(record->effects & PLAYER_MAGNETIC_BARRIER)) {
        return FALSE;
    }
    distance = njDistanceP2P(&work->pos, &player->pos);
    if (distance < ATTRACT_RANGE || TASK_TIMER(work) >= 1) {
        target.y = ATTRACT_HEIGHT;
        target.x = 0.0F;
        target.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, player->ang.y);
        njCalcPoint(NULL, &target, &target);
        njPopMatrix(1);
        target.x += player->pos.x;
        target.y += player->pos.y;
        target.z += player->pos.z;
        if (distance > ATTRACT_RANGE) {
            distance = ATTRACT_RANGE;
        }
        /* one FMUL by the constants' product, 0x3cd4fdf2, as SHC folds them */
        distance = distance * (0.02F * 1.3F);
        if (distance > 5.0F) {
            distance = 5.0F;
        } else if (distance < 0.85F) {
            distance = 0.85F;
        }
        /* @hack spelling: record set in the test, as above (TST R0,R0 at
         * 0x8c08c9ce); set apart, 3 of 183 words differ */
        if ((record = player_get(0)) != NULL) {
            speed.x = record->speed.x;
            speed.y = record->speed.y;
            speed.z = record->speed.z;
            distance *= (njScalor(&speed) * 0.5F) + 1.0F;
        }
        point_toward(&work->pos, &target, distance, &work->pos);
        work->timer++;
    }
    RING(work)->spin += 3.0F;
    collision_register(work);
    return TRUE;
}

void ring_scatter(Float x, Float y, Float z, int count)
{
    Float base;
    int i;
    struct task *lost;
    struct task_work *lost_work;

    base = rand() * RAND_SCALE * 360.0F;
    for (i = 0; i < count; i++) {
        lost = task_create(TASK_WORK | TASK_EXTRA, 2, lost_ring_step);
        if (lost != NULL) {
            lost_work = lost->work;
            lost_work->pos.x = x;
            lost_work->pos.y = y;
            lost_work->pos.z = z;
            /* the int division makes each heading's offset from base a whole
             * number of degrees */
            /* NOLINTBEGIN(bugprone-integer-division) */
            lost_work->ang.y =
                (Angle)(((i * 360 / count) + base) * ANGLE_PER_DEGREE);
            /* NOLINTEND(bugprone-integer-division) */
        }
    }
}
