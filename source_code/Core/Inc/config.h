/*
 * config.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-04-01
 *  Author:     Simon Schäffler
 *
 *  Central hardware configuration.
 *  Select ONE operating mode by uncommenting it.
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

/* ============================================================
 *  OPERATING MODE  –  uncomment exactly one
 * ============================================================ */

#define CFG_MODE_LAB          /* 48V AC lab supply  -> U_ZK = 67.88 V */
/* #define CFG_MODE_MAINS */  /* 230V AC mains      -> U_ZK = 325 V   */


/* ============================================================
 *  Derived parameters  –  do not edit below
 * ============================================================ */

#if defined(CFG_MODE_LAB)
    /* 48 V_ac * sqrt(2) = 67.88 V_dc */
    #define CFG_U_AC          48.0f       /* [V] AC input voltage (RMS)           */
    #define CFG_U_ZK          68          /* [V] DC link voltage (U_AC * sqrt(2)) */
    #define CFG_U_NENN        48          /* [V] nominal motor phase voltage      */
    #define CFG_F_NENN        50          /* [Hz] nominal motor frequency         */

#elif defined(CFG_MODE_MAINS)
    /* 230 V_ac * sqrt(2) = 325 V_dc */
    #define CFG_U_AC          230.0f      /* [V] AC input voltage (RMS)        */
    #define CFG_U_ZK          325         /* [V] DC link voltage (integer)      */
    #define CFG_U_NENN        230         /* [V] nominal motor phase voltage     */
    #define CFG_F_NENN        50          /* [Hz] nominal motor frequency        */

#else
    #error "config.h: No operating mode selected! Define CFG_MODE_LAB or CFG_MODE_MAINS."
#endif

#endif /* INC_CONFIG_H_ */
