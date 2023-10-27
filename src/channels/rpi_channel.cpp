/**
 * @file rpi_channel.cpp
 * @brief The Raspberry Pi channel.
 *
 * The definition of the Raspberry Pi channel.
 */
#include "channels/artemis_channels.h"
#include <pdu.h>

namespace Artemis {
  namespace Channels {
    /** @brief The Raspberry Pi channel. */
    namespace RPI {
      using Artemis::Devices::PDU;
      /** @brief The packet used throughout the channel. */
      PacketComm packet;
      /** @brief A flag to kill the channel and return. */
      bool       kill_channel = false;

      /**
       * @brief The top-level channel definition.
       *
       * This is the function that defines the Rapberry Pi channel. Like an
       * Arduino script, it has a setup() function that is run once, then loop()
       * runs forever.
       */
      void       rpi_channel() {
        setup();
        return loop();
      }

      /**
       * @brief The Raspberry Pi setup function.
       *
       * This function is run once, when the channel is started. It connects to
       * the Raspberry Pi over a serial connection.
       */
      void setup() {
        print_debug(Helpers::RPI, "RPI channel starting...");
        Serial2.begin(9600);
        while (!Serial2) {
        }
      }

      /**
       * @brief The Raspberry Pi loop function.
       *
       * This function runs in an infinite loop after setup() completes. It
       * routes packets going to and coming from the Raspberry Pi.
       */
      void loop() {
        while (true) {
          receive_from_pi();
          handle_queue();
          threads.delay(100);
          if (kill_channel) {
            return;
          }
        }
      }

      /**
       * @brief Helper function to receive a packet from the Raspberry Pi.
       *
       * @todo Complete this. If data is available from the serial connection,
       * read in the data and store it in a PacketComm packet. Add that packet
       * to the main_queue for routing.
       */
      void receive_from_pi() {}

      /**
       * @brief Helper function to handle packet queue.
       *
       * This is a helper function called in loop() that checks for packets and
       * routes them to the Raspberry Pi.
       */
      void handle_queue() {
        if (PullQueue(packet, rpi_queue, rpi_queue_mtx)) {
          print_debug(Helpers::RPI, "Pulled packet of type ",
                      (uint16_t)packet.header.type, " from queue.");
          switch (packet.header.type) {
            case PacketComm::TypeId::CommandEpsSwitchName: {
              if ((PDU::PDU_SW)packet.data[0] == PDU::PDU_SW::RPI &&
                  packet.data[1] == 0) {
                shut_down_pi();
              }
              break;
            }
            default: {
              send_to_pi();
              break;
            }
          }
        }
      }

      /** @brief Shuts down the Raspberry Pi and kills the channel.*/
      void shut_down_pi() {
        packet.header.type = PacketComm::TypeId::CommandObcHalt;
        send_to_pi();
        // Wait 20s to give the rpi time to turn off
        threads.delay(20 * SECONDS);
        digitalWrite(RPI_ENABLE, LOW);

        // Empty RPI Queue
        while (!rpi_queue.empty())
          rpi_queue.pop_front();

        print_debug(Helpers::RPI, "Killing RPi thread");
        if (!kill_thread(Channel_ID::RPI_CHANNEL)) {
          print_debug(Helpers::RPI, "Failed to kill RPi thread");
        }
        kill_channel = true;
      }

      /** @brief Helper function to send a packet to the Raspberry Pi. */
      void send_to_pi() {
        if (!packet.SLIPPacketize()) {
          print_debug(Helpers::RPI, "Failed to wrap and SLIP packetize");
        }
        print_hexdump(Helpers::RPI,
                      "Forwarding to RPi: ", &packet.packetized[0],
                      packet.packetized.size());
        for (size_t i = 0; i < packet.packetized.size(); i++) {
          if (Serial2.write(packet.packetized[i]) != 1) {
            print_debug(Helpers::RPI,
                        "Failed to send byte to RPi: ", (u_int32_t)i);
          }
        }
      }
    } // namespace RPI
  }   // namespace Channels
} // namespace Artemis
