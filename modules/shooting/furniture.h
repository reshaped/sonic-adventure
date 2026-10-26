/* the furniture: things from a house, which a hit burns up */
#ifndef SHOOTING_FURNITURE_H
#define SHOOTING_FURNITURE_H

#include "system/task.h"

/* the object list's steps for TV1, ISU1, ISU2, TANSU, KABIN, KAIGA, TV2,
 * REIZOU, SOFA, TANA and YAKAN */
void furniture_tv1_step(struct task *task);
void furniture_isu1_step(struct task *task);
void furniture_isu2_step(struct task *task);
void furniture_tansu_step(struct task *task);
void furniture_kabin_step(struct task *task);
void furniture_kaiga_step(struct task *task);
void furniture_tv2_step(struct task *task);
void furniture_reizou_step(struct task *task);
void furniture_sofa_step(struct task *task);
void furniture_tana_step(struct task *task);
void furniture_yakan_step(struct task *task);

#endif
