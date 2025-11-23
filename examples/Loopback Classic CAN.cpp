#include <Arduino.h>
#include "STM32DuinoCANFD.hpp"

FDCAN_Frame SendFrame;
FDCAN_Frame RecvFrame;
FDCAN_Instance can(FDCAN_Channel::CH1);

uint64_t sendLoop = 0;
uint64_t recvLoop = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Up");

    FDCAN_Settings Settings;
    Settings.Mode            = FDCAN_Mode::INTERNAL_LOOPBACK;
    Settings.NominalBitrate  = FDCAN_Bitrate::b500000;
    Settings.FrameFormat     = FDCAN_FrameFormat::CLASSIC;
    Settings.RxFifo0ElmtSize = FDCAN_ElmtSize::BYTES_8;
    Settings.TxElmtSize      = FDCAN_ElmtSize::BYTES_8;

    if (can.begin(&Settings) != FDCAN_Status::OK) {
        Serial.println("Failed to initialize CAN-FD Channel");
        while (true) { }
    }
}

void loop() {
    // send every 1000ms
    if (micros() - sendLoop >= 1000 * 1000) {
        SendFrame.canId = 0x100;
        SendFrame.canDlc = 2;
        SendFrame.clear();
        SendFrame.SetUnsigned(42, 0, 8);
        SendFrame.data[1] = 0xFF;

        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK) {
            Serial.print(millis());
            Serial.println(" - error sending can frame");
        }
        sendLoop = micros();
    }

    // check inbox every 1ms
    if (micros() - recvLoop >= 1 * 1000) {
        // pop returns FDCAN_Status::FIFO_EMPTY and does nothing
        // to the frame if the receive inbox is empty.
        if (can.inbox.pop(RecvFrame) == FDCAN_Status::OK) {
            Serial.print(millis());
            Serial.println("ms ---------------");
            Serial.print("Unsigned Value : ");
            Serial.println(RecvFrame.GetUnsigned(0, 8));
            Serial.print("Raw Value    : ");
            Serial.println(RecvFrame.data[1], HEX);
            Serial.println();
        }
        recvLoop = micros();
    }
}
