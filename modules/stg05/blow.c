/* O BLOW and O BLOW2, blowers that draw nothing. each advances its phases
 * and counters, and O BLOW plays a sound once every 318 steps */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg05/blow.h"
#include "system/task.h"

enum blow_mode { BLOW_INIT, BLOW_RUN };

/* counters A and B, at the indexes below. each holds a frame and the steps
 * left before the frame advances. nothing reads the frames */
struct blow_vars {
    Uint8 unk_00[4];
    Sint8 counts[4];
};

#define VARS(work) ((struct blow_vars *)&(work)->vars)
#define WAIT_A 0
#define FRAME_A 1
#define WAIT_B 2
#define FRAME_B 3

/* the phases, which advance PHASE_STEP a step and wrap to 0 at PHASE_CYCLE.
 * O BLOW2 runs only PHASE */
#define PHASE(work) ((work)->scl.z)
#define PHASE_BEHIND(work) ((work)->scl.y)
#define PHASE_STEP 0.5F
#define PHASE_CYCLE 159.0F
/* PHASE_BEHIND starts half a cycle behind PHASE */
#define PHASE_BEHIND_START (-PHASE_CYCLE / 2)

static void blow_draw(struct task *task);
static void blow_destroy(struct task *task);
static void blow_2_draw(struct task *task);
static void blow_2_destroy(struct task *task);

/* O BLOW's step. it advances both phases and counters A and B, and plays
 * SE_ROCK_BLAST as PHASE reaches 1. in the set editor, PHASE holds at 0 */
void blow_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        PHASE(work) = 0.0F;
    } else {
        switch (work->mode) {
        case BLOW_INIT:
            work->scl.x = 1.0F;
            PHASE(work) = 0.0F;
            PHASE_BEHIND(work) = PHASE_BEHIND_START;
            VARS(work)->counts[WAIT_A] = VARS(work)->counts[FRAME_A] = 0;
            VARS(work)->counts[WAIT_B] = VARS(work)->counts[FRAME_B] = 0;
            task->draw = blow_draw;
            task->destructor = blow_destroy;
            work->mode = BLOW_RUN;
            /* fall through */
        case BLOW_RUN:
            break;
        }
        PHASE(work) += PHASE_STEP;
        if (PHASE(work) >= PHASE_CYCLE) {
            PHASE(work) = 0.0F;
        }
        PHASE_BEHIND(work) += PHASE_STEP;
        if (PHASE_BEHIND(work) >= PHASE_CYCLE) {
            PHASE_BEHIND(work) = 0.0F;
        }
        if (PHASE(work) == 1.0F) {
            sound_play(SE_ROCK_BLAST, 0, 0, 0);
        }
        VARS(work)->counts[WAIT_A]--;
        if (VARS(work)->counts[WAIT_A] < 0) {
            VARS(work)->counts[WAIT_A] = 3;
            VARS(work)->counts[FRAME_A]++;
            if (VARS(work)->counts[FRAME_A] >= 2) {
                VARS(work)->counts[FRAME_A] = 0;
            }
        }
        VARS(work)->counts[WAIT_B]--;
        if (VARS(work)->counts[WAIT_B] < 0) {
            VARS(work)->counts[WAIT_B] = 4;
            VARS(work)->counts[FRAME_B]++;
            if (VARS(work)->counts[FRAME_B] >= 3) {
                VARS(work)->counts[FRAME_B] = 0;
            }
        }
    }
    blow_draw(task);
}

/* draws nothing */
static void blow_draw(struct task *task)
{
    /* @hack redundant: work, set and never read. SHC expands blow_draw in
     * blow_step, where the store keeps task in R13 and the constant 0 in R12
     * (0x0c911ae4, 0x0c911b14); without it, the two trade and 25 of the two
     * functions' 128 words differ */
    struct task_work *work = task->work;
}

static void blow_destroy(struct task *task)
{
}

/* O BLOW2's step. it advances PHASE and counters A and B. in the set editor,
 * it advances nothing */
void blow_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        work->scl.x = 1.0F;
    } else {
        switch (work->mode) {
        case BLOW_INIT:
            work->scl.x = 1.0F;
            PHASE(work) = 0.0F;
            VARS(work)->counts[WAIT_A] = VARS(work)->counts[FRAME_A] = 0;
            VARS(work)->counts[WAIT_B] = VARS(work)->counts[FRAME_B] = 0;
            task->draw = blow_2_draw;
            task->destructor = blow_2_destroy;
            work->mode = BLOW_RUN;
            /* fall through */
        case BLOW_RUN:
            break;
        }
        PHASE(work) += PHASE_STEP;
        if (PHASE(work) >= PHASE_CYCLE) {
            PHASE(work) = 0.0F;
        }
        VARS(work)->counts[WAIT_A]--;
        if (VARS(work)->counts[WAIT_A] < 0) {
            VARS(work)->counts[WAIT_A] = 3;
            VARS(work)->counts[FRAME_A]++;
            if (VARS(work)->counts[FRAME_A] >= 3) {
                VARS(work)->counts[FRAME_A] = 0;
            }
        }
        VARS(work)->counts[WAIT_B]--;
        if (VARS(work)->counts[WAIT_B] < 0) {
            VARS(work)->counts[WAIT_B] = 3;
            VARS(work)->counts[FRAME_B]++;
            if (VARS(work)->counts[FRAME_B] >= 3) {
                VARS(work)->counts[FRAME_B] = 0;
            }
        }
    }
    blow_2_draw(task);
}

/* draws nothing */
static void blow_2_draw(struct task *task)
{
    /* @hack redundant: work, set and never read. SHC expands blow_2_draw
     * in blow_2_step, where the store keeps a copy of task in R4 (MOV R13,R4
     * at 0x0c911d00); without it, blow_2_step is 2 bytes shorter and 8 of
     * the two functions' 101 words differ */
    struct task_work *work = task->work;
}

static void blow_2_destroy(struct task *task)
{
}
