/*  --------------------------------  *
 *  --  STM32DuinoCANFD.cpp       --  *
 *  --------------------------------  */
#include "STM32DuinoCANFD.h"
#include "boards/selector.c"

// global channel definition
#if defined FDCAN3
FDCAN_GlobalTypeDef * AvailableChannels[3] = { FDCAN1, FDCAN2, FDCAN3};
#elif defined FDCAN2
FDCAN_GlobalTypeDef * AvailableChannels[2] = { FDCAN1, FDCAN2};
#elif defined FDCAN1
FDCAN_GlobalTypeDef * AvailableChannels[1] = { FDCAN1 };
#endif

/* --------------------------------------------------- *
 * -- global interupt handlers                      -- *
 * -- these catch the global interupts and call the -- *
 * -- interupt handlers within each channel object  -- *
 * --------------------------------------------------- */
#if defined (ARDUINO_NUCLEO_G474RE) || defined (ARDUINO_NUCLEO_H753ZI)
#ifdef FDCAN1
extern "C" void FDCAN1_IT0_IRQHandler(void) {
    auto inst = FDCAN_Instance::getInstance(CH1);
    if (inst)
        HAL_FDCAN_IRQHandler(inst->getHandle());
}
#endif

#ifdef FDCAN2
extern "C" void FDCAN2_IT0_IRQHandler(void) {
    auto inst = FDCAN_Instance::getInstance(CH2);
    if (inst)
        HAL_FDCAN_IRQHandler(inst->getHandle());
}
#endif

#ifdef FDCAN3
extern "C" void FDCAN3_IT0_IRQHandler(void) {
    auto inst = FDCAN_Instance::getInstance(CH3);
    if (inst)
        HAL_FDCAN_IRQHandler(inst->getHandle());
}
#endif
#elif defined (ARDUINO_NUCLEO_G0B1RE)
extern "C" {
    FDCAN_HandleTypeDef *phfdcan1 = NULL;
    FDCAN_HandleTypeDef *phfdcan2 = NULL;
}
#endif

// look up table for canfd dlc
uint32_t DlcToLen(uint32_t dlcIn) {
    // sanitize input
    if (dlcIn < 0 || dlcIn > 15)
        return 0;

    uint32_t d[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                      12, 16, 20, 24, 32, 48, 64};

    return d[dlcIn];
}

/* --------------------------------------------------------------- *
 * -- METHOD DEFINITIONS: FDCAN_Inbox class                        -- *
 * --------------------------------------------------------------- */
FDCAN_Status FDCAN_Inbox::push(const FDCAN_RxHeaderTypeDef &rxHeader, const uint8_t *data) {
    // if next message is out of bounds, loop back around to start
    uint8_t next = (head + 1) % MAX_MESSAGES;

    // overwrite oldest if full
    if (next == tail)
        tail = (tail + 1) % MAX_MESSAGES; 

    // select oldest slot to write data into
    FDCAN_Frame &msg = buffer[head];

    msg.canId    = rxHeader.Identifier;
    msg.canDlc   = rxHeader.DataLength; // >> 16 // HAL encodes DLC in bits 19:16

    if (rxHeader.FDFormat == FDCAN_CLASSIC_CAN) {
        msg.format = FDCAN_FrameFormat::CLASSIC;
    }
    else if (rxHeader.BitRateSwitch == FDCAN_BRS_OFF) {
        msg.format = FDCAN_FrameFormat::FD_NO_BRS;
    }
    else {
        msg.format = FDCAN_FrameFormat::FD_BRS;
    }

    memcpy(msg.data, data, DlcToLen(msg.canDlc));

    head = next; 
    return FDCAN_Status::OK;
}

FDCAN_Status FDCAN_Inbox::pop(FDCAN_Frame &out) {
    if (head == tail)
        return FDCAN_Status::FIFO_EMPTY;

    out  = buffer[tail];
    tail = (tail + 1) % MAX_MESSAGES;

    return FDCAN_Status::OK;
}

/* --------------------------------------------------------------- *
 * -- METHOD DEFINITIONS: FDCAN_Instance class                    -- *
 * --------------------------------------------------------------- */
