#include <Arduino.h>
#include "STM32DuinoCANFD.h"

#define nominalBitrate 500000
#define dataBitrate    2000000
#define nominalSample  80
#define dataSample     80

uint32_t SendTime;
uint32_t RecvTime;

FDCAN_Frame SendFrame;
FDCAN_Frame RecvFrame;

FDCAN_Instance can(FDCAN_Channel::CH1);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up...");

    FDCAN_Settings settings(nominalBitrate, dataBitrate, nominalSample, dataSample);
    settings.Mode        = FDCAN_Mode::EXTERNAL_LOOPBACK;
    settings.FrameFormat = FDCAN_FrameFormat::FD_BRS;

    Serial.println("CAN-FD Clock Speed: " + String(getCanClock()/1e6) + "MHz");
    Serial.println("Actual nBr: " + String(settings.GetNominalBitrate()));
    Serial.println("Actual nSp: " + String(settings.GetNominalSamplePoint()));
    Serial.println("Actual dBr: " + String(settings.GetDataBitrate()));
    Serial.println("Actual dSp: " + String(settings.GetDataSamplePoint()));

    if (can.begin(&settings) != FDCAN_Status::OK) {
        Serial.println("Error initializing CAN peripheral");
        while (true) { }
    }
}

void loop() {
    if (millis() - SendTime >= 1000) {
        SendFrame.canDlc = 8;

        // auto increment first byte, rollover to 0 if we reach 255
        SendFrame.data[0] = (SendFrame.data[0] == 255) ? 0 : SendFrame.data[0] + 1;

        SendFrame.canId = 0x101;
        SendFrame.format = FDCAN_FrameFormat::CLASSIC;
        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK)
            Serial.println("error sending classic frame");

        SendFrame.canId = 0x102;
        SendFrame.format = FDCAN_FrameFormat::FD_NO_BRS;
        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK)
            Serial.println("error sending fd frame");

        SendFrame.canId = 0x103;
        SendFrame.format = FDCAN_FrameFormat::FD_BRS;
        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK)
            Serial.println("error sending fd brs frame");

        SendTime = millis();
    }

    while (can.inbox.pop(RecvFrame) == FDCAN_Status::OK) {
        Serial.print(String(millis()) + " RECV, ");

        if (RecvFrame.format == FDCAN_FrameFormat::CLASSIC) {
            Serial.print("CLASSIC  ");
        }
        else if (RecvFrame.format == FDCAN_FrameFormat::FD_NO_BRS) {
            Serial.print("FD_NO_BRS");
        }
        else if (RecvFrame.format == FDCAN_FrameFormat::FD_BRS) {
            Serial.print("FD_BRS   ");
        }

        Serial.print(" - ID:0x");
        Serial.print(RecvFrame.canId, HEX);
        Serial.print(" [" + String(RecvFrame.canDlc) + "] ");

        for (uint8_t i = 0; i < DlcToLen(RecvFrame.canDlc); i++) {
            Serial.print(" " + String(RecvFrame.data[i]));
        }
        Serial.println();
    }
}
