/*  -------------------------------  *
 *  -- G0B1RE_Defines.h          --  *
 *  -------------------------------  */
#ifndef G0B1RE_DEFINES_H_
#define G0B1RE_DEFINES_H_
#include "stm32_def.h"
#include "stm32g0b1xx.h"
#include "stm32g0xx_hal_def.h"
#include "stm32g0xx_hal_rcc.h"
#include "stm32g0xx_hal_pwr.h"
#include "stm32g0xx_hal_fdcan.h"
#include "stm32g0xx_hal_rcc_ex.h"

inline uint32_t getCanClock(void) {
    return HAL_RCC_GetPCLK1Freq();
}
#endif
