#include <artemis_channels.h>

void Artemis::Teensy::Channels::pdu_channel()
{
    Artemis::Teensy::PDU pdu(115200);

    // Enable burn wire
    pdu.PDU_SWITCH(Artemis::Teensy::PDU::BURN, true);
    delay(30000);
    pdu.PDU_SWITCH(Artemis::Teensy::PDU::BURN, false);

}