// constructor for FDCAN_Instance class
FDCAN_Instance::FDCAN_Instance(FDCAN_Channel chan) {
    ChannelID = chan;

    // store pointer to object in global array
    Instances[chan] = this;

    // get can interface handle 
    Interface.Instance = AvailableChannels[chan];
}

// global array with pointers back the each can channel object
#if defined FDCAN3
FDCAN_Instance* FDCAN_Instance::Instances[3] = { nullptr, nullptr, nullptr };
#elif defined FDCAN2
FDCAN_Instance* FDCAN_Instance::Instances[2] = { nullptr, nullptr};
#elif defined FDCAN1
FDCAN_Instance* FDCAN_Instance::Instances[2] = { nullptr };
#endif

void FDCAN_Instance::handleRxInterrupt() {
    FDCAN_RxHeaderTypeDef rxHeader; 
    uint8_t rxData[64];

    if (HAL_FDCAN_GetRxMessage(&Interface, FDCAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK) {
        Error_Handler();
        return;
    }

    inbox.push(rxHeader, rxData);
    timeLastRecv = HAL_GetTick();
}

FDCAN_Status FDCAN_Instance::begin(FDCAN_Settings *Settings) {
    // calculate clock sources from desired bitrates
    // retrieve scaler values using the index of the desired bitrate
    // bitrate = FDCAN_CLOCK / (Prescaler * (SyncSeg + TimeSeg1 + TimeSeg2))

    // configuration of arbitration phase
    Interface.Init.NominalPrescaler     = Settings->GetNominalPrescaler();
    Interface.Init.NominalSyncJumpWidth = Settings->GetNominalSyncJump();
    Interface.Init.NominalTimeSeg1      = Settings->GetNominalSegment1();
    Interface.Init.NominalTimeSeg2      = Settings->GetNominalSegment2();

    // configuration of data phase
    Interface.Init.DataPrescaler        = Settings->GetDataPrescaler();
    Interface.Init.DataSyncJumpWidth    = Settings->GetDataSyncJump();
    Interface.Init.DataTimeSeg1         = Settings->GetDataSegment1();
    Interface.Init.DataTimeSeg2         = Settings->GetDataSegment2();

    // setup static values
    Interface.Init.FrameFormat          = Settings->FrameFormat; 
    Interface.Init.Mode                 = Settings->Mode;
    Interface.Init.AutoRetransmission   = Settings->AutoRetransmission;
    Interface.Init.TransmitPause        = Settings->TransmitPause;
    Interface.Init.ProtocolException    = Settings->ProtocolException;

    // filter setup
    Interface.Init.StdFiltersNbr        = Settings->StdFiltersNbr;
    Interface.Init.ExtFiltersNbr        = Settings->ExtFiltersNbr;
    Interface.Init.TxFifoQueueMode      = Settings->TxFifoQueueMode;

    #ifdef ARDUINO_NUCLEO_H753ZI
    Interface.Init.MessageRAMOffset     = Settings->MessageRAMOffset;
    Interface.Init.RxFifo0ElmtsNbr      = Settings->RxFifo0ElmtsNbr;
    Interface.Init.RxFifo0ElmtSize      = Settings->RxFifo0ElmtSize;
    Interface.Init.RxFifo1ElmtsNbr      = Settings->RxFifo1ElmtsNbr;
    Interface.Init.RxFifo1ElmtSize      = Settings->RxFifo1ElmtSize;
    Interface.Init.RxBuffersNbr         = Settings->RxBuffersNbr;
    Interface.Init.RxBufferSize         = Settings->RxBufferSize;
    Interface.Init.TxEventsNbr          = Settings->TxEventsNbr;
    Interface.Init.TxBuffersNbr         = Settings->TxBuffersNbr;
    Interface.Init.TxFifoQueueElmtsNbr  = Settings->TxFifoQueueElmtsNbr;
    Interface.Init.TxElmtSize           = Settings->TxElmtSize;
    #endif

    // initialize interface
    if (HAL_FDCAN_Init(&Interface) != HAL_OK) {
        return FDCAN_Status::INIT_FAILED;
    }

    __HAL_RCC_FDCAN_CLK_ENABLE();

    if (HAL_FDCAN_Start(&Interface) != HAL_OK) {
        return FDCAN_Status::START_FAILED;
    }

    // enable receive callback. when a message is received, it fires 
    // the callback definied at the top of this file. that callback then
    // looks up a pointer to the object of the corresponding channel,
    // then fires the handleRxInterrupt() method. this add's the message 
    // to the channel object's included "mailbox", which is a ring buffer
    HAL_FDCAN_ActivateNotification(&Interface, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

    // Enable IRQ in NVIC
    #if defined (ARDUINO_NUCLEO_G474RE) || defined (ARDUINO_NUCLEO_H753ZI)
    if (Interface.Instance == FDCAN1) {
        HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    }
    #ifdef FDCAN2
    else if (Interface.Instance == FDCAN2) {
        HAL_NVIC_SetPriority(FDCAN2_IT0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
    }
    #endif
    #ifdef FDCAN3
    else if (Interface.Instance == FDCAN3) {
        HAL_NVIC_SetPriority(FDCAN3_IT0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
    }
    #endif

    #elif defined (ARDUINO_NUCLEO_G0B1RE)
    if (Interface.Instance == FDCAN1) {
        HAL_NVIC_SetPriority(TIM16_FDCAN_IT0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(TIM16_FDCAN_IT0_IRQn);
    }
    #ifdef FDCAN2
    else if (Interface.Instance == FDCAN2) {
        HAL_NVIC_SetPriority(TIM17_FDCAN_IT1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(TIM17_FDCAN_IT1_IRQn);
    }
    #endif
#endif
    return FDCAN_Status::OK;
}

// send frame function
FDCAN_Status FDCAN_Instance::sendFrame(FDCAN_Frame * Frame) {
    FDCAN_TxHeaderTypeDef TxHeader;

    uint8_t canDlc = Frame->canDlc;
    uint16_t canId = Frame->canId;

    // set min and max values
    canId  = (canId  < 0)     ? 0     : canId;  // min 0
    canId  = (canId  > 0x7FF) ? 0x7FF : canId;  // max 0x7FF (11 bit)
    canDlc = (canDlc < 0)     ? 0     : canDlc; // min 0
    canDlc = (canDlc > 15)    ? 15    : canDlc; // max 15 (64 bytes)

    TxHeader.Identifier          = canId;
    TxHeader.DataLength          = canDlc;
    TxHeader.IdType              = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE; 
    TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker       = 0;

    if (Frame->format == FDCAN_FrameFormat::CLASSIC) {
        TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
        TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    }
    else if (Frame->format == FDCAN_FrameFormat::FD_NO_BRS) {
        TxHeader.FDFormat = FDCAN_FD_CAN;
        TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    }
    else { // default to FDBRS
        TxHeader.FDFormat = FDCAN_FD_CAN;
        TxHeader.BitRateSwitch = FDCAN_BRS_ON;
    }

    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(&Interface, &TxHeader, Frame->data);

    if (status != HAL_OK) 
        return FDCAN_Status::SEND_FAILED;

    return FDCAN_Status::OK;
}

/* ---------------------------------------------------- *
 * -- RECEIVE CALLBACK FUNCTION - CALLED BY HAL      -- *
 * ---------------------------------------------------- */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if(!(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE))
        return;

    for (int i = 0; i < 3; i++) {
        // get hardware channel ID for iterator from enum 
        FDCAN_Channel c = static_cast<FDCAN_Channel>(i);

        // FDCAN_Instance::getInstance(FDCAN_Channel) returns pointer to object
        // if an instance hasn't been initialized yet, it returns a nullptr
        if (FDCAN_Instance::getInstance(c) && hfdcan->Instance == AvailableChannels[i]) {
            // use the interupt handler for the specific channel
            FDCAN_Instance::getInstance(c)->handleRxInterrupt();
            break;
        }
    }
}

// function to initialize the physical interface in the HAL 
// to be honest, I'm not sure where this is getting called
// it was generated by the STM32Cube project I used to develop
// this library, but I know nothing works if this isn't here
static uint32_t HAL_RCC_FDCAN_CLK_ENABLED = 0;
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef * fdcanHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (fdcanHandle->Instance == FDCAN1) {
        // initialize peripheral clocks 
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // fdcan1 clock enable 
        HAL_RCC_FDCAN_CLK_ENABLED++;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
            __HAL_RCC_FDCAN_CLK_ENABLE();

        // gpio clock enable
        #if defined (ARDUINO_NUCLEO_G474RE) || defined (ARDUINO_NUCLEO_H753ZI)
        __HAL_RCC_GPIOA_CLK_ENABLE();
        #elif defined (ARDUINO_NUCLEO_G0B1RE)
        __HAL_RCC_GPIOC_CLK_ENABLE();
        #endif

        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP; 
        GPIO_InitStruct.Pull  = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        #if defined (ARDUINO_NUCLEO_H753ZI) || defined (ARDUINO_NUCLEO_G474RE)
        GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        #elif defined (ARDUINO_NUCLEO_G0B1RE)
        GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
        GPIO_InitStruct.Alternate = GPIO_AF3_FDCAN1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        #endif
    }
    #ifdef FDCAN2
    else if (fdcanHandle->Instance == FDCAN2) {
        // initialize peripheral clocks 
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // fdcan1 clock enable 
        HAL_RCC_FDCAN_CLK_ENABLED++;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
            __HAL_RCC_FDCAN_CLK_ENABLE();

        
        // gpio clock enable
        #if defined (ARDUINO_NUCLEO_G474RE) || defined (ARDUINO_NUCLEO_H753ZI)
        __HAL_RCC_GPIOB_CLK_ENABLE();
        #elif defined (ARDUINO_NUCLEO_G0B1RE)
        __HAL_RCC_GPIOC_CLK_ENABLE();
        #endif

        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        #if defined (ARDUINO_NUCLEO_H753ZI) || defined (ARDUINO_NUCLEO_G474RE)
        GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        #elif defined (ARDUINO_NUCLEO_G0B1RE)
        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
        GPIO_InitStruct.Alternate = GPIO_AF3_FDCAN2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        #endif
    }
    #endif
    #ifdef FDCAN3
    else if (fdcanHandle->Instance == FDCAN3) {
        // initialize peripheral clocks 
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // fdcan1 clock enable 
        HAL_RCC_FDCAN_CLK_ENABLED++;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
            __HAL_RCC_FDCAN_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

#if defined (ARDUINO_NUCLEO_G474RE)
        GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif
    }
    #endif
}
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef * fdcanHandle) {
    if (fdcanHandle->Instance == FDCAN1) {
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if(HAL_RCC_FDCAN_CLK_ENABLED == 0) 
            __HAL_RCC_FDCAN_CLK_DISABLE();

        #if defined (ARDUINO_NUCLEO_H753ZI) || defined (ARDUINO_NUCLEO_G474RE)
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);
        #elif #defined (ARDUINO_NUCLEO_G0B1RE)
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_4|GPIO_PIN_5);
        #endif
    }
    #ifdef FDCAN2
    else if (fdcanHandle->Instance == FDCAN2) {
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if(HAL_RCC_FDCAN_CLK_ENABLED == 0) 
            __HAL_RCC_FDCAN_CLK_DISABLE();

        #if defined (ARDUINO_NUCLEO_H753ZI) || defined (ARDUINO_NUCLEO_G474RE)
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);
        #elif #defined (ARDUINO_NUCLEO_G0B1RE)
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2|GPIO_PIN_3);
        #endif
    }
    #endif // FDCAN2
    #ifdef FDCAN3
    else if (fdcanHandle->Instance == FDCAN3) {
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if(HAL_RCC_FDCAN_CLK_ENABLED == 0) 
            __HAL_RCC_FDCAN_CLK_DISABLE();
        #if defined (ARDUINO_NUCLEO_G474RE)
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);
        #endif 
    }
    #endif //FDCAN3
}

