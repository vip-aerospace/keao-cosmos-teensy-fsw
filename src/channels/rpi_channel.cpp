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
    /** @brief Whether the Raspberry Pi is on and active. */
    bool       piIsOn = false;

    /**
     * @brief The top-level channel definition.
     *
     * This is the function that defines the Raspberry Pi channel. Like an
     * Arduino script, it has a setup() function that is run once, then loop()
     * runs forever.
     */
    void       rpi_channel() {
      setup();
      loop();
    }

    /**
     * @brief The Raspberry Pi setup function.
     *
     * This function is run once, when the channel is started. It connects to
     * the Raspberry Pi over a serial connection.
     *
     * @todo Ensure the Pi is on before completing setup.
     */
    void setup() {
      print_debug(Helpers::RPI, "RPI channel starting...");
      Serial2.begin(9600);
      while (!Serial2) {
      }
      // Try pinging the RPi to ensure we can communicate with it.
      // Retry and wait until a ping is successful before continuing.
      // Set the piIsOn variable when successful.
    }

    /**
     * @brief The Raspberry Pi loop function.
     *
     * This function runs in an infinite loop after setup() completes. It routes
     * packets going to and coming from the Raspberry Pi.
     *
     * @todo Handle the case where the Pi is not on.
     */
    void loop() {
      while (true) {
        if (!piIsOn) {
          // Try pinging the RPi to ensure we can communicate with it.
          // Retry and wait until a ping is successful before continuing.
          // Set the piIsOn variable when successful.
        }
        receive_from_pi();
        handle_queue();
        threads.delay(100);
      }
    }

    /**
     * @brief Helper function to receive a packet from the Raspberry Pi.
     *
     * @todo Complete this. If data is available from the serial connection,
     * read in the data and store it in a PacketComm packet. Add that packet to
     * the main_queue for routing.
     */
    void receive_from_pi() {
      while(Serial2.available()){
        uint8_t inChar = Serial2.read();
        if(inChar == 0xC0){
          uint8_t readBuffer[2048];
          size_t readBytes = Serial2.readBytesUntil((SLIP_FEND), readBuffer, (size_t)2048);
          packet.packetized.resize(readBytes);
          memcpy(packet.packetized.data(), &readBuffer[0], readBytes);
          print_hexdump(Helpers::RPI, "Got raw from RPi: ", &packet.packetized[0],
                      packet.packetized.size());
          int32_t iretn = slip_unpack(packet.packetized, packet.wrapped);
          print_debug(Helpers::RPI, "slip_unpack iretn=", iretn);
          /*
          if(!packet.SLIPUnPacketize()){
            print_debug(Helpers::RPI, "Failed to SLIP unpacketize incoming packet");
            return;
          }
          print_hexdump(Helpers::RPI, "Got decoded header from RPi: ", (uint8_t*)&packet.header,
                      sizeof(PacketComm::Header));
          print_hexdump(Helpers::RPI, "Got decoded data from RPi: ", packet.data.data(),
                      packet.data.size());
          // PushQueue(packet, main_queue, main_queue_mtx);
          */
        }
      }
    }

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
      while (!rpi_queue.empty()) {
        rpi_queue.pop_front();
      }

      piIsOn = false;
    }

    /** @brief Helper function to send a packet to the Raspberry Pi. */
    void send_to_pi() {
      if (!packet.SLIPPacketize()) {
        print_debug(Helpers::RPI, "Failed to wrap and SLIP packetize");
      }
      print_hexdump(Helpers::RPI, "Forwarding to RPi: ", &packet.packetized[0],
                    packet.packetized.size());
      for (size_t i = 0; i < packet.packetized.size(); i++) {
        if (Serial2.write(packet.packetized[i]) != 1) {
          print_debug(Helpers::RPI,
                      "Failed to send byte to RPi: ", (u_int32_t)i);
          piIsOn = false;
        }
      }
    }
  } // namespace RPI
} // namespace Channels
} // namespace Artemis
