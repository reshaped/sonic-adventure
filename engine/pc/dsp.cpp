/* the AICA's DSP, from its registers' documented fields and what each does. a
 * step multiplies and adds into a 26-bit accumulator. what it writes is the
 * accumulator the step before left, shifted to 24 bits. that goes to a
 * temporary, the ring, an output, frc or adrs. temporaries and, without TABLE,
 * the ring are addressed from a count that falls by one a sample. so a fixed
 * address is a fixed delay. unless NOFL is set, the ring keeps a word as a
 * 16-bit float. it holds a sign, four bits of shift and 11 of mantissa. a word
 * read from the ring waits in memval until a later step's IWT copies it to
 * mems_ */
#include "dsp.h"

#include <string.h>

namespace pc {
namespace {

uint32_t read32(const uint8_t *at)
{
    return uint32_t{at[0]} | uint32_t{at[1]} << 8 | uint32_t{at[2]} << 16 |
           uint32_t{at[3]} << 24;
}

/* x's low bits, as many as bits, sign-extended */
int32_t sign(int32_t x, int bits)
{
    uint32_t m = 1U << (bits - 1);
    uint32_t v = static_cast<uint32_t>(x) & ((m << 1) - 1);

    return static_cast<int32_t>((v ^ m) - m);
}

int32_t clamp24(int32_t x)
{
    return x > 0x7fffff ? 0x7fffff : x < -0x800000 ? -0x800000 : x;
}

/* 24 bits to the ring's float. the shift counts the bits below the sign
 * that repeat it, at most 12. the bit that ends a shorter run is implied
 * and left out. the mantissa is the next 11 bits */
uint16_t pack(int32_t x)
{
    uint32_t v = static_cast<uint32_t>(x) & 0xffffff;
    uint32_t s = v >> 23;
    uint32_t shift = 0;

    while (shift < 12 && (v >> (22 - shift) & 1) == s) {
        shift++;
    }
    uint32_t mantissa = shift < 12 ? v >> (11 - shift) & 0x7ff : v & 0x7ff;

    return static_cast<uint16_t>(s << 15 | shift << 11 | mantissa);
}

int32_t unpack(uint16_t w)
{
    uint32_t s = w >> 15, shift = w >> 11 & 15, mantissa = w & 0x7ff;
    uint32_t v = s << 23 | mantissa << 11;

    /* under 12, the implied bit below the sign is its opposite. from 12,
     * that bit repeats the sign, and a shift of 11 brings the mantissa down */
    if (shift < 12) {
        v |= (s ^ 1) << 22;
    } else {
        v |= s << 22;
        shift = 11;
    }
    return sign(static_cast<int32_t>(v), 24) >> shift;
}

} // namespace

void Dsp::load(const uint8_t *image, uint32_t words)
{
    for (uint32_t i = 0; i < 128; i++) {
        coef_[i] = static_cast<int16_t>(read32(image + 4 * i));
    }
    for (uint32_t i = 0; i < 64; i++) {
        madrs_[i] = static_cast<uint16_t>(read32(image + 0x200 + 4 * i));
    }
    for (uint32_t i = 0; i < 128; i++) {
        const uint8_t *at = image + 0x400 + 16 * i;
        uint32_t w0 = read32(at) & 0xffff, w1 = read32(at + 4) & 0xffff;
        uint32_t w2 = read32(at + 8) & 0xffff, w3 = read32(at + 12) & 0xffff;
        Step &s = steps_[i];

        s.tra = w0 >> 9 & 0x7f;
        s.twt = w0 >> 8 & 1;
        s.twa = w0 >> 1 & 0x7f;
        s.xsel = w1 >> 15 & 1;
        s.ysel = w1 >> 13 & 3;
        s.ira = w1 >> 7 & 0x3f;
        s.iwt = w1 >> 6 & 1;
        s.iwa = w1 >> 1 & 0x1f;
        s.table = w2 >> 15 & 1;
        s.mwt = w2 >> 14 & 1;
        s.mrd = w2 >> 13 & 1;
        s.ewt = w2 >> 12 & 1;
        s.ewa = w2 >> 8 & 15;
        s.adrl = w2 >> 7 & 1;
        s.frcl = w2 >> 6 & 1;
        s.shift = w2 >> 4 & 3;
        s.yrl = w2 >> 3 & 1;
        s.negb = w2 >> 2 & 1;
        s.zero = w2 >> 1 & 1;
        s.bsel = w2 & 1;
        s.nofl = w3 >> 15 & 1;
        s.masa = w3 >> 9 & 0x3f;
        s.adreb = w3 >> 8 & 1;
        s.nxadr = w3 >> 7 & 1;
    }
    uint32_t size = RING_LEAST;

    while (size < words && size < RING) {
        size <<= 1;
    }
    ring_mask_ = size - 1;
    memset(temp_, 0, sizeof temp_);
    memset(mems_, 0, sizeof mems_);
    memset(mixs_, 0, sizeof mixs_);
    /* silence in the ring's float is pack(0), 0x6000. a word of 0 reads as
     * half the most, by its implied bit */
    for (uint16_t &w : ring_) {
        w = pack(0);
    }
    dec_ = 0;
    running_ = true;
}

void Dsp::set_coefficient(uint32_t step, int16_t value)
{
    coef_[step & 127] = value;
}

void Dsp::send(uint32_t input, int32_t sample)
{
    /* mixs_ holds 20 bits: a 16-bit sample from bit 4 */
    if (running_) {
        mixs_[input & 15] += sample * 16;
    }
}

void Dsp::run(int32_t outputs[16])
{
    int32_t acc = 0, frc = 0, yreg = 0, memval = 0;
    uint32_t adrs = 0;

    memset(outputs, 0, 16 * sizeof outputs[0]);
    if (!running_) {
        return;
    }
    for (uint32_t i = 0; i < 128; i++) {
        const Step &s = steps_[i];
        int32_t inputs = 0;

        if (s.ira < 0x20) {
            inputs = mems_[s.ira];
        } else if (s.ira < 0x30) {
            int32_t m = mixs_[s.ira - 0x20];

            m = m > 0x7ffff ? 0x7ffff : m < -0x80000 ? -0x80000 : m;
            inputs = m * 16;
        }
        if (s.iwt) {
            mems_[s.iwa] = memval;
            if (s.ira == s.iwa) {
                inputs = memval;
            }
        }
        int32_t temp = temp_[(s.tra + dec_) & 127];
        int32_t b = s.zero ? 0 : s.bsel ? acc : temp;
        int32_t x = s.xsel ? inputs : temp;
        int32_t y;

        if (s.negb) {
            b = -b;
        }
        switch (s.ysel) {
        case 0:
            y = frc;
            break;
        case 1:
            y = coef_[i] >> 3;
            break;
        case 2:
            y = yreg >> 11 & 0x1fff;
            break;
        default:
            y = yreg >> 4 & 0xfff;
            break;
        }
        y = sign(y, 13);
        if (s.yrl) {
            yreg = inputs;
        }
        /* the accumulator the step before left, doubled where SHIFT is odd.
         * SHIFT 0 and 1 hold it to 24 bits, and 2 and 3 wrap it */
        int32_t shifted = s.shift & 1 ? acc * 2 : acc;

        shifted = s.shift < 2 ? clamp24(shifted) : sign(shifted, 24);
        acc = sign(static_cast<int32_t>(static_cast<int64_t>(x) * y >> 12) + b,
                   26);
        if (s.twt) {
            temp_[(s.twa + dec_) & 127] = shifted;
        }
        if (s.frcl) {
            frc = s.shift == 3 ? shifted & 0xfff : shifted >> 11 & 0x1fff;
        }
        if (s.mrd || s.mwt) {
            uint32_t address = madrs_[s.masa];

            if (!s.table) {
                address += dec_;
            }
            if (s.adreb) {
                address += adrs & 0xfff;
            }
            address += s.nxadr;
            address &= s.table ? RING - 1 : ring_mask_;
            if (s.mrd) {
                memval = s.nofl ? sign(ring_[address] << 8, 24)
                                : unpack(ring_[address]);
            }
            if (s.mwt) {
                ring_[address] = s.nofl ? static_cast<uint16_t>(shifted >> 8)
                                        : pack(shifted);
            }
        }
        if (s.adrl) {
            adrs = static_cast<uint32_t>(s.shift == 3 ? shifted >> 12 & 0xfff
                                                      : inputs >> 16);
        }
        if (s.ewt) {
            outputs[s.ewa] += shifted >> 8;
        }
    }
    dec_--;
    memset(mixs_, 0, sizeof mixs_);
}

} // namespace pc
