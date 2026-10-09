/*
 * mathFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-29
 *  Author:     Simon Schäffler
 *
 *  Fixed-point Q0.15 sin-function using lookup table + interpolation
 *  No float, no FPU needed – fast on STM32F103
 */

#include "mathFunc.h"

typedef signed short   frac16_t;
typedef signed int     sint32;
typedef unsigned short uint16;
typedef unsigned int   uint32;

#define Q15_MAX       ((frac16_t)0x7FFF)   /* +1.0 in Q0.15        */
#define Q15_MIN       ((frac16_t)0x8000)   /* -1.0 in Q0.15        */
#define QUARTER_TURN  ((frac16_t)0x4000)   /* +PI/2 in angle format */

#define TABLE_BITS    8U                        /* 2^8 = 256 steps over 0..90 deg  */
#define TABLE_ENTRIES ((1U << TABLE_BITS) + 1U)  /* +1 for the closing 90 deg entry */
#define FRAC_BITS     (14U - TABLE_BITS)         /* remaining bits used for interpolation */
#define FRAC_MASK     ((1U << FRAC_BITS) - 1U)

/* abs() for frac16_t that correctly handles the asymmetric Q0.15 range
 * (0x8000 has no positive counterpart, so it is clamped to Q15_MAX). */
static frac16_t Saturated_Abs(frac16_t value)
{
    if (value == Q15_MIN)
    {
        return Q15_MAX;
    }
    return (value < 0) ? (frac16_t)(-value) : value;
}

/* Quarter-wave sine table for angles 0 .. 90 degrees in Q0.15 format.
     * Values were (re-)derived directly from sin(i * pi/2 / 256), i = 0..256,
     * truncated (not rounded) towards zero when converting to the Q0.15
     * step - cheaper to generate, at the cost of a slightly larger
     * worst-case quantization error than round-to-nearest would give. */
static const frac16_t sine_quadrant[TABLE_ENTRIES] =
{
    0x0000,0x00A0,0x0160,0x0220,0x0300,0x03A0,0x0480,0x0540,
    0x0600,0x06E0,0x07A0,0x0860,0x0940,0x09E0,0x0AC0,0x0B80,
    0x0C40,0x0D00,0x0DC0,0x0E80,0x0F40,0x1000,0x10C0,0x11C0,
    0x1280,0x1340,0x1400,0x14C0,0x1580,0x1640,0x1700,0x17C0,
    0x1880,0x1940,0x1A00,0x1AC0,0x1B80,0x1C40,0x1D00,0x1DC0,
    0x1E80,0x1F40,0x2000,0x20C0,0x2180,0x2240,0x2300,0x23C0,
    0x2480,0x2540,0x2600,0x26C0,0x2780,0x2840,0x2900,0x29C0,
    0x2A80,0x2B40,0x2C00,0x2CC0,0x2D80,0x2E40,0x2F00,0x2FC0,
    0x3080,0x3140,0x3200,0x32C0,0x3380,0x3440,0x3500,0x35C0,
    0x3680,0x3740,0x3800,0x38C0,0x3980,0x3A40,0x3B00,0x3BC0,
    0x3C80,0x3D40,0x3E00,0x3EC0,0x3F80,0x4040,0x4100,0x41C0,
    0x4280,0x4340,0x4400,0x44C0,0x4580,0x4640,0x4700,0x47C0,
    0x4880,0x4940,0x4A00,0x4AC0,0x4B80,0x4C40,0x4D00,0x4DC0,
    0x4E80,0x4F40,0x5000,0x50C0,0x5180,0x5240,0x5300,0x53C0,
    0x5480,0x5540,0x5600,0x56C0,0x5780,0x5840,0x5900,0x59C0,
    0x5A80,0x5B40,0x5C00,0x5CC0,0x5D80,0x5E40,0x5F00,0x5FC0,
    0x6080,0x6140,0x6200,0x62C0,0x6380,0x6440,0x6500,0x65C0,
    0x6680,0x6740,0x6800,0x68C0,0x6980,0x6A40,0x6B00,0x6BC0,
    0x6C80,0x6D40,0x6E00,0x6EC0,0x6F80,0x7040,0x7100,0x71C0,
    0x7280,0x7340,0x7400,0x74C0,0x7580,0x7640,0x7700,0x77C0,
    0x7880,0x7940,0x7A00,0x7AC0,0x7B80,0x7C40,0x7D00,0x7DC0,
    0x7E80,0x7F00,0x7F40,0x7F80,0x7FA0,0x7FC0,0x7FE0,0x7FF0,
    0x7FFF
};

    /* Folds an angle already known to lie in [0, +PI] down onto the first
     * quadrant [0, PI/2] by mirroring around the PI/2 axis, i.e.
     * angles above 90 degrees reuse sin(180 - x) = sin(x). */
    frac16_t Fold_Into_First_Quadrant(frac16_t absAngle)
    {
        if (absAngle > kQuarterTurn)
        {
            return static_cast<frac16_t>(kQ15Max - absAngle);
        }
        return absAngle;
    }

    /* Interpolates linearly between two neighbouring table entries given the
     * fractional position (0 .. kFracMask) between them. */
    frac16_t Interpolate(frac16_t lowerEntry, frac16_t upperEntry, unsigned fraction)
    {
        int32_t delta = static_cast<int32_t>(upperEntry) - static_cast<int32_t>(lowerEntry);
        int32_t step  = (delta * static_cast<int32_t>(fraction)) >> kFracBits;
        return static_cast<frac16_t>(lowerEntry + step);
    }
}

frac16_t MathF16_Sin(frac16_t angle)
{
    const frac16_t quadrantAngle = Fold_Into_First_Quadrant(Saturated_Abs(angle));

    const unsigned tableIndex = static_cast<unsigned>(static_cast<uint16_t>(quadrantAngle)) >> kFracBits;
    const unsigned fraction   = static_cast<unsigned>(static_cast<uint16_t>(quadrantAngle)) & kFracMask;

    const frac16_t lowerEntry = kSineQuadrant[tableIndex];

    frac16_t magnitude;
    if (tableIndex < (kTableEntries - 1U))
    {
        magnitude = Interpolate(lowerEntry, kSineQuadrant[tableIndex + 1U], fraction);
    }
    else
    {
        magnitude = lowerEntry;
    }

    return (angle < 0) ? static_cast<frac16_t>(-magnitude) : magnitude;
}
