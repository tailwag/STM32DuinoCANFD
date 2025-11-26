/*  -------------------------------  *
 *  -- G474RE_Defines.h          --  *
 *  -------------------------------  */
#ifndef G474RE_DEFINES_H_
#define G474RE_DEFINES_H_
#include "stm32def.h"
#include "stm32g474xx.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_rcc.h"
#include "stm32g4xx_hal_pwr.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_hal_rcc_ex.h"

inline uint32_t getCanClock(void) {
    return HAL_RCC_GetPCLK1Freq();
}
#endif
