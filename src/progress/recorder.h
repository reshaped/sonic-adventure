/* recorder: record pad 0 into memory, or play a recording back over it, for
 * the demos */
#ifndef PROGRESS_RECORDER_H
#define PROGRESS_RECORDER_H
#include <sg_xpt.h>

/* what recording_update does each pad scan: record, play back, or print
 * where a finished recording left recording_next */
enum recorder_mode { RECORDER_RECORD, RECORDER_PLAY, RECORDER_DONE };

/* what the next play asks of the recorder, as the title and the demos set
 * it. stage_reset_play starts recording_mode from it */
enum recording_kind { RECORDING_OFF, RECORDING_RECORD, RECORDING_PLAY };

/* the 128 KB that holds a recording: recording_update records into it, and
 * demo_advance reads a demo's recording file into it */
#define DEMO_RECORDING ((struct pad_sample *)0x0cbe0000)

/* a sample whose on field holds this ends the recording */
#define PAD_SAMPLE_END 0xffffffff

/* pad 0 as one pad scan reads it: 12 bytes */
struct pad_sample {
    Uint32 on; /* the buttons held, or PAD_SAMPLE_END */
    Uint16 l, r;
    Sint16 x1, y1;
};

/* TRUE while recording_update runs the recorder, and what it does */
extern Bool recording_on;
extern enum recorder_mode recording_mode;

extern enum recording_kind recording_kind;
/* the demo demo_advance plays next, 0 to 11 */
extern int demo_next;
/* the next sample to record or play. stage_reset_play sets it to
 * DEMO_RECORDING as play starts */
extern struct pad_sample *recording_next;
/* the buttons the last sample played gave pad 0. the next sample's press and
 * release are taken against them. stage_reset_play clears them */
extern Uint32 recording_buttons;
/* no code reads it. stage_reset_play sets it to 0x0cbc0000, 128 KB below
 * DEMO_RECORDING, as play starts */
extern void *dat_8c751454;

/* run the recorder for a pad scan. it does nothing unless recording_on is
 * set and demo_event is negative. recording copies pad 0 into the next
 * sample. playing writes the next sample over pad 0, but pad 0's own start
 * button stays as it is. holding x, or filling the 128 KB, ends a recording:
 * it writes PAD_SAMPLE_END and asks the host to save the recording, ignoring
 * the result. Eggman's and Tikal's recordings aren't saved. PAD_SAMPLE_END
 * ends a playback with STAGE_END_QUIT. so does passing the sample 1408 KB
 * past the buffer's end, so a recording without its end plays on into the
 * memory after it */
void recording_update(void);

#endif
