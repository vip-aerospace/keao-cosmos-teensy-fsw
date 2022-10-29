#include "artemis_channels.h"

Artemis::Teensy::Radio::RFM23 rfm23(RFM23_CS_PIN, RFM23_INT_PIN, hardware_spi1);

void Artemis::Teensy::Channels::rfm23_channel()
{
    rfm23.RFM23_INIT();
    PacketComm packet2;
    packet2.header.type = PacketComm::TypeId::CommandReset;
    // packet.RawPacketize();

    while (true)
    {
        rfm23.RFM23_SEND(packet.packetized.data(), packet.packetized.size());
        Serial.println("sending message");
        threads.delay(5000);
    }
}