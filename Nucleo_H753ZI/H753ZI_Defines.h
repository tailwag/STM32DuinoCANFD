/*  -------------------------------  *
 *  -- H753ZI_Defines.h          --  *
 *  -------------------------------  */
#ifndef H753ZI_DEFINES_H_
#define H753ZI_DEFINES_H_
#include "stm32_def.h"
#include "stm32h753xx.h"
#include "stm32h7xx_hal_def.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_pwr.h"
#include "stm32h7xx_hal_fdcan.h"
#include "stm32h7xx_hal_rcc_ex.h"

inline uint32_t getCanClock(void) {
    return HAL_RCC_GetPCLK1Freq();
}
#endif
