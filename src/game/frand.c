#include "dolphin.h"

static u32 frand_seed;

extern s32 rand8(void);

/**
 * @brief Advance the pseudo-random number generator state.
 *
 * @param [in] param Previous generator state mixed into the next state.
 *
 * @return The next 32-bit generator state.
 */
static inline u32 frandom(u32 param)
{
    s32 rand2, rand3;

    if (param == 0) {
        param = rand8();
        param = param ^ (s64)OSGetTime();
        param ^= 0xD826BC89;
    }

    rand2 = param / (u32)0x1F31D;
    rand3 = param - (rand2 * 0x1F31D);
    param = rand2 * 0xB14;
    param =  param - rand3 * 0x41A7;
    return param;
}

/**
 * @brief Advance the global LCG state and return the new 32-bit state.
 *
 * @return A pseudo-random 32-bit value from the shared generator state.
 */
u32 frand(void) {
    return frand_seed = frandom(frand_seed);
}

/**
 * @brief Generate a pseudo-random floating-point value in the unit interval.
 *
 * @return A pseudo-random floating-point value in the range `[0.0, 1.0)`.
 */
f32 frandf(void) {
    u32 value = frand();
    f32 ret;
    value &= 0x7FFFFFFF;
    ret = (f32)value/2147483648;
    return ret;
}

/**
 * @brief Generate a pseudo-random value below a modulus.
 *
 * @param [in] exclusive_upper_bound Exclusive modulus upper bound; must be nonzero.
 *
 * @return A pseudo-random integer in the range
 *         `[0, exclusive_upper_bound)`.
 */
u32 frandmod(u32 exclusive_upper_bound) {
    u32 ret;
    frand_seed = frandom(frand_seed);
    ret = (frand_seed & 0x7FFFFFFF)%exclusive_upper_bound;
    return ret;
}
