/* a Chao's motions: which one in its table it plays, and how far in */
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/chao.h"
#include "al_main/motion.h"
#include "system/task.h"
#include "system/util.h"

/* the table of a Chao's motions, which alife_motion_init gives each Chao */
extern struct chao_motion_entry dat_0ca52380[];

/* a forward motion stops or blends this far short of its last frame */
#define END_SHORT 0.01F

void alife_motion_init(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;
    Float first;
    Float last;

    motion->current = 0;
    motion->next = 0;
    motion->id = 0;
    motion->table = dat_0ca52380;
    motion->end = motion->table[motion->current].end;
    motion->rate = 1.0F;
    first = motion->table[motion->current].first;
    last = motion->table[motion->current].last;
    motion->frame = first + (RAND_SCALE * ((last - first) * rand()));
    motion->flags &= 0xffc0;
}

void alife_motion_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;
    /* @hack spelling: current read into a local ahead of the table, which SHC
     * loads in that order (MOV.W @(6,R4),R0 at 0x0c90c1c4, then MOV.L
     * @(32,R4),R5); without it, 97 of 234 words differ */
    Sint16 current = motion->current;
    struct chao_motion_entry *entry = motion->table;

    entry += current;
    if (motion->flags & MOTION_DONE) {
        motion->flags &= ~MOTION_DONE;
    }
    if (motion->flags & MOTION_CHANGED) {
        motion->timer = 0;
    }
    motion->timer++;
    if (motion->flags & MOTION_NEW) {
        motion->flags &= ~(MOTION_NEW | MOTION_ENDED);
        motion->next = motion->id;
        motion->next_frame = motion->table[motion->id].first;
        motion->flags |= MOTION_BLEND;
    }
    if ((motion->flags & MOTION_ENDED) || (motion->flags & MOTION_BLEND)) {
        return;
    }
    motion->frame += entry->speed * motion->rate;
    if (entry->speed > 0.0F) {
        if (motion->frame >= entry->last) {
            /* @hack spelling: MOTION_DONE and the rate set in each forward
             * case, which SHC merges into one tail after END_STOP's
             * (0x0c90c31c to 0x0c90c326); without it, 102 of 234 words
             * differ */
            switch (entry->end) {
            case END_LOOP:
                motion->frame -= entry->last - entry->first;
                motion->flags |= MOTION_DONE;
                motion->rate = 1.0F;
                break;
            case END_BLEND:
                motion->flags |= MOTION_BLEND;
                motion->next = entry->next;
                motion->frame = entry->last - END_SHORT;
                motion->next_frame = motion->table[entry->next].first;
                motion->flags |= MOTION_DONE;
                motion->rate = 1.0F;
                break;
            case END_CHANGE:
                motion->current = entry->next;
                motion->flags |= MOTION_CHANGED;
                motion->flags |= MOTION_DONE;
                motion->rate = 1.0F;
                break;
            case END_STOP:
                motion->frame = entry->last - END_SHORT;
                motion->flags |= MOTION_ENDED;
                motion->flags |= MOTION_DONE;
                motion->rate = 1.0F;
                break;
            default:
                return;
            }
            return;
        }
    } else if (entry->speed < 0.0F) {
        if (motion->frame > entry->last) {
            return;
        }
        switch (entry->end) {
        case END_LOOP:
            motion->frame -= entry->last - entry->first;
            break;
        case END_BLEND:
            motion->flags |= MOTION_BLEND;
            motion->flags |= MOTION_CHANGED;
            motion->flags |= MOTION_DONE;
            motion->rate = 1.0F;
            motion->next = entry->next;
            motion->frame = entry->last;
            motion->next_frame = motion->table[entry->next].first;
            return;
        case END_CHANGE:
            motion->current = entry->next;
            motion->flags |= MOTION_CHANGED;
            motion->rate = 1.0F;
            break;
        case END_STOP:
            motion->frame = entry->last;
            motion->flags |= MOTION_ENDED;
            break;
        default:
            return;
        }
        motion->flags |= MOTION_DONE;
        motion->rate = 1.0F;
    }
}
