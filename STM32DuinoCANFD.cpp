/*  --------------------------------  *
 *  --  STM32DuinoCANFD.cpp       --  *
 *  --------------------------------  */
#include "STM32DuinoCANFD.hpp"

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
 * -- METHOD DEFINITIONS: FDCAN_Frame class                        -- *
 * --------------------------------------------------------------- */

// initialize empty frame 
FDCAN_Frame::FDCAN_Frame() {
    canId = 0;
    canDlc = 0;
    brs = true;

    memset(data, 0, sizeof(data));
}

// Clear all data from frame's byte array.
void FDCAN_Frame::clear() {
    memset(data, 0, sizeof(data));
}

// get unsigned data. this is our most important read function. every other data 
// type relies on this function to first get the "raw bits" out of the message
uint32_t FDCAN_Frame::GetUnsigned(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    uint32_t retVal  =  0; // value that all our bits get or'd into
    int8_t firstByte = -1; // use as a flag and also byte offset

    for(uint8_t i = 0; i < length; i++) {
        uint16_t absBit = startBit + i;  // walk over each individual bit
        uint8_t byteIndex = absBit / 8;  // get the byte number we're on

        if (order == Intel) { // yay the bits are in the correct order
            uint8_t shiftBy  = absBit % 8; 

            // bring bits down to right place and isolate individual bit
            uint8_t bit = (data[byteIndex] >> shiftBy) & 1u;

            // add to return value
            retVal |= bit << i;
        }
        else {               // motorola format >:(
            // set first byte value. this is used to reverse the 
            // direction in which we travel through the byte array
            if (firstByte < 0) firstByte = byteIndex;

            // move up instead of down
            if (byteIndex != firstByte) 
                byteIndex = firstByte - (byteIndex - firstByte);

            // get shift value. positive/negative indicates direction
            int8_t shiftBy  = (absBit % 8) - i;

            // add value to return value
            if (shiftBy >= 0)
                retVal |= (data[byteIndex] >>  shiftBy) & (1u << i);
            else 
                retVal |= (data[byteIndex] << -shiftBy) & (1u << i);
        }
    }

    return retVal;
}

// arbitrary length signed values. we handle moving the sign
// bit on our own so we can return a fixed size signed int
int32_t FDCAN_Frame::GetSigned(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    // get raw bits in unsigned value
    uint32_t rawValue = GetUnsigned(startBit, length, order);

    // shift the sign bit down to determine if value is negative
    bool isNeg = rawValue >> (length - 1) & 1u; 

    if (isNeg) {
        // generate bit mask for later or
        // 00001010 | 11111000 = 11111010
        int32_t bitMask = -1 << (length - 1);
        return rawValue | bitMask; 
    }

    return static_cast<int32_t>(rawValue);
}

// this is the one time I like floats. the can float 
// data types are the same fixed lengths as in c++
float FDCAN_Frame::GetFloat(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    uint32_t rawValue = GetUnsigned(startBit, length, order);
    float retVal = * ( float * ) &rawValue; // evil floating point bit hacking 

    return retVal;
}

// main data set function. just like with the receive side, the other data 
// set commands rely on this function to actually write the data into the array 
FDCAN_Status FDCAN_Frame::SetUnsigned(uint32_t value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    int8_t firstByte = -1;
    uint32_t upper = (1u << length) - 1;     // get max unsigned value

    // fail out if value doesn't fit in bit allocation
    if (value > upper || value < 0)
        return FDCAN_Status::INVALID_VALUE;

    for (uint8_t i = 0; i < length; i++) {
        uint8_t bit, shiftVal;
        uint16_t absBit = startBit + i; // absolute position of bit we're on
        uint8_t byteIndex = absBit / 8; // calculate which byte we're on

        if (order == Motorola) {
            // set first byte value. this is used to reverse the
            // direction in which we travel through the byte array
            if (firstByte < 0) firstByte = byteIndex; 

            // go up not down
            if (byteIndex != firstByte)
                byteIndex = firstByte - (byteIndex - firstByte);
        }

        bit = (value >> i) & 1u;
        shiftVal  = absBit % 8; 

        if (bit)
            data[byteIndex] |=  (1u << shiftVal);
        else
            data[byteIndex] &= ~(1u << shiftVal);
    }

    return FDCAN_Status::OK;
}

// convert signed bits to unsigned int value and use SetUnsigned to set value
// we have to manually mover the sign bit, because can signed ints are variable length
FDCAN_Status FDCAN_Frame::SetSigned(int32_t value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    // bit manipulation to get upper and lower limits
    int32_t lower = -1 << (length - 1);
    int32_t upper = ~lower;

    // fail out if value doesn't fit in bit allocation
    if (value > upper || value < lower)
        return FDCAN_Status::INVALID_VALUE;

    // get & bitmask for final value 
    // we can reuse upper here 
    uint32_t bitmask = ((uint32_t)upper << 1) + 1u;

    // apply the bitmask to the final value 
    value &= bitmask; 

    // set the value 
    return SetUnsigned(value, startBit, length, order);
}

// convert float value bits to unsigned int value and use SetUnsigned to set value
// this is convenient, because can floats are always 32 bits
FDCAN_Status FDCAN_Frame::SetFloat(float value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    // check for NaN
    if (value != value)
        return FDCAN_Status::INVALID_VALUE;

    uint32_t longVal = * ( uint32_t * ) &value; // evil floating point bit hacking
    return SetUnsigned(longVal, startBit, length, order);
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
    msg.brs      = (rxHeader.BitRateSwitch == FDCAN_BRS_ON);
    msg.FDFormat = (FDCAN_FrameFormat)rxHeader.FDFormat;

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
    FDCAN_ScalerStruct BaseScalers = FDCANScalers[Settings->NominalBitrate];
    FDCAN_ScalerStruct DataScalers = FDCANScalers[Settings->DataBitrate];

    // configuration of arbitration phase
    Interface.Init.NominalPrescaler     = BaseScalers.Prescaler;
    Interface.Init.NominalSyncJumpWidth = BaseScalers.SyncJump;
    Interface.Init.NominalTimeSeg1      = BaseScalers.Segment1;
    Interface.Init.NominalTimeSeg2      = BaseScalers.Segment2;

    // configuration of data phase
    Interface.Init.DataPrescaler        = DataScalers.Prescaler;
    Interface.Init.DataSyncJumpWidth    = DataScalers.SyncJump;
    Interface.Init.DataTimeSeg1         = DataScalers.Segment1;
    Interface.Init.DataTimeSeg2         = DataScalers.Segment2;

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
    TxHeader.FDFormat            = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker       = 0;
 
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

