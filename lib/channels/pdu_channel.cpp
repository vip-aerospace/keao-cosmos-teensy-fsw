#include <artemis_channels.h>

namespace
{
    Artemis::Teensy::PDU pdu(115200);
    PacketComm packet;
}

void Artemis::Teensy::Channels::pdu_channel()
{
    // Enable burn wire
    pdu.PDU_SWITCH(Artemis::Teensy::PDU::BURN, true);
    delay(30000);
    pdu.PDU_SWITCH(Artemis::Teensy::PDU::BURN, false);

    while (true)
    {
        if (PullQueue(&packet, pdu_queue, pdu_queue_mtx))
        {
        }
    }
}
