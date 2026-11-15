/* the PC host's check of the DSP's recompiler (dsp_x86.cpp) against its
 * interpreter (dsp_interp.cpp), under HOST_DSP_CHECK=SECOND: at that second
 * of sound, sound.cpp saves the DSP's state, the AICA's registers and sound
 * RAM, runs a second's steps of the recompiled program and keeps each step's
 * outputs, EFREG's 16 words, and the sound RAM it leaves; it puts the state
 * back, runs the same steps through the interpreter, compares, and puts the
 * state back again, so that the sound goes on as if nothing had run.
 *
 * the interpreter is dsp_interp.cpp built again here, its runStep named
 * runStepInterpreter */
#define FLYCAST_DSP_INTERPRETER
#define runStep runStepInterpreter
#include "hw/aica/dsp_interp.cpp"
#undef runStep

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "hw/aica/aica_mem.h"

namespace aica::dsp
{
void runStep();
void recompile();
}

/* run the check, at the AICA's lock, where no sample is being made */
extern "C" void dsp_check(void)
{
    using namespace aica;
    const int steps = 44100;
    std::vector<u8> saved_ram(ARAM_SIZE), saved_regs(sizeof aica_reg), jit_ram(ARAM_SIZE);
    std::vector<u32> outputs[2];
    dsp::DSPState saved_state = dsp::state;
    int differ = 0, first = -1;

    memcpy(saved_ram.data(), &aica_ram[0], ARAM_SIZE);
    memcpy(saved_regs.data(), aica_reg, sizeof aica_reg);
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 0 && dsp::state.dirty) {
            dsp::recompile();
            dsp::state.dirty = false;
        }
        for (int k = 0; k < steps; k++) {
            /* the inputs as a mix would leave them: the MIXS the AICA's
             * channels feed, here a pattern of their own for each step */
            for (int m = 0; m < 16; m++) {
                dsp::state.MIXS[m] = (s32)(((unsigned)k * 2654435761u + (unsigned)m * 40503u) & 0xfffff) - 0x80000;
            }
            if (pass == 0) {
                dsp::runStep();
            } else {
                dsp::runStepInterpreter();
            }
            for (int r = 0; r < 16; r++) {
                outputs[pass].push_back(DSPData->EFREG[r]);
            }
        }
        if (pass == 0) {
            memcpy(jit_ram.data(), &aica_ram[0], ARAM_SIZE);
        }
        memcpy(&aica_ram[0], saved_ram.data(), ARAM_SIZE);
        memcpy(aica_reg, saved_regs.data(), sizeof aica_reg);
        dsp::state = saved_state;
    }
    int by_register[16] = {0}, low_differ = 0;

    for (size_t k = 0; k < outputs[0].size(); k++) {
        if (outputs[0][k] != outputs[1][k]) {
            differ++;
            by_register[k % 16]++;
            if (first < 0) {
                first = (int)k;
                fprintf(stderr, "host: DSP check: first difference, EFREG[%d] at step %d: %08x against %08x\n",
                        (int)(k % 16), (int)(k / 16), outputs[0][k], outputs[1][k]);
            }
        }
        /* the mixer reads each output's low 16 bits */
        low_differ += (outputs[0][k] & 0xffff) != (outputs[1][k] & 0xffff);
    }
    fprintf(stderr, "host: DSP check: by register:");
    for (int r = 0; r < 16; r++) {
        fprintf(stderr, " %d", by_register[r]);
    }
    fprintf(stderr, "; %d differ in the low 16 bits the mixer reads\n", low_differ);
    {
        /* the sound RAM the interpreter leaves, against the recompiler's */
        std::vector<u8> interp_ram(ARAM_SIZE);
        int ram_differ = 0;

        dsp::state = saved_state;
        for (int k = 0; k < steps; k++) {
            for (int m = 0; m < 16; m++) {
                dsp::state.MIXS[m] = (s32)(((unsigned)k * 2654435761u + (unsigned)m * 40503u) & 0xfffff) - 0x80000;
            }
            dsp::runStepInterpreter();
        }
        memcpy(interp_ram.data(), &aica_ram[0], ARAM_SIZE);
        for (size_t k = 0; k < ARAM_SIZE; k++) {
            ram_differ += interp_ram[k] != jit_ram[k];
        }
        memcpy(&aica_ram[0], saved_ram.data(), ARAM_SIZE);
        memcpy(aica_reg, saved_regs.data(), sizeof aica_reg);
        dsp::state = saved_state;
        fprintf(stderr,
                "host: DSP check: %s program, %d steps: %d of %zu outputs differ (first at step %d), %d bytes of "
                "sound RAM differ\n",
                saved_state.stopped ? "a stopped" : "a running", steps, differ, outputs[0].size(),
                first < 0 ? -1 : first / 16, ram_differ);
    }
}
