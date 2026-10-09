/*
 * mathFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-29
 *  Author:     Simon Schäffler
 *
 *  Fixed-point Q0.15 math library
 *  Range: -1.0 = 0x8000, +1.0 = 0x7FFF
 *  Angle: -PI  = 0x8000, +PI  = 0x7FFF
 */

#ifndef INC_MATHFUNC_H_
#define INC_MATHFUNC_H_

typedef signed short    frac16_t;
typedef signed short    sint16;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

#define MATHF16_MAX      (sint16)0x7FFF
#define MATHF16_MIN      (sint16)0x8000
#define FRAC16_POS_HALF  (frac16_t)0x4000   /* = +90°  */
#define FRAC16_ONE_THIRD (frac16_t)0x5555   /* ≈ 120° = 2π/3 */

frac16_t MathF16_Sin(frac16_t angle);

#endif /* INC_MATHFUNC_H_ */
