#ifndef _FDCANH
#define _FDCANH
#include "stm32_def.h"
#include "stm32g474xx.h"
#include "stm32g4xx.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_gpio_ex.h"
#include "stm32g4xx_hal_rcc.h"
#include "stm32g4xx_hal_rcc_ex.h"
#include <stdint.h>
#endif

#ifndef _MAINH
#include "main.h"
#endif // !_MAINH


#ifdef __cplusplus
extern "C" {
#endif

extern FDCAN_HandleTypeDef hfdcan1; // PA12, PA11
extern FDCAN_HandleTypeDef hfdcan2;

void MX_FDCAN1_Init(void);
void MX_FDCAN2_Init(void);

void MX_FDCAN1_Filter_Init(void);
void MX_FDCAN2_Filter_init(void);

uint8_t dlcToLen(uint8_t canDlc);


#ifdef __cplusplus 
}
#endif
