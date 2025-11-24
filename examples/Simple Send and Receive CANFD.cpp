#include <Arduino.h>
#include "STM32DuinoCANFD.hpp"

FDCAN_Frame SendFrame;
FDCAN_Frame RecvFrame;
FDCAN_Instance can(FDCAN_Channel::CH1);

uint64_t splitnum = 0;
uint64_t sendLoop = 0;
uint64_t recvLoop = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Up");

    FDCAN_Settings Settings; // defaults to 500k/2M

    if (can.begin(&Settings) != FDCAN_Status::OK) {
        Serial.println("Failed to initialize CAN-FD Channel");
        while (true) { }
    }
}

void loop() {
    // send every 100ms
    if (micros() - sendLoop >= 100 * 1000) {
        SendFrame.canId = 0x200;
        SendFrame.canDlc = 8;

        // rollover if we reached max value, should be basically impossible
        splitnum = (splitnum == ~(uint64_t)0) ? 0 : ++splitnum;

        // split 64 bit number into 8 bytes
        for (uint8_t i = 0; i < 8; i++) {
            SendFrame.data[0] = splitnum >> i * 8 & 0xFF;
        }

        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK) {
            Serial.println("Error sending CAN frame!");
        }

        sendLoop = micros();
    }

    // check inbox every 1ms
    if (micros() - recvLoop >= 1 * 1000) {
        // pop returns FDCAN_Status::FIFO_EMPTY and does nothing
        // to the frame if the receive inbox is empty.
        if (can.inbox.pop(RecvFrame) == FDCAN_Status::OK) {
            Serial.print(millis());             // time
            Serial.print(" - 0x");
            Serial.print(RecvFrame.canId, HEX); // arbitration ID
            Serial.print(" [");
            Serial.print(RecvFrame.canDlc);     // DLC
            Serial.print("] - ");

            uint32_t numBytes = DlcToLen(RecvFrame.canDlc); 
            for (uint8_t i = 0; i < numBytes; i++) {
                Serial.print(RecvFrame.data[i], HEX); // data byte
                Serial.print(" ");
            }
            Serial.println();
        }
        recvLoop = micros();
    }
}
