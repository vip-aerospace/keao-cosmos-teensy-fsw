#include "channels/artemis_channels.h"
#include <pdu.h>

namespace Artemis {
  namespace Channels {
    namespace RPI {
      using Artemis::Devices::PDU;
      PacketComm packet;

      void       rpi_channel() {
        setup();
        loop();
      }

      void setup() {
        Helpers::print_debug(Helpers::RPI, "RPI Thread starting..");
        Serial2.begin(9600);
      }

      void loop() {
        while (true) {
          handle_queue();
          receive_from_pi();
          threads.delay(100);
        }
      }

      void handle_queue() {
        if (PullQueue(packet, rpi_queue, rpi_queue_mtx)) {
          Helpers::print_debug(Helpers::RPI, "packet.header.type: ",
                               (u_int32_t)packet.header.type);
          switch (packet.header.type) {
            case PacketComm::TypeId::CommandEpsSwitchName:
              if ((PDU::PDU_SW)packet.data[0] == PDU::PDU_SW::RPI &&
                  packet.data[1] == 0) {
                packet.header.type = PacketComm::TypeId::CommandObcHalt;
                send_to_pi();
                // Wait 20s to give the rpi time to turn off
                threads.delay(20000);
                digitalWrite(RPI_ENABLE, LOW);

                // Empty RPI Queue
                while (!rpi_queue.empty())
                  rpi_queue.pop_front();

                Helpers::print_debug(Helpers::RPI, "Killing RPi thread");
                kill_thread(Channel_ID::RPI_CHANNEL);
                return;
              }
              break;
            default:
              send_to_pi();
              break;
          }
        }
      }

      void send_to_pi() {
        packet.SLIPPacketize();
        Helpers::print_hexdump(Helpers::RPI,
                               "Forwarding to RPi: ", &packet.packetized[0],
                               packet.packetized.size());
        for (size_t i = 0; i < packet.packetized.size(); i++) {
          Serial2.write(packet.packetized[i]);
        }
      }

      void receive_from_pi() {
        // TODO: Complete this.
        // If data is available from the serial connection, read in the data and
        // store it in a PacketComm packet. Add that packet to the main_queue
        // for routing.
      }
    } // namespace RPI
  }   // namespace Channels
} // namespace Artemis
