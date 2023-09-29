#include "channels/artemis_channels.h"
#include <pdu.h>

void sendToPi();

namespace {
  using namespace Artemis;
  PacketComm packet;
} // namespace

void Artemis::Channels::rpi_channel() {
  Serial.println("RPI Thread starting..");
  Serial2.begin(9600);

  while (true) {
    if (PullQueue(packet, rpi_queue, rpi_queue_mtx)) {
      Serial.println((uint16_t)packet.header.type);
      switch (packet.header.type) {
        case PacketComm::TypeId::CommandEpsSwitchName:
          if ((Devices::PDU::PDU_SW)packet.data[0] ==
                  Devices::PDU::PDU_SW::RPI &&
              packet.data[1] == 0) {
            packet.header.type = PacketComm::TypeId::CommandObcHalt;
            sendToPi();
            threads.delay(20000); // Wait 20s to give the rpi time to turn off
            digitalWrite(RPI_ENABLE, LOW);

            // Empty RPI Queue
            while (!rpi_queue.empty())
              rpi_queue.pop_front();

            Serial.println("Killing RPi thread");
            kill_thread(Artemis::Channels::Channel_ID::RPI_CHANNEL);
            return;
          }
          break;
        default:
          sendToPi();
          break;
      }
    }
    threads.delay(100);
  }
}

void sendToPi() {
  packet.SLIPPacketize();
  Serial.println("Forwarding to RPi");
  for (size_t i = 0; i < packet.packetized.size(); i++) {
    Serial.print((unsigned)packet.packetized[i]);
    Serial.print(" ");
    Serial2.write(packet.packetized[i]);
  }
  Serial.println();
}